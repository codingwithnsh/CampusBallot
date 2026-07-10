#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <memory>
#include "src/ui/components/Sidebar.h"
#include "SetupWizard.h" // Include SetupWizard header

namespace Ballot::ViewModels { class DashboardViewModel; class AuthViewModel; class ResultsViewModel; }

namespace Ballot::UI {

class MainWindow : public QMainWindow {
    Q_OBJECT
    Q_PROPERTY(QString currentViewId READ currentViewId NOTIFY currentViewIdChanged) // New property for active view

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

    Q_INVOKABLE void switchToView(const QString& viewId);
    void setUserAuthenticated(const QString& userName, const QString& role);

    QString currentViewId() const { return m_currentViewId; } // Getter for the new property

signals:
    void currentViewIdChanged(); // Signal for the new property

private:
    void setupUi();
    void setupSidebar();
    void connectSignals();
    void switchStackIndex(int newIndex);

    QStackedWidget *m_stackedWidget;
    Sidebar *m_sidebar;
    class DashboardView *m_dashboard;
    class VotingKiosk *m_kiosk;
    class LoginView *m_loginView;
    class ResultsView *m_resultsView;
    class AdminPanel *m_adminPanel;
    class UserManagementView *m_userManagement;
    class SettingsView *m_settingsView;
    class SetupWizard *m_setupWizard;

    // ViewModels
    class ViewModels::DashboardViewModel *m_dashboardViewModel;
    class ViewModels::AuthViewModel *m_authViewModel;
    class ViewModels::ResultsViewModel *m_resultsViewModel;

    QString m_currentViewId; // Member to store the current view ID
};

} // namespace Ballot::UI
