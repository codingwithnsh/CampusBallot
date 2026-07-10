#pragma once

#include <QWidget>
#include <QPainter>
// Removed unused QGraphicsBlurEffect, QGraphicsScene, QGraphicsPixmapItem includes
// #include <QGraphicsBlurEffect>
// #include <QGraphicsScene>
// #include <QGraphicsPixmapItem>

namespace Ballot::UI {

/**
 * @brief The AcrylicWidget class provides a custom widget that simulates an acrylic (blurred, translucent) effect.
 *
 * @note The current implementation simulates the blur effect visually with a translucent background
 * and noise overlay. It does NOT use QGraphicsBlurEffect to blur content behind it,
 * as that would require a more complex rendering pipeline (e.g., rendering to an offscreen buffer,
 * applying blur, then drawing). The 'blurRadius' property is currently a placeholder.
 */
class AcrylicWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor tintColor READ tintColor WRITE setTintColor)
    Q_PROPERTY(double tintOpacity READ tintOpacity WRITE setTintOpacity)
    // Q_PROPERTY(double blurRadius READ blurRadius WRITE setBlurRadius) // Temporarily commented out as not implemented
    Q_PROPERTY(bool acrylicEnabled READ acrylicEnabled WRITE setAcrylicEnabled)

public:
    explicit AcrylicWidget(QWidget* parent = nullptr);

    QColor tintColor() const { return m_tintColor; }
    void setTintColor(const QColor& color);
    double tintOpacity() const { return m_tintOpacity; }
    void setTintOpacity(double opacity);
    // double blurRadius() const { return m_blurRadius; } // Temporarily commented out
    // void setBlurRadius(double radius); // Temporarily commented out
    bool acrylicEnabled() const { return m_acrylicEnabled; }
    void setAcrylicEnabled(bool enabled);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor m_tintColor = QColor(26, 26, 46, 180);
    double m_tintOpacity = 0.6;
    double m_blurRadius = 30.0; // Kept as member for potential future use, but not exposed as property
    bool m_acrylicEnabled = true;
};

} // namespace Ballot::UI