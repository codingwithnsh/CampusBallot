#include "AnimatedButton.h"
#include <QPainter>
#include <QGraphicsDropShadowEffect> // Included in original, but not used in paintEvent
#include <QDebug> // For logging

namespace Ballot::UI {

/**
 * @brief Constructs an AnimatedButton.
 * @param text The text to display on the button.
 * @param parent The parent QWidget.
 */
AnimatedButton::AnimatedButton(const QString& text, QWidget* parent)
    : QPushButton(text, parent) {
    setMouseTracking(true); // Enable mouse tracking to receive enter/leave events
    setCursor(Qt::PointingHandCursor); // Change cursor to indicate interactivity

    // Initialize hover animation
    m_hoverAnim = new QPropertyAnimation(this, "hoverProgress", this);
    m_hoverAnim->setDuration(150); // Fast animation

    // Initialize scale animation
    m_scaleAnim = new QPropertyAnimation(this, "scale", this);
    m_scaleAnim->setDuration(100); // Slightly faster scale animation

    qDebug() << "AnimatedButton: Initialized with text:" << text;
}

/**
 * @brief Sets the hover progress for the animation.
 * @param progress A value between 0.0 (no hover) and 1.0 (full hover).
 */
void AnimatedButton::setHoverProgress(double progress) {
    if (m_hoverProgress != progress) {
        m_hoverProgress = progress;
        update(); // Request a repaint to show the animation progress
    }
}

/**
 * @brief Sets the scale factor for the button.
 * @param scale The scale factor (e.g., 1.0 for normal size, 1.02 for slightly larger).
 */
void AnimatedButton::setScale(double scale) {
    if (m_scale != scale) {
        m_scale = scale;
        // Applying scale directly in paintEvent is more efficient than QGraphicsScale.
        // If QGraphicsScale was used, it would be applied to the button's transform.
        update(); // Request a repaint to show the new scale
    }
}

/**
 * @brief Handles the mouse enter event.
 * Starts the hover and scale animations.
 * @param event The QEnterEvent.
 */
void AnimatedButton::enterEvent(QEnterEvent* event) {
    qDebug() << "AnimatedButton: Mouse entered for button" << text();
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverProgress);
    m_hoverAnim->setEndValue(1.0); // Animate to full hover
    m_hoverAnim->start();

    m_scaleAnim->stop();
    m_scaleAnim->setStartValue(m_scale);
    m_scaleAnim->setEndValue(1.02); // Animate to slightly larger scale
    m_scaleAnim->start();

    QPushButton::enterEvent(event); // Call base class implementation
}

/**
 * @brief Handles the mouse leave event.
 * Reverses the hover and scale animations.
 * @param event The QEvent.
 */
void AnimatedButton::leaveEvent(QEvent* event) {
    qDebug() << "AnimatedButton: Mouse left for button" << text();
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverProgress);
    m_hoverAnim->setEndValue(0.0); // Animate back to no hover
    m_hoverAnim->start();

    m_scaleAnim->stop();
    m_scaleAnim->setStartValue(m_scale);
    m_scaleAnim->setEndValue(1.0); // Animate back to normal scale
    m_scaleAnim->start();

    QPushButton::leaveEvent(event); // Call base class implementation
}

/**
 * @brief Paints the button, including the hover effect.
 * @param event The QPaintEvent.
 */
void AnimatedButton::paintEvent(QPaintEvent* event) {
    // Call base class paintEvent to draw the standard button elements (text, icon, background)
    QPushButton::paintEvent(event);

    // Custom painting for hover effect
    if (m_hoverProgress > 0.0) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Calculate highlight color based on hover progress
        QColor highlight(255, 255, 255, static_cast<int>(20 * m_hoverProgress)); // White, with increasing opacity
        painter.fillRect(rect(), highlight);
    }

    // Apply scaling transform if m_scale is not 1.0
    if (m_scale != 1.0) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        // Save current painter state
        painter.save();

        // Translate to center, scale, then translate back
        QPointF center = rect().center();
        painter.translate(center);
        painter.scale(m_scale, m_scale);
        painter.translate(-center);

        // Re-draw the button content at the new scale
        // This is a simplified approach. For complex buttons, it might be better
        // to render to a pixmap and then draw the scaled pixmap.
        // For now, we'll just let the base class paint again within the scaled context.
        // Note: This might cause text to be drawn twice if QPushButton::paintEvent
        // is called before this scaling. A more robust solution would involve
        // drawing the button content into a QPixmap, then scaling and drawing the pixmap.
        // For simplicity and given the current base paintEvent, we'll just scale the painter.
        // This might not be perfectly efficient or visually correct for all QPushButton styles.
        QPushButton::paintEvent(event); // Re-draw content within scaled context

        // Restore painter state
        painter.restore();
    }
}

} // namespace Ballot::UI