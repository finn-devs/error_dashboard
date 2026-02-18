#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QColor>  // ADD THIS LINE

struct ThreatMatch {
    QString id;
    QString severity;      // critical, high, medium, low
    QString category;      // Authentication, Privilege, Network, etc.
    QString description;
    QString pattern;
};

struct LogEntry {
    QString source;        // journald or dmesg
    QDateTime timestamp;
    QString group;         // critical, error, warning
    int priority;          // 0-4 journald priority
    QString unit;
    QString pid;
    QString exe;
    QString cmdline;
    QString hostname;
    QString bootId;
    QString message;
    QString messageId;
    QString transport;
    QString cursor;
    
    // Security threat fields
    QVector<ThreatMatch> threats;
    int threatCount = 0;
    QString maxThreatSeverity;  // highest severity among all threats
    
    // Display fields (computed)
    QString severityLabel() const {
        if (group == "critical") return "⛔ CRITICAL";
        if (group == "error") return "🔴 ERROR";
        if (group == "warning") return "⚠️ WARNING";
        return "";
    }
    
    QString threatBadge() const {
        if (threatCount == 0) return "";
        QString icon = "🛡";
        if (maxThreatSeverity == "critical") icon = "🚨";
        else if (maxThreatSeverity == "high") icon = "⚠️";
        else if (maxThreatSeverity == "medium") icon = "⚡";
        else if (maxThreatSeverity == "low") icon = "ℹ️";
        return QString("%1 %2").arg(icon).arg(threatCount);
    }
    
    QColor severityColor() const {
        if (group == "critical") return QColor("#FF2D55");
        if (group == "error") return QColor("#FF6B35");
        if (group == "warning") return QColor("#FFD60A");
        return QColor("#888");
    }
    
    QColor severityBgColor() const {
        if (group == "critical") return QColor("#180008");
        if (group == "error") return QColor("#140800");
        if (group == "warning") return QColor("#131100");
        return QColor("#13131a");
    }
};

#endif // LOGENTRY_H
