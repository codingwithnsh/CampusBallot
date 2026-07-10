#include "KioskPage.h"
#include <QScrollArea>
#include <QApplication>
#include <QFont>

namespace Ballot::UI {

KioskPage::KioskPage(const QString& title, const QString& subtitle, QWidget* parent)
    : QWidget(parent)
    , m_iconLabel(nullptr)
    , m_titleLabel(nullptr)
    , m_subtitleLabel(nullptr)
    , m_contentWidget(nullptr)
    , m_contentLayout(nullptr)
    , m_mainLayout(nullptr)
    , m_fontFamily("Segoe UI")
    , m_baseFontSize(14)
    , m_headingFontSize(24)
{
    setupUi();
    setTitle(title);
    setSubtitle(subtitle);
}

void KioskPage::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(40, 30, 40, 30);
    m_mainLayout->setSpacing(24);
    m_mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    createHeader();

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }"
                              "QScrollBar:vertical { width: 8px; background: transparent; }"
                              "QScrollBar::handle:vertical { background: #3d3d5c; border-radius: 4px; min-height: 30px; }"
                              "QScrollBar::handle:vertical:hover { background: #555; }");

    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet("background: transparent;");
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(16);
    m_contentLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    auto* customContent = createContent();
    if (customContent) {
        m_contentLayout->addWidget(customContent);
    }
    m_contentLayout->addStretch();

    scrollArea->setWidget(m_contentWidget);
    m_mainLayout->addWidget(scrollArea, 1);
}

void KioskPage::createHeader() {
    m_iconLabel = new QLabel(this);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setStyleSheet("background: transparent;");

    m_titleLabel = new QLabel(this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setStyleSheet("background: transparent;");

    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setWordWrap(true);
    m_subtitleLabel->setStyleSheet("background: transparent;");

    m_mainLayout->addWidget(m_iconLabel);
    m_mainLayout->addWidget(m_titleLabel);
    m_mainLayout->addWidget(m_subtitleLabel);
    m_mainLayout->addSpacing(8);
}

void KioskPage::setTitle(const QString& title) {
    if (m_titleLabel) {
        m_titleLabel->setText(title);
        QFont font(m_fontFamily, m_headingFontSize, QFont::DemiBold);
        m_titleLabel->setFont(font);
    }
}

void KioskPage::setSubtitle(const QString& subtitle) {
    if (m_subtitleLabel) {
        m_subtitleLabel->setText(subtitle);
        m_subtitleLabel->setVisible(!subtitle.isEmpty());
        QFont font(m_fontFamily, m_baseFontSize);
        m_subtitleLabel->setFont(font);
    }
}

void KioskPage::setIcon(const QString& iconText) {
    if (m_iconLabel) {
        m_iconLabel->setText(iconText);
        QFont font = m_iconLabel->font();
        font.setPointSize(48);
        m_iconLabel->setFont(font);
    }
}

void KioskPage::setIconSize(int size) {
    if (m_iconLabel) {
        QFont font = m_iconLabel->font();
        font.setPointSize(size);
        m_iconLabel->setFont(font);
    }
}

void KioskPage::applyTheme(const Ballot::Core::Models::ThemeColors& colors) {
    m_colors = colors;
    
    QString style = QString(
        "QLabel { color: %1; }"
        "QWidget { background-color: %2; }"
    ).arg(colors.onBackground, colors.background);
    
    setStyleSheet(style);
    
    if (m_titleLabel) m_titleLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(colors.onBackground));
    if (m_subtitleLabel) m_subtitleLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(colors.onSurface));
    if (m_iconLabel) m_iconLabel->setStyleSheet("background: transparent;");
    if (m_contentWidget) m_contentWidget->setStyleSheet("background: transparent;");
}

void KioskPage::applyFont(const QString& family, int baseSize, int headingSize) {
    m_fontFamily = family;
    m_baseFontSize = baseSize;
    m_headingFontSize = headingSize;
    
    if (m_titleLabel) {
        QFont font(family, headingSize, QFont::DemiBold);
        m_titleLabel->setFont(font);
    }
    if (m_subtitleLabel) {
        QFont font(family, baseSize);
        m_subtitleLabel->setFont(font);
    }
}

} // namespace Ballot::UI