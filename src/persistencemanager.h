#ifndef PERSISTENCEMANAGER_H
#define PERSISTENCEMANAGER_H

#include "logentry.h"
#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVector>
#include <QtSql/QSqlDatabase>

// TTL preset values in days
enum class TtlPreset {
    Days7   = 7,
    Days14  = 14,
    Days30  = 30,
    Days60  = 60,
    Days90  = 90,
    Days365 = 365,
    Custom  = -1
};

class PersistenceManager : public QObject {
    Q_OBJECT

public:
    explicit PersistenceManager(QObject* parent = nullptr);
    ~PersistenceManager();

    // Database lifecycle
    bool open(const QString& path);
    void close();
    bool isOpen() const;
    QString currentPath() const;
    qint64 databaseSizeBytes() const;

    // TTL management
    void setTtlDays(int days);
    int ttlDays() const;

    // Write path — upsert based on fingerprint. Returns true if new record inserted.
    bool upsertEvent(const LogEntry& entry);
    int upsertEvents(const QVector<LogEntry>& entries);

    // Read path — returns all non-expired events
    QVector<LogEntry> loadActiveEvents() const;

    // Maintenance
    int purgeExpired();
    bool clearAll();

    // Fingerprinting (public so tests can verify)
    static QString computeFingerprint(const LogEntry& entry);

signals:
    void databaseOpened(const QString& path);
    void databaseError(const QString& error);
    void purgeComplete(int removedCount);

private:
    bool createSchema();
    bool recordScanRun(int newEvents, int updatedEvents);
    QSqlDatabase m_db;
    QString m_path;
    int m_ttlDays = 30;
};

#endif // PERSISTENCEMANAGER_H
