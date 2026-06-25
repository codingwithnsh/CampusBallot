#include "DashboardView.h"
#include "src/core/SystemManager.h"
#include "src/modules/auth/RBACManager.h"
#include "src/modules/auth/AuthManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QGraphicsDropShadowEffect>
#include <QLocale>

namespace Ballot::UI {

DashboardView::DashboardView(QWidget *parent) : QWidget(parent) {
    setupUi();

    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
        if (m_viewModel) m_viewModel->refresh();
    });
    m_refreshTimer->start(5000);
}

void DashboardView::setViewModel(ViewModels::DashboardViewModel* vm) {
    m_viewModel = vm;
    if (m_viewModel) {
        connect(m_viewModel, &ViewModels::DashboardViewModel::votingStatusChanged, this, &DashboardView::updateUi);
        connect(m_viewModel, &ViewModels::DashboardViewModel::roleChanged, this, &DashboardView::updateUi);
        connect(m_viewModel, &ViewModels::DashboardViewModel::totalStudentsChanged, this, &DashboardView::updateUi);
        updateUi();
    }
}

void DashboardView::setupUi() {
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    auto *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(20);

    // Header
    auto *headerLayout = new QHBoxLayout();
    auto *headerLeft = new QVBoxLayout();
    auto *title = new QLabel("Election Dashboard");
    title->setObjectName("title");
    headerLeft->addWidget(title);

    m_electionTitle = new QLabel("Current Election: None");
    m_electionTitle->setObjectName("subtitle");
    headerLeft->addWidget(m_electionTitle);
    headerLayout->addLayout(headerLeft);

    headerLayout->addStretch();

    m_statusLabel = new QLabel("● Not Started");
    m_statusLabel->setStyleSheet(R"(
        background-color: #2d2d44;
        color: #ffb300;
        font-weight: 600;
        font-size: 14px;
        padding: 8px 20px;
        border-radius: 20px;
    )");
    headerLayout->addWidget(m_statusLabel);

    mainLayout->addLayout(headerLayout);

    // Stat Cards
    auto *cardLayout = new QGridLayout();
    cardLayout->setSpacing(16);

    m_registeredCard = new StatCard("REGISTERED STUDENTS", "0", "#0078d4", "", this);
    m_votedCard = new StatCard("VOTES CAST", "0", "#2e7d32", "", this);
    m_remainingCard = new StatCard("REMAINING", "0", "#f57c00", "", this);
    m_turnoutCard = new StatCard("LIVE TURNOUT", "0%", "#7b1fa2", "", this);

    cardLayout->addWidget(m_registeredCard, 0, 0);
    cardLayout->addWidget(m_votedCard, 0, 1);
    cardLayout->addWidget(m_remainingCard, 0, 2);
    cardLayout->addWidget(m_turnoutCard, 1, 0);

    mainLayout->addLayout(cardLayout);

    // System Health
    auto *healthSection = createSystemHealthSection();
    mainLayout->addWidget(healthSection);

    // Quick Actions
    auto *actionsSection = createQuickActionsSection();
    mainLayout->addWidget(actionsSection);

    mainLayout->addStretch();

    scrollArea->setWidget(content);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);
}

QWidget* DashboardView::createSystemHealthSection() {
    auto *section = new QFrame(this);
    section->setObjectName("card");

    auto *layout = new QVBoxLayout(section);
    auto *header = new QLabel("System Health");
    header->setObjectName("sectionTitle");
    layout->addWidget(header);

    auto *grid = new QGridLayout();
    grid->setSpacing(12);

    auto createHealthItem = [](const QString& label, QLabel*& valueLabel, const QString& defaultVal, const QString& color) {
        auto *item = new QFrame();
        item->setStyleSheet("background-color: #1e1e34; border-radius: 8px; padding: 12px;");
        auto *vl = new QVBoxLayout(item);
        auto *lbl = new QLabel(label);
        lbl->setStyleSheet("font-size: 12px; color: #9a9ab0; font-weight: 500; background: transparent;");
        valueLabel = new QLabel(defaultVal);
        valueLabel->setStyleSheet(QString("font-size: 16px; font-weight: 600; color: %1; background: transparent;").arg(color));
        vl->addWidget(lbl);
        vl->addWidget(valueLabel);
        return item;
    };

    grid->addWidget(createHealthItem("Database Status", m_dbStatus, "● Connected", "#4caf50"), 0, 0);
    grid->addWidget(createHealthItem("Storage Type", m_storageType, "SQLite", "#e0e0e0"), 0, 1);
    grid->addWidget(createHealthItem("Server Status", m_serverStatus, "● Online", "#4caf50"), 0, 2);
    grid->addWidget(createHealthItem("Audit Status", m_auditStatus, "● Active", "#4caf50"), 1, 0);
    grid->addWidget(createHealthItem("Backup Status", m_backupStatus, "● Active", "#4caf50"), 1, 1);

    layout->addLayout(grid);
    return section;
}

QWidget* DashboardView::createQuickActionsSection() {
    auto *section = new QFrame(this);
    section->setObjectName("card");

    auto *layout = new QVBoxLayout(section);
    auto *header = new QLabel("Administrative Controls");
    header->setObjectName("sectionTitle");
    layout->addWidget(header);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    m_startVotingBtn = new QPushButton("▶  Start Voting");
    m_startVotingBtn->setObjectName("successButton");
    m_startVotingBtn->setFixedHeight(42);
    m_startVotingBtn->setCursor(Qt::PointingHandCursor);

    m_endVotingBtn = new QPushButton("■  End Voting");
    m_endVotingBtn->setObjectName("dangerButton");
    m_endVotingBtn->setFixedHeight(42);
    m_endVotingBtn->setCursor(Qt::PointingHandCursor);

    m_kioskBtn = new QPushButton("🗳️ Open Kiosk");
    m_kioskBtn->setObjectName("primaryButton");
    m_kioskBtn->setFixedHeight(42);
    m_kioskBtn->setCursor(Qt::PointingHandCursor);

    btnLayout->addWidget(m_startVotingBtn);
    btnLayout->addWidget(m_endVotingBtn);
    btnLayout->addWidget(m_kioskBtn);
    btnLayout->addStretch();

    layout->addLayout(btnLayout);
    return section;
}

void DashboardView::updateUi() {
    if (!m_viewModel) return;

    auto& auth = Auth::AuthManager::instance();
    bool isAdmin = auth.hasPermission(Auth::RBACManager::PERM_VOTE_START);

    m_startVotingBtn->setVisible(isAdmin);
    m_endVotingBtn->setVisible(isAdmin);

    QString status = m_viewModel->votingStatus();
    m_statusLabel->setText("● " + status);

    if (status == "In Progress") {
        m_statusLabel->setStyleSheet("background-color: #1b5e20; color: #a5d6a7; font-weight: 600; font-size: 14px; padding: 8px 20px; border-radius: 20px;");
        m_startVotingBtn->setEnabled(false);
        m_endVotingBtn->setEnabled(true);
    } else if (status == "Not Started") {
        m_statusLabel->setStyleSheet("background-color: #2d2d44; color: #ffb300; font-weight: 600; font-size: 14px; padding: 8px 20px; border-radius: 20px;");
        m_startVotingBtn->setEnabled(true);
        m_endVotingBtn->setEnabled(false);
    } else {
        m_statusLabel->setStyleSheet("background-color: #b71c1c; color: #ef9a9a; font-weight: 600; font-size: 14px; padding: 8px 20px; border-radius: 20px;");
        m_startVotingBtn->setEnabled(false);
        m_endVotingBtn->setEnabled(false);
    }

    m_registeredCard->setValue(QLocale().toString(m_viewModel->totalStudents()));
    m_votedCard->setValue(QLocale().toString(m_viewModel->votesCast()));
    m_remainingCard->setValue(QLocale().toString(m_viewModel->totalStudents() - m_viewModel->votesCast()));
    m_turnoutCard->setValue(QString::number(m_viewModel->turnout(), 'f', 1) + "%");

    m_electionTitle->setText("Current Election: " + status);

    connect(m_startVotingBtn, &QPushButton::clicked, this, [this]() {
        if (m_viewModel) m_viewModel->startVoting();
    });
    connect(m_endVotingBtn, &QPushButton::clicked, this, [this]() {
        if (m_viewModel) m_viewModel->endVoting();
    });
    connect(m_kioskBtn, &QPushButton::clicked, this, [this]() {
        auto* mw = window();
        if (mw) {
            QMetaObject::invokeMethod(mw, "switchToView", Qt::QueuedConnection, Q_ARG(QString, "voting"));
        }
    });
}

} // namespace Ballot::UI
