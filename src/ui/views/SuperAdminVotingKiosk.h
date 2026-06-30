#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QTimer>
#include <QTableWidget>
#include <optional>
#include "src/core/Models.h"
#include "src/modules/election/ElectionManager.h"

namespace Ballot::UI {

class SuperAdminVotingKiosk : public QWidget {
    Q_OBJECT
public:
    explicit SuperAdminVotingKiosk(QWidget* parent = nullptr);
    void start();

private:
    void setupUi();
    void updateVotingState();
    void refreshElections();
    void refreshVoters(const QString& electionId);
    void refreshResults(const QString& electionId);
    void resetKiosk();

    QWidget* createDashboardPage();
    QWidget* createElectionSelectorPage();
    QWidget* createVoterManagementPage();
    QWidget* createResultsPage();
    QWidget* createControlPage();

    QStackedWidget* m_pages;
    int m_currentPage = 0;
    QTimer* m_refreshTimer;

    std::optional<Core::Election> m_selectedElection;
    QList<Core::Election> m_elections;
    QTableWidget* m_electionTable = nullptr;
    QTableWidget* m_voterTable = nullptr;
    QTableWidget* m_resultsTable = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_startElectionBtn = nullptr;
    QPushButton* m_pauseElectionBtn = nullptr;
    QPushButton* m_endElectionBtn = nullptr;
    QComboBox* m_electionCombo = nullptr;
};

} // namespace Ballot::UI