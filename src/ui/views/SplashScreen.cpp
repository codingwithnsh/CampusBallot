#include "SplashScreen.h"
#include "src/core/Constants.h" // For APP_NAME, APP_VERSION
#include <QVBoxLayout>
#include <QFont>
#include <QApplication>
#include <QPixmap>
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
    setFixedSize(560, 400);
    setAttribute(Qt::WA_TranslucentBackground); // Allow transparency
    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen); // Frameless and splash screen type

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(8);
    layout->setAlignment(Qt::AlignCenter);

    // --- Logo area ---
    m_logoLabel = new QLabel(this);
    m_logoLabel->setFixedSize(78, 78);
    m_logoLabel->setStyleSheet("background: transparent;");
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_logoLabel->setPixmap(QPixmap(":/assets/brand/app-mark.svg").scaled(78, 78, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(m_logoLabel, 0, Qt::AlignCenter);

    layout->addSpacing(8);

    // --- Title ---
    m_titleLabel = new QLabel(Core::Constants::APP_NAME, this); // Use constant for app name
    m_titleLabel->setStyleSheet("font-size: 28px; font-weight: 700; color: #ffffff; background: transparent;");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_titleLabel);

    // --- Subtitle ---
    m_subtitleLabel = new QLabel("Election Management System", this);
    m_subtitleLabel->setStyleSheet("font-size: 14px; color: #9a9ab0; background: transparent;");
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_subtitleLabel);

    layout->addStretch(); // Pushes content to the top

    // --- Status message ---
    m_statusLabel = new QLabel("Initializing...", this);
    m_statusLabel->setStyleSheet("font-size: 13px; color: #9a9ab0; background: transparent;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_statusLabel);

    // --- Progress bar ---
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(4);
    m_progressBar->setTextVisible(false); // Hide percentage text
    m_progressBar->setStyleSheet(R"(
        QProgressBar { background-color: rgba(255,255,255,0.1); border: none; border-radius: 2px; }
        QProgressBar::chunk { background-color: #0078d4; border-radius: 2px; }
    )");
    layout->addWidget(m_progressBar);

    // --- Background styling ---
    setStyleSheet(R"(
        SplashScreen {
            background-color: #1a1a2e;
            border: 1px solid #2d2d44;
            border-radius: 20px;
        }
    )");
    qDebug() << "SplashScreen: UI setup complete.";
}

/**
 * @brief Starts the loading simulation.
 */
void SplashScreen::startLoading() {
    qInfo() << "SplashScreen: Starting loading simulation.";
    m_progressValue = 0;
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
    if (mainWindow) {
        close();
    }
}

} // namespace Ballot::UI