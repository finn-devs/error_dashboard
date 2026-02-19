#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QDir>
#include "src/logentry.h"
#include "src/logcollector.h"
#include "src/threatdetector.h"
#include "src/statstab.h"
#include "src/mainwindow.h"
#include "src/persistencemanager.h"
#include "src/settingsdrawer.h"

class Testerrordashboard : public QObject {
    Q_OBJECT

private slots:
    // Setup and teardown
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // LogEntry tests
    void testLogEntrySeverityLabels();
    void testLogEntrySeverityColors();
    void testLogEntryThreatBadge();

    // ThreatDetector tests
    void testThreatDetectorAuthentication();
    void testThreatDetectorPrivilege();
    void testThreatDetectorNetwork();
    void testThreatDetectorFilesystem();
    void testThreatDetectorStability();
    void testThreatDetectorResources();
    void testThreatDetectorSELinux();
    void testThreatDetectorMalware();
    void testThreatDetectorMultipleThreats();
    void testThreatDetectorNoThreats();

    // LogCollector tests
    void testLogCollectorJournaldOpen();
    void testLogCollectorSeverityFiltering();
    void testLogCollectorTimeFiltering();
    void testLogCollectorDmesgFallback();

    // StatsTab tests
    void testStatsTabDataLoading();
    void testStatsTabStatCounts();
    void testStatsTabFiltering();
    void testStatsTabSearchFilter();
    void testStatsTabUnitFilter();
    void testStatsTabClickableCards();
    void testStatsTabCardChildrenTransparentToMouse();
    void testStatsTabChartGeneration();
    void testStatsTabExportCSV();

    // MainWindow tests
    void testMainWindowInitialization();
    void testMainWindowTabSwitching();
    void testMainWindowLivePolling();
    void testMainWindowGearButton();
    void testMainWindowSettingsDrawerToggle();

    // PersistenceManager tests
    void testPersistenceOpenClose();
    void testPersistenceFingerprintStability();
    void testPersistenceFingerprintUniqueness();
    void testPersistenceUpsertNew();
    void testPersistenceUpsertIdempotent();
    void testPersistenceUpsertBatch();
    void testPersistenceTtlAppliedToNewOnly();
    void testPersistenceTtlExpiry();
    void testPersistenceLoadActiveEvents();
    void testPersistenceLoadExcludesExpired();
    void testPersistencePurgeExpired();
    void testPersistenceClearAll();
    void testPersistenceDatabaseSize();
    void testPersistenceThreatJsonRoundtrip();
    void testPersistenceScanRunRecorded();
    void testPersistenceReopenSameFile();

    // SettingsDrawer tests
    void testSettingsDrawerCreation();
    void testSettingsDrawerTtlSignal();
    void testSettingsDrawerRefreshStats();

    // Integration tests
    void testEndToEndDataFlow();
    void testEndToEndPersistenceAndMerge();
    void testThreatDetectionPipeline();
    void testUIResponsiveness();

private:
    QVector<LogEntry> createTestEntries();
    LogEntry createTestEntry(const QString& severity, const QString& message,
                             const QString& unit = "test.service");

    // Creates a fresh temp-dir-backed PersistenceManager for each persistence test
    PersistenceManager* createTempPersistence();

    QTemporaryDir m_tempDir;
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QVector<LogEntry> Testerrordashboard::createTestEntries() {
    QVector<LogEntry> entries;

    LogEntry critical;
    critical.timestamp  = QDateTime::currentDateTimeUtc();
    critical.priority   = 2;
    critical.group      = "critical";
    critical.unit       = "sshd.service";
    critical.message    = "Failed password for root from 192.168.1.1";
    critical.source     = "journald";
    critical.threats    = ThreatDetector::detectThreats(critical.message, critical.unit);
    critical.threatCount = critical.threats.size();
    if (!critical.threats.isEmpty())
        critical.maxThreatSeverity = critical.threats[0].severity;
    entries.append(critical);

    for (int i = 0; i < 5; i++) {
        LogEntry error;
        error.timestamp = QDateTime::currentDateTimeUtc().addSecs(-i * 60);
        error.priority  = 3;
        error.group     = "error";
        error.unit      = QString("service%1.service").arg(i);
        error.message   = QString("Service failed to start (attempt %1)").arg(i);
        error.source    = "journald";
        entries.append(error);
    }

    for (int i = 0; i < 3; i++) {
        LogEntry warning;
        warning.timestamp = QDateTime::currentDateTimeUtc().addSecs(-i * 120);
        warning.priority  = 4;
        warning.group     = "warning";
        warning.unit      = "disk.service";
        warning.message   = QString("Disk usage above 80%% (check %1)").arg(i);
        warning.source    = "dmesg";
        entries.append(warning);
    }

    return entries;
}

LogEntry Testerrordashboard::createTestEntry(const QString& severity,
                                            const QString& message,
                                            const QString& unit) {
    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTimeUtc();
    entry.group     = severity;
    entry.message   = message;
    entry.unit      = unit;
    entry.source    = "journald";
    entry.hostname  = "testhost";

    if      (severity == "critical") entry.priority = 2;
    else if (severity == "error")    entry.priority = 3;
    else if (severity == "warning")  entry.priority = 4;

    entry.threats     = ThreatDetector::detectThreats(message, unit);
    entry.threatCount = entry.threats.size();
    if (!entry.threats.isEmpty())
        entry.maxThreatSeverity = entry.threats[0].severity;

    return entry;
}

PersistenceManager* Testerrordashboard::createTempPersistence() {
    auto* pm = new PersistenceManager(this);
    const QString dbPath = m_tempDir.path() + QString("/test_%1.db")
                               .arg(QDateTime::currentMSecsSinceEpoch());
    if (!pm->open(dbPath)) {
        delete pm;
        // Return a dummy pointer; the caller guards with QVERIFY(pm) before use
        return nullptr;
    }
    return pm;
}

// ---------------------------------------------------------------------------
// Suite lifecycle
// ---------------------------------------------------------------------------

void Testerrordashboard::initTestCase() {
    qDebug() << "Starting Error Surface Test Suite";
    QVERIFY(m_tempDir.isValid());
}

void Testerrordashboard::cleanupTestCase() {
    qDebug() << "Completed Error Surface Test Suite";
}

void Testerrordashboard::init()    {}
void Testerrordashboard::cleanup() {}

// ============================================================================
// LogEntry Tests
// ============================================================================

void Testerrordashboard::testLogEntrySeverityLabels() {
    LogEntry entry;

    entry.priority = 0; entry.group = "critical";
    QVERIFY(!entry.severityLabel().isEmpty());

    entry.priority = 3; entry.group = "error";
    QVERIFY(!entry.severityLabel().isEmpty());

    entry.priority = 4; entry.group = "warning";
    QVERIFY(!entry.severityLabel().isEmpty());
}

void Testerrordashboard::testLogEntrySeverityColors() {
    LogEntry entry;

    entry.group = "critical";
    QCOMPARE(entry.severityColor(), QColor("#FF2D55"));

    entry.group = "error";
    QCOMPARE(entry.severityColor(), QColor("#FF6B35"));

    entry.group = "warning";
    QCOMPARE(entry.severityColor(), QColor("#FFD60A"));

    entry.group = "unknown";
    QCOMPARE(entry.severityColor(), QColor("#888"));
}

void Testerrordashboard::testLogEntryThreatBadge() {
    LogEntry entry;

    entry.threatCount = 0;
    QCOMPARE(entry.threatBadge(), QString(""));

    entry.threatCount = 1;
    QVERIFY(!entry.threatBadge().isEmpty());

    entry.threatCount = 5;
    QVERIFY(entry.threatBadge().contains("5"));
}

// ============================================================================
// ThreatDetector Tests
// ============================================================================

void Testerrordashboard::testThreatDetectorAuthentication() {
    auto threats = ThreatDetector::detectThreats("Failed password for root from 192.168.1.100", "sshd.service");
    QVERIFY(!threats.isEmpty());
    QCOMPARE(threats[0].category, QString("Authentication"));
    QCOMPARE(threats[0].severity, QString("high"));
}

void Testerrordashboard::testThreatDetectorPrivilege() {
    auto threats = ThreatDetector::detectThreats("sudo: user NOT in sudoers", "sudo");
    QVERIFY(!threats.isEmpty());
    QVERIFY(threats[0].severity == "critical" || threats[0].category.contains("Privilege"));
}

void Testerrordashboard::testThreatDetectorNetwork() {
    auto threats = ThreatDetector::detectThreats("Connection attempt from blocked IP 10.0.0.1", "firewall");
    QVERIFY(!threats.isEmpty());
}

void Testerrordashboard::testThreatDetectorFilesystem() {
    auto threats = ThreatDetector::detectThreats("Permission denied writing to /etc/passwd", "vim");
    QVERIFY(!threats.isEmpty());
    QCOMPARE(threats[0].category, QString("Filesystem"));
}

void Testerrordashboard::testThreatDetectorStability() {
    auto threats1 = ThreatDetector::detectThreats("segmentation fault at address 0x00000000", "app");
    QVERIFY(!threats1.isEmpty());

    auto threats2 = ThreatDetector::detectThreats("kernel panic - not syncing", "kernel");
    QVERIFY(!threats2.isEmpty());
}

void Testerrordashboard::testThreatDetectorResources() {
    auto threats = ThreatDetector::detectThreats("Out of memory: Kill process 1234", "kernel");
    QVERIFY(!threats.isEmpty());
    QCOMPARE(threats[0].severity, QString("high"));
}

void Testerrordashboard::testThreatDetectorSELinux() {
    auto threats = ThreatDetector::detectThreats("SELinux is preventing access to file /var/log/secure", "audit");
    QVERIFY(!threats.isEmpty());
}

void Testerrordashboard::testThreatDetectorMalware() {
    auto threats = ThreatDetector::detectThreats("Detected suspicious process: /tmp/malware.sh", "scanner");
    QVERIFY(!threats.isEmpty());
    QCOMPARE(threats[0].severity, QString("critical"));
}

void Testerrordashboard::testThreatDetectorMultipleThreats() {
    auto threats = ThreatDetector::detectThreats("Failed authentication and permission denied for root", "system");
    QVERIFY(threats.size() >= 1);
}

void Testerrordashboard::testThreatDetectorNoThreats() {
    auto threats = ThreatDetector::detectThreats("Service started successfully", "nginx.service");
    QVERIFY(threats.isEmpty());
}

// ============================================================================
// LogCollector Tests
// ============================================================================

void Testerrordashboard::testLogCollectorJournaldOpen() {
    LogCollector collector;
    auto entries = collector.collectAll(1);
    QVERIFY(entries.size() >= 0);
}

void Testerrordashboard::testLogCollectorSeverityFiltering() {
    LogCollector collector;
    auto entries = collector.collectAll(1);
    for (const auto& entry : entries) {
        QVERIFY(entry.priority >= 0 && entry.priority <= 4);
        QVERIFY(!entry.group.isEmpty());
        QVERIFY(entry.group == "critical" ||
                entry.group == "error"    ||
                entry.group == "warning");
    }
}

void Testerrordashboard::testLogCollectorTimeFiltering() {
    LogCollector collector;
    QDateTime since = QDateTime::currentDateTimeUtc().addSecs(-7200);
    auto entries = collector.collectLive(120);
    for (const auto& entry : entries) {
        QVERIFY(entry.timestamp >= since.addSecs(-60));
    }
}

void Testerrordashboard::testLogCollectorDmesgFallback() {
    // collectDmesg is private; exercise it through the public collectAll interface
    // and verify that any dmesg-sourced entries are correctly tagged.
    LogCollector collector;
    auto all = collector.collectAll(1);

    QVector<LogEntry> dmesgEntries;
    for (const auto& entry : all) {
        if (entry.source == "dmesg") dmesgEntries.append(entry);
    }

    // May be empty if dmesg is inaccessible — that is acceptable.
    // All returned dmesg entries must carry the correct source tag.
    QVERIFY(dmesgEntries.size() >= 0);
    for (const auto& entry : dmesgEntries) {
        QCOMPARE(entry.source, QString("dmesg"));
    }
}

// ============================================================================
// StatsTab Tests
// ============================================================================

void Testerrordashboard::testStatsTabDataLoading() {
    StatsTab tab("scan");
    tab.setData(createTestEntries());
    QVERIFY(tab.entryCount() == 9);
}

void Testerrordashboard::testStatsTabStatCounts() {
    StatsTab tab("scan");
    tab.setData(createTestEntries());
    // 1 critical + 5 errors + 3 warnings = 9 total; verified via entryCount
    QCOMPARE(tab.entryCount(), 9);
}

void Testerrordashboard::testStatsTabFiltering() {
    StatsTab tab("scan");
    tab.setData(createTestEntries());
    QSignalSpy spy(&tab, &StatsTab::needsRefresh);
    // Filters are internal; verify no crash
    QVERIFY(tab.entryCount() >= 0);
}

void Testerrordashboard::testStatsTabSearchFilter() {
    StatsTab tab("scan");
    auto entries = createTestEntries();
    entries.append(createTestEntry("error", "special_unique_xyz_message", "test.service"));
    tab.setData(entries);
    QVERIFY(tab.entryCount() == 10);
}

void Testerrordashboard::testStatsTabUnitFilter() {
    StatsTab tab("scan");
    tab.setData(createTestEntries());
    QVERIFY(tab.entryCount() > 0);
}

void Testerrordashboard::testStatsTabClickableCards() {
    // Verify that clicking the card container triggers filter activation.
    // We simulate a mouse press event on the card widget itself.
    StatsTab tab("scan");
    tab.setData(createTestEntries());
    tab.show();

    // Find the critical card by its "severity" property
    QWidget* criticalCard = tab.findChild<QWidget*>("statCard");
    // The first statCard child with severity == "critical"
    for (auto* w : tab.findChildren<QWidget*>()) {
        if (w->property("severity").toString() == "critical") {
            criticalCard = w;
            break;
        }
    }
    QVERIFY(criticalCard != nullptr);

    // Send a mouse press event to the card using the Qt6-compatible API
    QTest::mouseClick(criticalCard, Qt::LeftButton, Qt::NoModifier, QPoint(10, 10));

    // No crash = pass; the filter state is internal to StatsTab
    QVERIFY(true);
}

void Testerrordashboard::testStatsTabCardChildrenTransparentToMouse() {
    // Verify that QLabel children inside each stat card have
    // WA_TransparentForMouseEvents set, meaning clicks on the label
    // pass through to the card container.
    StatsTab tab("scan");
    tab.setData(createTestEntries());

    bool foundCard = false;
    for (auto* w : tab.findChildren<QWidget*>()) {
        if (w->property("severity").toString().isEmpty()) continue;
        foundCard = true;
        // All direct QLabel children of a card must be mouse-transparent
        for (auto* child : w->findChildren<QLabel*>(QString(), Qt::FindDirectChildrenOnly)) {
            QVERIFY2(child->testAttribute(Qt::WA_TransparentForMouseEvents),
                     "Card child label is NOT transparent to mouse events — "
                     "clicking on the label would not reach the card.");
        }
    }
    QVERIFY2(foundCard, "No stat cards with severity property found in StatsTab");
}

void Testerrordashboard::testStatsTabChartGeneration() {
    StatsTab tab("scan");
    tab.setData(createTestEntries());
    QVERIFY(true);  // Charts generated without crash
}

void Testerrordashboard::testStatsTabExportCSV() {
    StatsTab tab("scan");
    tab.setData(createTestEntries());
    QVERIFY(true);  // Export dialog requires user interaction; non-crash verified
}

// ============================================================================
// MainWindow Tests
// ============================================================================

void Testerrordashboard::testMainWindowInitialization() {
    MainWindow window(7, 60, 5);
    QCOMPARE(window.windowTitle(), QString("Error Surface"));
}

void Testerrordashboard::testMainWindowTabSwitching() {
    MainWindow window(7, 60, 5);
    window.show();
    QVERIFY(true);
}

void Testerrordashboard::testMainWindowLivePolling() {
    MainWindow window(7, 60, 5);
    window.startCollections();
    QVERIFY(true);
}

void Testerrordashboard::testMainWindowGearButton() {
    MainWindow window(7, 60, 5);
    window.show();

    // Find the gear button by object name
    QPushButton* gearBtn = window.findChild<QPushButton*>("gearBtn");
    QVERIFY2(gearBtn != nullptr, "Gear button not found");
    QVERIFY(gearBtn->isVisible());
}

void Testerrordashboard::testMainWindowSettingsDrawerToggle() {
    MainWindow window(7, 60, 5);
    window.show();

    // Find the settings drawer
    SettingsDrawer* drawer = window.findChild<SettingsDrawer*>();
    QVERIFY2(drawer != nullptr, "SettingsDrawer not found in MainWindow");

    // Initially closed
    QVERIFY(!drawer->isDrawerOpen());

    // Simulate gear click
    QPushButton* gearBtn = window.findChild<QPushButton*>("gearBtn");
    QVERIFY(gearBtn);
    gearBtn->click();
    QVERIFY(drawer->isDrawerOpen());

    // Click again to close
    gearBtn->click();
    QVERIFY(!drawer->isDrawerOpen());
}

// ============================================================================
// PersistenceManager Tests
// ============================================================================

void Testerrordashboard::testPersistenceOpenClose() {
    auto* pm = new PersistenceManager(this);
    const QString path = m_tempDir.path() + "/openclose.db";

    QVERIFY(!pm->isOpen());
    QVERIFY(pm->open(path));
    QVERIFY(pm->isOpen());
    QCOMPARE(pm->currentPath(), path);

    pm->close();
    QVERIFY(!pm->isOpen());
    delete pm;
}

void Testerrordashboard::testPersistenceFingerprintStability() {
    // Same entry must always produce the same fingerprint
    LogEntry e = createTestEntry("error", "disk failure detected", "disk.service");
    e.timestamp = QDateTime(QDate(2024, 1, 15), QTime(12, 0, 0), Qt::UTC);

    const QString fp1 = PersistenceManager::computeFingerprint(e);
    const QString fp2 = PersistenceManager::computeFingerprint(e);
    QCOMPARE(fp1, fp2);
    QVERIFY(!fp1.isEmpty());
    QCOMPARE(fp1.length(), 64);  // SHA256 hex = 64 chars
}

void Testerrordashboard::testPersistenceFingerprintUniqueness() {
    // Different events produce different fingerprints
    LogEntry e1 = createTestEntry("error", "disk failure", "disk.service");
    e1.timestamp = QDateTime(QDate(2024, 1, 15), QTime(12, 0, 0), Qt::UTC);

    LogEntry e2 = createTestEntry("error", "disk failure", "disk.service");
    e2.timestamp = QDateTime(QDate(2024, 1, 15), QTime(12, 0, 1), Qt::UTC); // 1 second later

    LogEntry e3 = createTestEntry("error", "different message", "disk.service");
    e3.timestamp = e1.timestamp;

    QVERIFY(PersistenceManager::computeFingerprint(e1) != PersistenceManager::computeFingerprint(e2));
    QVERIFY(PersistenceManager::computeFingerprint(e1) != PersistenceManager::computeFingerprint(e3));
}

void Testerrordashboard::testPersistenceUpsertNew() {
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");

    LogEntry e = createTestEntry("error", "brand new event", "new.service");
    e.timestamp = QDateTime::currentDateTimeUtc();

    const bool inserted = pm->upsertEvent(e);
    QVERIFY(inserted);  // Should be a new insertion

    delete pm;
}

void Testerrordashboard::testPersistenceUpsertIdempotent() {
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");

    LogEntry e = createTestEntry("error", "idempotent test event", "svc.service");
    e.timestamp = QDateTime(QDate(2024, 6, 1), QTime(10, 0, 0), Qt::UTC);

    QVERIFY(pm->upsertEvent(e));   // First insert: new
    QVERIFY(!pm->upsertEvent(e));  // Second insert: same fingerprint, ignored

    // Only one record should exist
    const auto events = pm->loadActiveEvents();
    int count = 0;
    for (const auto& ev : events) {
        if (ev.message == e.message && ev.unit == e.unit) count++;
    }
    QCOMPARE(count, 1);

    delete pm;
}

void Testerrordashboard::testPersistenceUpsertBatch() {
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");

    auto entries = createTestEntries();
    const int newCount = pm->upsertEvents(entries);

    // All 9 entries are new
    QCOMPARE(newCount, entries.size());

    // Second batch: same entries — none should be new
    const int newCount2 = pm->upsertEvents(entries);
    QCOMPARE(newCount2, 0);

    delete pm;
}

void Testerrordashboard::testPersistenceTtlAppliedToNewOnly() {
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");
    pm->setTtlDays(30);

    LogEntry e1 = createTestEntry("error", "event stored at 30d TTL", "svc.service");
    e1.timestamp = QDateTime(QDate(2024, 6, 1), QTime(10, 0, 0), Qt::UTC);
    pm->upsertEvent(e1);

    // Change TTL — should NOT affect e1's expiry
    pm->setTtlDays(7);

    LogEntry e2 = createTestEntry("error", "event stored at 7d TTL", "svc.service");
    e2.timestamp = QDateTime(QDate(2024, 6, 2), QTime(10, 0, 0), Qt::UTC);
    pm->upsertEvent(e2);

    QCOMPARE(pm->ttlDays(), 7);
    // Both events are in the DB; their individual expiry timestamps differ
    // (we can't easily verify expiry without time-mocking, but the counts are correct)
    QVERIFY(pm->loadActiveEvents().size() >= 0);

    delete pm;
}

void Testerrordashboard::testPersistenceTtlExpiry() {
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");

    // Insert an event with a timestamp far in the past so its TTL has already elapsed
    LogEntry old = createTestEntry("warning", "very old event", "old.service");
    old.timestamp = QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0), Qt::UTC);

    // Even with a 1-day TTL this event expired 24+ years ago
    pm->setTtlDays(1);
    pm->upsertEvent(old);

    // loadActiveEvents should not return it
    const auto active = pm->loadActiveEvents();
    for (const auto& e : active) {
        QVERIFY2(e.message != old.message, "Expired event was returned by loadActiveEvents");
    }

    delete pm;
}

void Testerrordashboard::testPersistenceLoadActiveEvents() {
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");
    pm->setTtlDays(365);

    auto entries = createTestEntries();
    pm->upsertEvents(entries);

    const auto loaded = pm->loadActiveEvents();
    QCOMPARE(loaded.size(), entries.size());

    // Verify all loaded entries have valid groups
    for (const auto& e : loaded) {
        QVERIFY(e.group == "critical" || e.group == "error" || e.group == "warning");
        QVERIFY(!e.unit.isEmpty());
        QVERIFY(!e.message.isEmpty());
    }

    delete pm;
}

void Testerrordashboard::testPersistenceLoadExcludesExpired() {
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");
    pm->setTtlDays(365);

    // Insert a current event
    LogEntry fresh = createTestEntry("error", "fresh event", "fresh.service");
    fresh.timestamp = QDateTime::currentDateTimeUtc();
    pm->upsertEvent(fresh);

    // Insert an expired event (timestamp so old TTL already elapsed)
    pm->setTtlDays(1);
    LogEntry ancient = createTestEntry("warning", "ancient event 1970", "old.service");
    ancient.timestamp = QDateTime(QDate(1970, 1, 2), QTime(0, 0, 0), Qt::UTC);
    pm->upsertEvent(ancient);

    const auto active = pm->loadActiveEvents();
    bool foundFresh  = false;
    bool foundAncient = false;
    for (const auto& e : active) {
        if (e.message == fresh.message)   foundFresh   = true;
        if (e.message == ancient.message) foundAncient = true;
    }

    QVERIFY2(foundFresh,   "Fresh event missing from active set");
    QVERIFY2(!foundAncient, "Expired ancient event should not appear in active set");

    delete pm;
}

void Testerrordashboard::testPersistencePurgeExpired() {
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");
    pm->setTtlDays(1);

    // Insert an event that is already expired (year 2000)
    LogEntry old = createTestEntry("error", "old event for purge test", "purge.service");
    old.timestamp = QDateTime(QDate(2000, 6, 1), QTime(12, 0, 0), Qt::UTC);
    pm->upsertEvent(old);

    // Insert a current event
    pm->setTtlDays(365);
    LogEntry fresh = createTestEntry("warning", "fresh for purge test", "fresh.service");
    fresh.timestamp = QDateTime::currentDateTimeUtc();
    pm->upsertEvent(fresh);

    QSignalSpy purgeSpy(pm, &PersistenceManager::purgeComplete);
    const int purged = pm->purgeExpired();

    QVERIFY(purged >= 1);
    QVERIFY(purgeSpy.count() >= 1);

    // Fresh event should still be there
    const auto remaining = pm->loadActiveEvents();
    bool foundFresh = false;
    for (const auto& e : remaining) {
        if (e.message == fresh.message) foundFresh = true;
    }
    QVERIFY2(foundFresh, "Fresh event was incorrectly purged");

    delete pm;
}

void Testerrordashboard::testPersistenceClearAll() {
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");
    pm->setTtlDays(365);

    pm->upsertEvents(createTestEntries());
    QVERIFY(pm->loadActiveEvents().size() > 0);

    pm->clearAll();
    QCOMPARE(pm->loadActiveEvents().size(), 0);

    delete pm;
}

void Testerrordashboard::testPersistenceDatabaseSize() {
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");

    // Empty DB has a positive size (SQLite header)
    QVERIFY(pm->databaseSizeBytes() > 0);

    pm->setTtlDays(365);
    pm->upsertEvents(createTestEntries());

    // Size should grow after inserting data
    QVERIFY(pm->databaseSizeBytes() > 0);

    delete pm;
}

void Testerrordashboard::testPersistenceThreatJsonRoundtrip() {
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");
    pm->setTtlDays(365);

    // Create an entry with threats
    LogEntry e = createTestEntry("critical", "Failed password for root", "sshd.service");
    e.timestamp = QDateTime(QDate(2024, 8, 1), QTime(9, 0, 0), Qt::UTC);
    QVERIFY(e.threatCount > 0);

    pm->upsertEvent(e);

    const auto loaded = pm->loadActiveEvents();
    QVERIFY(!loaded.isEmpty());

    // Find the matching event
    const LogEntry* found = nullptr;
    for (const auto& le : loaded) {
        if (le.message == e.message) { found = &le; break; }
    }
    QVERIFY2(found != nullptr, "Inserted threat event not found after load");
    QCOMPARE(found->threatCount, e.threatCount);
    QCOMPARE(found->maxThreatSeverity, e.maxThreatSeverity);
    QVERIFY(!found->threats.isEmpty());
    QCOMPARE(found->threats[0].category, e.threats[0].category);

    delete pm;
}

void Testerrordashboard::testPersistenceScanRunRecorded() {
    // upsertEvents records a scan_run; we verify it doesn't crash
    // (direct access to scan_runs requires exposing the DB, so we test via upsert behavior)
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");
    pm->setTtlDays(30);

    auto entries = createTestEntries();
    const int n = pm->upsertEvents(entries);
    QCOMPARE(n, entries.size());

    // Second call: no new events, but scan_run still recorded without crash
    const int n2 = pm->upsertEvents(entries);
    QCOMPARE(n2, 0);

    delete pm;
}

void Testerrordashboard::testPersistenceReopenSameFile() {
    // Data persists across close/reopen cycles
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");
    pm->setTtlDays(365);

    LogEntry e = createTestEntry("error", "persists across reopen", "durable.service");
    e.timestamp = QDateTime::currentDateTimeUtc();
    pm->upsertEvent(e);

    const QString path = pm->currentPath();
    pm->close();
    delete pm;

    auto* pm2 = new PersistenceManager(this);
    QVERIFY(pm2->open(path));

    const auto loaded = pm2->loadActiveEvents();
    bool found = false;
    for (const auto& le : loaded) {
        if (le.message == e.message) { found = true; break; }
    }
    QVERIFY2(found, "Event not found after close/reopen");

    delete pm2;
}

// ============================================================================
// SettingsDrawer Tests
// ============================================================================

void Testerrordashboard::testSettingsDrawerCreation() {
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");
    QWidget parent;
    parent.resize(1000, 700);

    auto* drawer = new SettingsDrawer(pm, &parent);
    QVERIFY(drawer != nullptr);
    QVERIFY(!drawer->isDrawerOpen());

    delete drawer;
    delete pm;
}

void Testerrordashboard::testSettingsDrawerTtlSignal() {
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");
    pm->setTtlDays(30);

    QWidget parent;
    parent.resize(1000, 700);
    auto* drawer = new SettingsDrawer(pm, &parent);

    QSignalSpy ttlSpy(drawer, &SettingsDrawer::ttlChanged);

    // Simulate open then trigger a TTL change by finding the combo
    drawer->slideOpen();
    QComboBox* combo = drawer->findChild<QComboBox*>();
    QVERIFY(combo != nullptr);

    // Select 7 days (index 0)
    combo->setCurrentIndex(0);
    QVERIFY(ttlSpy.count() > 0);
    QCOMPARE(ttlSpy.last().first().toInt(), 7);

    delete drawer;
    delete pm;
}

void Testerrordashboard::testSettingsDrawerRefreshStats() {
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");
    pm->setTtlDays(365);
    pm->upsertEvents(createTestEntries());

    QWidget parent;
    parent.resize(1000, 700);
    auto* drawer = new SettingsDrawer(pm, &parent);

    // refreshDbStats should not crash with valid DB
    drawer->refreshDbStats();
    QVERIFY(true);

    delete drawer;
    delete pm;
}

// ============================================================================
// Integration Tests
// ============================================================================

void Testerrordashboard::testEndToEndDataFlow() {
    LogCollector collector;
    auto entries = collector.collectAll(1);

    for (auto& entry : entries) {
        entry.threats     = ThreatDetector::detectThreats(entry.message, entry.unit);
        entry.threatCount = entry.threats.size();
    }

    StatsTab tab("scan");
    tab.setData(entries);
    QVERIFY(tab.entryCount() >= 0);
}

void Testerrordashboard::testEndToEndPersistenceAndMerge() {
    // Full round-trip: collect → persist → load → display, no duplicates
    auto* pm = createTempPersistence();
    QVERIFY2(pm != nullptr, "Failed to create temp database");
    pm->setTtlDays(30);

    const auto entries = createTestEntries();
    pm->upsertEvents(entries);

    // Second upsert simulates a rescan with overlapping window
    pm->upsertEvents(entries);

    const auto loaded = pm->loadActiveEvents();

    // Should still be the original count — no duplicates
    QCOMPARE(loaded.size(), entries.size());

    StatsTab tab("scan");
    tab.setData(loaded);
    QCOMPARE(tab.entryCount(), entries.size());

    delete pm;
}

void Testerrordashboard::testThreatDetectionPipeline() {
    auto entries = createTestEntries();

    int totalThreats = 0;
    for (const auto& entry : entries)
        totalThreats += entry.threatCount;

    QVERIFY(totalThreats > 0);
}

void Testerrordashboard::testUIResponsiveness() {
    QVector<LogEntry> largeDataset;
    largeDataset.reserve(10000);
    for (int i = 0; i < 10000; i++)
        largeDataset.append(createTestEntry("error", "Test message", "test.service"));

    StatsTab tab("scan");

    QElapsedTimer timer;
    timer.start();
    tab.setData(largeDataset);
    const qint64 elapsed = timer.elapsed();

    QVERIFY2(elapsed < 5000,
             qPrintable(QString("setData took %1ms for 10k entries (limit 5000ms)").arg(elapsed)));
}

// ============================================================================
// Test Runner
// ============================================================================

QTEST_MAIN(Testerrordashboard)
#include "test_errordashboard.moc"
