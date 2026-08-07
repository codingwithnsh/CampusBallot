#include "SystemManager.h"
#include "src/modules/storage/providers/SQLiteStorageProvider.h" // Specific storage provider
#include "src/modules/security/AES256Provider.h" // Specific crypto provider
#include "src/modules/security/TamperDetector.h" // Specific tamper detection
#include "src/core/ThemeManager.h"
#include "src/core/Utils.h" // For SystemInfo namespace
#include "src/modules/audit/AuditManager.h" // Dependency for audit logging
#include "src/modules/backup/BackupManager.h" // Dependency for backup management
#include "src/modules/integration/FirebaseRealtimeSyncManager.h"

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QDebug> // For logging

namespace Ballot::Core {

SystemManager& SystemManager::instance() {
    static SystemManager inst;
    return inst;
}

SystemManager::SystemManager() {
    // Initialize machine-specific identifiers using the refactored SystemInfo namespace
    m_machineId = SystemInfo::getMachineId();
    m_machineName = SystemInfo::getMachineName();
    qDebug() << "SystemManager: Initialized with Machine ID:" << m_machineId << "Name:" << m_machineName;
}

SystemManager::~SystemManager() = default;

bool SystemManager::initialize(const QVariantMap& config) {
    if (m_initialized) {
        qDebug() << "SystemManager: Already initialized.";
        return true;
    }

    qDebug() << "SystemManager: Starting initialization...";
    m_firebaseSyncEnabled = config.value("auth_type").toString().trimmed().compare("firebase", Qt::CaseInsensitive) == 0;

    // Initialize security components
    m_crypto = std::make_unique<Security::AES256Provider>();
    m_tamperDetector = std::make_unique<Security::TamperDetector>();
    qDebug() << "SystemManager: Security components initialized.";

    // Initialize storage provider
    if (!initializeStorage(config)) {
        qCritical() << "SystemManager: Failed to initialize storage.";
        return false;
    }
    qDebug() << "SystemManager: Storage initialized.";

    // Load system settings from storage
    auto s = m_storage->getSystemSettings();
    if (s) {
        m_settings = *s;
        qDebug() << "SystemManager: System settings loaded.";
    } else {
        qWarning() << "SystemManager: No system settings found or failed to load. Using defaults.";
        // Optionally, save default settings if none exist
        if (m_storage && !m_storage->updateSystemSettings(m_settings)) {
            qCritical() << "SystemManager: Failed to save default system settings.";
        }
    }

    // Apply initial theme based on loaded settings
    applyTheme(m_settings.theme);

    // Register the current machine with the system
    registerCurrentMachine();
    qDebug() << "SystemManager: Current machine registered.";

    // Initialize other managers that depend on SystemManager's state or storage
    Audit::AuditManager::instance().initialize();
    Backup::BackupManager::instance().initialize();
    qDebug() << "SystemManager: Audit and Backup managers initialized.";

    m_initialized = true;
    emit initialized();
    qInfo() << "SystemManager: Initialization complete.";
    return true;
}

bool SystemManager::initializeStorage(const QVariantMap& config) {
    QString providerType = config.value("storage_type", "sqlite").toString();
    qDebug() << "SystemManager: Initializing storage with type:" << providerType;

    // Correctly handle "local_device" as SQLite
    if (providerType == "sqlite" || providerType == "local_device") {
        m_storage = std::make_unique<Storage::SQLiteStorageProvider>();
    }
    // TODO: Add other storage providers here (e.g., Firebase, PostgreSQL)
    // else if (providerType == "firebase") {
    //     m_storage = std::make_unique<Storage::FirebaseStorageProvider>();
    // }
    else {
        qCritical() << "SystemManager: Unknown storage provider type specified:" << providerType;
        return false;
    }

    if (!m_storage) {
        qCritical() << "SystemManager: Failed to create storage provider instance for type:" << providerType;
        return false;
    }

    if (!m_storage->connect(config)) {
        qCritical() << "SystemManager: Failed to connect to storage provider:" << m_storage->providerName();
        return false;
    }

    emit storageChanged(m_storage->providerName());
    qDebug() << "SystemManager: Storage provider connected:" << m_storage->providerName();
    return true;
}

bool SystemManager::shutdown() {
    qInfo() << "SystemManager: Shutting down...";
    if (m_tamperDetector) {
        m_tamperDetector->stopIntegrityMonitoring();
        qDebug() << "SystemManager: Tamper detector stopped.";
    }
    if (m_storage) {
        m_storage->disconnect();
        qDebug() << "SystemManager: Storage disconnected.";
    }
    m_initialized = false;
    emit shutdownRequested();
    qInfo() << "SystemManager: Shutdown complete.";
    return true;
}

IStorageProvider* SystemManager::storage() const { return m_storage.get(); }

SystemSettings SystemManager::settings() const { return m_settings; }

bool SystemManager::updateSettings(const SystemSettings& newSettings) {
    if (!m_storage) {
        qCritical() << "SystemManager: Cannot update settings, storage provider not available.";
        return false;
    }

    // Check if theme or accent color changed before updating
    const bool themeChangedValue = m_settings.theme != newSettings.theme;
    const bool accentChangedValue = m_settings.accentColor != newSettings.accentColor;

    if (!m_storage->updateSystemSettings(newSettings)) {
        qCritical() << "SystemManager: Failed to persist updated system settings to storage.";
        return false;
    }

    m_settings = newSettings; // Update internal settings after successful persistence
    qDebug() << "SystemManager: System settings updated and persisted.";

    // Propagate changes to other managers
    Audit::AuditManager::instance().enableImmutability(m_settings.auditAllActions);
    Backup::BackupManager::instance().setAutoBackup(m_settings.autoBackupEnabled, m_settings.backupIntervalHours);

    // Emit signals for UI updates
    if (themeChangedValue) {
        applyTheme(m_settings.theme); // Apply new theme immediately
        emit themeChanged(m_settings.theme);
        qDebug() << "SystemManager: Theme changed to" << m_settings.theme;
    }
    if (accentChangedValue) {
        emit accentColorChanged(m_settings.accentColor);
        qDebug() << "SystemManager: Accent color changed to" << m_settings.accentColor;
    }
    emit settingsChanged();
    qDebug() << "SystemManager: settingsChanged signal emitted.";
    return true;
}

bool SystemManager::isMaster() const { return m_settings.masterMachineId == m_machineId; }

QString SystemManager::machineId() const { return m_machineId; }

QString SystemManager::machineName() const { return m_machineName; }

void SystemManager::setAccentColor(const QString& color) {
    if (m_settings.accentColor == color) return; // No change
    SystemSettings tempSettings = m_settings;
    tempSettings.accentColor = color;
    if (!updateSettings(tempSettings)) {
        qWarning() << "SystemManager: Failed to set accent color to" << color;
    }
}

void SystemManager::setTheme(const QString& theme) {
    if (m_settings.theme == theme) return; // No change
    SystemSettings tempSettings = m_settings;
    tempSettings.theme = theme;
    if (!updateSettings(tempSettings)) {
        qWarning() << "SystemManager: Failed to set theme to" << theme;
    }
}

void SystemManager::applyTheme(const QString& themeName) {
    // Theme application is centralized in ThemeManager so the app uses one
    // stylesheet path, one persistence mechanism, and one source of truth.
    ThemeManager::instance().applyTheme(themeName);
    qInfo() << "SystemManager: Delegated theme application to ThemeManager:" << themeName;
}

QString SystemManager::accentColor() const { return m_settings.accentColor; }
QString SystemManager::theme() const { return m_settings.theme; }
bool SystemManager::firebaseSyncEnabled() const {
    return m_firebaseSyncEnabled && Integration::FirebaseRealtimeSyncManager::instance().isConfigured();
}

void SystemManager::registerCurrentMachine() {
    if (!m_storage) {
        qCritical() << "SystemManager: Cannot register machine, storage provider not available.";
        return;
    }

    MachineInfo info;
    info.id = m_machineId;
    info.name = m_machineName;
    info.lastSeen = QDateTime::currentDateTime();
    info.ipAddress = SystemInfo::getIpAddress(); // Use refactored SystemInfo
    info.osVersion = QSysInfo::prettyProductName(); // More descriptive OS version
    info.appVersion = Constants::APP_VERSION; // Use centralized constant
    info.isOnline = true;

    // Determine if this machine should be the master
    auto currentStoredSettings = m_storage->getSystemSettings();
    if (currentStoredSettings) {
        if (currentStoredSettings->masterMachineId == m_machineId) {
            info.isMaster = true;
        } else if (currentStoredSettings->masterMachineId.isEmpty()) {
            // If no master is set, this machine becomes the master
            info.isMaster = true;
            m_settings.masterMachineId = m_machineId;
            if (!m_storage->updateSystemSettings(m_settings)) {
                qCritical() << "SystemManager: Failed to set current machine as master in settings.";
            } else {
                qInfo() << "SystemManager: Current machine set as master.";
            }
        }
    } else {
        qWarning() << "SystemManager: Could not retrieve system settings to determine master status. Assuming not master.";
    }


    if (!m_storage->registerMachine(info)) {
        qCritical() << "SystemManager: Failed to register current machine with storage.";
    } else {
        qDebug() << "SystemManager: Machine registered successfully:" << info.name << "(" << info.id << ")";
    }
}

} // namespace Ballot::Core
