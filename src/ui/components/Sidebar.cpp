#include "Sidebar.h"
#include <QPainter>
#include <QGraphicsDropShadowEffect>

namespace Ballot::UI {

// ============ SidebarButton ============
SidebarButton::SidebarButton(const QString& text, const QString& iconChar, QWidget* parent)
    : QPushButton(iconChar.isEmpty() ? text : QString("%1  %2").arg(iconChar, text), parent),
      m_iconChar(iconChar) {
    setFixedHeight(44);
    setCursor(Qt::PointingHandCursor);
    setCheckable(true);
    setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: none;
            border-radius: 8px;
            padding: 8px 14px;
            text-align: left;
            font-size: 14px;
            font-weight: 500;
            color: #9a9ab0;
        }
        QPushButton:hover {
            background-color: rgba(255, 255, 255, 0.06);
            color: #e0e0e0;
        }
    )");

    m_badge = new QLabel(this);
    m_badge->setVisible(false);
    m_badge->setFixedSize(22, 22);
    m_badge->setStyleSheet(R"(
        background-color: #d32f2f;
        color: white;
        border-radius: 11px;
        font-size: 11px;
        font-weight: bold;
        padding: 2px;
    )");
    m_badge->setAlignment(Qt::AlignCenter);
}

void SidebarButton::setActive(bool active) {
    m_active = active;
    setChecked(active);
    if (active) {
        setStyleSheet(R"(
            QPushButton {
                background-color: rgba(0, 120, 212, 0.15);
                border: none;
                border-radius: 8px;
                padding: 8px 14px;
                text-align: left;
                font-size: 14px;
                font-weight: 600;
                color: #0078d4;
            }
        )");
    } else {
        setStyleSheet(R"(
            QPushButton {
                background-color: transparent;
                border: none;
                border-radius: 8px;
                padding: 8px 14px;
                text-align: left;
                font-size: 14px;
                font-weight: 500;
                color: #9a9ab0;
            }
            QPushButton:hover {
                background-color: rgba(255, 255, 255, 0.06);
                color: #e0e0e0;
            }
        )");
    }
}

void SidebarButton::setBadge(int count) {
    m_badge->setText(QString::number(count));
    m_badge->setVisible(count > 0);
}

void SidebarButton::paintEvent(QPaintEvent* event) {
    QPushButton::paintEvent(event);
    if (m_badge->isVisible()) {
        m_badge->move(width() - m_badge->width() - 12, (height() - m_badge->height()) / 2);
    }
}

// ============ Sidebar ============
Sidebar::Sidebar(QWidget* parent) : QWidget(parent) {
    setFixedWidth(240);
    setObjectName("sidebar");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(2);

    // User info section
    auto* userFrame = new QFrame(this);
    userFrame->setStyleSheet("background: transparent;");
    auto* userLayout = new QHBoxLayout(userFrame);

    m_avatar = new QLabel(userFrame);
    m_avatar->setFixedSize(40, 40);
    m_avatar->setStyleSheet("background-color: #0078d4; border-radius: 20px;");
    userLayout->addWidget(m_avatar);

    auto* nameLayout = new QVBoxLayout();
    m_userName = new QLabel("User", userFrame);
    m_userName->setStyleSheet("font-weight: 600; font-size: 14px; color: #e0e0e0; background: transparent;");
    m_userRole = new QLabel("Role", userFrame);
    m_userRole->setStyleSheet("font-size: 12px; color: #9a9ab0; background: transparent;");
    nameLayout->addWidget(m_userName);
    nameLayout->addWidget(m_userRole);
    userLayout->addLayout(nameLayout);
    userLayout->addStretch();
    mainLayout->addWidget(userFrame);

    mainLayout->addSpacing(16);

    // Separator
    auto* sep = new QFrame(this);
    sep->setFixedHeight(1);
    sep->setStyleSheet("background-color: #2d2d44;");
    mainLayout->addWidget(sep);
    mainLayout->addSpacing(8);

    // Scroll area for items
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto* scrollContent = new QWidget();
    scrollContent->setStyleSheet("background: transparent;");
    m_itemsLayout = new QVBoxLayout(scrollContent);
    m_itemsLayout->setContentsMargins(0, 0, 0, 0);
    m_itemsLayout->setSpacing(2);
    m_itemsLayout->addStretch();

    scroll->setWidget(scrollContent);
    mainLayout->addWidget(scroll, 1);

    // App version
    auto* versionLabel = new QLabel("v1.0.0", this);
    versionLabel->setStyleSheet("font-size: 11px; color: #5a5a7a; padding: 8px; background: transparent;");
    versionLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(versionLabel);

    setStyleSheet(R"(
        #sidebar {
            background-color: #1a1a2e;
            border-right: 1px solid #2d2d44;
        }
    )");
}

void Sidebar::addItem(const QString& id, const QString& text, const QString& icon) {
    auto* btn = new SidebarButton(text, icon, this);
    m_buttons[id] = btn;
    m_itemsLayout->insertWidget(m_itemsLayout->count() - 1, btn);

    connect(btn, &QPushButton::clicked, this, [this, id]() {
        setActiveItem(id);
        emit itemClicked(id);
    });
}

void Sidebar::setActiveItem(const QString& id) {
    if (!m_activeId.isEmpty() && m_buttons.contains(m_activeId)) {
        m_buttons[m_activeId]->setActive(false);
    }
    m_activeId = id;
    if (m_buttons.contains(id)) {
        m_buttons[id]->setActive(true);
    }
}

void Sidebar::setItemBadge(const QString& id, int count) {
    if (m_buttons.contains(id)) {
        m_buttons[id]->setBadge(count);
    }
}

void Sidebar::setItemVisible(const QString& id, bool visible) {
    if (m_buttons.contains(id)) m_buttons[id]->setVisible(visible);
}

void Sidebar::setUserInfo(const QString& name, const QString& role, const QPixmap& avatar) {
    m_userName->setText(name);
    m_userRole->setText(role);
    if (!avatar.isNull()) {
        m_avatar->setPixmap(avatar.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

} // namespace Ballot::UI
