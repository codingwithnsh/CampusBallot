#pragma once

#include <QPushButton>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

namespace Ballot::UI {

class AnimatedButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(double hoverProgress READ hoverProgress WRITE setHoverProgress)
    Q_PROPERTY(double scale READ scale WRITE setScale)

public:
    explicit AnimatedButton(const QString& text = "", QWidget* parent = nullptr);

    double hoverProgress() const { return m_hoverProgress; }
    void setHoverProgress(double progress);
    double scale() const { return m_scale; }
    void setScale(double scale);

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    double m_hoverProgress = 0.0;
    double m_scale = 1.0;
    QPropertyAnimation* m_hoverAnim;
    QPropertyAnimation* m_scaleAnim;
};

} // namespace Ballot::UI
