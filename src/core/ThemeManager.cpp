#include "ThemeManager.h"
#include <QFile>
#include <QDebug>
#include <QMetaEnum>
#include <QSettings> // Include QSettings

namespace Ballot::Core {

ThemeManager::ThemeManager(QObject *parent) : QObject(parent), m_currentTheme(Theme::Modern)
{
    // Default theme can be loaded here or explicitly set after creation
}

ThemeManager& ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}

void ThemeManager::applyTheme(Theme theme)
{
    QString stylesheetPath = getThemeStylesheet(theme);
    QFile file(stylesheetPath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString stylesheet = QLatin1String(file.readAll());
        qApp->setStyleSheet(stylesheet);
        m_currentTheme = theme;
        qInfo() << "ThemeManager: Applied theme:" << QMetaEnum::fromType<Theme>().valueToKey(theme);
        emit themeChanged(theme);

        // Save the selected theme to QSettings
        QSettings settings;
        settings.setValue("theme", QMetaEnum::fromType<Theme>().valueToKey(theme));
    } else {
        qWarning() << "ThemeManager: Could not open stylesheet file:" << stylesheetPath;
    }
}

void ThemeManager::applyTheme(const QString& themeName)
{
    Theme theme = Theme::Modern; // Default to Modern if not found
    if (themeName.compare("Light", Qt::CaseInsensitive) == 0) {
        theme = Theme::Light;
    } else if (themeName.compare("Dark", Qt::CaseInsensitive) == 0) {
        theme = Theme::Dark;
    }
    // Add more themes here as they are implemented

    applyTheme(theme);
}

QString ThemeManager::getThemeStylesheet(Theme theme)
{
    switch (theme) {
        case Theme::Modern: return ":/src/ui/styles/modern.qss";
        case Theme::Light:  return ":/src/ui/styles/light.qss";
        case Theme::Dark:   return ":/src/ui/styles/dark.qss"; // Assuming a dark.qss will be added
        default:            return ":/src/ui/styles/modern.qss";
    }
}

} // namespace Ballot::Core