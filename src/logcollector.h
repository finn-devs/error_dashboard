#ifndef LOGCOLLECTOR_H
#define LOGCOLLECTOR_H

#include "logentry.h"
#include <QVector>
#include <QDateTime>
#include <QObject>

class LogCollector : public QObject {
    Q_OBJECT
    
public:
    explicit LogCollector(QObject* parent = nullptr);
    
    // Collect logs from journald and dmesg
    QVector<LogEntry> collectAll(int lookbackDays = 7);
    QVector<LogEntry> collectLive(int windowMinutes = 60);
    
signals:
    void collectionProgress(int current, int total);
    void collectionComplete(int entryCount);
    void collectionError(const QString& error);
    
private:
    QVector<LogEntry> collectJournald(const QDateTime& since, int maxEntries = 10000);
    QVector<LogEntry> collectDmesg(const QDateTime& since);
    
    LogEntry parseJournaldEntry(const QByteArray& jsonLine, const QDateTime& since);
    LogEntry parseDmesgLine(const QString& line, const QDateTime& since);
    
    QString groupForPriority(int priority);
};

#endif // LOGCOLLECTOR_H
