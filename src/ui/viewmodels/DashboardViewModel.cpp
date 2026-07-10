#include "DashboardViewModel.h"
#include "src/core/SystemManager.h"
#include "src/modules/audit/AuditManager.h"
#include "src/modules/backup/BackupManager.h"
#include "src/modules/election/ElectionManager.h"
#include "src/modules/election/VoteManager.h" // Include VoteManager for vote counts
#include <QDebug>

namespace Ballot::ViewModels {

DashboardViewModel::DashboardViewModel(QObject *parent)
    : QObject(parent) {
    qDebug() << "DashboardViewModel: Initializing...";

    // Connect to relevant manager signals for automatic refresh
    connect(&Core::SystemManager::instance(), &Core::SystemManager::settingsChanged, this, &DashboardViewModel::refresh);
    connect(&Core::SystemManager::instance(), &Core::SystemManager::storageChanged, this, &DashboardViewModel::refresh);
    connect(&Election::ElectionManager::instance(), &Election::ElectionManager::electionStateChanged, this, &DashboardViewModel::refresh);
    connect(&Election::ElectionManager::instance(), &Election::ElectionManager::electionCreated, this, &DashboardViewModel::refresh);
    connect(&Election::ElectionManager::instance(), &Election::ElectionManager::electionDeleted, this, &DashboardViewModel::refresh);
    connect(&Election::VoteManager::instance(), &Election::VoteManager::voteCast, this, &DashboardViewModel::refresh);
    connect(&Election::VoteManager::instance(), &Election::VoteManager::resultsUpdated, this, &DashboardViewModel::refresh);
    connect(&Audit::AuditManager::instance(), &Audit::AuditManager::logAdded, this, &DashboardViewModel::refresh); // Refresh on new audit log

    // Setup refresh timer
    m_refreshTimer.setInterval(Core::Constants::DASHBOARD_REFRESH_MS); // Use constant for refresh interval
    connect(&m_refreshTimer, &QTimer::timeout, this, &DashboardViewModel::refresh);
    m_refreshTimer.start();

    refresh(); // Initial refresh
    qDebug() << "DashboardViewModel: Initialized and refresh timer started.";
}

double DashboardViewModel::turnout() const {
    if (m_totalStudents == 0) return 0.0;
    return (static_cast<double>(m_votesCast) / m_totalStudents) * 100.0;
}

bool DashboardViewModel::isMaster() const {
    return Core::SystemManager::instance().isMaster();
}

QString DashboardViewModel::votingStatus() const {
    switch (m_status) {
        case Core::VotingState::Idle:   return "Not Started";
        case Core::VotingState::Voting: return "In Progress";
        case Core::VotingState::Ended:  return "Ended";
        case Core::VotingState::Paused: return "Paused";
        case Core::VotingState::Unknown: return "Unknown";
    }
    return "Unknown"; // Should not be reached
}

void DashboardViewModel::refresh() {
    qDebug() << "DashboardViewModel: Refreshing data...";
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage || !storage->isConnected()) {
        qWarning() << "DashboardViewModel: Storage not connected during refresh.";
        m_totalStudents = 0;
        m_votesCast = 0;
        m_currentElectionTitle = "Storage not connected";
        m_status = Core::VotingState::Unknown; // Or Idle, depending on desired behavior
        m_dbStatus = "● Disconnected";
        m_storageType = "N/A";
        m_serverStatus = "● Offline";
        m_auditStatus = "● Inactive";
        m_backupStatus = "● Inactive";

        emit totalStudentsChanged();
        emit votesCastChanged();
        emit turnoutChanged();
        emit votingStatusChanged();
        emit currentElectionTitleChanged();
        emit dbStatusChanged();
        emit storageTypeChanged();
        emit serverStatusChanged();
        emit auditStatusChanged();
        emit backupStatusChanged();
        emit roleChanged(); // isMaster might change
        return;
    }

    // --- Update Election-related Stats ---
    m_totalStudents = storage->getStudentCount();
    emit totalStudentsChanged();

    std::optional<Core::Election> activeElectionOpt = Election::ElectionManager::instance().getActiveElection();
    if (activeElectionOpt) {
        Core::Election activeElection = *activeElectionOpt;
        m_currentElectionTitle = activeElection.title;
        m_status = activeElection.state;
        m_votesCast = Election::VoteManager::instance().getVoteCount(activeElection.id);
        qDebug() << "DashboardViewModel: Active election found:" << activeElection.title << ", Votes cast:" << m_votesCast;
    } else {
        m_currentElectionTitle = "No active election";
        m_status = Core::VotingState::Idle;
        m_votesCast = 0;
        qDebug() << "DashboardViewModel: No active election found.";
    }
    emit votesCastChanged();
    emit turnoutChanged();
    emit votingStatusChanged();
    emit currentElectionTitleChanged();

    // --- Update System Health Statuses ---
    QString oldDbStatus = m_dbStatus;
    m_dbStatus = storage->isConnected() ? "● Connected" : "● Disconnected";
    if (oldDbStatus != m_dbStatus) emit dbStatusChanged();

    QString oldStorageType = m_storageType;
    m_storageType = storage->providerName();
    if (oldStorageType != m_storageType) emit storageTypeChanged();

    QString oldServerStatus = m_serverStatus;
    if (Core::SystemManager::instance().isMaster()) {
        m_serverStatus = "● Online (Master)";
    } else {
        // This needs more sophisticated logic for client status, e.g., checking connection to master
        m_serverStatus = "● Online (Client)";
    }
    if (oldServerStatus != m_serverStatus) emit serverStatusChanged();

    QString oldAuditStatus = m_auditStatus;
    m_auditStatus = Audit::AuditManager::instance().isInitialized() ? "● Active" : "● Inactive";
    if (oldAuditStatus != m_auditStatus) emit auditStatusChanged();

    QString oldBackupStatus = m_backupStatus;
    m_backupStatus = Backup::BackupManager::instance().isInitialized() ? "● Active" : "● Inactive";
    if (oldBackupStatus != m_backupStatus) emit backupStatusChanged();

    // isMaster might change, so always emit roleChanged
    emit roleChanged();
    qDebug() << "DashboardViewModel: Refresh complete.";
}

void DashboardViewModel::startVoting() {
    qInfo() << "DashboardViewModel: Start voting requested.";
    if (!isMaster()) {
        emit errorOccurred("Cannot start voting: Not the master machine.");
        qWarning() << "DashboardViewModel: Attempt to start voting on non-master machine.";
        return;
    }

    auto activeElectionOpt = Election::ElectionManager::instance().getActiveElection();
    if (activeElectionOpt) {
        // If there's already an active election, try to start it (e.g., resume from paused)
        if (Election::ElectionManager::instance().startElection(activeElectionOpt->id)) {
            qInfo() << "DashboardViewModel: Successfully started/resumed election" << activeElectionOpt->title;
        } else {
            emit errorOccurred("Failed to start/resume the active election.");
            qCritical() << "DashboardViewModel: Failed to start/resume election" << activeElectionOpt->id;
        }
    } else {
        // Find an idle or paused election to start
        auto elections = Election::ElectionManager::instance().getElections();
        std::optional<Core::Election> electionToStart;
        for (const auto& election : elections) {
            if (election.state == Core::VotingState::Idle || election.state == Core::VotingState::Paused) {
                electionToStart = election;
                break;
            }
        }

        if (electionToStart) {
            if (Election::ElectionManager::instance().startElection(electionToStart->id)) {
                qInfo() << "DashboardViewModel: Successfully started election" << electionToStart->title;
            } else {
                emit errorOccurred("Failed to start the selected election.");
                qCritical() << "DashboardViewModel: Failed to start election" << electionToStart->id;
            }
        } else {
            emit errorOccurred("No eligible election found to start voting.");
            qWarning() << "DashboardViewModel: No eligible election found to start.";
        }
    }
    refresh(); // Refresh UI after action
}

void DashboardViewModel::endVoting() {
    qInfo() << "DashboardViewModel: End voting requested.";
    if (!isMaster()) {
        emit errorOccurred("Cannot end voting: Not the master machine.");
        qWarning() << "DashboardViewModel: Attempt to end voting on non-master machine.";
        return;
    }

    auto activeElectionOpt = Election::ElectionManager::instance().getActiveElection();
    if (activeElectionOpt) {
        if (Election::ElectionManager::instance().endElection(activeElectionOpt->id)) {
            qInfo() << "DashboardViewModel: Successfully ended election" << activeElectionOpt->title;
        } else {
            emit errorOccurred("Failed to end the active election.");
            qCritical() << "DashboardViewModel: Failed to end election" << activeElectionOpt->id;
        }
    } else {
        emit errorOccurred("No active election found to end voting.");
        qWarning() << "DashboardViewModel: No active election found to end.";
    }
    refresh(); // Refresh UI after action
}

} // namespace Ballot::ViewModels