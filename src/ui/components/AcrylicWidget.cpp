#include "AcrylicWidget.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QDebug> // For logging

namespace Ballot::UI {

AcrylicWidget::AcrylicWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    qDebug() << "AcrylicWidget: Initialized.";
}

void AcrylicWidget::setTintColor(const QColor& color) {
    if (m_tintColor != color) {
        m_tintColor = color;
        update(); // Request a repaint
        qDebug() << "AcrylicWidget: Tint color set to" << color;
    }
}

void AcrylicWidget::setTintOpacity(double opacity) {
    if (m_tintOpacity != opacity) {
        m_tintOpacity = opacity;
        update(); // Request a repaint
        qDebug() << "AcrylicWidget: Tint opacity set to" << opacity;
    }
}

// Removed setBlurRadius as it's not currently implemented for actual blur
/*
void AcrylicWidget::setBlurRadius(double radius) {
    if (m_blurRadius != radius) {
        m_blurRadius = radius;
        update(); // Request a repaint
        qDebug() << "AcrylicWidget: Blur radius set to" << radius;
    }
}
*/

void AcrylicWidget::setAcrylicEnabled(bool enabled) {
    if (m_acrylicEnabled != enabled) {
        m_acrylicEnabled = enabled;
        update(); // Request a repaint
        qDebug() << "AcrylicWidget: Acrylic effect enabled set to" << enabled;
    }
}

void AcrylicWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event); // Mark event as unused to suppress warnings

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!m_acrylicEnabled) {
        // If acrylic is disabled, just paint a solid background or let parent handle it
        painter.fillRect(rect(), palette().window()); // Use widget's background color
        return;
    }

    // --- Acrylic Effect Simulation ---
    // This implementation simulates the acrylic effect by drawing a semi-transparent
    // colored layer and adding a subtle noise overlay.
    // It does NOT actually blur the content *behind* this widget.
    // For a true blur effect of content behind the widget, a more complex approach
    // involving QGraphicsEffect or rendering to an offscreen buffer would be needed.

    // 1. Draw semi-transparent background with tint color
    QColor bgColor = m_tintColor;
    bgColor.setAlphaF(m_tintOpacity); // Apply opacity to the tint color
    painter.fillRect(rect(), bgColor);

    // 2. Add subtle border glow (optional visual enhancement)
    QPen pen(QColor(255, 255, 255, 20), 1); // Light grey, low opacity
    painter.setPen(pen);
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 12, 12); // Adjust for pen width

    // 3. Add noise overlay for texture (simulates frosted glass)
    // This can be performance intensive if too many points are drawn.
    // Consider pre-rendering a noise texture if performance is an issue.
    painter.setPen(QPen(QColor(255, 255, 255, 4))); // Very light white, very low opacity
    QRandomGenerator* rng = QRandomGenerator::global();
    for (int i = 0; i < 50; ++i) { // Draw 50 random points for noise
        int x = rng->bounded(width());
        int y = rng->bounded(height());
        painter.drawPoint(x, y);
    }
}

} // namespace Ballot::UI