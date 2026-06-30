#include "DashboardViewModel.h"
#include "src/core/SystemManager.h"
#include "src/modules/audit/AuditManager.h" // Include AuditManager
#include "src/modules/backup/BackupManager.h" // Include BackupManager
#include "src/modules/election/ElectionManager.h" // Include ElectionManager for voting state

namespace Ballot::ViewModels {

DashboardViewModel::DashboardViewModel(Core::IStorageProvider* storage, QObject *parent)
    : QObject(parent), m_storage(storage) {
    // Initial refresh
    refresh();
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
        default:                        return "Unknown";
    }
}

// Removed explicit definitions of getters as they are inline in .h
/*
QString DashboardViewModel::currentElectionTitle() const {
    return m_currentElectionTitle;
}

QString DashboardViewModel::dbStatus() const {
    return m_dbStatus;
}

QString DashboardViewModel::storageType() const {
    return m_storageType;
}

QString DashboardViewModel::serverStatus() const {
    return m_serverStatus;
}

QString DashboardViewModel::auditStatus() const {
    return m_auditStatus;
}

QString DashboardViewModel::backupStatus() const {
    return m_backupStatus;
}
*/

void DashboardViewModel::refresh() {
    // Update student and vote counts
    if (m_storage && m_storage->isConnected()) {
        m_totalStudents = m_storage->getStudentCount();

        // Get the most recent active election for vote counts and status
        auto elections = m_storage->getElections();
        if (!elections.isEmpty()) {
            // Prefer a running election, otherwise show the next startable one.
            std::optional<Core::Election> currentActiveElection;
            for (const auto& election : elections) {
                if (election.isActive) {
                    currentActiveElection = election;
                    break;
                }
            }
            if (!currentActiveElection) {
                for (const auto& election : elections) {
                    if (election.state == Core::VotingState::Idle || election.state == Core::VotingState::Paused) {
                        currentActiveElection = election;
                        break;
                    }
                }
            }

            if (currentActiveElection) {
                m_currentElectionTitle = currentActiveElection->title;
                m_status = currentActiveElection->state;
                m_votesCast = m_storage->getVoteCount(currentActiveElection->id);
            } else {
                m_currentElectionTitle = "No active elections";
                m_status = Core::VotingState::Idle;
                m_votesCast = 0;
            }
        } else {
            m_currentElectionTitle = "No elections configured";
            m_status = Core::VotingState::Idle;
            m_votesCast = 0;
        }
    } else {
        m_totalStudents = 0;
        m_votesCast = 0;
        m_currentElectionTitle = "Storage not connected";
        m_status = Core::VotingState::Idle;
    }

    emit totalStudentsChanged();
    emit votesCastChanged();
    emit turnoutChanged();
    emit votingStatusChanged();
    emit currentElectionTitleChanged();

    // Update system health statuses
    // DB Status
    QString oldDbStatus = m_dbStatus;
    if (m_storage && m_storage->isConnected()) {
        m_dbStatus = "● Connected";
    } else {
        m_dbStatus = "● Disconnected";
    }
    if (oldDbStatus != m_dbStatus) emit dbStatusChanged();

    // Storage Type
    QString oldStorageType = m_storageType;
    if (m_storage) {
        m_storageType = m_storage->providerName();
    } else {
        m_storageType = "Unknown";
    }
    if (oldStorageType != m_storageType) emit storageTypeChanged();

    // Server Status (simplified: assumes master machine is the "server")
    QString oldServerStatus = m_serverStatus;
    if (Core::SystemManager::instance().isMaster()) {
        m_serverStatus = "● Online (Master)";
    } else {
        m_serverStatus = "● Offline"; // Or "Online (Client)" if connected to master
    }
    if (oldServerStatus != m_serverStatus) emit serverStatusChanged();

    // Audit Status
    QString oldAuditStatus = m_auditStatus;
    if (Audit::AuditManager::instance().isInitialized()) {
        m_auditStatus = "● Active";
    } else {
        m_auditStatus = "● Inactive";
    }
    if (oldAuditStatus != m_auditStatus) emit auditStatusChanged();

    // Backup Status
    QString oldBackupStatus = m_backupStatus;
    if (Backup::BackupManager::instance().isInitialized()) {
        m_backupStatus = "● Active";
    } else {
        m_backupStatus = "● Inactive";
    }
    if (oldBackupStatus != m_backupStatus) emit backupStatusChanged();

    // Role changed might affect button visibility, so emit it
    emit roleChanged();
}

void DashboardViewModel::startVoting() {
    if (!m_storage || !isMaster()) {
        emit errorOccurred("Cannot start voting: Storage not available or not master machine.");
        return;
    }

    // Resume the active election or start the most recently created idle one.
    auto elections = m_storage->getElections();
    std::optional<Core::Election> currentActiveElection;
    for (const auto& election : elections) {
        if (election.isActive) {
            currentActiveElection = election;
            break;
        }
    }
    if (!currentActiveElection) {
        for (const auto& election : elections) {
            if (election.state == Core::VotingState::Idle || election.state == Core::VotingState::Paused) {
                currentActiveElection = election;
                break;
            }
        }
    }

    if (currentActiveElection) {
        Election::ElectionManager::instance().startElection(currentActiveElection->id); // Corrected namespace
        refresh(); // Refresh UI after action
    } else {
        emit errorOccurred("No active election to start voting for.");
    }
}

void DashboardViewModel::endVoting() {
    if (!m_storage || !isMaster()) {
        emit errorOccurred("Cannot end voting: Storage not available or not master machine.");
        return;
    }

    // Assuming there's an active election to end
    auto elections = m_storage->getElections();
    std::optional<Core::Election> currentActiveElection;
    for (const auto& election : elections) {
        if (election.isActive) {
            currentActiveElection = election;
            break;
        }
    }

    if (currentActiveElection) {
        Election::ElectionManager::instance().endElection(currentActiveElection->id); // Corrected namespace
        refresh(); // Refresh UI after action
    } else {
        emit errorOccurred("No active election to end voting for.");
    }
}

} // namespace Ballot::ViewModels
