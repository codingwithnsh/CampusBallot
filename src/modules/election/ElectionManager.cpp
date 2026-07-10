#include "ElectionManager.h"
#include "src/core/SystemManager.h"
#include "src/modules/audit/AuditManager.h"
#include "src/core/Utils.h" // Include for Core::SystemInfo and Core::IdGenerator
#include "src/modules/election/VoteManager.h" // Include VoteManager for voting operations
#include <QDateTime>
#include <QDebug> // For debugging

namespace Ballot::Election {

ElectionManager& ElectionManager::instance() {
    static ElectionManager inst;
    return inst;
}

ElectionManager::ElectionManager() {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ElectionManager::checkElectionTimers);
    m_timer->start(10000); // Check every 10 seconds
    qDebug() << "ElectionManager: Initialized. Timer started for election state checks.";
}

bool ElectionManager::createElection(const Core::Election& election) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "ElectionManager: Storage not available. Cannot create election.";
        return false;
    }

    Core::Election e = election;
    if (e.title.trimmed().isEmpty()) {
        qWarning() << "ElectionManager: Cannot create an election without a title.";
        return false;
    }
    if (e.startDate.isValid() && e.endDate.isValid() && e.endDate < e.startDate) {
        qWarning() << "ElectionManager: Election end date precedes start date.";
        return false;
    }
    if (e.maxVotesPerStudent < 1) {
        qWarning() << "ElectionManager: maxVotesPerStudent must be at least one.";
        return false;
    }
    if (e.id.isEmpty()) {
        e.id = Core::IdGenerator::generateId(); // Use centralized ID generator
    }
    e.createdAt = QDateTime::currentDateTime();

    if (storage->createElection(e)) {
        Audit::AuditManager::instance().log(
            Core::AuditAction::ElectionCreated,
            "Election created: " + e.title,
            e.createdBy);
        emit electionCreated(e.id);
        qInfo() << "ElectionManager: Election created successfully:" << e.title << "(" << e.id << ")";
        return true;
    }
    qCritical() << "ElectionManager: Failed to create election:" << e.title << ". Storage error.";
    return false;
}

bool ElectionManager::updateElection(const Core::Election& election) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "ElectionManager: Storage not available. Cannot update election.";
        return false;
    }

    if (election.title.trimmed().isEmpty()) {
        qWarning() << "ElectionManager: Cannot update an election without a title.";
        return false;
    }
    if (election.startDate.isValid() && election.endDate.isValid() && election.endDate < election.startDate) {
        qWarning() << "ElectionManager: Election end date precedes start date.";
        return false;
    }
    if (election.maxVotesPerStudent < 1) {
        qWarning() << "ElectionManager: maxVotesPerStudent must be at least one.";
        return false;
    }

    if (!storage->updateElection(election)) {
        qCritical() << "ElectionManager: Failed to update election:" << election.title << ". Storage error.";
        return false;
    }

    Audit::AuditManager::instance().log(
        Core::AuditAction::ElectionModified,
        "Election modified: " + election.title,
        election.createdBy); // Assuming createdBy is the last modifier for audit purposes
    emit electionUpdated(election.id);
    qInfo() << "ElectionManager: Election updated successfully:" << election.title << "(" << election.id << ")";
    return true;
}

bool ElectionManager::deleteElection(const QString& id) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "ElectionManager: Storage not available. Cannot delete election.";
        return false;
    }

    auto electionOpt = storage->getElection(id);
    QString electionTitle = electionOpt ? electionOpt->title : "Unknown";

    if (!storage->deleteElection(id)) {
        qCritical() << "ElectionManager: Failed to delete election:" << electionTitle << "(" << id << "). Storage error.";
        return false;
    }

    Audit::AuditManager::instance().log(
        Core::AuditAction::ElectionDeleted,
        "Election deleted: " + electionTitle,
        "System"); // Assuming system action for deletion
    emit electionDeleted(id);
    qInfo() << "ElectionManager: Election deleted successfully:" << electionTitle << "(" << id << ")";
    return true;
}

bool ElectionManager::startElection(const QString& id) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "ElectionManager: Storage not available. Cannot start election.";
        return false;
    }

    std::optional<Core::Election> electionOpt = storage->getElection(id);
    if (!electionOpt) {
        qWarning() << "ElectionManager: Election with ID" << id << "not found. Cannot start election.";
        return false;
    }
    Core::Election election = *electionOpt;

    if (election.state == Core::VotingState::Voting) {
        qWarning() << "ElectionManager: Election" << id << "is already in Voting state.";
        return true; // Already started, consider it a success
    }
    if (election.state == Core::VotingState::Ended) {
        qWarning() << "ElectionManager: Election" << id << "has already ended. Cannot restart.";
        return false;
    }

    const bool wasIdle = election.state == Core::VotingState::Idle;
    election.state = Core::VotingState::Voting;
    election.isActive = true;
    // Preserve the original start time when resuming a paused election.
    if (wasIdle) {
        election.startDate = QDateTime::currentDateTime();
    }

    if (!storage->updateElection(election)) {
        qCritical() << "ElectionManager: Failed to update election state to Voting for" << election.title << ". Storage error.";
        return false;
    }

    Audit::AuditManager::instance().log(
        Core::AuditAction::ElectionStarted,
        "Election started: " + election.title,
        "System"); // System action or user who initiated
    emit electionStarted(id);
    emit electionStateChanged(id, Core::VotingState::Voting);
    qInfo() << "ElectionManager: Election" << election.title << "(" << id << ") started.";
    return true;
}

bool ElectionManager::endElection(const QString& id) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "ElectionManager: Storage not available. Cannot end election.";
        return false;
    }

    std::optional<Core::Election> electionOpt = storage->getElection(id);
    if (!electionOpt) {
        qWarning() << "ElectionManager: Election with ID" << id << "not found. Cannot end election.";
        return false;
    }
    Core::Election election = *electionOpt;

    if (election.state == Core::VotingState::Ended) {
        qWarning() << "ElectionManager: Election" << id << "has already ended.";
        return true; // Already ended, consider it a success
    }

    election.state = Core::VotingState::Ended;
    election.isActive = false;
    election.endDate = QDateTime::currentDateTime();

    if (!storage->updateElection(election)) {
        qCritical() << "ElectionManager: Failed to update election state to Ended for" << election.title << ". Storage error.";
        return false;
    }

    Audit::AuditManager::instance().log(
        Core::AuditAction::ElectionEnded,
        "Election ended: " + election.title,
        "System");
    emit electionEnded(id);
    emit electionStateChanged(id, Core::VotingState::Ended);
    qInfo() << "ElectionManager: Election" << election.title << "(" << id << ") ended.";
    return true;
}

bool ElectionManager::pauseElection(const QString& id) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "ElectionManager: Storage not available. Cannot pause election.";
        return false;
    }

    std::optional<Core::Election> electionOpt = storage->getElection(id);
    if (!electionOpt) {
        qWarning() << "ElectionManager: Election with ID" << id << "not found. Cannot pause election.";
        return false;
    }
    Core::Election election = *electionOpt;

    if (election.state == Core::VotingState::Paused) {
        qWarning() << "ElectionManager: Election" << id << "is already in Paused state.";
        return true; // Already paused, consider it a success
    }
    if (election.state != Core::VotingState::Voting) {
        qWarning() << "ElectionManager: Election" << id << "is not in Voting state. Cannot pause.";
        return false;
    }

    election.state = Core::VotingState::Paused;

    if (!storage->updateElection(election)) {
        qCritical() << "ElectionManager: Failed to update election state to Paused for" << election.title << ". Storage error.";
        return false;
    }

    Audit::AuditManager::instance().log(
        Core::AuditAction::ElectionPaused,
        "Election paused: " + election.title,
        "System");
    emit electionStateChanged(id, Core::VotingState::Paused);
    qInfo() << "ElectionManager: Election" << election.title << "(" << id << ") paused.";
    return true;
}

std::optional<Core::Election> ElectionManager::getElection(const QString& id) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "ElectionManager: Storage not available. Cannot get election.";
        return std::nullopt;
    }
    return storage->getElection(id);
}

std::optional<Core::Election> ElectionManager::getActiveElection() const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "ElectionManager: Storage not available. Cannot get active election.";
        return std::nullopt;
    }

    QList<Core::Election> elections = storage->getElections();
    for (const auto& election : elections) {
        if (election.isActive && election.state == Core::VotingState::Voting) {
            return election;
        }
    }
    return std::nullopt;
}

QList<Core::Election> ElectionManager::getElections() const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "ElectionManager: Storage not available. Cannot get elections.";
        return {};
    }
    return storage->getElections();
}

QList<Core::Election> ElectionManager::getActiveElections() const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "ElectionManager: Storage not available. Cannot get active elections.";
        return {};
    }
    QList<Core::Election> activeElections;
    for (const auto& election : storage->getElections()) {
        if (election.isActive && election.state == Core::VotingState::Voting) { // Only truly active and voting elections
            activeElections.append(election);
        }
    }
    return activeElections;
}

bool ElectionManager::addCandidate(const Core::Candidate& candidate) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "ElectionManager: Storage not available. Cannot add candidate.";
        return false;
    }

    Core::Candidate c = candidate;
    if (c.id.isEmpty()) {
        c.id = Core::IdGenerator::generateId(); // Use centralized ID generator
    }
    c.registeredAt = QDateTime::currentDateTime();

    if (!storage->addCandidate(c)) {
        qCritical() << "ElectionManager: Failed to add candidate:" << c.name << ". Storage error.";
        return false;
    }

    Audit::AuditManager::instance().log(
        Core::AuditAction::CandidateAdded,
        "Candidate added: " + c.name + " for election " + c.electionId,
        "System"); // Assuming system action or user who added
    qInfo() << "ElectionManager: Candidate" << c.name << "(" << c.id << ") added successfully to election" << c.electionId;
    return true;
}

bool ElectionManager::updateCandidate(const Core::Candidate& candidate) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "ElectionManager: Storage not available. Cannot update candidate.";
        return false;
    }

    if (!storage->updateCandidate(candidate)) {
        qCritical() << "ElectionManager: Failed to update candidate:" << candidate.name << ". Storage error.";
        return false;
    }

    Audit::AuditManager::instance().log(
        Core::AuditAction::CandidateModified,
        "Candidate modified: " + candidate.name + " for election " + candidate.electionId,
        "System");
    qInfo() << "ElectionManager: Candidate" << candidate.name << "(" << candidate.id << ") updated successfully.";
    return true;
}

bool ElectionManager::deleteCandidate(const QString& id) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "ElectionManager: Storage not available. Cannot delete candidate.";
        return false;
    }

    auto candidateOpt = storage->getCandidate(id);
    QString candidateName = candidateOpt ? candidateOpt->name : "Unknown";

    if (!storage->deleteCandidate(id)) {
        qCritical() << "ElectionManager: Failed to delete candidate:" << candidateName << "(" << id << "). Storage error.";
        return false;
    }

    Audit::AuditManager::instance().log(
        Core::AuditAction::CandidateDeleted,
        "Candidate deleted: " + candidateName,
        "System");
    qInfo() << "ElectionManager: Candidate" << candidateName << "(" << id << ") deleted successfully.";
    return true;
}

QList<Core::Candidate> ElectionManager::getCandidates(const QString& electionId) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "ElectionManager: Storage not available. Cannot get candidates.";
        return {};
    }
    return storage->getCandidates(electionId);
}

Core::VotingState ElectionManager::getElectionState(const QString& electionId) const {
    std::optional<Core::Election> e = getElection(electionId);
    if (e.has_value()) {
        return e->state;
    }
    qWarning() << "ElectionManager: Election" << electionId << "not found. Returning Unknown state.";
    return Core::VotingState::Unknown;
}

// Removed: bool ElectionManager::canVote(...)
// Removed: bool ElectionManager::castVote(...)

void ElectionManager::checkElectionTimers() {
    auto elections = getElections();
    auto now = QDateTime::currentDateTime();
    for (const auto& e : elections) {
        // Automatically start elections
        if (e.state == Core::VotingState::Idle && e.startDate.isValid() && now >= e.startDate) {
            qInfo() << "ElectionManager: Auto-starting election" << e.title << "(" << e.id << ")";
            startElection(e.id); // This will log and emit signals
        }
        // Automatically end elections
        if (e.state == Core::VotingState::Voting && e.endDate.isValid() && now >= e.endDate) {
            qInfo() << "ElectionManager: Auto-ending election" << e.title << "(" << e.id << ")";
            endElection(e.id); // This will log and emit signals
        }
    }
}

} // namespace Ballot::Election
