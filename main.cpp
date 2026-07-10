#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QIcon>
#include <QMessageBox>
#include <QUuid>
#include <QCoreApplication> // Required for qApp
#include <QTextStream>
#include <QDateTime>

#include "src/core/SystemManager.h"
#include "src/core/Constants.h"
#include "src/core/ThemeManager.h" // Include ThemeManager
#include "src/modules/security/HashProvider.h"
#include "src/modules/auth/AuthManager.h"
#include "src/modules/auth/RBACManager.h"
#include "src/modules/audit/AuditManager.h"
#include "src/modules/backup/BackupManager.h"
#include "src/modules/election/ElectionManager.h"
#include "src/modules/plugin/PluginManager.h"
#include "src/ui/views/SplashScreen.h"
#include "src/ui/views/MainWindow.h"
#include "src/ui/views/SetupWizard.h"

using namespace Ballot;

namespace {

QString appDataPath()
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    return dataDir;
}

QString normalizeDbPath(const QVariant& value)
{
    QString path = value.toString().trimmed();
    if (path.isEmpty()) {
        path = Core::Constants::DB_FILENAME;
    }

    QFileInfo info(path);
    if (info.isRelative()) {
        path = QDir(appDataPath()).filePath(path);
    }

    QDir().mkpath(QFileInfo(path).absolutePath());
    return QDir::toNativeSeparators(path);
}

void installFileLogger()
{
    static QFile logFile(QDir(appDataPath()).filePath(Core::Constants::LOG_FILE));
    if (!logFile.open(QIODevice::Append | QIODevice::Text)) {
        return;
    }

    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& context, const QString& message) {
        const char* level = "DEBUG";
        switch (type) {
            case QtInfoMsg: level = "INFO"; break;
            case QtWarningMsg: level = "WARN"; break;
            case QtCriticalMsg: level = "ERROR"; break;
            case QtFatalMsg: level = "FATAL"; break;
            case QtDebugMsg: break;
        }

        QTextStream out(&logFile);
        out << QDateTime::currentDateTime().toString(Qt::ISODate)
            << " [" << level << "] " << message;
        if (context.file) {
            out << " (" << context.file << ':' << context.line << ')';
        }
        out << Qt::endl;
        out.flush();

        if (type == QtFatalMsg) {
            abort();
        }
    });
}

bool createInitialAdministrator(const QVariantMap& config)
{
    if (!config.contains("admin_email") || (!config.contains("admin_password_hash") && !config.contains("admin_password"))) {
        return true;
    }

    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return false;

    const QString email = config.value("admin_email").toString().trimmed();
    if (storage->getUserByEmail(email)) return true;

    Core::User admin;
    admin.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    admin.name = config.value("admin_name").toString().trimmed();
    admin.email = email;
    
    if (config.contains("admin_password_hash")) {
        admin.passwordHashAndSalt = QByteArray::fromHex(config.value("admin_password_hash").toString().toUtf8());
    } else {
        // Hash plain text password if hash is not provided
        QByteArray salt = Security::HashProvider::generateSalt();
        QByteArray hashedPassword = Security::HashProvider::argon2Hash(config.value("admin_password").toString(), salt);
        admin.passwordHashAndSalt = salt + hashedPassword;
    }
    
    admin.role = Core::UserRole::SuperAdministrator;
    admin.isActive = true;
    admin.createdAt = QDateTime::currentDateTime();
    return storage->createUser(admin);
}

void saveConfiguration(QSettings& settings, const QVariantMap& config)
{
    settings.setValue("first_run", false);
    settings.setValue("storage_type", "sqlite");
    settings.setValue("db_path", normalizeDbPath(config.value("db_path", Core::Constants::DB_FILENAME)));
    settings.sync();
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qApp->setApplicationName(Core::Constants::APP_NAME);
    qApp->setOrganizationName(Core::Constants::ORG_NAME);
    qApp->setApplicationVersion(Core::Constants::APP_VERSION);
    qApp->setWindowIcon(QIcon(":/assets/brand/app-mark.svg"));
    installFileLogger();

    // Ensure app data directory exists
    appDataPath();

    QSettings settings; // Moved settings here to be accessible for theme loading

    // Apply theme using ThemeManager
    QString savedThemeName = settings.value("theme", "Modern").toString(); // Default to "Modern"
    Core::ThemeManager::instance().applyTheme(savedThemeName);

    // Load RBAC roles (can be done early as it doesn't depend on storage config)
    Auth::RBACManager::instance();

    // Initialize plugin system (can be done early)
    Plugin::PluginManager::instance().loadAll();

    // Show splash screen
    UI::SplashScreen splash;
    splash.show();
    qApp->processEvents(); // Use qApp

    bool isFirstRun = settings.value("first_run", true).toBool();

    // When splash finishes, proceed with main initialization
    auto* settingsPtr = &settings;

    QObject::connect(&splash, &UI::SplashScreen::loadingFinished, [&, settingsPtr]() {
        if (isFirstRun) {
            auto* wizard = new UI::SetupWizard();
            QObject::connect(wizard, &UI::SetupWizard::setupCompleted, [wizard, settingsPtr](const QVariantMap& config) mutable {
                if (!Core::SystemManager::instance().initialize(config) || !createInitialAdministrator(config)) {
                    QMessageBox::critical(wizard, "Setup failed", "The local database could not be initialized. Check the selected path and try again.");
                    return;
                }
                Auth::AuthManager::instance().initialize(config);
                saveConfiguration(*settingsPtr, config);
                auto* mainWindow = new UI::MainWindow();
                mainWindow->show();
                wizard->deleteLater();
            });
            wizard->show();
            splash.close();
        } else {
            // Load configuration from settings
            QVariantMap loadedConfig;
            loadedConfig["storage_type"] = "sqlite";
            loadedConfig["db_path"] = normalizeDbPath(settingsPtr->value("db_path", Core::Constants::DB_FILENAME));
            if (!Core::SystemManager::instance().initialize(loadedConfig)) {
                QMessageBox::critical(nullptr, "Startup failed", "The configured database could not be opened.");
                QApplication::quit();
                return;
            }
            Auth::AuthManager::instance().initialize(loadedConfig);
            auto* mainWindow = new UI::MainWindow();
            mainWindow->show();
            splash.finish(mainWindow);
        }
    });

    splash.startLoading(); // Start the splash screen loading animation

    return qApp->exec(); // Changed a.exec() to qApp->exec()
}
