#pragma once

#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>

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

    QLabel *m_logoLabel;
    QLabel *m_titleLabel;
    QLabel *m_subtitleLabel;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    QTimer *m_timer;
    int m_progressValue = 0;
};

} // namespace Ballot::UI
