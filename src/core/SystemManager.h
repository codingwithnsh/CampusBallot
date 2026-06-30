#pragma once

#include <QObject>
#include <memory>
#include <QVariantMap>
#include "src/modules/storage/interfaces/IStorageProvider.h"
#include "src/core/Models.h"
#include "src/core/Constants.h"
#include "src/modules/auth/IAuthProvider.h"
#include "src/modules/plugin/IPlugin.h"

namespace Ballot::Security { class AES256Provider; class TamperDetector; }

namespace Ballot::Core {

class SystemManager : public QObject {
    Q_OBJECT
public:
    static SystemManager& instance();
    ~SystemManager() override;

    bool initialize(const QVariantMap& config);
    bool initializeStorage(const QVariantMap& config);
    bool shutdown();

    IStorageProvider* storage() const;
    SystemSettings settings() const;
    bool updateSettings(const SystemSettings& settings);
    bool isMaster() const;
    QString machineId() const;
    QString machineName() const;

    void setAccentColor(const QString& color);
    void setTheme(const QString& theme);
    QString accentColor() const;
    QString theme() const;

    bool isInitialized() const { return m_initialized; }

signals:
    void initialized();
    void shutdownRequested();
    void settingsChanged();
    void themeChanged(const QString& theme);
    void accentColorChanged(const QString& color);
    void storageChanged(const QString& providerName);

private:
    SystemManager();
    void registerCurrentMachine();

    std::unique_ptr<IStorageProvider> m_storage;
    std::unique_ptr<Security::AES256Provider> m_crypto;
    std::unique_ptr<Security::TamperDetector> m_tamperDetector;
    SystemSettings m_settings;
    QString m_machineId;
    QString m_machineName;
    bool m_initialized = false;
};

} // namespace Ballot::Core
