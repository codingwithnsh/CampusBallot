#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>
#include <QApplication>

namespace Ballot::Core {

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    enum Theme {
        Modern,
        Light,
        Dark
    };
    Q_ENUM(Theme)

    static ThemeManager& instance();

    static Theme fromString(const QString& themeName);
    static QString toString(Theme theme);

    void applyTheme(Theme theme);
    void applyTheme(const QString& themeName);
    Theme currentTheme() const { return m_currentTheme; }

signals:
    void themeChanged(Theme theme);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    Q_DISABLE_COPY(ThemeManager)

    static QString getThemeStylesheet(Theme theme); // Made static

    Theme m_currentTheme;
};

} // namespace Ballot::Core

#endif // THEMEMANAGER_H