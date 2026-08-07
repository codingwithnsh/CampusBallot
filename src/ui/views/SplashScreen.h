#pragma once

#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>
#include <QFrame>

namespace Ballot::UI {

class SplashScreen : public QWidget {
    Q_OBJECT
public:
    explicit SplashScreen(QWidget* parent = nullptr);
    void startLoading();
    void finish(QWidget* mainWindow);

signals:
    void loadingFinished();

private:
    void setupUi();
    void simulateLoading();

    QFrame *m_card;
    QLabel *m_logoLabel;
    QLabel *m_titleLabel;
    QLabel *m_subtitleLabel;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    QTimer *m_timer;
    QPropertyAnimation *m_introAnimation;
    QPropertyAnimation *m_logoFloatAnimation;
    int m_progressValue = 0;
};

} // namespace Ballot::UI
