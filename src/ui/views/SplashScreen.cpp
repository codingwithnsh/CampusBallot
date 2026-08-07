#include "SplashScreen.h"
#include "src/core/Constants.h" // For APP_NAME, APP_VERSION
#include <QVBoxLayout>
#include <QFont>
#include <QApplication>
#include <QPixmap>
#include <QEasingCurve>
#include <QDebug> // For logging

namespace Ballot::UI {

SplashScreen::SplashScreen(QWidget* parent) : QWidget(parent) {
    setupUi();
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SplashScreen::simulateLoading);
    qDebug() << "SplashScreen: Initialized.";
}

/**
 * @brief Sets up the user interface for the splash screen.
 */
void SplashScreen::setupUi() {
    setFixedSize(620, 420);
    setAttribute(Qt::WA_TranslucentBackground); // Allow transparency
    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen); // Frameless and splash screen type

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setAlignment(Qt::AlignCenter);

    m_card = new QFrame(this);
    m_card->setObjectName("splashCard");
    auto *cardLayout = new QVBoxLayout(m_card);
    cardLayout->setContentsMargins(44, 34, 44, 34);
    cardLayout->setSpacing(10);
    cardLayout->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_card);

    // --- Logo area ---
    m_logoLabel = new QLabel(m_card);
    m_logoLabel->setFixedSize(86, 86);
    m_logoLabel->setStyleSheet("background: transparent;");
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_logoLabel->setPixmap(QPixmap(":/assets/brand/app-mark.svg").scaled(86, 86, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    cardLayout->addWidget(m_logoLabel, 0, Qt::AlignCenter);

    cardLayout->addSpacing(4);

    // --- Title ---
    m_titleLabel = new QLabel(Core::Constants::APP_NAME, m_card); // Use constant for app name
    m_titleLabel->setStyleSheet("font-size: 30px; font-weight: 700; color: #ffffff; background: transparent;");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_titleLabel);

    // --- Subtitle ---
    m_subtitleLabel = new QLabel("Secure, simple, playful election operations", m_card);
    m_subtitleLabel->setStyleSheet("font-size: 14px; color: #d1d8ff; background: transparent;");
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_subtitleLabel);

    cardLayout->addStretch(); // Pushes content to the top

    // --- Status message ---
    m_statusLabel = new QLabel("Initializing secure modules...", m_card);
    m_statusLabel->setStyleSheet("font-size: 13px; color: #c7d2fe; background: transparent;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_statusLabel);

    // --- Progress bar ---
    m_progressBar = new QProgressBar(m_card);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(8);
    m_progressBar->setTextVisible(false); // Hide percentage text
    m_progressBar->setStyleSheet(R"(
        QProgressBar { background-color: rgba(255,255,255,0.16); border: none; border-radius: 4px; }
        QProgressBar::chunk {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #22d3ee, stop:1 #a78bfa);
            border-radius: 4px;
        }
    )");
    cardLayout->addWidget(m_progressBar);

    // --- Background styling ---
    setStyleSheet(R"(
        QWidget {
            background: transparent;
        }
        QFrame#splashCard {
            background-color: rgba(18, 24, 45, 232);
            border: 1px solid rgba(148, 163, 184, 96);
            border-radius: 26px;
        }
    )");

    m_introAnimation = new QPropertyAnimation(this, "windowOpacity", this);
    m_introAnimation->setDuration(500);
    m_introAnimation->setStartValue(0.0);
    m_introAnimation->setEndValue(1.0);
    m_introAnimation->setEasingCurve(QEasingCurve::OutCubic);

    m_logoFloatAnimation = new QPropertyAnimation(m_logoLabel, "pos", this);
    m_logoFloatAnimation->setDuration(1500);
    m_logoFloatAnimation->setLoopCount(-1);
    m_logoFloatAnimation->setEasingCurve(QEasingCurve::InOutSine);

    qDebug() << "SplashScreen: UI setup complete.";
}

/**
 * @brief Starts the loading simulation.
 */
void SplashScreen::startLoading() {
    qInfo() << "SplashScreen: Starting loading simulation.";
    m_progressValue = 0;
    setWindowOpacity(0.0);
    m_introAnimation->start();
    const QPoint logoBasePos = m_logoLabel->pos();
    m_logoFloatAnimation->setStartValue(logoBasePos);
    m_logoFloatAnimation->setKeyValueAt(0.5, logoBasePos + QPoint(0, -8));
    m_logoFloatAnimation->setEndValue(logoBasePos);
    m_logoFloatAnimation->start();
    m_timer->start(Core::Constants::SPLASH_DURATION_MS / Core::Constants::SPLASH_STEPS); // Divide total duration by steps
}

/**
 * @brief Simulates the loading process by updating the progress bar and status messages.
 */
void SplashScreen::simulateLoading() {
    // Increment progress value, ensuring it doesn't exceed 100
    m_progressValue += (100 / Core::Constants::SPLASH_STEPS);
    if (m_progressValue > 100) m_progressValue = 100;
    m_progressBar->setValue(m_progressValue);

    // Update status messages based on progress
    if (m_progressValue <= 20)
        m_statusLabel->setText("Initializing security modules...");
    else if (m_progressValue <= 40)
        m_statusLabel->setText("Connecting to database...");
    else if (m_progressValue <= 60)
        m_statusLabel->setText("Loading election data...");
    else if (m_progressValue <= 80)
        m_statusLabel->setText("Preparing user interface...");
    else if (m_progressValue < 100) // Use < 100 to ensure "Ready!" is the final message
        m_statusLabel->setText("Finalizing setup...");
    else if (m_progressValue >= 100) {
        m_timer->stop();
        m_statusLabel->setText("Ready!");
        qInfo() << "SplashScreen: Loading simulation finished.";
        // Emit signal after a small delay to allow "Ready!" to be seen
        QTimer::singleShot(300, this, [this]() {
            emit loadingFinished();
        });
    }
}

void SplashScreen::finish(QWidget* mainWindow) {
    if (!mainWindow) {
        close();
        return;
    }
    mainWindow->show();
    m_logoFloatAnimation->stop();
    auto *fadeOut = new QPropertyAnimation(this, "windowOpacity", this);
    fadeOut->setDuration(240);
    fadeOut->setStartValue(windowOpacity());
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InCubic);
    connect(fadeOut, &QPropertyAnimation::finished, this, &QWidget::close);
    fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace Ballot::UI