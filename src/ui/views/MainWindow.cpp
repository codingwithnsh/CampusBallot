#include "MainWindow.h"
#include "DashboardView.h"
#include "VotingKiosk.h"
#include "LoginView.h"
#include "ResultsView.h"
#include "AdminPanel.h"
#include "UserManagementView.h"
#include "SettingsView.h"
#include "SetupWizard.h"
#include "src/ui/viewmodels/DashboardViewModel.h"
#include "src/ui/viewmodels/AuthViewModel.h"
#include "src/ui/viewmodels/ResultsViewModel.h"
#include "src/core/SystemManager.h"
#include "src/core/config/ApplicationBootstrap.h"
#include "src/modules/auth/AuthManager.h"
#include "src/modules/auth/RBACManager.h"
#include "src/ui/components/ToastNotification.h" // For displaying messages
#include <QHBoxLayout>
#include <QApplication>
#include <QIcon>
#include <QScreen> // For screen geometry
#include <QDebug>
#include <QSettings>

namespace Ballot::UI {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();
    setupSidebar();
    connectSignals();

    // Initial view based on system state
    auto* storage = Core::SystemManager::instance().storage();
    // Check if storage is connected and if there's at least one SuperAdministrator
    if (!storage || !storage->isConnected() || storage->getUsersByRole(Core::UserRole::SuperAdministrator).isEmpty()) {
        qInfo() << "MainWindow: No SuperAdministrator found or storage not connected. Switching to SetupWizard.";
        switchToView("setupWizard");
    } else {
        qInfo() << "MainWindow: SuperAdministrator found. Switching to LoginView.";
        switchToView("login");
    }
    qDebug() << "MainWindow: Initialization complete.";
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

    // Sidebar
    m_sidebar = new Sidebar(centralWidget);
    m_sidebar->setVisible(false); // Initially hidden
    hLayout->addWidget(m_sidebar);

    // Stacked widget for views
    m_stackedWidget = new QStackedWidget(centralWidget);
    m_stackedWidget->setStyleSheet("background-color: #1a1a2e;"); // Ensure background for stacked widget

    hLayout->addWidget(m_stackedWidget, 1);

    setCentralWidget(centralWidget);

    // Create ViewModels (pass this as parent)
    m_dashboardViewModel = new ViewModels::DashboardViewModel(this);
    m_authViewModel = new ViewModels::AuthViewModel(this);
    m_resultsViewModel = new ViewModels::ResultsViewModel(this);

    // Create views (pass this as parent)
    m_loginView = new LoginView(this);
    m_dashboard = new DashboardView(this);
    m_kiosk = new VotingKiosk(this);
    m_resultsView = new ResultsView(this);
    m_adminPanel = new AdminPanel(this);
    m_userManagement = new UserManagementView(this);
    m_settingsView = new SettingsView(this);
    m_setupWizard = new SetupWizard(this);

    // Connect view models to views
    m_dashboard->setViewModel(m_dashboardViewModel);
    m_loginView->setViewModel(m_authViewModel);
    m_resultsView->setViewModel(m_resultsViewModel);

    // Add views to stack
    m_stackedWidget->addWidget(m_loginView);      // Index 0
    m_stackedWidget->addWidget(m_dashboard);     // Index 1
    m_stackedWidget->addWidget(m_kiosk);         // Index 2
    m_stackedWidget->addWidget(m_resultsView);   // Index 3
    m_stackedWidget->addWidget(m_adminPanel);    // Index 4
    m_stackedWidget->addWidget(m_userManagement); // Index 5
    m_stackedWidget->addWidget(m_settingsView);  // Index 6
    m_stackedWidget->addWidget(m_setupWizard);   // Index 7

    qDebug() << "MainWindow: UI setup complete.";
}

void MainWindow::setupSidebar() {
    // Using modern icons (emojis or FontAwesome-like characters)
    m_sidebar->addItem("dashboard", "Dashboard", "📊");
    m_sidebar->addItem("voting", "Voting Kiosk", "🗳️");
    m_sidebar->addItem("results", "Results", "📈");
    m_sidebar->addItem("users", "User Management", "👥");
    m_sidebar->addItem("admin", "Administration", "⚙️");
    m_sidebar->addItem("settings", "Settings", "🛠️");
    m_sidebar->addItem("logout", "Logout", "🚪");

    connect(m_sidebar, &Sidebar::itemClicked, this, [this](const QString& id) {
        qDebug() << "MainWindow: Sidebar item clicked:" << id;
        if (id == "logout") {
            Auth::AuthManager::instance().logout();
            // logout will emit logoutOccurred, which will call switchToView("login")
        } else {
            switchToView(id);
        }
    });
    qDebug() << "MainWindow: Sidebar setup complete.";
}

void MainWindow::connectSignals() {
    auto& auth = Auth::AuthManager::instance();
    connect(&auth, &Auth::AuthManager::loginSuccessful, this, [this](const QString& userId) {
        Q_UNUSED(userId);
        qInfo() << "MainWindow: Login successful for user" << userId;
        auto user = Auth::AuthManager::instance().currentUser();
        setUserAuthenticated(user.name, Auth::RBACManager::instance().roleToString(user.role));
        switchToView("dashboard");
    });
    connect(&auth, &Auth::AuthManager::logoutOccurred, this, [this]() {
        qInfo() << "MainWindow: Logout occurred. Switching to login view.";
        setUserAuthenticated("Guest", "N/A"); // Clear user info in sidebar
        switchToView("login");
    });
    connect(&auth, &Auth::AuthManager::loginFailed, this, [this](const QString& reason) {
        qWarning() << "MainWindow: Login failed:" << reason;
        ToastNotification::show(this, reason, ToastNotification::Error);
    });

    connect(m_loginView, &LoginView::loginRequested, this, [this](const QString& email, const QString& password, const QString& authType) {
        m_authViewModel->login(email, password, authType);
    });

    connect(m_loginView, &LoginView::signupRequested, this, [this]() {
        qInfo() << "MainWindow: Signup requested. Switching to SetupWizard.";
        auto* storage = Core::SystemManager::instance().storage();
        if (storage && !storage->getUsersByRole(Core::UserRole::SuperAdministrator).isEmpty()) {
            ToastNotification::show(this, "Setup is already complete. Please log in with an administrator account.", ToastNotification::Info);
            return;
        }
        switchToView("setupWizard");
    });

    connect(m_setupWizard, &SetupWizard::setupCompleted, this, [this](const QVariantMap& config) {
        qInfo() << "MainWindow: SetupWizard completed. Processing configuration.";
        const Core::BootstrapResult result = Core::ApplicationBootstrap::initializeRuntime(config);
        if (!result.success) {
            ToastNotification::show(this, result.errorMessage, ToastNotification::Error);
            return;
        }

        QSettings settings;
        Core::ApplicationBootstrap::saveConfiguration(settings, result.sanitizedConfig);
        switchToView("login");
    });

    qDebug() << "MainWindow: Signals connected.";
}

/**
 * @brief Switches the stacked widget page directly.
 *
 * A previous implementation applied QGraphicsOpacityEffect to the whole
 * QStackedWidget. Several child widgets also use graphics effects for shadows,
 * which causes Qt painter/effect failures on Windows during login transitions.
 * Direct switching is stable and avoids rendering artifacts.
 * @param newIndex The index of the page to switch to.
 */
void MainWindow::switchStackIndex(int newIndex) {
    if (m_stackedWidget->currentIndex() == newIndex) {
        qDebug() << "MainWindow: Already on target view index" << newIndex << ".";
        return;
    }

    m_stackedWidget->setCurrentIndex(newIndex);
    qDebug() << "MainWindow: Switched view. Current index:" << newIndex;
}

void MainWindow::switchToView(const QString& viewId) {
    qInfo() << "MainWindow: Request to switch to view:" << viewId;
    static const QHash<QString, int> viewMap = {
        {"login", 0}, {"dashboard", 1}, {"voting", 2}, {"voting_kiosk", 2},
        {"results", 3}, {"admin", 4}, {"administration", 4},
        {"users", 5}, {"user_management", 5}, {"settings", 6},
        {"setupWizard", 7}
    };

    // Authentication and Authorization checks
    if (viewId != "login" && viewId != "setupWizard") {
        auto& auth = Auth::AuthManager::instance();
        if (!auth.isAuthenticated()) {
            qWarning() << "MainWindow: Not authenticated. Redirecting to login.";
            ToastNotification::show(this, "Please log in to access this feature.", ToastNotification::Warning);
            switchToView("login");
            return;
        }

        bool allowed = true;
        if (viewId == "results") allowed = auth.hasPermission(Auth::RBACManager::PERM_RESULTS_VIEW);
        else if (viewId == "users") allowed = auth.hasPermission(Auth::RBACManager::PERM_USER_MANAGE);
        else if (viewId == "admin" || viewId == "administration") allowed = auth.hasPermission(Auth::RBACManager::PERM_ELECTION_MODIFY);
        else if (viewId == "settings") allowed = auth.hasPermission(Auth::RBACManager::PERM_SETTINGS_MODIFY);
        else if (viewId == "voting" || viewId == "voting_kiosk") {
            allowed = auth.hasPermission(Auth::RBACManager::PERM_VOTE_VERIFY)
                    || auth.hasPermission(Auth::RBACManager::PERM_VOTE_START);
        }
        // Add more permission checks for other views as needed

        if (!allowed) {
            qWarning() << "MainWindow: Permission denied for view:" << viewId << ". Redirecting to dashboard.";
            ToastNotification::show(this, "Permission Denied: You do not have access to this feature.", ToastNotification::Error);
            switchToView("dashboard"); // Redirect to a default view if not allowed
            return;
        }
    }

    int newIndex = viewMap.value(viewId, -1); // Default to -1 if not found
    if (newIndex == -1) {
        qWarning() << "MainWindow: Attempted to switch to unknown view ID:" << viewId;
        ToastNotification::show(this, "Error: Unknown view requested.", ToastNotification::Error);
        // Fallback to dashboard or login if viewId is unknown
        if (Auth::AuthManager::instance().isAuthenticated()) {
            switchToView("dashboard");
        } else {
            switchToView("login");
        }
        return;
    }

    // Update currentViewId property
    if (m_currentViewId != viewId) {
        m_currentViewId = viewId;
        emit currentViewIdChanged();
    }

    // Handle sidebar visibility
    bool showSidebar = (viewId != "login" && viewId != "setupWizard" && viewId != "voting" && viewId != "voting_kiosk");
    m_sidebar->setVisible(showSidebar);

    if (showSidebar) {
        m_sidebar->setActiveItem(viewId);
    }

    // Toggle fullscreen for kiosk mode
    if (viewId == "voting" || viewId == "voting_kiosk") {
        showFullScreen();
        m_sidebar->setVisible(false); // Ensure sidebar is hidden in kiosk mode
        m_kiosk->start(); // Start kiosk specific logic
    } else {
        showNormal();
        // Ensure kiosk is reset if exiting kiosk mode
        if (m_stackedWidget->currentWidget() == m_kiosk) {
            m_kiosk->resetKiosk();
        }
    }

    switchStackIndex(newIndex);

    // Refresh data for relevant views
    if (viewId == "dashboard") m_dashboardViewModel->refresh();
    if (viewId == "results") m_resultsViewModel->refresh();
    // Add refresh calls for other views as they become active
}

void MainWindow::setUserAuthenticated(const QString& userName, const QString& role) {
    m_sidebar->setUserInfo(userName, role);
    auto& auth = Auth::AuthManager::instance();

    // Update sidebar item visibility based on permissions
    m_sidebar->setItemVisible("voting", auth.hasPermission(Auth::RBACManager::PERM_VOTE_VERIFY)
                                       || auth.hasPermission(Auth::RBACManager::PERM_VOTE_START));
    m_sidebar->setItemVisible("results", auth.hasPermission(Auth::RBACManager::PERM_RESULTS_VIEW));
    m_sidebar->setItemVisible("users", auth.hasPermission(Auth::RBACManager::PERM_USER_MANAGE));
    m_sidebar->setItemVisible("admin", auth.hasPermission(Auth::RBACManager::PERM_ELECTION_MODIFY));
    m_sidebar->setItemVisible("settings", auth.hasPermission(Auth::RBACManager::PERM_SETTINGS_MODIFY));
    m_sidebar->setItemVisible("logout", true); // Logout is always visible when authenticated
    qInfo() << "MainWindow: User authenticated. Sidebar permissions updated.";
}

} // namespace Ballot::UI
