#pragma once

#include <QObject>
#include <memory>
#include <QVariantMap>
#include "src/modules/storage/interfaces/IStorageProvider.h"
#include "src/core/Models.h"
#include "src/core/Constants.h"
#include "src/modules/auth/IAuthProvider.h" // Dependency on authentication provider interface
#include "src/modules/plugin/IPlugin.h" // Dependency on plugin interface

namespace Ballot::Security { class AES256Provider; class TamperDetector; } // Forward declarations for security components

namespace Ballot::Core {

/**
 * @brief The SystemManager class is a singleton responsible for managing core application services
 * and global state.
 *
 * This class acts as a central hub for:
 * - Application initialization and shutdown.
 * - Providing access to the chosen storage provider (IStorageProvider).
 * - Managing application-wide settings (SystemSettings).
 * - Handling security components like encryption (AES256Provider) and tamper detection (TamperDetector).
 * - Providing machine-specific information (ID, name).
 * - Managing application theme and accent colors.
 *
 * @note This class currently aggregates many responsibilities. In future refactoring,
 * consider decomposing it into smaller, more focused service managers (e.g., SettingsManager,
 * ThemeManager, StorageService) to adhere more closely to the Single Responsibility Principle
 * and improve testability.
 */
class SystemManager : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Returns the singleton instance of the SystemManager.
     * @return Reference to the SystemManager instance.
     */
    static SystemManager& instance();

    /**
     * @brief Destructor. Ensures proper cleanup of managed resources.
     */
    ~SystemManager() override;

    /**
     * @brief Initializes the core components of the application.
     * @param config A map containing configuration parameters for various subsystems.
     * @return True if initialization is successful, false otherwise.
     */
    bool initialize(const QVariantMap& config);

    /**
     * @brief Initializes the storage provider based on the given configuration.
     * @param config A map containing storage-specific configuration.
     * @return True if storage initialization is successful, false otherwise.
     */
    bool initializeStorage(const QVariantMap& config);

    /**
     * @brief Shuts down the application's core components and releases resources.
     * @return True if shutdown is successful, false otherwise.
     */
    bool shutdown();

    /**
     * @brief Provides access to the currently active storage provider.
     * @return Pointer to the IStorageProvider implementation.
     */
    IStorageProvider* storage() const;

    /**
     * @brief Retrieves the current system settings.
     * @return The current SystemSettings object.
     */
    SystemSettings settings() const;

    /**
     * @brief Updates the system settings and persists them.
     * @param settings The new SystemSettings to apply.
     * @return True if settings are updated successfully, false otherwise.
     */
    bool updateSettings(const SystemSettings& settings);

    /**
     * @brief Checks if the current machine is designated as the master machine.
     * @return True if the machine is master, false otherwise.
     */
    bool isMaster() const;

    /**
     * @brief Retrieves the unique identifier for the current machine.
     * @return The machine ID.
     */
    QString machineId() const;

    /**
     * @brief Retrieves the name of the current machine.
     * @return The machine name.
     */
    QString machineName() const;

    /**
     * @brief Sets the application's accent color.
     * @param color The new accent color (e.g., "#RRGGBB").
     */
    void setAccentColor(const QString& color);

    /**
     * @brief Sets the application's theme (e.g., "dark", "light").
     * @param theme The new theme name.
     */
    void setTheme(const QString& theme);

    /**
     * @brief Applies the specified theme (QSS) to the entire application.
     * @param themeName The name of the theme to apply (e.g., "dark", "light").
     */
    void applyTheme(const QString& themeName); // NEW

    /**
     * @brief Retrieves the current accent color.
     * @return The current accent color string.
     */
    QString accentColor() const;

    /**
     * @brief Retrieves the current theme name.
     * @return The current theme name string.
     */
    QString theme() const;

    /**
     * @brief Checks if the SystemManager has been successfully initialized.
     * @return True if initialized, false otherwise.
     */
    bool isInitialized() const { return m_initialized; }

signals:
    /**
     * @brief Emitted when the SystemManager has completed its initialization.
     */
    void initialized();

    /**
     * @brief Emitted when a shutdown of the application is requested.
     */
    void shutdownRequested();

    /**
     * @brief Emitted when system settings have been changed.
     */
    void settingsChanged();

    /**
     * @brief Emitted when the application theme has changed.
     * @param theme The new theme name.
     */
    void themeChanged(const QString& theme);

    /**
     * @brief Emitted when the application accent color has changed.
     * @param color The new accent color.
     */
    void accentColorChanged(const QString& color);

    /**
     * @brief Emitted when the storage provider has been changed or re-initialized.
     * @param providerName The name of the new storage provider.
     */
    void storageChanged(const QString& providerName);

private:
    /**
     * @brief Private constructor to enforce singleton pattern.
     */
    SystemManager();

    /**
     * @brief Registers the current machine with the system, potentially storing its ID and name.
     */
    void registerCurrentMachine();

    std::unique_ptr<IStorageProvider> m_storage; ///< Manages the application's data storage.
    std::unique_ptr<Security::AES256Provider> m_crypto; ///< Provides AES256 encryption/decryption services.
    std::unique_ptr<Security::TamperDetector> m_tamperDetector; ///< Monitors for system tampering.
    SystemSettings m_settings; ///< Stores the current application settings.
    QString m_machineId; ///< Unique identifier for the current machine.
    QString m_machineName; ///< Name of the current machine.
    bool m_initialized = false; ///< Flag indicating if the manager is initialized.

    // Private copy constructor and assignment operator to prevent copying
    SystemManager(const SystemManager&) = delete;
    SystemManager& operator=(const SystemManager&) = delete;
};

} // namespace Ballot::Core