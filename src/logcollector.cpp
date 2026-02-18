#include "logcollector.h"
#include "threatdetector.h"
#include <unistd.h>  // ADD THIS LINE for getuid()
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <systemd/sd-journal.h>

LogCollector::LogCollector(QObject* parent) : QObject(parent) {}

QString LogCollector::groupForPriority(int priority) {
    if (priority <= 2) return "critical";
    if (priority == 3) return "error";
    if (priority == 4) return "warning";
    return "";
}

QVector<LogEntry> LogCollector::collectAll(int lookbackDays) {
    QDateTime since = QDateTime::currentDateTimeUtc().addDays(-lookbackDays);
    QVector<LogEntry> entries = collectJournald(since);
    entries.append(collectDmesg(since));
    
    // Sort by timestamp descending
    std::sort(entries.begin(), entries.end(), [](const LogEntry& a, const LogEntry& b) {
        return a.timestamp > b.timestamp;
    });
    
    emit collectionComplete(entries.size());
    return entries;
}

QVector<LogEntry> LogCollector::collectLive(int windowMinutes) {
    QDateTime since = QDateTime::currentDateTimeUtc().addSecs(-windowMinutes * 60);
    QVector<LogEntry> entries = collectJournald(since, 5000);
    entries.append(collectDmesg(since));
    
    std::sort(entries.begin(), entries.end(), [](const LogEntry& a, const LogEntry& b) {
        return a.timestamp > b.timestamp;
    });
    
    emit collectionComplete(entries.size());
    return entries;
}

QVector<LogEntry> LogCollector::collectJournald(const QDateTime& since, int maxEntries) {
    QVector<LogEntry> entries;
    
     qDebug() << "collectJournald called, since:" << since;

    sd_journal* j = nullptr;
    int ret = sd_journal_open(&j, SD_JOURNAL_LOCAL_ONLY);
    qDebug() << "sd_journal_open returned:" << ret;
    
    if (ret < 0) {
        qDebug() << "sd_journal_open FAILED.";
        emit collectionError("Failed to open systemd journal");
        return entries;
    }
    qDebug() << "Journal opened successfully";
     // Filter: priority 0-4 (emergency through warning)
    qDebug() << "Skipping priority filters - will filter in loop";
    // Seek to timestamp
    sd_journal_seek_realtime_usec(j, since.toSecsSinceEpoch() * 1000000ULL);
        qDebug() << "Starting to read entries, maxEntries:" << maxEntries;
    
    // Read entries (newest first via SD_JOURNAL_FOREACH_BACKWARDS would require seeking to end)
    // For simplicity, read forward and reverse in memory
    QVector<LogEntry> tempEntries;
    int loopCount = 0;
    
    while (sd_journal_next(j) > 0 && tempEntries.size() < maxEntries) {
        loopCount++;
        LogEntry entry;
        entry.source = "journald";
        
        // Timestamp
        uint64_t usec;
        if (sd_journal_get_realtime_usec(j, &usec) >= 0) {
            entry.timestamp = QDateTime::fromSecsSinceEpoch(usec / 1000000, Qt::UTC);
        } else {
            entry.timestamp = QDateTime::currentDateTimeUtc();
        }
        
        // Skip if before since
        if (entry.timestamp < since) continue;
        
        // Priority
        const char* priority_str;
        size_t len;
        int prio = 7;
        if (sd_journal_get_data(j, "PRIORITY", (const void**)&priority_str, &len) >= 0) {
            QString prioVal = QString::fromUtf8(priority_str, len).section('=', 1);
            prio = prioVal.toInt();
        }
        entry.priority = prio;
        entry.group = groupForPriority(prio);
        
        if (entry.group.isEmpty()) continue; // Skip if not 0-4
        
        // Message
        const char* msg;
        if (sd_journal_get_data(j, "MESSAGE", (const void**)&msg, &len) >= 0) {
            entry.message = QString::fromUtf8(msg, len).section('=', 1);
        }
        
        // Unit
        const char* unit;
        if (sd_journal_get_data(j, "_SYSTEMD_UNIT", (const void**)&unit, &len) >= 0) {
            entry.unit = QString::fromUtf8(unit, len).section('=', 1);
        } else if (sd_journal_get_data(j, "SYSLOG_IDENTIFIER", (const void**)&unit, &len) >= 0) {
            entry.unit = QString::fromUtf8(unit, len).section('=', 1);
        } else {
            entry.unit = "unknown";
        }
        
        // PID
        const char* pid;
        if (sd_journal_get_data(j, "_PID", (const void**)&pid, &len) >= 0) {
            entry.pid = QString::fromUtf8(pid, len).section('=', 1);
        } else if (sd_journal_get_data(j, "SYSLOG_PID", (const void**)&pid, &len) >= 0) {
            entry.pid = QString::fromUtf8(pid, len).section('=', 1);
        }
        
        // Executable
        const char* exe;
        if (sd_journal_get_data(j, "_EXE", (const void**)&exe, &len) >= 0) {
            entry.exe = QString::fromUtf8(exe, len).section('=', 1);
        }
        
        // Cmdline
        const char* cmdline;
        if (sd_journal_get_data(j, "_CMDLINE", (const void**)&cmdline, &len) >= 0) {
            entry.cmdline = QString::fromUtf8(cmdline, len).section('=', 1);
        }
        
        // Hostname
        const char* host;
        if (sd_journal_get_data(j, "_HOSTNAME", (const void**)&host, &len) >= 0) {
            entry.hostname = QString::fromUtf8(host, len).section('=', 1);
        }
        
        // Boot ID
        const char* boot;
        if (sd_journal_get_data(j, "_BOOT_ID", (const void**)&boot, &len) >= 0) {
            QString bootId = QString::fromUtf8(boot, len).section('=', 1);
            entry.bootId = bootId.left(8);
        }
        
        // Message ID
        const char* msgid;
        if (sd_journal_get_data(j, "MESSAGE_ID", (const void**)&msgid, &len) >= 0) {
            entry.messageId = QString::fromUtf8(msgid, len).section('=', 1);
        }
        
        // Transport
        const char* transport;
        if (sd_journal_get_data(j, "_TRANSPORT", (const void**)&transport, &len) >= 0) {
            entry.transport = QString::fromUtf8(transport, len).section('=', 1);
        }
        
        // Cursor
        char* cursor;
        if (sd_journal_get_cursor(j, &cursor) >= 0) {
            entry.cursor = QString::fromUtf8(cursor);
            free(cursor);
        }
        
        // Detect threats
        entry.threats = ThreatDetector::detectThreats(entry.message, entry.unit);
        entry.threatCount = entry.threats.size();
        
        if (entry.threatCount > 0) {
            // Find max severity
            QMap<QString, int> sevOrder = {{"critical", 0}, {"high", 1}, {"medium", 2}, {"low", 3}};
            int minOrder = 99;
            for (const auto& threat : entry.threats) {
                int order = sevOrder.value(threat.severity, 99);
                if (order < minOrder) {
                    minOrder = order;
                    entry.maxThreatSeverity = threat.severity;
                }
            }
        }
        
        tempEntries.append(entry);
    }
        qDebug() << "Loop completed. Ran" << loopCount << "times, collected" << tempEntries.size() << "entries";
    sd_journal_close(j);
    
    // Reverse to get newest first
    std::reverse(tempEntries.begin(), tempEntries.end());
    
    return tempEntries;
}

QVector<LogEntry> LogCollector::collectDmesg(const QDateTime& since) {
    QVector<LogEntry> entries;
    
    QProcess process;
    QStringList args = {"--level=emerg,alert,crit,err,warn", "--time-format=iso"};
    
    // Try with sudo first
    bool useSudo = (getuid() != 0);
    if (useSudo) {
        process.start("sudo", QStringList() << "-n" << "dmesg" << args);
    } else {
        process.start("dmesg", args);
    }
    
    if (!process.waitForFinished(15000)) {
        LogEntry error;
        error.source = "dmesg";
        error.timestamp = QDateTime::currentDateTimeUtc();
        error.group = "warning";
        error.priority = 4;
        error.unit = "dmesg-collector";
        error.message = "[dmesg timeout] Failed to collect kernel logs. Run with sudo or add to 'adm' group.";
        error.transport = "collector";
        entries.append(error);
        return entries;
    }
    
    if (process.exitCode() != 0) {
        LogEntry error;
        error.source = "dmesg";
        error.timestamp = QDateTime::currentDateTimeUtc();
        error.group = "warning";
        error.priority = 4;
        error.unit = "dmesg-collector";
        error.message = QString("[dmesg unavailable] %1. Add to 'adm' group: sudo usermod -aG adm $USER")
                            .arg(QString::fromUtf8(process.readAllStandardError()).trimmed());
        error.transport = "collector";
        entries.append(error);
        return entries;
    }
    
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QRegularExpression isoPattern(R"(^(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}[.,]\d+[+-]\d{4})\s+(.+)$)");
    QRegularExpression levelPattern(R"(^\[\s*(\w+)\s*\]\s*(.*)$)");
    
    QMap<QString, int> levelMap = {
        {"emerg", 0}, {"alert", 1}, {"crit", 2}, {"err", 3}, {"warn", 4}
    };
    
    for (const QString& line : lines) {
        QRegularExpressionMatch match = isoPattern.match(line);
        if (!match.hasMatch()) continue;
        
        QString tsStr = match.captured(1);
        QString rest = match.captured(2);
        
        // Parse timestamp
        tsStr.replace(',', '.');
        QDateTime timestamp = QDateTime::fromString(tsStr, Qt::ISODate);
        if (!timestamp.isValid() || timestamp < since) continue;
        
        // Extract level
        QRegularExpressionMatch levelMatch = levelPattern.match(rest);
        QString levelStr = "err";
        QString message = rest;
        if (levelMatch.hasMatch()) {
            levelStr = levelMatch.captured(1).toLower();
            message = levelMatch.captured(2);
        }
        
        int priority = levelMap.value(levelStr, 3);
        QString group = groupForPriority(priority);
        
        LogEntry entry;
        entry.source = "dmesg";
        entry.timestamp = timestamp;
        entry.group = group;
        entry.priority = priority;
        entry.unit = "kernel";
        entry.message = message;
        entry.transport = "kernel";
        
        // Detect threats
        entry.threats = ThreatDetector::detectThreats(entry.message, entry.unit);
        entry.threatCount = entry.threats.size();
        
        if (entry.threatCount > 0) {
            QMap<QString, int> sevOrder = {{"critical", 0}, {"high", 1}, {"medium", 2}, {"low", 3}};
            int minOrder = 99;
            for (const auto& threat : entry.threats) {
                int order = sevOrder.value(threat.severity, 99);
                if (order < minOrder) {
                    minOrder = order;
                    entry.maxThreatSeverity = threat.severity;
                }
            }
        }
        
        entries.append(entry);
    }
    
    return entries;
}
