#include "threatdetector.h"
#include <QRegularExpression>

QVector<ThreatDetector::ThreatPattern> ThreatDetector::getThreatPatterns() {
    return {
        {
            "auth_failure",
            {
                "authentication failure", "failed password", "invalid user",
                "failed login", "authentication error", "pam_unix.*auth.*failure",
                "failed publickey", "connection closed by.*\\[preauth\\]",
                "disconnected.*\\[preauth\\]"
            },
            "high", "Authentication", "Failed authentication attempt"
        },
        {
            "privilege_escalation",
            {
                "sudo:.*command not allowed", "sudo:.*incorrect password",
                "su:.*authentication failure", "granted sudo", "became root",
                "pkexec.*not authorized"
            },
            "critical", "Privilege", "Privilege escalation attempt or suspicious sudo activity"
        },
        {
            "suspicious_network",
            {
                "port scan", "SYN flood", "DDoS", "connection refused.*repeated",
                "firewall.*blocked", "iptables.*drop", "refused connect from",
                "possible break-in attempt"
            },
            "high", "Network", "Suspicious network activity detected"
        },
        {
            "filesystem_tampering",
            {
                "/etc/passwd.*modified", "/etc/shadow.*modified",
                "audit.*\\bwrite\\b.*/etc/", "changed.*/etc/sudoers",
                "inode.*changed", "file.*removed unexpectedly"
            },
            "critical", "Filesystem", "Critical system file modification"
        },
        {
            "service_crash",
            {
                "segmentation fault", "core dumped", "killed by signal",
                "abnormal termination", "panic", "oops", "bug:"
            },
            "medium", "Stability", "Service crash or kernel panic"
        },
        {
            "resource_exhaustion",
            {
                "out of memory", "oom-killer", "no space left", "disk.*full",
                "too many open files", "resource temporarily unavailable",
                "cannot allocate memory"
            },
            "high", "Resources", "Resource exhaustion detected"
        },
        {
            "selinux_violation",
            {
                "avc:.*denied", "selinux.*denied", "type=avc"
            },
            "medium", "SELinux", "SELinux policy violation"
        },
        {
            "malware_indicator",
            {
                "rootkit", "trojan", "malware", "backdoor",
                "suspicious.*binary", "unknown.*process.*root"
            },
            "critical", "Malware", "Potential malware or rootkit detected"
        }
    };
}

QVector<ThreatMatch> ThreatDetector::detectThreats(const QString& message, const QString& unit) {
    QVector<ThreatMatch> threats;
    QString msgLower = message.toLower();
    
    for (const auto& pattern : getThreatPatterns()) {
        for (const QString& patternStr : pattern.patterns) {
            QRegularExpression regex(patternStr, QRegularExpression::CaseInsensitiveOption);
            if (regex.match(msgLower).hasMatch()) {
                threats.append({
                    pattern.id,
                    pattern.severity,
                    pattern.category,
                    pattern.description,
                    patternStr
                });
                break; // One match per threat type
            }
        }
    }
    
    return threats;
}
