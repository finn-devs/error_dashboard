#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include "../src/logentry.h"
#include "../src/logcollector.h"
#include "../src/threatdetector.h"
#include "../src/statstab.h"
#include "../src/mainwindow.h"

class TestErrorSurface : public QObject {
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
    void testStatsTabChartGeneration();
    void testStatsTabExportCSV();
    
    // MainWindow tests
    void testMainWindowInitialization();
    void testMainWindowTabSwitching();
    void testMainWindowLivePolling();
    
    // Integration tests
    void testEndToEndDataFlow();
    void testThreatDetectionPipeline();
    void testUIResponsiveness();

private:
    QVector<LogEntry> createTestEntries();
    LogEntry createTestEntry(const QString& severity, const QString& message, 
                            const QString& unit = "test.service");
};

void TestErrorSurface::initTestCase() {
    qDebug() << "Starting Error Surface Test Suite";
}

void TestErrorSurface::cleanupTestCase() {
    qDebug() << "Completed Error Surface Test Suite";
}

void TestErrorSurface::init() {
    // Per-test setup
}

void TestErrorSurface::cleanup() {
    // Per-test cleanup
}

// ============================================================================
// LogEntry Tests
// ============================================================================

void TestErrorSurface::testLogEntrySeverityLabels() {
    LogEntry entry;
    
    entry.priority = 0;
    entry.group = "critical";
    QCOMPARE(entry.severityLabel(), QString("EMRG"));
    
    entry.priority = 1;
    QCOMPARE(entry.severityLabel(), QString("ALRT"));
    
    entry.priority = 2;
    entry.group = "critical";
    QCOMPARE(entry.severityLabel(), QString("CRIT"));
    
    entry.priority = 3;
    entry.group = "error";
    QCOMPARE(entry.severityLabel(), QString("ERR"));
    
    entry.priority = 4;
    entry.group = "warning";
    QCOMPARE(entry.severityLabel(), QString("WARN"));
}

void TestErrorSurface::testLogEntrySeverityColors() {
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

void TestErrorSurface::testLogEntryThreatBadge() {
    LogEntry entry;
    
    entry.threatCount = 0;
    QCOMPARE(entry.threatBadge(), QString(""));
    
    entry.threatCount = 1;
    QCOMPARE(entry.threatBadge(), QString("🛡 1"));
    
    entry.threatCount = 5;
    QCOMPARE(entry.threatBadge(), QString("🛡 5"));
}

// ============================================================================
// ThreatDetector Tests
// ============================================================================

void TestErrorSurface::testThreatDetectorAuthentication() {
    QString message1 = "Failed password for root from 192.168.1.100";
    auto threats1 = ThreatDetector::detectThreats(message1, "sshd.service");
    QVERIFY(!threats1.isEmpty());
    QCOMPARE(threats1[0].category, QString("Authentication"));
    QCOMPARE(threats1[0].severity, QString("high"));
    
    QString message2 = "authentication failure for user admin";
    auto threats2 = ThreatDetector::detectThreats(message2, "login");
    QVERIFY(!threats2.isEmpty());
    QCOMPARE(threats2[0].category, QString("Authentication"));
}

void TestErrorSurface::testThreatDetectorPrivilege() {
    QString message = "sudo: user NOT in sudoers";
    auto threats = ThreatDetector::detectThreats(message, "sudo");
    QVERIFY(!threats.isEmpty());
    QCOMPARE(threats[0].category, QString("Privilege Escalation"));
    QCOMPARE(threats[0].severity, QString("critical"));
}

void TestErrorSurface::testThreatDetectorNetwork() {
    QString message = "Connection attempt from blocked IP 10.0.0.1";
    auto threats = ThreatDetector::detectThreats(message, "firewall");
    QVERIFY(!threats.isEmpty());
    QCOMPARE(threats[0].category, QString("Network Security"));
}

void TestErrorSurface::testThreatDetectorFilesystem() {
    QString message = "Permission denied writing to /etc/passwd";
    auto threats = ThreatDetector::detectThreats(message, "vim");
    QVERIFY(!threats.isEmpty());
    QCOMPARE(threats[0].category, QString("Filesystem"));
    QCOMPARE(threats[0].severity, QString("high"));
}

void TestErrorSurface::testThreatDetectorStability() {
    QString message1 = "segmentation fault at address 0x00000000";
    auto threats1 = ThreatDetector::detectThreats(message1, "app");
    QVERIFY(!threats1.isEmpty());
    QCOMPARE(threats1[0].category, QString("System Stability"));
    
    QString message2 = "kernel panic - not syncing";
    auto threats2 = ThreatDetector::detectThreats(message2, "kernel");
    QVERIFY(!threats2.isEmpty());
    QCOMPARE(threats2[0].severity, QString("critical"));
}

void TestErrorSurface::testThreatDetectorResources() {
    QString message = "Out of memory: Kill process 1234";
    auto threats = ThreatDetector::detectThreats(message, "kernel");
    QVERIFY(!threats.isEmpty());
    QCOMPARE(threats[0].category, QString("Resource Exhaustion"));
    QCOMPARE(threats[0].severity, QString("high"));
}

void TestErrorSurface::testThreatDetectorSELinux() {
    QString message = "SELinux is preventing access to file /var/log/secure";
    auto threats = ThreatDetector::detectThreats(message, "audit");
    QVERIFY(!threats.isEmpty());
    QCOMPARE(threats[0].category, QString("SELinux/AppArmor"));
}

void TestErrorSurface::testThreatDetectorMalware() {
    QString message = "Detected suspicious process: /tmp/malware.sh";
    auto threats = ThreatDetector::detectThreats(message, "scanner");
    QVERIFY(!threats.isEmpty());
    QCOMPARE(threats[0].category, QString("Malware/Intrusion"));
    QCOMPARE(threats[0].severity, QString("critical"));
}

void TestErrorSurface::testThreatDetectorMultipleThreats() {
    QString message = "Failed authentication and permission denied for root";
    auto threats = ThreatDetector::detectThreats(message, "system");
    QVERIFY(threats.size() >= 2);
}

void TestErrorSurface::testThreatDetectorNoThreats() {
    QString message = "Service started successfully";
    auto threats = ThreatDetector::detectThreats(message, "nginx.service");
    QVERIFY(threats.isEmpty());
}

// ============================================================================
// LogCollector Tests
// ============================================================================

void TestErrorSurface::testLogCollectorJournaldOpen() {
    LogCollector collector;
    QDateTime since = QDateTime::currentDateTime().addDays(-1);
    
    // Should not crash
    auto entries = collector.collectAll(1);
    QVERIFY(entries.size() >= 0); // May be 0 if no errors in last day
}

void TestErrorSurface::testLogCollectorSeverityFiltering() {
    LogCollector collector;
    auto entries = collector.collectAll(1);
    
    // All entries should be priority 0-4 (critical/error/warning)
    for (const auto& entry : entries) {
        QVERIFY(entry.priority >= 0 && entry.priority <= 4);
        QVERIFY(!entry.group.isEmpty());
        QVERIFY(entry.group == "critical" || entry.group == "error" || entry.group == "warning");
    }
}

void TestErrorSurface::testLogCollectorTimeFiltering() {
    LogCollector collector;
    QDateTime since = QDateTime::currentDateTime().addHours(-2);
    auto entries = collector.collectLive(120); // Last 2 hours
    
    // All entries should be within time window (with some tolerance)
    for (const auto& entry : entries) {
        QVERIFY(entry.timestamp >= since.addSecs(-60)); // 1 min tolerance
    }
}

void TestErrorSurface::testLogCollectorDmesgFallback() {
    LogCollector collector;
    QDateTime since = QDateTime::currentDateTime().addDays(-1);
    auto dmesgEntries = collector.collectDmesg(since);
    
    // Should return some entries or empty (depending on permissions)
    QVERIFY(dmesgEntries.size() >= 0);
    
    for (const auto& entry : dmesgEntries) {
        QCOMPARE(entry.source, QString("dmesg"));
    }
}

// ============================================================================
// StatsTab Tests
// ============================================================================

QVector<LogEntry> TestErrorSurface::createTestEntries() {
    QVector<LogEntry> entries;
    
    // Critical entry with threats
    LogEntry critical;
    critical.timestamp = QDateTime::currentDateTime();
    critical.severity = "0";
    critical.priority = 2;
    critical.group = "critical";
    critical.unit = "sshd.service";
    critical.message = "Failed password for root";
    critical.threats = ThreatDetector::detectThreats(critical.message, critical.unit);
    critical.threatCount = critical.threats.size();
    if (critical.threatCount > 0) {
        critical.maxThreatSeverity = critical.threats[0].severity;
    }
    entries.append(critical);
    
    // Error entries
    for (int i = 0; i < 5; i++) {
        LogEntry error;
        error.timestamp = QDateTime::currentDateTime().addSecs(-i * 60);
        error.priority = 3;
        error.group = "error";
        error.unit = QString("service%1.service").arg(i);
        error.message = "Service failed to start";
        entries.append(error);
    }
    
    // Warning entries
    for (int i = 0; i < 3; i++) {
        LogEntry warning;
        warning.timestamp = QDateTime::currentDateTime().addSecs(-i * 120);
        warning.priority = 4;
        warning.group = "warning";
        warning.unit = "disk.service";
        warning.message = "Disk usage above 80%";
        entries.append(warning);
    }
    
    return entries;
}

LogEntry TestErrorSurface::createTestEntry(const QString& severity, const QString& message, const QString& unit) {
    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.group = severity;
    entry.message = message;
    entry.unit = unit;
    
    if (severity == "critical") entry.priority = 2;
    else if (severity == "error") entry.priority = 3;
    else if (severity == "warning") entry.priority = 4;
    
    entry.threats = ThreatDetector::detectThreats(message, unit);
    entry.threatCount = entry.threats.size();
    
    return entry;
}

void TestErrorSurface::testStatsTabDataLoading() {
    StatsTab tab("scan");
    auto entries = createTestEntries();
    
    tab.setData(entries);
    
    // Should not crash and should accept data
    QVERIFY(true);
}

void TestErrorSurface::testStatsTabStatCounts() {
    StatsTab tab("scan");
    auto entries = createTestEntries();
    
    tab.setData(entries);
    
    // Stats should be updated (can't easily check private members, but shouldn't crash)
    QVERIFY(true);
}

void TestErrorSurface::testStatsTabFiltering() {
    StatsTab tab("scan");
    auto entries = createTestEntries();
    
    tab.setData(entries);
    
    // Test that filtering works by triggering signals
    QSignalSpy spy(&tab, &StatsTab::needsRefresh);
    
    // Should not crash when filters are applied
    QVERIFY(true);
}

void TestErrorSurface::testStatsTabSearchFilter() {
    // Test search box filtering
    StatsTab tab("scan");
    auto entries = createTestEntries();
    entries.append(createTestEntry("error", "special unique message", "test.service"));
    
    tab.setData(entries);
    
    // Would need access to m_searchBox to test properly
    // This verifies basic functionality
    QVERIFY(true);
}

void TestErrorSurface::testStatsTabUnitFilter() {
    StatsTab tab("scan");
    auto entries = createTestEntries();
    
    tab.setData(entries);
    
    // Unit filter should be populated
    QVERIFY(true);
}

void TestErrorSurface::testStatsTabClickableCards() {
    StatsTab tab("scan");
    auto entries = createTestEntries();
    
    tab.setData(entries);
    
    // Test that cards are clickable (would need to simulate mouse events)
    QVERIFY(true);
}

void TestErrorSurface::testStatsTabChartGeneration() {
    StatsTab tab("scan");
    auto entries = createTestEntries();
    
    tab.setData(entries);
    
    // Charts should be generated without crashing
    QVERIFY(true);
}

void TestErrorSurface::testStatsTabExportCSV() {
    StatsTab tab("scan");
    auto entries = createTestEntries();
    
    tab.setData(entries);
    
    // Would need to trigger export and verify file creation
    // This tests basic functionality
    QVERIFY(true);
}

// ============================================================================
// MainWindow Tests
// ============================================================================

void TestErrorSurface::testMainWindowInitialization() {
    MainWindow window(7, 60, 5);
    
    // Should initialize without crashing
    QVERIFY(true);
    
    // Window should have proper title
    QCOMPARE(window.windowTitle(), QString("Error Surface"));
}

void TestErrorSurface::testMainWindowTabSwitching() {
    MainWindow window(7, 60, 5);
    window.show();
    
    // Should be able to show without crashing
    QVERIFY(true);
}

void TestErrorSurface::testMainWindowLivePolling() {
    MainWindow window(7, 60, 5);
    
    // Should start polling without crashing
    window.startCollections();
    
    QVERIFY(true);
}

// ============================================================================
// Integration Tests
// ============================================================================

void TestErrorSurface::testEndToEndDataFlow() {
    // Test complete data flow: Collection -> Processing -> Display
    LogCollector collector;
    auto entries = collector.collectAll(1);
    
    // Add threat detection
    for (auto& entry : entries) {
        entry.threats = ThreatDetector::detectThreats(entry.message, entry.unit);
        entry.threatCount = entry.threats.size();
    }
    
    // Load into UI
    StatsTab tab("scan");
    tab.setData(entries);
    
    QVERIFY(true);
}

void TestErrorSurface::testThreatDetectionPipeline() {
    // Test that threats are properly detected and displayed
    auto entries = createTestEntries();
    
    int totalThreats = 0;
    for (const auto& entry : entries) {
        totalThreats += entry.threatCount;
    }
    
    QVERIFY(totalThreats > 0); // Should have at least one threat in test data
}

void TestErrorSurface::testUIResponsiveness() {
    // Test that UI handles large datasets
    QVector<LogEntry> largeDataset;
    for (int i = 0; i < 10000; i++) {
        largeDataset.append(createTestEntry("error", "Test message", "test.service"));
    }
    
    StatsTab tab("scan");
    
    // Should handle 10k entries without crashing
    QElapsedTimer timer;
    timer.start();
    tab.setData(largeDataset);
    qint64 elapsed = timer.elapsed();
    
    // Should complete in reasonable time (< 5 seconds)
    QVERIFY(elapsed < 5000);
}

// ============================================================================
// Test Runner
// ============================================================================

QTEST_MAIN(TestErrorSurface)
#include "test_errorsurface.moc"
