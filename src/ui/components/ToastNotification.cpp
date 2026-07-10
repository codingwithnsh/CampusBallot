#include "ToastNotification.h"
#include <QVBoxLayout>
#include <QApplication>
#include <QScreen> // For positioning on screen
#include <QDebug> // For logging

namespace Ballot::UI {

/**
 * @brief Static method to show a toast notification.
 * This is the primary way to create and display a toast.
 * @param parent The parent widget. The toast will be positioned relative to this parent.
 * @param message The message to display in the toast.
 * @param type The type of toast (Info, Success, Warning, Error) to determine styling.
 * @param durationMs How long the toast should remain visible in milliseconds.
 */
void ToastNotification::show(QWidget* parent, const QString& message,
                              Type type, int durationMs) {
    // Create a new ToastNotification instance
    auto* toast = new ToastNotification(parent, message, type, durationMs);
    toast->raise(); // Bring to front
    toast->animateIn(); // Start the fade-in animation
    qInfo() << "ToastNotification: Showing type" << type << "message:" << message;
}

/**
 * @brief Private constructor for ToastNotification.
 * @param parent The parent widget.
 * @param message The message to display.
 * @param type The type of toast.
 * @param durationMs The duration in milliseconds.
 */
ToastNotification::ToastNotification(QWidget* parent, const QString& message,
                                       Type type, int durationMs)
    : QWidget(parent), m_type(type) {
    // Set window flags for a frameless, always-on-top, tool window
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground); // Allow transparency
    setAttribute(Qt::WA_DeleteOnClose); // Automatically delete when closed

    setFixedWidth(400); // Fixed width for the toast

    // Setup opacity effect for fade animations
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(0.0); // Start fully transparent
    setGraphicsEffect(m_opacityEffect);

    // Main layout for the toast content
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12); // Padding inside the toast

    // Determine background color based on toast type
    QString bgColor;
    switch (type) {
        case Info:    bgColor = "#2b2b40"; break; // Dark grey/blue
        case Success: bgColor = "#1b5e20"; break; // Dark green
        case Warning: bgColor = "#e65100"; break; // Orange
        case Error:   bgColor = "#b71c1c"; break; // Dark red
        default:      bgColor = "#2b2b40"; break; // Fallback
    }

    // Apply styling
    setStyleSheet(QString(
        "background-color: %1; border-radius: 8px; border: 1px solid rgba(255,255,255,0.1);").arg(bgColor));

    m_messageLabel = new QLabel(message, this);
    m_messageLabel->setStyleSheet("color: white; font-size: 13px; font-weight: 500; background: transparent;");
    m_messageLabel->setWordWrap(true); // Enable word wrapping for long messages
    layout->addWidget(m_messageLabel);

    // Position the toast at the top-right of the parent or screen
    if (parent) {
        // Position relative to parent's top-right corner
        QPoint pos = parent->mapToGlobal(QPoint(parent->width() - width() - 20, 20));
        move(pos);
    } else {
        // If no parent, position relative to the primary screen's top-right
        QScreen *screen = QApplication::primaryScreen();
        if (screen) {
            QRect screenRect = screen->availableGeometry();
            QPoint pos(screenRect.topRight().x() - width() - 20, screenRect.topRight().y() + 20);
            move(pos);
        }
    }

    // Setup timer for auto-hide
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true); // Timer fires only once
    connect(m_timer, &QTimer::timeout, this, &ToastNotification::animateOut); // Connect to animateOut slot
    m_timer->start(durationMs); // Start the timer

    // Setup fade animation
    m_fadeAnim = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_fadeAnim->setDuration(300); // Fade duration
}

/**
 * @brief Starts the fade-in animation for the toast.
 */
void ToastNotification::animateIn() {
    QWidget::show(); // Make the widget visible
    m_fadeAnim->stop();
    m_fadeAnim->setStartValue(0.0); // Start from fully transparent
    m_fadeAnim->setEndValue(1.0); // End at fully opaque
    m_fadeAnim->start();
    qDebug() << "ToastNotification: Fade-in animation started.";
}

/**
 * @brief Starts the fade-out animation for the toast.
 */
void ToastNotification::animateOut() {
    m_fadeAnim->stop();
    // Connect animation finished signal to close the widget only for fade-out
    connect(m_fadeAnim, &QPropertyAnimation::finished, this, &QWidget::close);
    m_fadeAnim->setStartValue(1.0); // Start from fully opaque
    m_fadeAnim->setEndValue(0.0); // End at fully transparent
    m_fadeAnim->start();
    qDebug() << "ToastNotification: Fade-out animation started.";
}

} // namespace Ballot::UI