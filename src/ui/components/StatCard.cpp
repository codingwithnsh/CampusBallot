#include "StatCard.h"
#include <QLocale>
#include <QGraphicsDropShadowEffect>
#include <QPainter> // For custom painting if needed
#include <QDebug> // For logging

namespace Ballot::UI {

/**
 * @brief Constructs a StatCard.
 * @param title The title of the statistic.
 * @param value The initial value to display.
 * @param accentColor The accent color for the card.
 * @param icon The path to an icon image.
 * @param parent The parent QWidget.
 */
StatCard::StatCard(const QString& title, const QString& value,
                   const QString& accentColor, const QString& icon,
                   QWidget* parent)
    : QFrame(parent), m_accentColor(accentColor) {
    setObjectName("StatCard"); // For QSS targeting
    setFixedHeight(140); // Fixed height for consistent layout

    // Main layout for the card content
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 16, 20, 16); // Padding inside the card
    mainLayout->setSpacing(8); // Spacing between elements

    // --- Accent Bar ---
    m_accentBar = new QFrame(this);
    m_accentBar->setFixedHeight(4);
    m_accentBar->setObjectName("AccentBar");
    m_accentBar->setStyleSheet(QString("background-color: %1; border-radius: 2px;").arg(accentColor));
    mainLayout->addWidget(m_accentBar);

    // --- Header Layout (for icon and potential other elements) ---
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0,0,0,0); // No extra margins
    headerLayout->setSpacing(0); // No spacing

    m_iconLabel = nullptr; // Initialize to nullptr
    if (!icon.isEmpty()) {
        m_iconLabel = new QLabel(this);
        m_iconLabel->setFixedSize(24, 24); // Fixed size for the icon
        m_iconLabel->setStyleSheet("background: transparent;");
        setIcon(icon); // Set the icon pixmap
        headerLayout->addWidget(m_iconLabel);
    }
    headerLayout->addStretch(); // Pushes icon to the left
    mainLayout->addLayout(headerLayout);

    // --- Title Label ---
    m_titleLabel = new QLabel(title, this);
    m_titleLabel->setObjectName("StatTitleLabel");
    mainLayout->addWidget(m_titleLabel);

    // --- Value Label ---
    m_valueLabel = new QLabel(value, this);
    m_valueLabel->setObjectName("StatValueLabel");
    mainLayout->addWidget(m_valueLabel);

    mainLayout->addStretch(); // Pushes content to the top

    // --- Shadow Effect ---
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 40)); // Dark grey with some transparency
    shadow->setOffset(0, 4); // Offset shadow downwards
    setGraphicsEffect(shadow);

    // --- Value Animation ---
    m_valueAnim = new QPropertyAnimation(this, "displayValue", this);
    m_valueAnim->setEasingCurve(QEasingCurve::OutCubic); // Smooth animation curve

    // Apply base styling for the card
    setStyleSheet(R"(
        #StatCard {
            background-color: #2a2a3e; /* Dark background for the card */
            border-radius: 12px; /* Rounded corners */
        }
        #StatTitleLabel {
            color: #9a9ab0; /* Muted text color for title */
            font-size: 13px;
            font-weight: 500;
        }
        #StatValueLabel {
            color: #e0e0e0; /* Lighter text color for value */
            font-size: 28px;
            font-weight: 700;
        }
    )");
    qDebug() << "StatCard: Created with title:" << title << "and value:" << value;
}

/**
 * @brief Sets the displayed value of the card.
 * @param value The new value as a QString.
 */
void StatCard::setValue(const QString& value) {
    if (m_valueLabel->text() != value) {
        m_valueLabel->setText(value);
        qDebug() << "StatCard:" << m_titleLabel->text() << "value set to" << value;
    }
}

/**
 * @brief Sets the accent color of the card.
 * @param color The new accent color (e.g., "#RRGGBB").
 */
void StatCard::setAccentColor(const QString& color) {
    if (m_accentColor != color) {
        m_accentColor = color;
        m_accentBar->setStyleSheet(QString("background-color: %1; border-radius: 2px;").arg(color));
        qDebug() << "StatCard:" << m_titleLabel->text() << "accent color set to" << color;
    }
}

/**
 * @brief Animates the numeric value of the card from its current display value to a target value.
 * @param targetValue The final integer value.
 * @param duration The duration of the animation in milliseconds.
 */
void StatCard::animateValue(int targetValue, int duration) {
    if (m_valueAnim->state() == QAbstractAnimation::Running) {
        m_valueAnim->stop();
    }
    m_valueAnim->setDuration(duration);
    m_valueAnim->setStartValue(m_displayValue); // Start from current displayed value
    m_valueAnim->setEndValue(targetValue);
    m_valueAnim->start();
    qDebug() << "StatCard:" << m_titleLabel->text() << "animating value from" << m_displayValue << "to" << targetValue;
}

/**
 * @brief Sets the icon for the card from a file path.
 * @param iconPath The path to the icon image.
 */
void StatCard::setIcon(const QString& iconPath) {
    if (m_iconLabel) {
        QPixmap pixmap(iconPath);
        if (!pixmap.isNull()) {
            m_iconLabel->setPixmap(pixmap.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            qDebug() << "StatCard:" << m_titleLabel->text() << "icon set from" << iconPath;
        } else {
            qWarning() << "StatCard: Failed to load icon from" << iconPath;
        }
    } else {
        qWarning() << "StatCard: Attempted to set icon, but iconLabel is null. Ensure icon is passed in constructor.";
    }
}

/**
 * @brief Property setter for displayValue, used by QPropertyAnimation.
 * Updates the value label with the animated integer.
 * @param value The current animated integer value.
 */
void StatCard::setDisplayValue(int value) {
    if (m_displayValue != value) {
        m_displayValue = value;
        m_valueLabel->setText(QLocale().toString(value)); // Format number with locale
    }
}

} // namespace Ballot::UI