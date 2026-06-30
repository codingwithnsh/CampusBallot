#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QTimer>
#include <QDir>
#include <QStandardPaths>
#include <QIcon>
#include <QMessageBox>
#include <QUuid>
#include "src/core/SystemManager.h"
#include "src/core/Constants.h"
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

bool createInitialAdministrator(const QVariantMap& config)
{
    if (!config.contains("admin_email") || !config.contains("admin_password_hash")) {
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
    admin.digitalSignature = QByteArray::fromHex(config.value("admin_password_hash").toString().toUtf8());
    admin.role = Core::UserRole::SuperAdministrator;
    admin.isActive = true;
    admin.createdAt = QDateTime::currentDateTime();
    return storage->createUser(admin);
}

void saveConfiguration(QSettings& settings, const QVariantMap& config)
{
    settings.setValue("first_run", false);
    settings.setValue("storage_type", config.value("storage_type").toString());
    settings.setValue("db_path", config.value("db_path", Core::Constants::DB_FILENAME).toString());
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName(Core::Constants::APP_NAME);
    a.setOrganizationName(Core::Constants::ORG_NAME);
    a.setApplicationVersion(Core::Constants::APP_VERSION);
    a.setWindowIcon(QIcon(":/assets/brand/app-mark.svg"));

    // Ensure app data directory exists
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    // Load stylesheet
    QFile styleFile(":/src/ui/styles/modern.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        a.setStyleSheet(styleFile.readAll());
    }

    // Load RBAC roles (can be done early as it doesn't depend on storage config)
    Auth::RBACManager::instance();

    // Initialize plugin system (can be done early)
    Plugin::PluginManager::instance().loadAll();

    // Show splash screen
    UI::SplashScreen splash;
    splash.show();
    a.processEvents();

    QSettings settings;
    bool isFirstRun = settings.value("first_run", true).toBool();

    // When splash finishes, proceed with main initialization
    QObject::connect(&splash, &UI::SplashScreen::loadingFinished, [&]() {
        if (isFirstRun) {
            UI::SetupWizard* wizard = new UI::SetupWizard();
            QObject::connect(wizard, &UI::SetupWizard::setupCompleted, [wizard, &settings](const QVariantMap& config) {
                if (!Core::SystemManager::instance().initialize(config) || !createInitialAdministrator(config)) {
                    QMessageBox::critical(wizard, "Setup failed", "The local database could not be initialized. Check the selected path and try again.");
                    return;
                }
                saveConfiguration(settings, config);
                auto* mainWindow = new UI::MainWindow();
                mainWindow->show();
                wizard->deleteLater();
            });
            wizard->show();
            splash.close();
        } else {
            // Load configuration from settings
            QVariantMap loadedConfig;
            loadedConfig["storage_type"] = settings.value("storage_type", "local_device").toString();
            loadedConfig["db_path"] = settings.value("db_path", Core::Constants::DB_FILENAME).toString();
            if (!Core::SystemManager::instance().initialize(loadedConfig)) {
                QMessageBox::critical(nullptr, "Startup failed", "The configured database could not be opened.");
                QApplication::quit();
                return;
            }
            auto* mainWindow = new UI::MainWindow();
            mainWindow->show();
            splash.finish(mainWindow);
        }
    });

    splash.startLoading(); // Start the splash screen loading animation

    return a.exec();
}
