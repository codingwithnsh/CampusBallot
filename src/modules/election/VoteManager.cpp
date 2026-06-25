#include "VoteManager.h"
#include "src/core/SystemManager.h"
#include "src/modules/audit/AuditManager.h"
#include "src/security/AES256Provider.h"
#include "src/security/HashProvider.h"
#include "src/security/DigitalSignature.h"
#include "src/core/Utils.h"
#include <QUuid>
#include <QDateTime>

namespace Ballot::Election {

VoteManager& VoteManager::instance() {
    static VoteManager inst;
    return inst;
}

VoteManager::VoteManager() {}

bool VoteManager::castVote(const QString& electionId, const QString& studentId, const QString& candidateId) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage || !storage->isConnected()) return false;

    // Check if student already voted
    if (storage->hasStudentVoted(studentId, electionId)) {
        emit duplicateVoteAttempt(studentId, electionId);

        Audit::AuditManager::instance().log(
            Core::AuditAction::VoteCompleted,
            QString("Duplicate vote attempt blocked - Student: %1, Election: %2").arg(studentId, electionId));

        return false;
    }

    // Check if election is active
    auto election = storage->getElection(electionId);
    if (!election || election->state != Core::VotingState::Voting) return false;

    Core::Vote vote;
    vote.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    vote.electionId = electionId;
    vote.studentId = studentId;
    vote.encryptedCandidateId = encryptVote(candidateId);
    vote.timestamp = QDateTime::currentDateTime();
    vote.machineId = Core::Utils::getMachineId();
    vote.voteHash = Security::HashProvider::sha256(vote.id.toUtf8() + vote.studentId.toUtf8());

    // Digitally sign the vote
    try {
        auto keys = Security::DigitalSignature::generateKeyPair();
        vote.digitalSignature = Security::DigitalSignature::sign(
            vote.voteHash + vote.encryptedCandidateId.toUtf8(), keys.first);
    } catch (...) {
        // Continue without digital signature if key generation fails
    }

    bool success = storage->castVote(vote);
    if (success) {
        // Update student voting status
        auto student = storage->getStudent(studentId);
        if (student) {
            student->hasVoted = true;
            storage->updateStudent(*student);
        }

        Audit::AuditManager::instance().log(
            Core::AuditAction::VoteCompleted,
            QString("Vote recorded - Election: %1").arg(electionId),
            studentId);

        emit voteCast(electionId, studentId);
        emit resultsUpdated(electionId);
    }

    return success;
}

bool VoteManager::hasStudentVoted(const QString& studentId, const QString& electionId) const {
    auto* storage = Core::SystemManager::instance().storage();
    return storage && storage->hasStudentVoted(studentId, electionId);
}

int VoteManager::getVoteCount(const QString& electionId) const {
    auto* storage = Core::SystemManager::instance().storage();
    return storage ? storage->getVoteCount(electionId) : 0;
}

int VoteManager::getTotalVoters(const QString& electionId) const {
    auto* storage = Core::SystemManager::instance().storage();
    return storage ? storage->getVoterCount(electionId) : 0;
}

QList<Core::ElectionResult> VoteManager::getResults(const QString& electionId) const {
    auto* storage = Core::SystemManager::instance().storage();
    return storage ? storage->getResults(electionId) : QList<Core::ElectionResult>();
}

QList<Core::ElectionResult> VoteManager::getResultsByClass(const QString& electionId, const QString& className) const {
    auto* storage = Core::SystemManager::instance().storage();
    return storage ? storage->getResultsByClass(electionId, className) : QList<Core::ElectionResult>();
}

QList<Core::ElectionResult> VoteManager::getResultsByDepartment(const QString& electionId, const QString& department) const {
    auto* storage = Core::SystemManager::instance().storage();
    return storage ? storage->getResultsByDepartment(electionId, department) : QList<Core::ElectionResult>();
}

QList<Core::ElectionResult> VoteManager::getResultsByGender(const QString& electionId, const QString& gender) const {
    auto* storage = Core::SystemManager::instance().storage();
    return storage ? storage->getResultsByGender(electionId, gender) : QList<Core::ElectionResult>();
}

double VoteManager::getTurnout(const QString& electionId) const {
    int votes = getVoteCount(electionId);
    int total = getTotalVoters(electionId);
    if (total == 0) return 0.0;
    return (static_cast<double>(votes) / total) * 100.0;
}

QByteArray VoteManager::encryptVote(const QString& candidateId) const {
    try {
        Security::AES256Provider crypto;
        QByteArray key = crypto.generateKey(32);
        QByteArray encrypted = crypto.encrypt(candidateId.toUtf8(), key);
        // Store encrypted vote; key is discarded to prevent decryption
        return encrypted;
    } catch (...) {
        return candidateId.toUtf8();
    }
}

QString VoteManager::decryptVote(const QByteArray& encrypted) const {
    return "***ENCRYPTED***";
}

} // namespace Ballot::Election
