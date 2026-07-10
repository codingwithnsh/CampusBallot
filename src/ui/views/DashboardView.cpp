#include "DashboardView.h"
#include "src/core/SystemManager.h"
#include "src/modules/auth/RBACManager.h"
#include "src/modules/auth/AuthManager.h"
#include "src/ui/components/ToastNotification.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QGraphicsDropShadowEffect>
#include <QLocale>
#include <QDebug> // For logging

namespace Ballot::UI {

DashboardView::DashboardView(QWidget *parent) : QWidget(parent) {
    setupUi();
    qDebug() << "DashboardView: Initialized.";
}

/**
 * @brief Sets the DashboardViewModel for this view.
 * Connects signals from the ViewModel to update the UI.
 * @param vm The DashboardViewModel instance.
 */
void DashboardView::setViewModel(ViewModels::DashboardViewModel* vm) {
    if (m_viewModel == vm) return; // Avoid reconnecting if same VM
    if (m_viewModel) {
        // Disconnect old connections if a ViewModel was already set
        disconnect(m_viewModel, nullptr, this, nullptr);
    }

    m_viewModel = vm;
    if (m_viewModel) {
        // Connect ViewModel signals to updateUi slot
        connect(m_viewModel, &ViewModels::DashboardViewModel::totalStudentsChanged, this, &DashboardView::updateUi);
        connect(m_viewModel, &ViewModels::DashboardViewModel::votesCastChanged, this, &DashboardView::updateUi);
        connect(m_viewModel, &ViewModels::DashboardViewModel::turnoutChanged, this, &DashboardView::updateUi);
        connect(m_viewModel, &ViewModels::DashboardViewModel::roleChanged, this, &DashboardView::updateUi);
        connect(m_viewModel, &ViewModels::DashboardViewModel::votingStatusChanged, this, &DashboardView::updateUi);
        connect(m_viewModel, &ViewModels::DashboardViewModel::currentElectionTitleChanged, this, &DashboardView::updateUi);
        connect(m_viewModel, &ViewModels::DashboardViewModel::dbStatusChanged, this, &DashboardView::updateUi);
        connect(m_viewModel, &ViewModels::DashboardViewModel::storageTypeChanged, this, &DashboardView::updateUi);
        connect(m_viewModel, &ViewModels::DashboardViewModel::serverStatusChanged, this, &DashboardView::updateUi);
        connect(m_viewModel, &ViewModels::DashboardViewModel::auditStatusChanged, this, &DashboardView::updateUi);
        connect(m_viewModel, &ViewModels::DashboardViewModel::backupStatusChanged, this, &DashboardView::updateUi);
        connect(m_viewModel, &ViewModels::DashboardViewModel::errorOccurred, this, [this](const QString& error) {
            ToastNotification::show(this, error, ToastNotification::Error);
            qWarning() << "DashboardView: ViewModel error:" << error;
        });
        updateUi(); // Initial UI update
        qDebug() << "DashboardView: ViewModel set and signals connected.";
    } else {
        qWarning() << "DashboardView: Attempted to set null ViewModel.";
    }
}

/**
 * @brief Sets up the user interface for the dashboard view.
 */
void DashboardView::setupUi() {
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    auto *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(20);

    // --- Header Section ---
    auto *headerLayout = new QHBoxLayout();
    auto *headerLeft = new QVBoxLayout();
    auto *title = new QLabel("Election Dashboard", this);
    title->setObjectName("title");
    title->setStyleSheet("font-size: 32px; font-weight: 700; color: #e0e0e0;");
    headerLeft->addWidget(title);

    m_electionTitle = new QLabel("Current Election: None", this);
    m_electionTitle->setObjectName("subtitle");
    m_electionTitle->setStyleSheet("font-size: 16px; color: #9a9ab0;");
    headerLeft->addWidget(m_electionTitle);
    headerLayout->addLayout(headerLeft);

    headerLayout->addStretch();

    m_statusLabel = new QLabel("● Not Started", this);
    m_statusLabel->setStyleSheet(R"(
        background-color: #2d2d44; color: #ffb300; font-weight: 600;
        font-size: 14px; padding: 8px 20px; border-radius: 20px;
    )");
    headerLayout->addWidget(m_statusLabel);

    mainLayout->addLayout(headerLayout);

    // --- Stat Cards Section ---
    auto *cardLayout = new QGridLayout();
    cardLayout->setSpacing(16);

    m_registeredCard = new StatCard("REGISTERED STUDENTS", "0", "#0078d4", "", this);
    m_votedCard = new StatCard("VOTES CAST", "0", "#2e7d32", "", this);
    m_remainingCard = new StatCard("REMAINING", "0", "#f57c00", "", this);
    m_turnoutCard = new StatCard("LIVE TURNOUT", "0%", "#7b1fa2", "", this);

    cardLayout->addWidget(m_registeredCard, 0, 0);
    cardLayout->addWidget(m_votedCard, 0, 1);
    cardLayout->addWidget(m_remainingCard, 0, 2);
    cardLayout->addWidget(m_turnoutCard, 1, 0); // Placed in a new row for better layout

    mainLayout->addLayout(cardLayout);

    // --- System Health Section ---
    auto *healthSection = createSystemHealthSection();
    mainLayout->addWidget(healthSection);

    // --- Quick Actions Section ---
    auto *actionsSection = createQuickActionsSection();
    mainLayout->addWidget(actionsSection);

    mainLayout->addStretch(); // Pushes content to the top

    scrollArea->setWidget(content);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);

    // Connect buttons once in setupUi
    connect(m_startVotingBtn, &QPushButton::clicked, this, [this]() {
        qInfo() << "DashboardView: Start Voting button clicked.";
        if (m_viewModel) m_viewModel->startVoting();
    });
    connect(m_endVotingBtn, &QPushButton::clicked, this, [this]() {
        qInfo() << "DashboardView: End Voting button clicked.";
        if (m_viewModel) m_viewModel->endVoting();
    });
    connect(m_kioskBtn, &QPushButton::clicked, this, [this]() {
        qInfo() << "DashboardView: Open Kiosk button clicked.";
        auto* mw = window(); // Get the parent QMainWindow
        if (mw) {
            // Use QMetaObject::invokeMethod for thread-safe signal/slot connection across threads
            // or if the slot is in a different object. Here, it's just a clean way to call a public slot.
            QMetaObject::invokeMethod(mw, "switchToView", Qt::QueuedConnection, Q_ARG(QString, "voting"));
        } else {
            qWarning() << "DashboardView: Parent window not found for switchToView.";
        }
    });
    qDebug() << "DashboardView: UI setup complete.";
}

/**
 * @brief Creates the system health status section.
 * @return A QWidget containing the system health indicators.
 */
QWidget* DashboardView::createSystemHealthSection() {
    auto *section = new QFrame(this);
    section->setObjectName("card"); // For QSS targeting
    section->setStyleSheet("QFrame#card { background-color: #2a2a3e; border-radius: 12px; padding: 16px; }");

    auto *layout = new QVBoxLayout(section);
    auto *header = new QLabel("System Health", this);
    header->setObjectName("sectionTitle");
    header->setStyleSheet("font-size: 18px; font-weight: 600; color: #e0e0e0; margin-bottom: 10px;");
    layout->addWidget(header);

    auto *grid = new QGridLayout();
    grid->setSpacing(12);

    // Helper lambda to create a health item (label + value)
    auto createHealthItem = [&](const QString& label, QLabel*& valueLabel, const QString& defaultVal, const QString& color) {
        auto *item = new QFrame(this);
        item->setStyleSheet("background-color: #1e1e34; border-radius: 8px; padding: 12px;");
        auto *vl = new QVBoxLayout(item);
        vl->setContentsMargins(0,0,0,0);
        vl->setSpacing(4);

        auto *lbl = new QLabel(label, item);
        lbl->setStyleSheet("font-size: 12px; color: #9a9ab0; font-weight: 500; background: transparent;");
        valueLabel = new QLabel(defaultVal, item);
        valueLabel->setStyleSheet(QString("font-size: 16px; font-weight: 600; color: %1; background: transparent;").arg(color));
        vl->addWidget(lbl);
        vl->addWidget(valueLabel);
        return item;
    };

    grid->addWidget(createHealthItem("Database Status", m_dbStatus, "● Disconnected", "#f44336"), 0, 0);
    grid->addWidget(createHealthItem("Storage Type", m_storageType, "Unknown", "#e0e0e0"), 0, 1);
    grid->addWidget(createHealthItem("Server Status", m_serverStatus, "● Offline", "#f44336"), 0, 2);
    grid->addWidget(createHealthItem("Audit Status", m_auditStatus, "● Inactive", "#f44336"), 1, 0);
    grid->addWidget(createHealthItem("Backup Status", m_backupStatus, "● Inactive", "#f44336"), 1, 1);

    layout->addLayout(grid);
    return section;
}

/**
 * @brief Creates the quick administrative actions section.
 * @return A QWidget containing the action buttons.
 */
QWidget* DashboardView::createQuickActionsSection() {
    auto *section = new QFrame(this);
    section->setObjectName("card");
    section->setStyleSheet("QFrame#card { background-color: #2a2a3e; border-radius: 12px; padding: 16px; }");

    auto *layout = new QVBoxLayout(section);
    auto *header = new QLabel("Administrative Controls", this);
    header->setObjectName("sectionTitle");
    header->setStyleSheet("font-size: 18px; font-weight: 600; color: #e0e0e0; margin-bottom: 10px;");
    layout->addWidget(header);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    m_startVotingBtn = new QPushButton("▶  Start Voting", this);
    m_startVotingBtn->setObjectName("successButton");
    m_startVotingBtn->setFixedHeight(42);
    m_startVotingBtn->setCursor(Qt::PointingHandCursor);
    m_startVotingBtn->setStyleSheet(R"(
        QPushButton#successButton {
            background-color: #4CAF50; color: white; border: none; border-radius: 8px;
            font-size: 15px; font-weight: 600; padding: 8px 16px;
        }
        QPushButton#successButton:hover { background-color: #66BB6A; }
        QPushButton#successButton:pressed { background-color: #388E3C; }
        QPushButton#successButton:disabled { background-color: #607d8b; }
    )");

    m_endVotingBtn = new QPushButton("■  End Voting", this);
    m_endVotingBtn->setObjectName("dangerButton");
    m_endVotingBtn->setFixedHeight(42);
    m_endVotingBtn->setCursor(Qt::PointingHandCursor);
    m_endVotingBtn->setStyleSheet(R"(
        QPushButton#dangerButton {
            background-color: #d32f2f; color: white; border: none; border-radius: 8px;
            font-size: 15px; font-weight: 600; padding: 8px 16px;
        }
        QPushButton#dangerButton:hover { background-color: #ef5350; }
        QPushButton#dangerButton:pressed { background-color: #b71c1c; }
        QPushButton#dangerButton:disabled { background-color: #607d8b; }
    )");

    m_kioskBtn = new QPushButton("🗳️ Open Kiosk", this);
    m_kioskBtn->setObjectName("primaryButton");
    m_kioskBtn->setFixedHeight(42);
    m_kioskBtn->setCursor(Qt::PointingHandCursor);
    m_kioskBtn->setStyleSheet(R"(
        QPushButton#primaryButton {
            background-color: #0078d4; color: white; border: none; border-radius: 8px;
            font-size: 15px; font-weight: 600; padding: 8px 16px;
        }
        QPushButton#primaryButton:hover { background-color: #1a8ae8; }
        QPushButton#primaryButton:pressed { background-color: #006cbd; }
    )");

    btnLayout->addWidget(m_startVotingBtn);
    btnLayout->addWidget(m_endVotingBtn);
    btnLayout->addWidget(m_kioskBtn);
    btnLayout->addStretch();

    layout->addLayout(btnLayout);
    return section;
}

/**
 * @brief Updates the UI elements with data from the ViewModel.
 * This slot is connected to various signals from the DashboardViewModel.
 */
void DashboardView::updateUi() {
    if (!m_viewModel) {
        qWarning() << "DashboardView: ViewModel is null during updateUi.";
        return;
    }
    qDebug() << "DashboardView: Updating UI from ViewModel.";

    auto& auth = Auth::AuthManager::instance();
    const bool canStartEndVoting = auth.isAuthenticated()
            && auth.hasPermission(Auth::RBACManager::PERM_VOTE_START)
            && m_viewModel->isMaster();

    // Update voting status label and buttons
    QString votingStatusText = m_viewModel->votingStatus();
    m_statusLabel->setText("● " + votingStatusText);

    if (votingStatusText == "In Progress") {
        m_statusLabel->setStyleSheet("background-color: #1b5e20; color: #a5d6a7; font-weight: 600; font-size: 14px; padding: 8px 20px; border-radius: 20px;");
        m_startVotingBtn->setEnabled(false);
        m_endVotingBtn->setEnabled(canStartEndVoting);
    } else if (votingStatusText == "Not Started") {
        m_statusLabel->setStyleSheet("background-color: #2d2d44; color: #ffb300; font-weight: 600; font-size: 14px; padding: 8px 20px; border-radius: 20px;");
        m_startVotingBtn->setEnabled(canStartEndVoting);
        m_endVotingBtn->setEnabled(false);
    } else if (votingStatusText == "Paused") {
        m_statusLabel->setStyleSheet("background-color: #e65100; color: #ffcc80; font-weight: 600; font-size: 14px; padding: 8px 20px; border-radius: 20px;");
        m_startVotingBtn->setEnabled(canStartEndVoting); // Can resume from paused
        m_endVotingBtn->setEnabled(canStartEndVoting);
    } else { // Ended or Unknown
        m_statusLabel->setStyleSheet("background-color: #b71c1c; color: #ef9a9a; font-weight: 600; font-size: 14px; padding: 8px 20px; border-radius: 20px;");
        m_startVotingBtn->setEnabled(false);
        m_endVotingBtn->setEnabled(false);
    }

    // Update StatCards
    m_registeredCard->setValue(QLocale().toString(m_viewModel->totalStudents()));
    m_votedCard->setValue(QLocale().toString(m_viewModel->votesCast()));
    m_remainingCard->setValue(QLocale().toString(m_viewModel->totalStudents() - m_viewModel->votesCast()));
    m_turnoutCard->setValue(QString::number(m_viewModel->turnout(), 'f', 1) + "%");

    // Update election title
    m_electionTitle->setText("Current Election: " + m_viewModel->currentElectionTitle());

    // Update system health statuses
    m_dbStatus->setText(m_viewModel->dbStatus());
    m_dbStatus->setStyleSheet(QString("font-size: 16px; font-weight: 600; color: %1; background: transparent;").arg(m_viewModel->dbStatus().contains("Connected") ? "#4caf50" : "#f44336"));

    m_storageType->setText(m_viewModel->storageType());
    m_storageType->setStyleSheet(QString("font-size: 16px; font-weight: 600; color: %1; background: transparent;").arg(m_viewModel->storageType() == "Unknown" ? "#f57c00" : "#e0e0e0"));


    m_serverStatus->setText(m_viewModel->serverStatus());
    m_serverStatus->setStyleSheet(QString("font-size: 16px; font-weight: 600; color: %1; background: transparent;").arg(m_viewModel->serverStatus().contains("Online") ? "#4caf50" : "#f44336"));

    m_auditStatus->setText(m_viewModel->auditStatus());
    m_auditStatus->setStyleSheet(QString("font-size: 16px; font-weight: 600; color: %1; background: transparent;").arg(m_viewModel->auditStatus().contains("Active") ? "#4caf50" : "#f44336"));

    m_backupStatus->setText(m_viewModel->backupStatus());
    m_backupStatus->setStyleSheet(QString("font-size: 16px; font-weight: 600; color: %1; background: transparent;").arg(m_viewModel->backupStatus().contains("Active") ? "#4caf50" : "#f44336"));
}

} // namespace Ballot::UI
