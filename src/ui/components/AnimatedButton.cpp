#include "AnimatedButton.h"
#include <QPainter>
#include <QGraphicsDropShadowEffect>

namespace Ballot::UI {

AnimatedButton::AnimatedButton(const QString& text, QWidget* parent)
    : QPushButton(text, parent) {
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);

    m_hoverAnim = new QPropertyAnimation(this, "hoverProgress", this);
    m_hoverAnim->setDuration(150);

    m_scaleAnim = new QPropertyAnimation(this, "scale", this);
    m_scaleAnim->setDuration(100);
}

void AnimatedButton::setHoverProgress(double progress) {
    m_hoverProgress = progress;
    update();
}

void AnimatedButton::setScale(double scale) {
    m_scale = scale;
    update();
}

void AnimatedButton::enterEvent(QEnterEvent* event) {
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverProgress);
    m_hoverAnim->setEndValue(1.0);
    m_hoverAnim->start();

    m_scaleAnim->stop();
    m_scaleAnim->setStartValue(m_scale);
    m_scaleAnim->setEndValue(1.02);
    m_scaleAnim->start();

    QPushButton::enterEvent(event);
}

void AnimatedButton::leaveEvent(QEvent* event) {
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverProgress);
    m_hoverAnim->setEndValue(0.0);
    m_hoverAnim->start();

    m_scaleAnim->stop();
    m_scaleAnim->setStartValue(m_scale);
    m_scaleAnim->setEndValue(1.0);
    m_scaleAnim->start();

    QPushButton::leaveEvent(event);
}

void AnimatedButton::paintEvent(QPaintEvent* event) {
    QPushButton::paintEvent(event);

    if (m_hoverProgress > 0.0) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QColor highlight(255, 255, 255, static_cast<int>(20 * m_hoverProgress));
        painter.fillRect(rect(), highlight);
    }
}

} // namespace Ballot::UI
