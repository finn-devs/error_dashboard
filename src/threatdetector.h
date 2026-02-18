#ifndef THREATDETECTOR_H
#define THREATDETECTOR_H

#include "logentry.h"
#include <QVector>
#include <QRegularExpression>

class ThreatDetector {
public:
    static QVector<ThreatMatch> detectThreats(const QString& message, const QString& unit);
    
private:
    struct ThreatPattern {
        QString id;
        QVector<QString> patterns;
        QString severity;
        QString category;
        QString description;
    };
    
    static QVector<ThreatPattern> getThreatPatterns();
};

#endif // THREATDETECTOR_H
