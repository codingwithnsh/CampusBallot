#include "ElectionManager.h"
#include "src/core/SystemManager.h"
#include "src/modules/audit/AuditManager.h"
#include <QUuid>
#include <QDateTime>

namespace Ballot::Election {

ElectionManager& ElectionManager::instance() {
    static ElectionManager inst;
    return inst;
}

ElectionManager::ElectionManager() {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ElectionManager::checkElectionTimers);
    m_timer->start(10000);
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

    auto election = storage->getElection(id);
    if (!election) return false;

    election->state = Core::VotingState::Voting;
    election->isActive = true;
    election->startDate = QDateTime::currentDateTime();

    if (storage->updateElection(*election)) {
        Audit::AuditManager::instance().log(
            Core::AuditAction::ElectionCreated,
            "Election started: " + election->title);
        emit electionStarted(id);
        emit electionStateChanged(id, Core::VotingState::Voting);
        return true;
    }
    return false;
}

bool ElectionManager::endElection(const QString& id) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return false;

    auto election = storage->getElection(id);
    if (!election) return false;

    election->state = Core::VotingState::Ended;
    election->isActive = false;
    election->endDate = QDateTime::currentDateTime();

    if (storage->updateElection(*election)) {
        emit electionEnded(id);
        emit electionStateChanged(id, Core::VotingState::Ended);
        return true;
    }
    return false;
}

bool ElectionManager::pauseElection(const QString& id) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return false;

    auto election = storage->getElection(id);
    if (!election) return false;

    election->state = Core::VotingState::Paused;

    if (storage->updateElection(*election)) {
        emit electionStateChanged(id, Core::VotingState::Paused);
        return true;
    }
    return false;
}

Core::Election ElectionManager::getElection(const QString& id) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return {};
    auto e = storage->getElection(id);
    return e.value_or(Core::Election());
}

QList<Core::Election> ElectionManager::getElections() const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return {};
    return storage->getElections();
}

QList<Core::Election> ElectionManager::getActiveElections() const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return {};
    return storage->getActiveElections();
}

bool ElectionManager::addCandidate(const Core::Candidate& candidate) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return false;

    Core::Candidate c = candidate;
    if (c.id.isEmpty()) {
        c.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    c.registeredAt = QDateTime::currentDateTime();

    return storage->addCandidate(c);
}

bool ElectionManager::updateCandidate(const Core::Candidate& candidate) {
    auto* storage = Core::SystemManager::instance().storage();
    return storage && storage->updateCandidate(candidate);
}

bool ElectionManager::deleteCandidate(const QString& id) {
    auto* storage = Core::SystemManager::instance().storage();
    return storage && storage->deleteCandidate(id);
}

QList<Core::Candidate> ElectionManager::getCandidates(const QString& electionId) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return {};
    return storage->getCandidates(electionId);
}

Core::VotingState ElectionManager::getElectionState(const QString& electionId) const {
    auto e = getElection(electionId);
    return e.state;
}

bool ElectionManager::canVote(const QString& studentId, const QString& electionId) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return false;
    return !storage->hasStudentVoted(studentId, electionId);
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
