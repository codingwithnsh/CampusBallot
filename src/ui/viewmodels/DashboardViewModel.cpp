#include "DashboardViewModel.h"
#include "src/core/SystemManager.h"

namespace Ballot::ViewModels {

DashboardViewModel::DashboardViewModel(Core::IStorageProvider* storage, QObject *parent)
    : QObject(parent), m_storage(storage) {
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

void DashboardViewModel::refresh() {
    if (!m_storage || !m_storage->isConnected()) return;

    m_totalStudents = m_storage->getStudentCount();

    // Get the most recent active election for vote counts
    auto elections = m_storage->getElections();
    if (!elections.isEmpty()) {
        auto election = elections.first();
        m_currentElection = election.title;
        m_status = election.state;
        m_votesCast = m_storage->getVoteCount(election.id);
    } else {
        m_currentElection = "No active elections";
        m_votesCast = 0;
    }

    emit totalStudentsChanged();
    emit votesCastChanged();
    emit turnoutChanged();
    emit roleChanged();
    emit votingStatusChanged();
    emit electionChanged();
}

void DashboardViewModel::startVoting() {
    if (!m_storage || !isMaster()) return;
    auto settings = m_storage->getSystemSettings();
    if (settings) {
        settings->votingStatus = Core::VotingState::Voting;
        m_storage->updateSystemSettings(*settings);
        m_status = Core::VotingState::Voting;
        emit votingStatusChanged();
    }
}

void DashboardViewModel::endVoting() {
    if (!m_storage || !isMaster()) return;
    auto settings = m_storage->getSystemSettings();
    if (settings) {
        settings->votingStatus = Core::VotingState::Ended;
        m_storage->updateSystemSettings(*settings);
        m_status = Core::VotingState::Ended;
        emit votingStatusChanged();
    }
}

} // namespace Ballot::ViewModels
