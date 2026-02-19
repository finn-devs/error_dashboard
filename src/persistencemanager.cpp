#include "persistencemanager.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QDebug>

PersistenceManager::PersistenceManager(QObject* parent)
    : QObject(parent)
{
}

PersistenceManager::~PersistenceManager() {
    close();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool PersistenceManager::open(const QString& path) {
    if (m_db.isOpen()) {
        m_db.close();
    }

    // Each instance uses a unique connection name to avoid Qt's "connection
    // already exists" warning when the object is re-opened.
    const QString connName = QString("es_persist_%1").arg(reinterpret_cast<quintptr>(this));
    m_db = QSqlDatabase::addDatabase("QSQLITE", connName);
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        const QString err = m_db.lastError().text();
        qWarning() << "PersistenceManager: failed to open database:" << err;
        emit databaseError(err);
        return false;
    }

    m_path = path;

    // Enable WAL for better concurrent read performance
    QSqlQuery pragma(m_db);
    pragma.exec("PRAGMA journal_mode=WAL");
    pragma.exec("PRAGMA foreign_keys=ON");
    pragma.exec("PRAGMA synchronous=NORMAL");

    if (!createSchema()) {
        return false;
    }

    emit databaseOpened(path);
    return true;
}

void PersistenceManager::close() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool PersistenceManager::isOpen() const {
    return m_db.isOpen();
}

QString PersistenceManager::currentPath() const {
    return m_path;
}

qint64 PersistenceManager::databaseSizeBytes() const {
    if (m_path.isEmpty()) return 0;
    return QFileInfo(m_path).size();
}

// ---------------------------------------------------------------------------
// Schema
// ---------------------------------------------------------------------------

bool PersistenceManager::createSchema() {
    QSqlQuery q(m_db);

    // log_events — one row per unique log occurrence.
    // fingerprint is the SHA256 of (event_timestamp_usec + unit + message).
    // expires_at is set at INSERT time using the TTL that was active when
    // the event was first recorded; changing TTL later does NOT retroactively
    // alter this column.
    const bool ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS log_events (
            fingerprint     TEXT    PRIMARY KEY,
            event_timestamp INTEGER NOT NULL,
            expires_at      INTEGER NOT NULL,
            source          TEXT,
            grp             TEXT,
            priority        INTEGER,
            unit            TEXT,
            pid             TEXT,
            exe             TEXT,
            cmdline         TEXT,
            hostname        TEXT,
            boot_id         TEXT,
            message         TEXT,
            message_id      TEXT,
            transport       TEXT,
            cursor_id       TEXT,
            threat_count    INTEGER DEFAULT 0,
            max_threat_sev  TEXT,
            threat_json     TEXT
        )
    )");

    if (!ok) {
        const QString err = q.lastError().text();
        qWarning() << "PersistenceManager: schema creation failed:" << err;
        emit databaseError(err);
        return false;
    }

    // Indexes for the most common query patterns
    q.exec("CREATE INDEX IF NOT EXISTS idx_expires   ON log_events(expires_at)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_timestamp ON log_events(event_timestamp DESC)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_grp       ON log_events(grp)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_unit      ON log_events(unit)");

    // scan_runs — audit trail of every collection run
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS scan_runs (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            run_at          INTEGER NOT NULL,
            new_events      INTEGER DEFAULT 0,
            scan_days       INTEGER DEFAULT 0
        )
    )");

    return true;
}

// ---------------------------------------------------------------------------
// TTL
// ---------------------------------------------------------------------------

void PersistenceManager::setTtlDays(int days) {
    m_ttlDays = qMax(1, days);
}

int PersistenceManager::ttlDays() const {
    return m_ttlDays;
}

// ---------------------------------------------------------------------------
// Fingerprinting
// ---------------------------------------------------------------------------

QString PersistenceManager::computeFingerprint(const LogEntry& entry) {
    // The fingerprint uniquely identifies a specific occurrence of an event.
    // We use: UTC timestamp (to the second) + unit + message.
    // This means the same log line seen in two overlapping scans produces the
    // same hash and will be skipped on the second insert (idempotent upsert).
    const QString raw = QString("%1|%2|%3")
        .arg(entry.timestamp.toSecsSinceEpoch())
        .arg(entry.unit)
        .arg(entry.message);

    return QCryptographicHash::hash(raw.toUtf8(), QCryptographicHash::Sha256).toHex();
}

// ---------------------------------------------------------------------------
// Write path
// ---------------------------------------------------------------------------

static QString threatJsonSerialize(const QVector<ThreatMatch>& threats) {
    // Minimal hand-rolled JSON — avoids pulling in QJsonDocument for
    // what is effectively a small structured field.
    if (threats.isEmpty()) return "[]";
    QString json = "[";
    for (int i = 0; i < threats.size(); ++i) {
        const auto& t = threats[i];
        if (i > 0) json += ",";
        json += QString(R"({"id":"%1","sev":"%2","cat":"%3","desc":"%4"})")
                    .arg(t.id, t.severity, t.category,
                         QString(t.description).replace("\"", "\\\""));
    }
    json += "]";
    return json;
}

bool PersistenceManager::upsertEvent(const LogEntry& entry) {
    if (!m_db.isOpen()) return false;

    const QString fp      = computeFingerprint(entry);
    const qint64  evTs    = entry.timestamp.toSecsSinceEpoch();
    const qint64  expires = evTs + (static_cast<qint64>(m_ttlDays) * 86400);

    // INSERT OR IGNORE — if fingerprint already exists (duplicate scan window
    // overlap) we do nothing; the event is already stored.
    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT OR IGNORE INTO log_events
            (fingerprint, event_timestamp, expires_at, source, grp, priority,
             unit, pid, exe, cmdline, hostname, boot_id, message, message_id,
             transport, cursor_id, threat_count, max_threat_sev, threat_json)
        VALUES
            (:fp, :evts, :exp, :src, :grp, :prio,
             :unit, :pid, :exe, :cmd, :host, :boot, :msg, :msgid,
             :trans, :cursor, :tc, :mts, :tj)
    )");

    q.bindValue(":fp",     fp);
    q.bindValue(":evts",   evTs);
    q.bindValue(":exp",    expires);
    q.bindValue(":src",    entry.source);
    q.bindValue(":grp",    entry.group);
    q.bindValue(":prio",   entry.priority);
    q.bindValue(":unit",   entry.unit);
    q.bindValue(":pid",    entry.pid);
    q.bindValue(":exe",    entry.exe);
    q.bindValue(":cmd",    entry.cmdline);
    q.bindValue(":host",   entry.hostname);
    q.bindValue(":boot",   entry.bootId);
    q.bindValue(":msg",    entry.message);
    q.bindValue(":msgid",  entry.messageId);
    q.bindValue(":trans",  entry.transport);
    q.bindValue(":cursor", entry.cursor);
    q.bindValue(":tc",     entry.threatCount);
    q.bindValue(":mts",    entry.maxThreatSeverity);
    q.bindValue(":tj",     threatJsonSerialize(entry.threats));

    if (!q.exec()) {
        qWarning() << "PersistenceManager: insert failed:" << q.lastError().text();
        return false;
    }

    // numRowsAffected() == 1 means a new row was inserted;
    // 0 means the fingerprint already existed (ignored).
    return q.numRowsAffected() == 1;
}

int PersistenceManager::upsertEvents(const QVector<LogEntry>& entries) {
    if (!m_db.isOpen() || entries.isEmpty()) return 0;

    int newCount = 0;

    // Wrap the entire batch in a single transaction — dramatically faster
    // than auto-committing each INSERT individually.
    m_db.transaction();
    for (const auto& entry : entries) {
        if (upsertEvent(entry)) ++newCount;
    }
    m_db.commit();

    // Record the run in the audit table
    recordScanRun(newCount, entries.size() - newCount);

    return newCount;
}

bool PersistenceManager::recordScanRun(int newEvents, int /*updatedEvents*/) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO scan_runs (run_at, new_events) VALUES (:ts, :ne)");
    q.bindValue(":ts", QDateTime::currentDateTimeUtc().toSecsSinceEpoch());
    q.bindValue(":ne", newEvents);
    return q.exec();
}

// ---------------------------------------------------------------------------
// Read path
// ---------------------------------------------------------------------------

static QVector<ThreatMatch> threatJsonDeserialize(const QString& json) {
    // Minimal parser that handles the format produced by threatJsonSerialize.
    // A proper JSON parser would be cleaner but this avoids the dependency and
    // is safe because we control the serialization format.
    QVector<ThreatMatch> threats;
    if (json.isEmpty() || json == "[]") return threats;

    // Split on "},{" to get individual objects
    QString stripped = json.mid(1, json.length() - 2); // Remove outer [ ]
    QStringList objects = stripped.split("},{");

    for (const QString& obj : objects) {
        ThreatMatch t;
        auto extract = [&](const QString& key) -> QString {
            const QString search = QString("\"%1\":\"").arg(key);
            int start = obj.indexOf(search);
            if (start < 0) return {};
            start += search.length();
            int end = obj.indexOf("\"", start);
            if (end < 0) return {};
            return obj.mid(start, end - start);
        };
        t.id          = extract("id");
        t.severity    = extract("sev");
        t.category    = extract("cat");
        t.description = extract("desc");
        if (!t.id.isEmpty()) threats.append(t);
    }
    return threats;
}

QVector<LogEntry> PersistenceManager::loadActiveEvents() const {
    QVector<LogEntry> entries;
    if (!m_db.isOpen()) return entries;

    const qint64 now = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();

    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT fingerprint, event_timestamp, source, grp, priority,
               unit, pid, exe, cmdline, hostname, boot_id, message,
               message_id, transport, cursor_id, threat_count, max_threat_sev,
               threat_json
        FROM log_events
        WHERE expires_at > :now
        ORDER BY event_timestamp DESC
    )");
    q.bindValue(":now", now);

    if (!q.exec()) {
        qWarning() << "PersistenceManager: load failed:" << q.lastError().text();
        return entries;
    }

    while (q.next()) {
        LogEntry e;
        e.cursor          = q.value(0).toString();  // fingerprint stored as cursor surrogate for display
        e.timestamp       = QDateTime::fromSecsSinceEpoch(q.value(1).toLongLong(), Qt::UTC);
        e.source          = q.value(2).toString();
        e.group           = q.value(3).toString();
        e.priority        = q.value(4).toInt();
        e.unit            = q.value(5).toString();
        e.pid             = q.value(6).toString();
        e.exe             = q.value(7).toString();
        e.cmdline         = q.value(8).toString();
        e.hostname        = q.value(9).toString();
        e.bootId          = q.value(10).toString();
        e.message         = q.value(11).toString();
        e.messageId       = q.value(12).toString();
        e.transport       = q.value(13).toString();
        // cursor_id col (14) is the journal cursor; we stuffed the fingerprint
        // in e.cursor above for the detail panel display.
        e.threatCount         = q.value(15).toInt();
        e.maxThreatSeverity   = q.value(16).toString();
        e.threats             = threatJsonDeserialize(q.value(17).toString());
        entries.append(e);
    }

    return entries;
}

// ---------------------------------------------------------------------------
// Maintenance
// ---------------------------------------------------------------------------

int PersistenceManager::purgeExpired() {
    if (!m_db.isOpen()) return 0;

    const qint64 now = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM log_events WHERE expires_at <= :now");
    q.bindValue(":now", now);

    if (!q.exec()) {
        qWarning() << "PersistenceManager: purge failed:" << q.lastError().text();
        return 0;
    }

    const int removed = q.numRowsAffected();
    if (removed > 0) {
        q.exec("VACUUM");
        emit purgeComplete(removed);
    }
    return removed;
}

bool PersistenceManager::clearAll() {
    if (!m_db.isOpen()) return false;

    QSqlQuery q(m_db);
    const bool ok = q.exec("DELETE FROM log_events") && q.exec("DELETE FROM scan_runs");
    if (ok) q.exec("VACUUM");
    return ok;
}
