#include "Sidebar.h"
#include "src/core/Constants.h" // For APP_VERSION
#include <QPainter>
#include <QGraphicsDropShadowEffect> // Included in original, but not used in paintEvent
#include <QDebug> // For logging
#include <QStyleOptionButton> // For proper button painting

namespace Ballot::UI {

// ============ SidebarButton ============

/**
 * @brief Constructs a SidebarButton.
 * @param text The text to display on the button.
 * @param iconChar The character code for an icon (e.g., FontAwesome).
 * @param parent The parent QWidget.
 */
SidebarButton::SidebarButton(const QString& text, const QString& iconChar, QWidget* parent)
    : QPushButton(iconChar.isEmpty() ? text : QString("%1  %2").arg(iconChar, text), parent),
      m_iconChar(iconChar) {
    setFixedHeight(44);
    setCursor(Qt::PointingHandCursor);
    setCheckable(true); // Buttons can be checked/unchecked for active state
    setObjectName("SidebarButton"); // For QSS targeting

    // Initial stylesheet for inactive state
    setStyleSheet(R"(
        #SidebarButton {
            background-color: transparent;
            border: none;
            border-radius: 8px;
            padding: 8px 14px;
            text-align: left;
            font-size: 14px;
            font-weight: 500;
            color: #9a9ab0; /* Default inactive text color */
        }
        #SidebarButton:hover {
            background-color: rgba(255, 255, 255, 0.06); /* Subtle hover background */
            color: #e0e0e0; /* Lighter text on hover */
        }
        #SidebarButton:checked { /* Style for active state, overridden by setActive() */
            background-color: rgba(0, 120, 212, 0.15); /* Active background color */
            color: #0078d4; /* Active text color */
            font-weight: 600;
        }
    )");

    m_badge = new QLabel(this);
    m_badge->setVisible(false);
    m_badge->setFixedSize(22, 22);
    m_badge->setStyleSheet(R"(
        QLabel {
            background-color: #d32f2f; /* Red badge color */
            color: white;
            border-radius: 11px;
            font-size: 11px;
            font-weight: bold;
            padding: 2px;
        }
    )");
    m_badge->setAlignment(Qt::AlignCenter);
    qDebug() << "SidebarButton: Created button with text:" << text << "and icon:" << iconChar;
}

/**
 * @brief Sets the active state of the button.
 * @param active True to set as active, false otherwise.
 */
void SidebarButton::setActive(bool active) {
    if (m_active != active) {
        m_active = active;
        setChecked(active); // Update QAbstractButton's checked state
        // The stylesheet for :checked pseudo-state should handle the visual change.
        // If more complex logic is needed, apply specific QSS here.
        qDebug() << "SidebarButton:" << text() << "active state set to" << active;
        update(); // Request repaint
    }
}

/**
 * @brief Sets the badge count for the button.
 * @param count The number to display in the badge. If 0, the badge is hidden.
 */
void SidebarButton::setBadge(int count) {
    if (count < 0) count = 0; // Ensure non-negative count
    m_badge->setText(QString::number(count));
    m_badge->setVisible(count > 0);
    qDebug() << "SidebarButton:" << text() << "badge count set to" << count;
    update(); // Request repaint to reposition badge if needed
}

/**
 * @brief Custom paint event for the button.
 * Used to position the badge label.
 * @param event The QPaintEvent.
 */
void SidebarButton::paintEvent(QPaintEvent* event) {
    // Let QPushButton draw itself first
    QPushButton::paintEvent(event);

    // Position the badge label
    // It's generally more efficient to handle widget positioning in resizeEvent or layout management
    // rather than paintEvent, which can be called frequently. For this audit, we'll keep it here
    // but note it as an area for potential optimization.
    if (m_badge->isVisible()) {
        // Position badge in the top-right corner of the button, with some padding
        m_badge->move(width() - m_badge->width() - 12, (height() - m_badge->height()) / 2);
    }
}

// ============ Sidebar ============

/**
 * @brief Constructs a Sidebar widget.
 * @param parent The parent QWidget.
 */
Sidebar::Sidebar(QWidget* parent) : QWidget(parent) {
    setFixedWidth(240); // Fixed width for the sidebar
    setObjectName("Sidebar"); // For QSS targeting

    // Main layout for the sidebar
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(2);

    // --- User Info Section ---
    auto* userFrame = new QFrame(this);
    userFrame->setObjectName("UserFrame"); // For QSS targeting
    userFrame->setStyleSheet("background: transparent;"); // Ensure transparent background
    auto* userLayout = new QHBoxLayout(userFrame);
    userLayout->setContentsMargins(0, 0, 0, 0); // No extra margins for user info layout

    m_avatar = new QLabel(userFrame);
    m_avatar->setFixedSize(40, 40);
    m_avatar->setStyleSheet("background-color: #0078d4; border-radius: 20px;"); // Default avatar style
    m_avatar->setAlignment(Qt::AlignCenter);
    userLayout->addWidget(m_avatar);

    auto* nameLayout = new QVBoxLayout();
    nameLayout->setContentsMargins(8, 0, 0, 0); // Padding between avatar and text
    m_userName = new QLabel("Guest", userFrame); // Default user name
    m_userName->setObjectName("UserNameLabel");
    m_userName->setStyleSheet("font-weight: 600; font-size: 14px; color: #e0e0e0; background: transparent;");
    m_userRole = new QLabel("Role", userFrame); // Default role
    m_userRole->setObjectName("UserRoleLabel");
    m_userRole->setStyleSheet("font-size: 12px; color: #9a9ab0; background: transparent;");
    nameLayout->addWidget(m_userName);
    nameLayout->addWidget(m_userRole);
    userLayout->addLayout(nameLayout);
    userLayout->addStretch(); // Pushes content to the left
    mainLayout->addWidget(userFrame);

    mainLayout->addSpacing(16); // Spacing after user info

    // --- Separator ---
    auto* sep = new QFrame(this);
    sep->setFixedHeight(1);
    sep->setObjectName("Separator");
    sep->setStyleSheet("background-color: #2d2d44;"); // Separator color
    mainLayout->addWidget(sep);
    mainLayout->addSpacing(8); // Spacing after separator

    // --- Scroll Area for Navigation Items ---
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }"); // Transparent background for scroll area

    auto* scrollContent = new QWidget();
    scrollContent->setStyleSheet("background: transparent;");
    m_itemsLayout = new QVBoxLayout(scrollContent);
    m_itemsLayout->setContentsMargins(0, 0, 0, 0);
    m_itemsLayout->setSpacing(2);
    m_itemsLayout->addStretch(); // Pushes buttons to the top

    scroll->setWidget(scrollContent);
    mainLayout->addWidget(scroll, 1); // Add scroll area, stretching to fill available space

    // --- Application Version Label ---
    auto* versionLabel = new QLabel(QString("v%1").arg(Core::Constants::APP_VERSION), this); // Use constant for version
    versionLabel->setObjectName("VersionLabel");
    versionLabel->setStyleSheet("font-size: 11px; color: #5a5a7a; padding: 8px; background: transparent;");
    versionLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(versionLabel);

    // --- Overall Sidebar Styling ---
    setStyleSheet(R"(
        #Sidebar {
            background-color: #1a1a2e; /* Dark background */
            border-right: 1px solid #2d2d44; /* Subtle right border */
        }
    )");
    qDebug() << "Sidebar: Created.";
}

/**
 * @brief Adds a new navigation item (button) to the sidebar.
 * @param id A unique identifier for the item.
 * @param text The text to display on the button.
 * @param icon The character code for an icon (e.g., FontAwesome).
 */
void Sidebar::addItem(const QString& id, const QString& text, const QString& icon) {
    if (m_buttons.contains(id)) {
        qWarning() << "Sidebar: Item with ID" << id << "already exists. Skipping.";
        return;
    }
    auto* btn = new SidebarButton(text, icon, this);
    m_buttons[id] = btn;
    // Insert before the stretch to ensure new items are at the top
    m_itemsLayout->insertWidget(m_itemsLayout->count() - 1, btn);

    connect(btn, &QPushButton::clicked, this, [this, id]() {
        qDebug() << "Sidebar: Item clicked:" << id;
        setActiveItem(id);
        emit itemClicked(id);
    });
    qInfo() << "Sidebar: Added item:" << id << "with text:" << text;
}

/**
 * @brief Sets the currently active navigation item.
 * @param id The ID of the item to set as active.
 */
void Sidebar::setActiveItem(const QString& id) {
    if (m_activeId == id) {
        qDebug() << "Sidebar: Item" << id << "is already active.";
        return;
    }

    // Deactivate previous active item
    if (!m_activeId.isEmpty() && m_buttons.contains(m_activeId)) {
        m_buttons[m_activeId]->setActive(false);
        qDebug() << "Sidebar: Deactivated previous item:" << m_activeId;
    }

    // Activate new item
    if (m_buttons.contains(id)) {
        m_activeId = id;
        m_buttons[id]->setActive(true);
        qInfo() << "Sidebar: Set active item to:" << id;
    } else {
        qWarning() << "Sidebar: Attempted to set unknown item as active:" << id;
        m_activeId.clear(); // Clear active ID if not found
    }
}

/**
 * @brief Sets the badge count for a specific navigation item.
 * @param id The ID of the item.
 * @param count The badge count.
 */
void Sidebar::setItemBadge(const QString& id, int count) {
    if (m_buttons.contains(id)) {
        m_buttons[id]->setBadge(count);
        qDebug() << "Sidebar: Item" << id << "badge set to" << count;
    } else {
        qWarning() << "Sidebar: Attempted to set badge for unknown item:" << id;
    }
}

/**
 * @brief Sets the visibility of a specific navigation item.
 * @param id The ID of the item.
 * @param visible True to make visible, false to hide.
 */
void Sidebar::setItemVisible(const QString& id, bool visible) {
    if (m_buttons.contains(id)) {
        m_buttons[id]->setVisible(visible);
        qDebug() << "Sidebar: Item" << id << "visibility set to" << visible;
    } else {
        qWarning() << "Sidebar: Attempted to set visibility for unknown item:" << id;
    }
}

/**
 * @brief Sets the user information displayed in the sidebar.
 * @param name The user's name.
 * @param role The user's role.
 * @param avatar The user's avatar pixmap.
 */
void Sidebar::setUserInfo(const QString& name, const QString& role, const QPixmap& avatar) {
    m_userName->setText(name);
    m_userRole->setText(role);
    if (!avatar.isNull()) {
        // Scale avatar to fit the fixed size while maintaining aspect ratio
        m_avatar->setPixmap(avatar.scaled(m_avatar->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        qDebug() << "Sidebar: User avatar set.";
    } else {
        // Clear pixmap or set a default if no avatar is provided
        m_avatar->clear();
        qDebug() << "Sidebar: User avatar cleared or no avatar provided.";
    }
    qInfo() << "Sidebar: User info updated to Name:" << name << ", Role:" << role;
}

} // namespace Ballot::UI