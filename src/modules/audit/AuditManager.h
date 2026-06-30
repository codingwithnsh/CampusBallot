#pragma once

#include <QObject>
#include <QTimer>
#include "src/core/Models.h"

namespace Ballot::Audit {

class AuditManager : public QObject {
    Q_OBJECT
public:
    static AuditManager& instance();

    void initialize();
    void log(Core::AuditAction action, const QString& details, const QString& userId = QString());
    void log(const Core::AuditLogEntry& entry);

    QList<Core::AuditLogEntry> getLogs(const QDateTime& from = QDateTime(), const QDateTime& to = QDateTime()) const;
    QList<Core::AuditLogEntry> getLogsByUser(const QString& userId) const;
    QList<Core::AuditLogEntry> getLogsByAction(Core::AuditAction action) const;
    int getLogCount() const;

    bool exportLogs(const QString& filePath, const QString& format = "json");
    bool verifyLogIntegrity() const;
    void enableImmutability(bool enable);
    bool isImmutable() const;
    bool isInitialized() const;

signals:
    void logAdded(const Core::AuditLogEntry& entry);
    void integrityViolation(const QString& details);

private:
    AuditManager();
    void addToChain(Core::AuditLogEntry& entry);
    QByteArray calculateEntryHash(const Core::AuditLogEntry& entry, const QByteArray& previousHash) const;

    bool m_immutable = true;
    QByteArray m_chainHash;
    bool m_initialized = false; // New member variable
};

} // namespace Ballot::Audit
