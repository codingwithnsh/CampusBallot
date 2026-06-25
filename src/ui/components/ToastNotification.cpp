#include "ToastNotification.h"
#include <QVBoxLayout>
#include <QApplication>

namespace Ballot::UI {

void ToastNotification::show(QWidget* parent, const QString& message,
                              Type type, int durationMs) {
    auto* toast = new ToastNotification(parent, message, type, durationMs);
    toast->raise();
    toast->animateIn();
}

ToastNotification::ToastNotification(QWidget* parent, const QString& message,
                                       Type type, int durationMs)
    : QWidget(parent), m_type(type) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setFixedWidth(400);

    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(0.0);
    setGraphicsEffect(m_opacityEffect);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);

    QString bgColor;
    switch (type) {
        case Info:    bgColor = "#2b2b40"; break;
        case Success: bgColor = "#1b5e20"; break;
        case Warning: bgColor = "#e65100"; break;
        case Error:   bgColor = "#b71c1c"; break;
    }

    setStyleSheet(QString(
        "background-color: %1; border-radius: 8px; border: 1px solid rgba(255,255,255,0.1);").arg(bgColor));

    m_messageLabel = new QLabel(message, this);
    m_messageLabel->setStyleSheet("color: white; font-size: 13px; font-weight: 500; background: transparent;");
    m_messageLabel->setWordWrap(true);
    layout->addWidget(m_messageLabel);

    // Position at top-right of parent
    if (parent) {
        QPoint pos = parent->mapToGlobal(QPoint(parent->width() - 420, 20));
        move(pos);
    }

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &ToastNotification::animateOut);
    m_timer->start(durationMs);

    m_fadeAnim = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_fadeAnim->setDuration(300);
}

void ToastNotification::animateIn() {
    QWidget::show();
    m_fadeAnim->setStartValue(0.0);
    m_fadeAnim->setEndValue(1.0);
    m_fadeAnim->start();
}

void ToastNotification::animateOut() {
    m_fadeAnim->setStartValue(1.0);
    m_fadeAnim->setEndValue(0.0);
    connect(m_fadeAnim, &QPropertyAnimation::finished, this, &QWidget::close);
    m_fadeAnim->start();
}

} // namespace Ballot::UI
