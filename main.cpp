#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>

#include "src/core/Constants.h"
#include "src/core/ThemeManager.h"
#include "src/core/config/ApplicationBootstrap.h"
#include "src/modules/auth/RBACManager.h"
#include "src/modules/plugin/PluginManager.h"
#include "src/ui/views/MainWindow.h"
#include "src/ui/views/SetupWizard.h"
#include "src/ui/views/SplashScreen.h"

using namespace Ballot;

namespace {

void installFileLogger()
{
    static QFile logFile(QDir(Core::ApplicationBootstrap::appDataPath()).filePath(Core::Constants::LOG_FILE));
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

void runSetupWizard(QSettings& settings, UI::SplashScreen* splash = nullptr)
{
    auto* wizard = new UI::SetupWizard();
    QObject::connect(wizard, &UI::SetupWizard::setupCompleted, [wizard, &settings](const QVariantMap& config) mutable {
        const Core::BootstrapResult result = Core::ApplicationBootstrap::initializeRuntime(config);
        if (!result.success) {
            QMessageBox::critical(wizard, "Setup failed", result.errorMessage);
            return;
        }

        Core::ApplicationBootstrap::saveConfiguration(settings, result.sanitizedConfig);
        auto* mainWindow = new UI::MainWindow();
        mainWindow->show();
        wizard->deleteLater();
    });
    wizard->show();
    if (splash) {
        splash->close();
    }
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

    Core::ApplicationBootstrap::appDataPath();

    QSettings settings;
    QString resetError;
    if (!Core::ApplicationBootstrap::consumeResetRequest(settings, &resetError)) {
        QMessageBox::critical(nullptr, "Reset failed", resetError);
        return 1;
    }

    const bool isFirstRun = Core::ApplicationBootstrap::needsFirstRunSetup(settings);

    const QString savedThemeName = settings.value(
        "theme",
        Core::ThemeManager::toString(Core::ThemeManager::Modern)).toString();
    Core::ThemeManager::instance().applyTheme(savedThemeName);

    Auth::RBACManager::instance();
    Plugin::PluginManager::instance().loadAll();

    UI::SplashScreen splash;
    splash.show();
    qApp->processEvents();

    QObject::connect(&splash, &UI::SplashScreen::loadingFinished, [&]() {
        if (isFirstRun) {
            runSetupWizard(settings, &splash);
            return;
        }

        const QVariantMap loadedConfig = Core::ApplicationBootstrap::loadStoredConfiguration(settings);
        const Core::BootstrapResult result = Core::ApplicationBootstrap::initializeRuntime(loadedConfig);
        if (!result.success) {
            const auto choice = QMessageBox::warning(nullptr,
                                                     "Startup failed",
                                                     result.errorMessage + "\n\nWould you like to re-run setup now?",
                                                     QMessageBox::Yes | QMessageBox::No,
                                                     QMessageBox::Yes);
            if (choice == QMessageBox::Yes) {
                settings.setValue("first_run", true);
                settings.sync();
                runSetupWizard(settings, &splash);
                return;
            }

            QApplication::quit();
            return;
        }

        auto* mainWindow = new UI::MainWindow();
        splash.finish(mainWindow);
    });

    splash.startLoading();

    return qApp->exec();
}
