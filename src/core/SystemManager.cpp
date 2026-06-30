#include "SystemManager.h"
#include "src/modules/storage/providers/SQLiteStorageProvider.h"
#include "src/security/AES256Provider.h"
#include "src/security/TamperDetector.h"
#include "src/core/Utils.h"
#include "src/modules/audit/AuditManager.h"
#include "src/modules/backup/BackupManager.h"
#include "src/modules/auth/AuthManager.h"
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

namespace Ballot::Core {

SystemManager& SystemManager::instance() {
    static SystemManager inst;
    return inst;
}

SystemManager::SystemManager() {
    m_machineId = Utils::getMachineId();
    m_machineName = Utils::getMachineName();
}
SystemManager::~SystemManager() = default;

bool SystemManager::initialize(const QVariantMap& config) {
    if (m_initialized) return true;

    m_crypto = std::make_unique<Security::AES256Provider>();
    m_tamperDetector = std::make_unique<Security::TamperDetector>();

    if (!initializeStorage(config)) return false;

    auto s = m_storage->getSystemSettings();
    if (s) m_settings = *s;

    registerCurrentMachine();

    Audit::AuditManager::instance().initialize();
    Backup::BackupManager::instance().initialize();

    m_initialized = true;
    emit initialized();
    return true;
}

bool SystemManager::initializeStorage(const QVariantMap& config) {
    QString providerType = config.value("storage_type", "sqlite").toString();

    // Correctly handle "local_device" as SQLite
    if (providerType == "sqlite" || providerType == "local_device") {
        m_storage = std::make_unique<Storage::SQLiteStorageProvider>();
    }
    // Add other storage providers here if they exist (e.g., Firebase)
    // else if (providerType == "firebase") {
    //     m_storage = std::make_unique<Storage::FirebaseStorageProvider>();
    // }

    if (!m_storage) return false;

    if (!m_storage->connect(config)) return false;

    emit storageChanged(m_storage->providerName());
    return true;
}

bool SystemManager::shutdown() {
    if (m_tamperDetector) m_tamperDetector->stopIntegrityMonitoring();
    if (m_storage) m_storage->disconnect();
    m_initialized = false;
    emit shutdownRequested();
    return true;
}

IStorageProvider* SystemManager::storage() const { return m_storage.get(); }

SystemSettings SystemManager::settings() const { return m_settings; }

bool SystemManager::updateSettings(const SystemSettings& settings) {
    if (!m_storage || !m_storage->updateSystemSettings(settings)) return false;
    const bool themeChangedValue = m_settings.theme != settings.theme;
    const bool accentChangedValue = m_settings.accentColor != settings.accentColor;
    m_settings = settings;
    Audit::AuditManager::instance().enableImmutability(settings.auditAllActions);
    Backup::BackupManager::instance().setAutoBackup(settings.autoBackupEnabled, settings.backupIntervalHours);
    if (themeChangedValue) emit themeChanged(settings.theme);
    if (accentChangedValue) emit accentColorChanged(settings.accentColor);
    emit settingsChanged();
    return true;
}

bool SystemManager::isMaster() const { return m_settings.masterMachineId == m_machineId; }

QString SystemManager::machineId() const { return m_machineId; }

QString SystemManager::machineName() const { return m_machineName; }

void SystemManager::setAccentColor(const QString& color) {
    m_settings.accentColor = color;
    if (m_storage) {
        m_storage->updateSystemSettings(m_settings);
    }
    emit accentColorChanged(color);
}

void SystemManager::setTheme(const QString& theme) {
    m_settings.theme = theme;
    if (m_storage) {
        m_storage->updateSystemSettings(m_settings);
    }
    emit themeChanged(theme);
}

QString SystemManager::accentColor() const { return m_settings.accentColor; }
QString SystemManager::theme() const { return m_settings.theme; }

void SystemManager::registerCurrentMachine() {
    if (!m_storage) return;

    MachineInfo info;
    info.id = m_machineId;
    info.name = m_machineName;
    info.lastSeen = QDateTime::currentDateTime();
    info.ipAddress = Utils::getIpAddress();
    info.isOnline = true;

    auto settings = m_storage->getSystemSettings();
    if (settings && settings->masterMachineId == m_machineId) {
        info.isMaster = true;
    } else if (settings && settings->masterMachineId.isEmpty()) {
        info.isMaster = true;
        m_settings.masterMachineId = m_machineId;
        m_storage->updateSystemSettings(m_settings);
    }

    m_storage->registerMachine(info);
}

} // namespace Ballot::Core
