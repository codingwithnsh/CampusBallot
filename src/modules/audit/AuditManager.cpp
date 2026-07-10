#include "AuditManager.h"
#include "src/core/SystemManager.h"
#include "src/core/Utils.h" // For Core::SystemInfo and Core::IdGenerator
#include "src/modules/security/HashProvider.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QUuid> // Still included for QJsonDocument::Indented or if needed elsewhere
#include <QDebug>
#include <algorithm>

namespace Ballot::Audit {
namespace {

QByteArray csvCell(const QString& value) {
    QString safe = value;
    if (!safe.isEmpty()) {
        const QChar first = safe.at(0);
        if (first == '=' || first == '+' || first == '-' || first == '@' || first == '\t' || first == '\r') {
            safe.prepend('\'');
        }
    }
    safe.replace("\"", "\"\"");
    return QString("\"%1\"").arg(safe).toUtf8();
}

} // namespace

AuditManager& AuditManager::instance() {
    static AuditManager inst;
    return inst;
}

AuditManager::AuditManager() : m_immutable(true), m_initialized(false) {
    // Constructor should be minimal. Initialization logic in initialize().
}

/**
 * @brief Initializes the AuditManager, loading settings and the last chain hash from storage.
 */
void AuditManager::initialize() {
    if (m_initialized) {
        qDebug() << "AuditManager: Already initialized.";
        return;
    }

    qDebug() << "AuditManager: Initializing...";
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "AuditManager: Storage not available during initialization. Audit functionality may be limited.";
        m_initialized = false;
        return;
    }

    auto settingsOpt = storage->getSystemSettings();
    if (settingsOpt) {
        m_immutable = settingsOpt->auditAllActions;
        qDebug() << "AuditManager: Immutability set to" << m_immutable << "from system settings.";
    } else {
        qWarning() << "AuditManager: Could not load system settings. Immutability defaults to" << m_immutable;
    }

    // Load all logs to reconstruct the chain hash
    QList<Core::AuditLogEntry> allLogs = storage->getAuditLogs(QDateTime(), QDateTime()); // Get all logs
    if (!allLogs.isEmpty()) {
        // Storage returns newest first for audit logs. The latest persisted hash
        // is the current chain head.
        m_chainHash = allLogs.first().hash;
        qDebug() << "AuditManager: Initial chain hash set from latest log entry.";
    } else {
        m_chainHash = QByteArray(); // Genesis hash for an empty chain
        qDebug() << "AuditManager: No existing audit logs found. Initializing with empty chain hash.";
    }

    m_initialized = true;
    qInfo() << "AuditManager: Initialization complete.";
}

bool AuditManager::isInitialized() const {
    return m_initialized;
}

/**
 * @brief Logs an audit action with provided details.
 * @param action The type of audit action.
 * @param details A detailed description of the action.
 * @param userId The ID of the user performing the action (optional).
 * @param machineId The ID of the machine where the action occurred (optional, defaults to current).
 */
void AuditManager::log(Core::AuditAction action, const QString& details, const QString& userId) {
    Core::AuditLogEntry entry;
    entry.id = Core::IdGenerator::generateId();
    entry.timestamp = QDateTime::currentDateTime();
    entry.userId = userId;
    // Attempt to get userName if userId is provided
    if (!userId.isEmpty()) {
        auto* storage = Core::SystemManager::instance().storage();
        if (storage) {
            auto userOpt = storage->getUser(userId);
            if (userOpt) {
                entry.userName = userOpt->name;
            } else {
                qWarning() << "AuditManager: User with ID" << userId << "not found for audit log. Using ID as name.";
                entry.userName = userId;
            }
        } else {
            qCritical() << "AuditManager: Storage not available to fetch user name for audit log.";
            entry.userName = userId; // Fallback
        }
    } else {
        entry.userName = "System/Anonymous";
    }

    entry.action = action;
    entry.details = details;
    entry.ipAddress = Core::SystemInfo::getIpAddress(); // Capture IP address
    entry.machineId = Core::SystemInfo::getMachineId();
    entry.isImmutable = m_immutable;

    log(entry); // Delegate to the overload that takes an AuditLogEntry
}

/**
 * @brief Logs a pre-constructed audit log entry.
 * @param entry The AuditLogEntry object to log.
 */
void AuditManager::log(Core::AuditLogEntry& entry) { // Pass by reference to update hash
    if (!m_initialized) {
        qCritical() << "AuditManager: Not initialized. Cannot log entry.";
        return;
    }

    // Ensure essential fields are populated if not already
    if (entry.id.isEmpty()) entry.id = Core::IdGenerator::generateId();
    if (!entry.timestamp.isValid()) entry.timestamp = QDateTime::currentDateTime();
    if (entry.machineId.isEmpty()) entry.machineId = Core::SystemInfo::getMachineId();
    if (entry.ipAddress.isEmpty()) entry.ipAddress = Core::SystemInfo::getIpAddress();
    entry.isImmutable = m_immutable; // Ensure immutability flag is consistent with manager's state

    addToChain(entry); // Calculate and set hash

    auto* storage = Core::SystemManager::instance().storage();
    if (storage) {
        if (!storage->logAction(entry)) {
            qCritical() << "AuditManager: Failed to persist audit log entry to storage:" << entry.details;
        } else {
            qDebug() << "AuditManager: Audit log persisted:" << entry.details;
        }
    } else {
        qCritical() << "AuditManager: Storage not available. Audit log entry not persisted:" << entry.details;
    }

    emit logAdded(entry);
}

QList<Core::AuditLogEntry> AuditManager::getLogs(const QDateTime& from, const QDateTime& to) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "AuditManager: Storage not available. Cannot retrieve audit logs.";
        return {};
    }
    return storage->getAuditLogs(from, to);
}

QList<Core::AuditLogEntry> AuditManager::getLogsByUser(const QString& userId) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "AuditManager: Storage not available. Cannot retrieve audit logs by user.";
        return {};
    }
    return storage->getAuditLogsByUser(userId);
}

QList<Core::AuditLogEntry> AuditManager::getLogsByAction(Core::AuditAction action) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "AuditManager: Storage not available. Cannot retrieve audit logs by action.";
        return {};
    }
    return storage->getAuditLogsByAction(action);
}

int AuditManager::getLogCount() const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "AuditManager: Storage not available. Cannot get audit log count.";
        return 0;
    }
    return storage->getAuditLogCount();
}

bool AuditManager::exportLogs(const QString& filePath, const QString& format) {
    QList<Core::AuditLogEntry> logs = getLogs(QDateTime(), QDateTime()); // Get all logs
    if (logs.isEmpty()) {
        qInfo() << "AuditManager: No audit logs to export.";
        return true;
    }

    QByteArray data;
    if (format.toLower() == "json") {
        QJsonArray arr;
        for (const auto& log : logs) {
            arr.append(log.toJson());
        }
        data = QJsonDocument(arr).toJson(QJsonDocument::Indented);
    } else if (format.toLower() == "csv") {
        data.append("ID,Timestamp,User ID,User Name,Action,Details,IP Address,Machine ID,Hash,Immutable\n");
        for (const auto& log : logs) {
            QByteArrayList row;
            row << csvCell(log.id)
                << csvCell(log.timestamp.toString(Qt::ISODate))
                << csvCell(log.userId)
                << csvCell(log.userName)
                << csvCell(QString::number(static_cast<int>(log.action)))
                << csvCell(log.details)
                << csvCell(log.ipAddress)
                << csvCell(log.machineId)
                << csvCell(QString(log.hash.toHex()))
                << csvCell(log.isImmutable ? "1" : "0");
            data.append(row.join(','));
            data.append('\n');
        }
    } else {
        qWarning() << "AuditManager: Unsupported export format:" << format;
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qCritical() << "AuditManager: Failed to open file for export:" << filePath << "-" << file.errorString();
        return false;
    }

    qint64 bytesWritten = file.write(data);
    if (bytesWritten == -1) {
        qCritical() << "AuditManager: Failed to write data to export file:" << filePath << "-" << file.errorString();
        file.close();
        return false;
    }
    file.close();
    qInfo() << "AuditManager: Successfully exported" << logs.size() << "logs to" << filePath;
    return true;
}

/**
 * @brief Verifies the integrity of the audit log chain.
 * Iterates through all immutable logs and recalculates their hashes to ensure the chain is unbroken.
 * @return True if the integrity is intact, false otherwise.
 */
bool AuditManager::verifyLogIntegrity() const {
    if (!m_initialized) {
        qCritical() << "AuditManager: Not initialized. Cannot verify log integrity.";
        return false;
    }
    if (!m_immutable) {
        qInfo() << "AuditManager: Immutability is disabled. Integrity verification skipped.";
        return true; // No integrity to verify if immutability is off
    }

    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "AuditManager: Storage not available. Cannot verify log integrity.";
        return false;
    }

    QList<Core::AuditLogEntry> logs = storage->getAuditLogs(QDateTime(), QDateTime()); // Get all logs
    if (logs.isEmpty()) {
        qInfo() << "AuditManager: No immutable logs to verify. Integrity considered intact.";
        return true;
    }

    // Storage returns newest first. Reverse to rebuild the hash chain in the
    // original append order; random UUIDs and equal timestamps are not reliable
    // ordering anchors.
    std::reverse(logs.begin(), logs.end());

    QByteArray expectedPreviousHash; // Starts as empty (genesis hash)
    bool integrityOK = true;

    for (const auto& log : logs) {
        if (log.isImmutable) {
            QByteArray computedHash = calculateEntryHash(log, expectedPreviousHash);
            if (computedHash != log.hash) {
                qCritical() << "AuditManager: INTEGRITY VIOLATION DETECTED!";
                qCritical() << "  Log ID:" << log.id;
                qCritical() << "  Expected Hash:" << QString(log.hash.toHex());
                qCritical() << "  Computed Hash:" << QString(computedHash.toHex());
                qCritical() << "  Previous Hash in chain:" << QString(expectedPreviousHash.toHex());
                emit integrityViolation(QString("Integrity violation for log ID: %1").arg(log.id));
                integrityOK = false;
                break; // Stop on first violation
            }
            expectedPreviousHash = log.hash; // This log's hash becomes the previous hash for the next
        }
    }

    if (integrityOK) {
        qInfo() << "AuditManager: Audit log integrity verified successfully.";
    } else {
        qCritical() << "AuditManager: Audit log integrity verification FAILED.";
    }
    return integrityOK;
}

void AuditManager::enableImmutability(bool enable) {
    if (m_immutable == enable) {
        qDebug() << "AuditManager: Immutability already" << (enable ? "enabled" : "disabled") << ". No change.";
        return;
    }
    m_immutable = enable;
    qInfo() << "AuditManager: Immutability set to" << (enable ? "enabled" : "disabled");
}

bool AuditManager::isImmutable() const {
    return m_immutable;
}

/**
 * @brief Adds an entry to the audit chain, calculating its hash based on the previous entry's hash.
 * @param entry The AuditLogEntry to add. Its hash field will be populated.
 */
void AuditManager::addToChain(Core::AuditLogEntry& entry) {
    if (!m_immutable) {
        entry.hash.clear(); // Clear hash if immutability is off
        return;
    }
    entry.hash = calculateEntryHash(entry, m_chainHash);
    m_chainHash = entry.hash; // Update the manager's current chain hash
    qDebug() << "AuditManager: Added entry" << entry.id << "to chain. New chain hash:" << QString(m_chainHash.toHex());
}

/**
 * @brief Calculates the SHA256 hash for an audit log entry.
 * The hash includes key fields of the entry and the hash of the previous entry in the chain.
 * @param entry The audit log entry.
 * @param previousHash The hash of the previous entry in the chain.
 * @return The calculated SHA256 hash.
 */
QByteArray AuditManager::calculateEntryHash(const Core::AuditLogEntry& entry, const QByteArray& previousHash) const {
    QByteArray data;
    data.append(entry.id.toUtf8());
    data.append(entry.timestamp.toString(Qt::ISODate).toUtf8());
    data.append(entry.userId.toUtf8());
    data.append(entry.userName.toUtf8()); // Include userName in hash
    data.append(QByteArray::number(static_cast<int>(entry.action)));
    data.append(entry.details.toUtf8());
    data.append(entry.ipAddress.toUtf8()); // Include IP address in hash
    data.append(entry.machineId.toUtf8());
    data.append(previousHash); // Crucial for chaining
    return Security::HashProvider::sha256(data);
}

} // namespace Ballot::Audit
