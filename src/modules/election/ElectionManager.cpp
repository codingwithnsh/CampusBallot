#include "ElectionManager.h"
#include "src/core/SystemManager.h"
#include "src/modules/audit/AuditManager.h"
#include "src/core/Utils.h" // Include for Core::Utils::getMachineId()
#include "src/security/HashProvider.h"
#include <QUuid>
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
}

bool ElectionManager::createElection(const Core::Election& election) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return false;

    Core::Election e = election;
    if (e.id.isEmpty()) {
        e.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    e.createdAt = QDateTime::currentDateTime();

    if (storage->createElection(e)) {
        Audit::AuditManager::instance().log(
            Core::AuditAction::ElectionCreated,
            "Election created: " + e.title);
        emit electionCreated(e.id);
        return true;
    }
    return false;
}

bool ElectionManager::updateElection(const Core::Election& election) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage || !storage->updateElection(election)) return false;

    Audit::AuditManager::instance().log(
        Core::AuditAction::ElectionModified,
        "Election modified: " + election.title);
    emit electionUpdated(election.id);
    return true;
}

bool ElectionManager::deleteElection(const QString& id) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage || !storage->deleteElection(id)) return false;

    Audit::AuditManager::instance().log(
        Core::AuditAction::ElectionDeleted,
        "Election deleted: " + id);
    emit electionDeleted(id);
    return true;
}

bool ElectionManager::startElection(const QString& id) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return false;

    std::optional<Core::Election> electionOpt = storage->getElection(id);
    if (!electionOpt) return false;
    Core::Election election = *electionOpt; // Get the value from optional

    if (election.state != Core::VotingState::Idle && election.state != Core::VotingState::Paused) {
        return false;
    }

    const bool wasIdle = election.state == Core::VotingState::Idle;
    election.state = Core::VotingState::Voting;
    election.isActive = true;
    // Preserve the original start time when resuming a paused election.
    if (wasIdle) {
        election.startDate = QDateTime::currentDateTime();
    }


    if (storage->updateElection(election)) {
        Audit::AuditManager::instance().log(
            Core::AuditAction::ElectionStarted, // Changed to ElectionStarted
            "Election started: " + election.title);
        emit electionStarted(id);
        emit electionStateChanged(id, Core::VotingState::Voting);
        return true;
    }
    return false;
}

bool ElectionManager::endElection(const QString& id) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return false;

    std::optional<Core::Election> electionOpt = storage->getElection(id);
    if (!electionOpt) return false;
    Core::Election election = *electionOpt;

    if (election.state != Core::VotingState::Voting && election.state != Core::VotingState::Paused) {
        return false;
    }

    election.state = Core::VotingState::Ended;
    election.isActive = false;
    election.endDate = QDateTime::currentDateTime();

    if (storage->updateElection(election)) {
        Audit::AuditManager::instance().log(
            Core::AuditAction::ElectionEnded, // Changed to ElectionEnded
            "Election ended: " + election.title);
        emit electionEnded(id);
        emit electionStateChanged(id, Core::VotingState::Ended);
        return true;
    }
    return false;
}

bool ElectionManager::pauseElection(const QString& id) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return false;

    std::optional<Core::Election> electionOpt = storage->getElection(id);
    if (!electionOpt) return false;
    Core::Election election = *electionOpt;

    if (election.state != Core::VotingState::Voting) return false;

    election.state = Core::VotingState::Paused;

    if (storage->updateElection(election)) {
        Audit::AuditManager::instance().log(
            Core::AuditAction::ElectionPaused, // Assuming this enum exists
            "Election paused: " + election.title);
        emit electionStateChanged(id, Core::VotingState::Paused);
        return true;
    }
    return false;
}

std::optional<Core::Election> ElectionManager::getElection(const QString& id) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return std::nullopt;
    return storage->getElection(id);
}

std::optional<Core::Election> ElectionManager::getActiveElection() const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return std::nullopt;

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
    if (!storage) return {};
    return storage->getElections();
}

// This method is redundant with getActiveElection() but kept for now
QList<Core::Election> ElectionManager::getActiveElections() const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return {};
    QList<Core::Election> activeElections;
    for (const auto& election : storage->getElections()) {
        if (election.isActive) {
            activeElections.append(election);
        }
    }
    return activeElections;
}

bool ElectionManager::addCandidate(const Core::Candidate& candidate) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return false;

    Core::Candidate c = candidate;
    if (c.id.isEmpty()) {
        c.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    c.registeredAt = QDateTime::currentDateTime();

    if (storage->addCandidate(c)) {
        Audit::AuditManager::instance().log(
            Core::AuditAction::CandidateAdded, // Assuming this enum exists
            "Candidate added: " + c.name + " for election " + c.electionId);
        return true;
    }
    return false;
}

bool ElectionManager::updateCandidate(const Core::Candidate& candidate) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage || !storage->updateCandidate(candidate)) return false;

    Audit::AuditManager::instance().log(
        Core::AuditAction::CandidateModified, // Assuming this enum exists
        "Candidate modified: " + candidate.name + " for election " + candidate.electionId);
    return true;
}

bool ElectionManager::deleteCandidate(const QString& id) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage || !storage->deleteCandidate(id)) return false;

    Audit::AuditManager::instance().log(
        Core::AuditAction::CandidateDeleted, // Assuming this enum exists
        "Candidate deleted: " + id);
    return true;
}

QList<Core::Candidate> ElectionManager::getCandidates(const QString& electionId) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return {};
    return storage->getCandidates(electionId);
}

Core::VotingState ElectionManager::getElectionState(const QString& electionId) const {
    std::optional<Core::Election> e = getElection(electionId);
    if (e.has_value()) {
        return e->state;
    }
    return Core::VotingState::Unknown; // Return an appropriate default or error state
}

bool ElectionManager::canVote(const QString& studentId, const QString& electionId) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return false;

    std::optional<Core::Election> electionOpt = getElection(electionId);
    if (!electionOpt || !electionOpt->isActive || electionOpt->state != Core::VotingState::Voting) {
        return false; // Cannot vote if election is not active or not in voting state
    }

    const auto student = storage->getStudent(studentId);
    if (!student) return false;
    if (!electionOpt->eligibleClasses.isEmpty() && !electionOpt->eligibleClasses.contains(student->className)) return false;

    // Check if student has already voted
    return !storage->hasStudentVoted(studentId, electionId);
}

bool ElectionManager::castVote(const QString& electionId, const QString& studentId, const QString& candidateId) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return false;

    // Basic validation
    if (electionId.isEmpty() || studentId.isEmpty() || candidateId.isEmpty()) {
        qWarning() << "ElectionManager::castVote: Invalid parameters.";
        return false;
    }

    // Check if election is active and in voting state
    std::optional<Core::Election> electionOpt = getElection(electionId);
    if (!electionOpt || !electionOpt->isActive || electionOpt->state != Core::VotingState::Voting) {
        qWarning() << "ElectionManager::castVote: Election not active or not in voting state.";
        return false;
    }

    // Check if student is eligible and hasn't voted yet
    if (!canVote(studentId, electionId)) {
        qWarning() << "ElectionManager::castVote: Student not eligible or already voted.";
        return false;
    }

    const auto candidate = storage->getCandidate(candidateId);
    if (!candidate || candidate->electionId != electionId || !candidate->isApproved) {
        qWarning() << "ElectionManager::castVote: Candidate is invalid or not approved.";
        return false;
    }

    Core::Vote vote;
    vote.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    vote.electionId = electionId;
    vote.studentId = studentId;
    // Results aggregation requires a stable candidate identifier. Access to the
    // votes table must therefore be protected at the database/file level.
    vote.encryptedCandidateId = candidateId;
    vote.timestamp = QDateTime::currentDateTime();
    vote.machineId = Core::Utils::getMachineId();
    vote.isAudited = false; // Default to false, audit process will update this

    vote.voteHash = Security::HashProvider::sha256(
        vote.id.toUtf8() + vote.electionId.toUtf8() + vote.encryptedCandidateId.toUtf8()
        + vote.timestamp.toString(Qt::ISODateWithMs).toUtf8() + vote.machineId.toUtf8());

    if (storage->castVote(vote)) {
        // Update student's voted status
        std::optional<Core::Student> studentOpt = storage->getStudent(studentId);
        if (studentOpt) {
            Core::Student student = *studentOpt;
            student.hasVoted = true;
            storage->updateStudent(student);
        }

        Audit::AuditManager::instance().log(
            Core::AuditAction::VoteCast,
            "Vote recorded for election " + electionId,
            studentId);
        emit voteCast(electionId, studentId, candidateId);
        return true;
    }

    return false;
}


void ElectionManager::checkElectionTimers() {
    auto elections = getElections();
    auto now = QDateTime::currentDateTime();
    for (const auto& e : elections) {
        if (e.state == Core::VotingState::Idle && e.startDate.isValid() && now >= e.startDate) {
            startElection(e.id);
        }
        if (e.state == Core::VotingState::Voting && e.endDate.isValid() && now >= e.endDate) {
            endElection(e.id);
        }
    }
}

} // namespace Ballot::Election
