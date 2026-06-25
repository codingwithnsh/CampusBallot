#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <memory>
#include "src/ui/components/Sidebar.h"

namespace Ballot::ViewModels { class DashboardViewModel; class AuthViewModel; class ResultsViewModel; }

namespace Ballot::UI {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

    void switchToView(const QString& viewId);
    void setUserAuthenticated(const QString& userName, const QString& role);

private:
    void setupUi();
    void setupSidebar();
    void connectSignals();

    QStackedWidget *m_stackedWidget;
    Sidebar *m_sidebar;
    class DashboardView *m_dashboard;
    class VotingKiosk *m_kiosk;
    class LoginView *m_loginView;
    class ResultsView *m_resultsView;
    class AdminPanel *m_adminPanel;
    class UserManagementView *m_userManagement;
    class SettingsView *m_settingsView;

    // ViewModels
    class ViewModels::DashboardViewModel *m_dashboardViewModel;
    class ViewModels::AuthViewModel *m_authViewModel;
    class ViewModels::ResultsViewModel *m_resultsViewModel;
};

} // namespace Ballot::UI
