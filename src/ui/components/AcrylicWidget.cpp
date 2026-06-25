#include "AcrylicWidget.h"
#include <QPainter>
#include <QWidget>
#include <QMainWindow>
#include <QRandomGenerator>

namespace Ballot::UI {

AcrylicWidget::AcrylicWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
}

void AcrylicWidget::setTintColor(const QColor& color) {
    m_tintColor = color;
    update();
}

void AcrylicWidget::setTintOpacity(double opacity) {
    m_tintOpacity = opacity;
    update();
}

void AcrylicWidget::setBlurRadius(double radius) {
    m_blurRadius = radius;
    update();
}

void AcrylicWidget::setAcrylicEnabled(bool enabled) {
    m_acrylicEnabled = enabled;
    update();
}

void AcrylicWidget::paintEvent(QPaintEvent* event) {
    if (!m_acrylicEnabled) {
        QWidget::paintEvent(event);
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Semi-transparent background with acrylic effect simulation
    QColor bg = m_tintColor;
    bg.setAlphaF(m_tintOpacity);
    painter.fillRect(rect(), bg);

    // Subtle border glow
    QPen pen(QColor(255, 255, 255, 20), 1);
    painter.setPen(pen);
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 12, 12);

    // Noise overlay for acrylic effect
    QRandomGenerator* rng = QRandomGenerator::global();
    painter.setPen(QPen(QColor(255, 255, 255, 4)));
    for (int i = 0; i < 50; ++i) {
        int x = rng->bounded(width());
        int y = rng->bounded(height());
        painter.drawPoint(x, y);
    }
}

} // namespace Ballot::UI
