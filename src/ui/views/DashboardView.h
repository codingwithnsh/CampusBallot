#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QTimer>
#include "src/ui/viewmodels/DashboardViewModel.h"
#include "src/ui/components/StatCard.h"

namespace Ballot::UI {

class DashboardView : public QWidget {
    Q_OBJECT
public:
    explicit DashboardView(QWidget *parent = nullptr);
    void setViewModel(ViewModels::DashboardViewModel* vm);

private:
    void setupUi();
    void updateUi();
    QWidget* createSystemHealthSection();
    QWidget* createQuickActionsSection();

    ViewModels::DashboardViewModel* m_viewModel = nullptr;
    QTimer* m_refreshTimer;

    StatCard* m_registeredCard;
    StatCard* m_votedCard;
    StatCard* m_remainingCard;
    StatCard* m_turnoutCard;

    QLabel* m_statusLabel;
    QLabel* m_electionTitle;
    QLabel* m_dbStatus;
    QLabel* m_storageType;
    QLabel* m_serverStatus;
    QLabel* m_auditStatus;
    QLabel* m_backupStatus;

    QPushButton* m_startVotingBtn;
    QPushButton* m_endVotingBtn;
    QPushButton* m_kioskBtn;
};

} // namespace Ballot::UI
