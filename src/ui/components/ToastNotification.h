#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

namespace Ballot::UI {

class ToastNotification : public QWidget {
    Q_OBJECT
public:
    enum Type { Info, Success, Warning, Error };

    static void show(QWidget* parent, const QString& message,
                     Type type = Info, int durationMs = 3000);

private:
    explicit ToastNotification(QWidget* parent, const QString& message,
                                Type type, int durationMs);
    void animateIn();
    void animateOut();

    QLabel* m_messageLabel;
    QTimer* m_timer;
    QGraphicsOpacityEffect* m_opacityEffect;
    QPropertyAnimation* m_fadeAnim;
    Type m_type;
};

} // namespace Ballot::UI
