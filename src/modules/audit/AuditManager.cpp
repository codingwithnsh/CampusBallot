#include "AuditManager.h"
#include "src/core/SystemManager.h"
#include "src/core/Utils.h"
#include "src/security/HashProvider.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QUuid>
#include <QDebug>

namespace Ballot::Audit {

AuditManager& AuditManager::instance() {
    static AuditManager inst;
    return inst;
}

AuditManager::AuditManager() {}

void AuditManager::initialize() {
    auto* storage = Core::SystemManager::instance().storage();
    if (storage) {
        auto settings = storage->getSystemSettings();
        if (settings) {
            m_immutable = settings->auditAllActions;
        }
        const auto logs = storage->getAuditLogs();
        if (!logs.isEmpty()) m_chainHash = logs.first().hash;
    }
    m_initialized = true; // Set initialized flag
}

bool AuditManager::isInitialized() const {
    return m_initialized;
}

void AuditManager::log(Core::AuditAction action, const QString& details, const QString& userId) {
    Core::AuditLogEntry entry;
    entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.timestamp = QDateTime::currentDateTime();
    entry.userId = userId;
    entry.action = action;
    entry.details = details;
    entry.machineId = Core::Utils::getMachineId();
    entry.isImmutable = m_immutable;

    addToChain(entry);

    auto* storage = Core::SystemManager::instance().storage();
    if (storage) {
        storage->logAction(entry);
    }

    emit logAdded(entry);
}

void AuditManager::log(const Core::AuditLogEntry& entry) {
    Core::AuditLogEntry e = entry;
    if (e.id.isEmpty()) {
        e.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    if (e.timestamp.isNull()) {
        e.timestamp = QDateTime::currentDateTime();
    }
    if (e.machineId.isEmpty()) {
        e.machineId = Core::Utils::getMachineId();
    }
    e.isImmutable = m_immutable;

    addToChain(e);

    auto* storage = Core::SystemManager::instance().storage();
    if (storage) {
        storage->logAction(e);
    }

    emit logAdded(e);
}

QList<Core::AuditLogEntry> AuditManager::getLogs(const QDateTime& from, const QDateTime& to) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return {};
    return storage->getAuditLogs(from, to);
}

QList<Core::AuditLogEntry> AuditManager::getLogsByUser(const QString& userId) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return {};
    return storage->getAuditLogsByUser(userId);
}

QList<Core::AuditLogEntry> AuditManager::getLogsByAction(Core::AuditAction action) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return {};
    return storage->getAuditLogsByAction(action);
}

int AuditManager::getLogCount() const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return 0;
    return storage->getAuditLogCount();
}

bool AuditManager::exportLogs(const QString& filePath, const QString& format) {
    auto logs = getLogs();
    QJsonArray arr;
    for (const auto& log : logs) {
        arr.append(log.toJson());
    }

    QByteArray data;
    if (format == "json") {
        data = QJsonDocument(arr).toJson(QJsonDocument::Indented);
    } else if (format == "csv") {
        data.append("ID,Timestamp,User,Action,Details\n");
        for (const auto& log : logs) {
            data.append(QString("\"%1\",\"%2\",\"%3\",%4,\"%5\"\n")
                .arg(log.id, log.timestamp.toString(Qt::ISODate),
                     log.userName, QString::number(static_cast<int>(log.action)), log.details).toUtf8());
        }
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(data);
    file.close();
    return true;
}

bool AuditManager::verifyLogIntegrity() const {
    auto logs = getLogs();
    QByteArray previousHash;
    for (auto it = logs.crbegin(); it != logs.crend(); ++it) {
        const auto& log = *it;
        if (log.isImmutable && !log.hash.isEmpty()) {
            QByteArray computed = calculateEntryHash(log, previousHash);
            if (computed != log.hash) return false;
            previousHash = log.hash;
        }
    }
    return true;
}

void AuditManager::enableImmutability(bool enable) {
    m_immutable = enable;
}

bool AuditManager::isImmutable() const {
    return m_immutable;
}

void AuditManager::addToChain(Core::AuditLogEntry& entry) {
    if (!m_immutable) return;
    entry.hash = calculateEntryHash(entry, m_chainHash);
    m_chainHash = entry.hash;
}

QByteArray AuditManager::calculateEntryHash(const Core::AuditLogEntry& entry, const QByteArray& previousHash) const {
    QByteArray data;
    data.append(entry.id.toUtf8());
    data.append(entry.timestamp.toString(Qt::ISODate).toUtf8());
    data.append(entry.userId.toUtf8());
    data.append(QByteArray::number(static_cast<int>(entry.action)));
    data.append(entry.details.toUtf8());
    data.append(entry.machineId.toUtf8());
    data.append(previousHash);
    return Security::HashProvider::sha256(data);
}

} // namespace Ballot::Audit
