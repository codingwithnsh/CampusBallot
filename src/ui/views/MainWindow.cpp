#include "MainWindow.h"
#include "DashboardView.h"
#include "VotingKiosk.h"
#include "LoginView.h"
#include "ResultsView.h"
#include "AdminPanel.h"
#include "UserManagementView.h"
#include "SettingsView.h"
#include "src/ui/viewmodels/DashboardViewModel.h"
#include "src/ui/viewmodels/AuthViewModel.h"
#include "src/ui/viewmodels/ResultsViewModel.h"
#include "src/core/SystemManager.h"
#include "src/modules/auth/AuthManager.h"
#include "src/modules/auth/RBACManager.h"
#include <QHBoxLayout>
#include <QApplication>
#include <QIcon>

namespace Ballot::UI {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();
    setupSidebar();
    connectSignals();

    // Show login by default
    switchToView("login");
}

void MainWindow::setupUi() {
    setWindowTitle("Campus Ballot - Election Management System");
    setWindowIcon(QIcon(":/assets/brand/app-mark.svg"));
    resize(1400, 900);
    setMinimumSize(1024, 768);

    // Central widget with horizontal layout
    auto *centralWidget = new QWidget(this);
    auto *hLayout = new QHBoxLayout(centralWidget);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(0);

    // Sidebar (initially hidden)
    m_sidebar = new Sidebar(centralWidget);
    m_sidebar->setVisible(false);
    hLayout->addWidget(m_sidebar);

    // Stacked widget for views
    m_stackedWidget = new QStackedWidget(centralWidget);
    m_stackedWidget->setStyleSheet("background-color: #1a1a2e;");
    hLayout->addWidget(m_stackedWidget, 1);

    setCentralWidget(centralWidget);

    // Create ViewModels
    auto* storage = Core::SystemManager::instance().storage();
    m_dashboardViewModel = new ViewModels::DashboardViewModel(storage, this);
    m_authViewModel = new ViewModels::AuthViewModel(this);
    m_resultsViewModel = new ViewModels::ResultsViewModel(storage, this);

    // Create views
    m_loginView = new LoginView(this);
    m_dashboard = new DashboardView(this);
    m_kiosk = new VotingKiosk(this);
    m_resultsView = new ResultsView(this);
    m_adminPanel = new AdminPanel(this);
    m_userManagement = new UserManagementView(this);
    m_settingsView = new SettingsView(this);

    // Connect view models
    m_dashboard->setViewModel(m_dashboardViewModel);
    m_loginView->setViewModel(m_authViewModel);
    m_resultsView->setViewModel(m_resultsViewModel);

    // Add views to stack
    m_stackedWidget->addWidget(m_loginView);
    m_stackedWidget->addWidget(m_dashboard);
    m_stackedWidget->addWidget(m_kiosk);
    m_stackedWidget->addWidget(m_resultsView);
    m_stackedWidget->addWidget(m_adminPanel);
    m_stackedWidget->addWidget(m_userManagement);
    m_stackedWidget->addWidget(m_settingsView);
}

void MainWindow::setupSidebar() {
    m_sidebar->addItem("dashboard", "Dashboard", "DB");
    m_sidebar->addItem("voting", "Voting Kiosk", "VK");
    m_sidebar->addItem("results", "Results", "RS");
    m_sidebar->addItem("users", "User Management", "UM");
    m_sidebar->addItem("admin", "Administration", "AD");
    m_sidebar->addItem("settings", "Settings", "ST");
    m_sidebar->addItem("logout", "Logout", "LO");

    connect(m_sidebar, &Sidebar::itemClicked, this, [this](const QString& id) {
        if (id == "logout") {
            Auth::AuthManager::instance().logout();
            switchToView("login");
        } else {
            switchToView(id);
        }
    });
}

void MainWindow::connectSignals() {
    auto& auth = Auth::AuthManager::instance();
    connect(&auth, &Auth::AuthManager::loginSuccessful, this, [this](const QString& userId) {
        Q_UNUSED(userId);

        auto user = Auth::AuthManager::instance().currentUser();
        setUserAuthenticated(user.name, Auth::RBACManager::instance().roleToString(user.role));
        switchToView("dashboard");
    });
    connect(&auth, &Auth::AuthManager::logoutOccurred, this, [this]() {
        switchToView("login");
    });

    connect(m_loginView, &LoginView::loginRequested, this, [this](const QString& email, const QString& password) {
        m_authViewModel->login(email, password);
    });
}

void MainWindow::switchToView(const QString& viewId) {
    static const QHash<QString, int> viewMap = {
        {"login", 0}, {"dashboard", 1}, {"voting", 2}, {"voting_kiosk", 2},
        {"results", 3}, {"admin", 4}, {"administration", 4},
        {"users", 5}, {"user_management", 5}, {"settings", 6}
    };

    int index = viewMap.value(viewId, 1);
    m_stackedWidget->setCurrentIndex(index);

    bool showSidebar = (viewId != "login");
    m_sidebar->setVisible(showSidebar);

    if (showSidebar) {
        m_sidebar->setActiveItem(viewId);
    }

    // Toggle fullscreen for kiosk mode
    if (viewId == "voting" || viewId == "voting_kiosk") {
        showFullScreen();
    } else {
        showNormal();
    }

    // Refresh data
    if (viewId == "dashboard") m_dashboardViewModel->refresh();
    if (viewId == "results") m_resultsViewModel->refresh();
}

void MainWindow::setUserAuthenticated(const QString& userName, const QString& role) {
    m_sidebar->setUserInfo(userName, role);
}

} // namespace Ballot::UI

