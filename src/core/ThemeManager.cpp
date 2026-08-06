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

ThemeManager::Theme ThemeManager::fromString(const QString& themeName)
{
    const QString normalized = themeName.trimmed();
    if (normalized.compare("Light", Qt::CaseInsensitive) == 0) {
        return Theme::Light;
    }
    if (normalized.compare("Dark", Qt::CaseInsensitive) == 0) {
        return Theme::Dark;
    }
    return Theme::Modern;
}

QString ThemeManager::toString(Theme theme)
{
    return QString::fromLatin1(QMetaEnum::fromType<Theme>().valueToKey(theme));
}

void ThemeManager::applyTheme(Theme theme)
{
    QString stylesheetPath = getThemeStylesheet(theme);
    QFile file(stylesheetPath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << "ThemeManager: Could not open stylesheet file:" << stylesheetPath;
        if (theme == Theme::Modern) {
            return;
        }

        theme = Theme::Modern;
        stylesheetPath = getThemeStylesheet(theme);
        file.setFileName(stylesheetPath);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            qCritical() << "ThemeManager: Failed to load fallback stylesheet:" << stylesheetPath;
            return;
        }
    }

    const QString stylesheet = QString::fromUtf8(file.readAll());
    qApp->setStyleSheet(stylesheet);
    m_currentTheme = theme;
    qInfo() << "ThemeManager: Applied theme:" << toString(theme);
    emit themeChanged(theme);

    // Save the selected theme to QSettings
    QSettings settings;
    settings.setValue("theme", toString(theme));
}

void ThemeManager::applyTheme(const QString& themeName)
{
    applyTheme(fromString(themeName));
}

QString ThemeManager::getThemeStylesheet(Theme theme)
{
    switch (theme) {
        case Theme::Modern: return ":/src/ui/styles/modern.qss";
        case Theme::Light:  return ":/src/ui/styles/light.qss";
        case Theme::Dark:   return ":/src/ui/styles/dark.qss";
        default:            return ":/src/ui/styles/modern.qss";
    }
}

} // namespace Ballot::Core
