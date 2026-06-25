#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QTimer>
#include <QDir>
#include <QStandardPaths>
#include <QIcon>
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

    // Initialize core managers
    Core::SystemManager::instance().initialize({
        {"db_path", Core::Constants::DB_FILENAME},
        {"storage_type", "sqlite"}
    });

    // Load RBAC roles
    Auth::RBACManager::instance();

    // Initialize plugin system
    Plugin::PluginManager::instance().loadAll();

    // Show splash screen
    UI::SplashScreen splash;
    splash.show();
    a.processEvents();

    QSettings settings;
    bool isFirstRun = settings.value("first_run", true).toBool();

    // Initialize system
    splash.startLoading();

    // When splash finishes, proceed
    UI::MainWindow* mainWindow = new UI::MainWindow();

    QObject::connect(&splash, &UI::SplashScreen::loadingFinished, [&]() {
        if (isFirstRun) {
            UI::SetupWizard* wizard = new UI::SetupWizard();
            QObject::connect(wizard, &UI::SetupWizard::setupCompleted, [mainWindow, &settings](const QVariantMap& config) {
                settings.setValue("first_run", false);
                settings.setValue("db_path", config.value("db_path", Core::Constants::DB_FILENAME).toString());
                Core::SystemManager::instance().initializeStorage(config);
                mainWindow->show();
            });
            wizard->show();
        } else {
            mainWindow->show();
        }

        splash.finish(mainWindow);
    });

    return a.exec();
}
