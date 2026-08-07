#pragma once

#include <QString>
#include <QVariantMap>

class QSettings;

namespace Ballot::Core {

struct BootstrapResult {
    bool success = false;
    QString errorMessage;
    QVariantMap sanitizedConfig;
};

class ApplicationBootstrap {
public:
    static QString appDataPath();
    static QString normalizeDbPath(const QVariant& value);
    static QVariantMap loadStoredConfiguration(QSettings& settings);
    static void saveConfiguration(QSettings& settings, const QVariantMap& config);
    static bool consumeResetRequest(QSettings& settings, QString* errorMessage = nullptr);
    static BootstrapResult initializeRuntime(const QVariantMap& config);
    static bool needsFirstRunSetup(QSettings& settings);

private:
    static bool createInitialAdministrator(const QVariantMap& config, QString* errorMessage = nullptr);
};

} // namespace Ballot::Core
