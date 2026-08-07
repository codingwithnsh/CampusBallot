#include "ApplicationBootstrap.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>

#include "src\core\Constants.h"
#include "src\core\SystemManager.h"
#include "src\modules\auth\AuthManager.h"
#include "src\modules\security\HashProvider.h"

namespace Ballot::Core {

QString ApplicationBootstrap::appDataPath()
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    return dataDir;
}

QString ApplicationBootstrap::normalizeDbPath(const QVariant& value)
{
    QString path = value.toString().trimmed();
    if (path.isEmpty()) {
        path = Constants::DB_FILENAME;
    }

    QFileInfo info(path);
    if (info.isRelative()) {
        path = QDir(appDataPath()).filePath(path);
    }

    QDir().mkpath(QFileInfo(path).absolutePath());
    return QDir::toNativeSeparators(path);
}

QVariantMap ApplicationBootstrap::loadStoredConfiguration(QSettings& settings)
{
    QVariantMap config;
    config["storage_type"] = settings.value("storage_type", "sqlite").toString();
    config["auth_type"] = settings.value("auth_type", "local").toString();
    config["db_path"] = normalizeDbPath(settings.value("db_path", Constants::DB_FILENAME));
    config["api_key"] = settings.value("firebase_api_key").toString();
    config["project_id"] = settings.value("firebase_project_id").toString();
    config["database_url"] = settings.value("firebase_database_url").toString();
    return config;
}

void ApplicationBootstrap::saveConfiguration(QSettings& settings, const QVariantMap& config)
{
    settings.setValue("first_run", false);
    settings.setValue("storage_type", config.value("storage_type", "sqlite").toString());
    settings.setValue("auth_type", config.value("auth_type", "local").toString());
    settings.setValue("db_path", normalizeDbPath(config.value("db_path", Constants::DB_FILENAME)));
    settings.setValue("firebase_api_key", config.value("api_key").toString());
    settings.setValue("firebase_project_id", config.value("project_id").toString());
    settings.setValue("firebase_database_url", config.value("database_url").toString());
    settings.sync();
}

bool ApplicationBootstrap::consumeResetRequest(QSettings& settings, QString* errorMessage)
{
    if (!settings.value("reset_requested", false).toBool()) {
        return true;
    }

    const QString dbPath = normalizeDbPath(settings.value("db_path", Constants::DB_FILENAME));
    if (QFile::exists(dbPath) && !QFile::remove(dbPath)) {
        if (errorMessage) {
            *errorMessage = QString("The database at %1 could not be deleted.").arg(dbPath);
        }
        return false;
    }

    settings.setValue("reset_requested", false);
    settings.setValue("first_run", true);
    settings.sync();
    return true;
}

BootstrapResult ApplicationBootstrap::initializeRuntime(const QVariantMap& config)
{
    BootstrapResult result;
    result.sanitizedConfig = config;
    const QString requestedStorage = config.value("storage_type", "sqlite").toString().trimmed().toLower();
    if (requestedStorage == "sqlite" || requestedStorage == "local_device") {
        result.sanitizedConfig["storage_type"] = requestedStorage;
    } else {
        result.sanitizedConfig["storage_type"] = "sqlite";
    }
    result.sanitizedConfig["db_path"] = normalizeDbPath(config.value("db_path", Constants::DB_FILENAME));
    result.sanitizedConfig["auth_type"] = config.value("auth_type", "local").toString();

    if (!SystemManager::instance().initialize(result.sanitizedConfig)) {
        result.errorMessage = "The local database could not be initialized. Check the selected path and try again.";
        return result;
    }

    if (!createInitialAdministrator(result.sanitizedConfig, &result.errorMessage)) {
        return result;
    }

    Auth::AuthManager::instance().initialize(result.sanitizedConfig);
    result.sanitizedConfig.remove("admin_password");
    result.success = true;
    return result;
}

bool ApplicationBootstrap::needsFirstRunSetup(QSettings& settings)
{
    return settings.value("first_run", true).toBool();
}

bool ApplicationBootstrap::createInitialAdministrator(const QVariantMap& config, QString* errorMessage)
{
    if (!config.contains("admin_email") || !config.contains("admin_password_hash")) {
        return true;
    }

    auto* storage = SystemManager::instance().storage();
    if (!storage) {
        if (errorMessage) {
            *errorMessage = "Storage is unavailable while creating the administrator account.";
        }
        return false;
    }

    const QString email = config.value("admin_email").toString().trimmed().toLower();
    if (email.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Administrator email is required.";
        }
        return false;
    }

    if (storage->getUserByEmail(email).has_value()) {
        return true;
    }

    Core::User admin;
    admin.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    admin.name = config.value("admin_name").toString().trimmed();
    admin.email = email;
    admin.passwordHashAndSalt = QByteArray::fromHex(config.value("admin_password_hash").toString().toUtf8());
    admin.role = Core::UserRole::SuperAdministrator;
    admin.isActive = true;
    admin.createdAt = QDateTime::currentDateTime();

    if (!storage->createUser(admin)) {
        if (errorMessage) {
            *errorMessage = "The administrator account could not be created.";
        }
        return false;
    }

    return true;
}

} // namespace Ballot::Core
