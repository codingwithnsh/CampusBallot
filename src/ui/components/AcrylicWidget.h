#pragma once

#include <QWidget>
#include <QPainter>
#include <QGraphicsBlurEffect>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

namespace Ballot::UI {

class AcrylicWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor tintColor READ tintColor WRITE setTintColor)
    Q_PROPERTY(double tintOpacity READ tintOpacity WRITE setTintOpacity)
    Q_PROPERTY(double blurRadius READ blurRadius WRITE setBlurRadius)
    Q_PROPERTY(bool acrylicEnabled READ acrylicEnabled WRITE setAcrylicEnabled)

public:
    explicit AcrylicWidget(QWidget* parent = nullptr);

    QColor tintColor() const { return m_tintColor; }
    void setTintColor(const QColor& color);
    double tintOpacity() const { return m_tintOpacity; }
    void setTintOpacity(double opacity);
    double blurRadius() const { return m_blurRadius; }
    void setBlurRadius(double radius);
    bool acrylicEnabled() const { return m_acrylicEnabled; }
    void setAcrylicEnabled(bool enabled);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor m_tintColor = QColor(26, 26, 46, 180);
    double m_tintOpacity = 0.6;
    double m_blurRadius = 30.0;
    bool m_acrylicEnabled = true;
};

} // namespace Ballot::UI
