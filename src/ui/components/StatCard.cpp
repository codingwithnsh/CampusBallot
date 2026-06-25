#include "StatCard.h"
#include <QLocale>
#include <QGraphicsDropShadowEffect>

namespace Ballot::UI {

StatCard::StatCard(const QString& title, const QString& value,
                   const QString& accentColor, const QString& icon,
                   QWidget* parent)
    : QFrame(parent), m_accentColor(accentColor) {
    setObjectName("statCard");
    setFixedHeight(140);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(8);

    // Accent bar at top
    m_accentBar = new QFrame(this);
    m_accentBar->setFixedHeight(4);
    m_accentBar->setStyleSheet(QString("background-color: %1; border-radius: 2px;").arg(accentColor));
    layout->addWidget(m_accentBar);

    // Icon row
    auto* headerLayout = new QHBoxLayout();
    if (!icon.isEmpty()) {
        m_iconLabel = new QLabel(this);
        m_iconLabel->setFixedSize(24, 24);
        m_iconLabel->setStyleSheet("background: transparent;");
        headerLayout->addWidget(m_iconLabel);
    }
    headerLayout->addStretch();
    layout->addLayout(headerLayout);

    // Title
    m_titleLabel = new QLabel(title, this);
    m_titleLabel->setObjectName("statLabel");
    layout->addWidget(m_titleLabel);

    // Value
    m_valueLabel = new QLabel(value, this);
    m_valueLabel->setObjectName("statValue");
    layout->addWidget(m_valueLabel);

    // Shadow effect
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 40));
    shadow->setOffset(0, 4);
    setGraphicsEffect(shadow);

    m_valueAnim = new QPropertyAnimation(this, "displayValue", this);
    m_valueAnim->setEasingCurve(QEasingCurve::OutCubic);
}

void StatCard::setValue(const QString& value) {
    m_valueLabel->setText(value);
}

void StatCard::setAccentColor(const QString& color) {
    m_accentColor = color;
    m_accentBar->setStyleSheet(QString("background-color: %1; border-radius: 2px;").arg(color));
}

void StatCard::animateValue(int targetValue, int duration) {
    m_valueAnim->setDuration(duration);
    m_valueAnim->setStartValue(m_displayValue);
    m_valueAnim->setEndValue(targetValue);
    m_valueAnim->start();
}

void StatCard::setIcon(const QString& iconPath) {
    if (m_iconLabel) {
        m_iconLabel->setPixmap(QPixmap(iconPath).scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void StatCard::setDisplayValue(int value) {
    m_displayValue = value;
    m_valueLabel->setText(QLocale().toString(value));
}

} // namespace Ballot::UI
