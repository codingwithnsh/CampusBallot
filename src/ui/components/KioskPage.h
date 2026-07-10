#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "src/core/models/ElectionConfiguration.h"

namespace Ballot::UI {

class KioskPage : public QWidget {
    Q_OBJECT
public:
    explicit KioskPage(const QString& title, const QString& subtitle = QString(), QWidget* parent = nullptr);
    ~KioskPage() override = default;

    void setTitle(const QString& title);
    void setSubtitle(const QString& subtitle);
    void setIcon(const QString& iconText);
    void setIconSize(int size);

    QVBoxLayout* contentLayout() const { return m_contentLayout; }
    QWidget* contentWidget() const { return m_contentWidget; }

    virtual void onPageShown() {}
    virtual void onPageHidden() {}
    virtual bool canNavigateForward() const { return true; }
    virtual bool canNavigateBackward() const { return true; }

    void applyTheme(const Ballot::Core::Models::ThemeColors& colors);
    void applyFont(const QString& family, int baseSize, int headingSize);

signals:
    void navigateForwardRequested();
    void navigateBackwardRequested();
    void pageCompleted();

protected:
    QLabel* m_iconLabel;
    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
    QVBoxLayout* m_mainLayout;

    Ballot::Core::Models::ThemeColors m_colors;
    QString m_fontFamily;
    int m_baseFontSize;
    int m_headingFontSize;

    void setupUi();
    void createHeader();
    virtual QWidget* createContent() { return nullptr; }
};

} // namespace Ballot::UI