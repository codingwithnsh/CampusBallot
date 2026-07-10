#include "VoteManager.h"
#include "src/core/SystemManager.h"
#include "src/modules/audit/AuditManager.h"
#include "src/modules/security/AES256Provider.h"
#include "src/modules/security/HashProvider.h"
#include "src/modules/security/DigitalSignature.h"
#include "src/core/Utils.h" // For SystemInfo namespace
#include <QUuid>
#include <QDateTime>
#include <QDebug> // For logging
#include <QMutex> // For thread safety

namespace Ballot::Election {

// Static mutex to protect castVote from concurrent access
static QMutex voteMutex;

VoteManager& VoteManager::instance() {
    static VoteManager inst;
    return inst;
}

VoteManager::VoteManager() {}

bool VoteManager::castVote(const QString& electionId, const QString& studentId, const QString& candidateId) {
    QMutexLocker locker(&voteMutex); // Ensure only one vote is processed at a time

    auto* storage = Core::SystemManager::instance().storage();
    if (!storage || !storage->isConnected()) {
        qCritical() << "VoteManager: Storage not available or not connected. Cannot cast vote.";
        Audit::AuditManager::instance().log(
            Core::AuditAction::VoteCompleted,
            QString("Vote failed: Storage not available - Student: %1, Election: %2").arg(studentId, electionId),
            studentId);
        return false;
    }

    // --- Pre-checks for existence ---
    auto election = storage->getElection(electionId);
    if (!election) {
        qWarning() << "VoteManager: Election with ID" << electionId << "not found. Cannot cast vote.";
        Audit::AuditManager::instance().log(
            Core::AuditAction::VoteCompleted,
            QString("Vote failed: Election not found - Student: %1, Election: %2").arg(studentId, electionId),
            studentId);
        return false;
    }
    if (election->state != Core::VotingState::Voting) {
        qWarning() << "VoteManager: Election" << electionId << "is not in Voting state. Current state:" << static_cast<int>(election->state);
        Audit::AuditManager::instance().log(
            Core::AuditAction::VoteCompleted,
            QString("Vote failed: Election not active - Student: %1, Election: %2, State: %3").arg(studentId, electionId).arg(static_cast<int>(election->state)),
            studentId);
        return false;
    }

    auto student = storage->getStudent(studentId);
    if (!student) {
        qWarning() << "VoteManager: Student with ID" << studentId << "not found. Cannot cast vote.";
        Audit::AuditManager::instance().log(
            Core::AuditAction::VoteCompleted,
            QString("Vote failed: Student not found - Student: %1, Election: %2").arg(studentId, electionId),
            studentId);
        return false;
    }

    auto candidate = storage->getCandidate(candidateId);
    if (!candidate) {
        qWarning() << "VoteManager: Candidate with ID" << candidateId << "not found. Cannot cast vote.";
        Audit::AuditManager::instance().log(
            Core::AuditAction::VoteCompleted,
            QString("Vote failed: Candidate not found - Student: %1, Election: %2, Candidate: %3").arg(studentId, electionId, candidateId),
            studentId);
        return false;
    }

    // --- Duplicate vote check ---
    if (storage->hasStudentVoted(studentId, electionId)) {
        qWarning() << "VoteManager: Duplicate vote attempt blocked for Student:" << studentId << "in Election:" << electionId;
        emit duplicateVoteAttempt(studentId, electionId);
        Audit::AuditManager::instance().log(
            Core::AuditAction::VoteCompleted,
            QString("Duplicate vote attempt blocked - Student: %1, Election: %2").arg(studentId, electionId),
            studentId);
        return false;
    }

    // --- Start Transaction ---
    if (!storage->beginTransaction()) {
        qCritical() << "VoteManager: Failed to begin transaction for vote casting.";
        Audit::AuditManager::instance().log(
            Core::AuditAction::VoteCompleted,
            QString("Vote failed: Failed to begin transaction - Student: %1, Election: %2").arg(studentId, electionId),
            studentId);
        return false;
    }

    Core::Vote vote;
    vote.id = Core::IdGenerator::generateId();
    vote.electionId = electionId;
    vote.studentId = studentId;
    vote.candidateId = candidateId; // Now storing the unencrypted candidateId
    vote.timestamp = QDateTime::currentDateTime();
    vote.machineId = Core::SystemInfo::getMachineId();

    // --- Digital Signature ---
    // The vote hash will now include the unencrypted candidateId for database integrity,
    // but the hash itself will be signed to ensure tamper detection.
    QByteArray dataToHash = vote.id.toUtf8() + vote.electionId.toUtf8() + vote.studentId.toUtf8() + vote.candidateId.toUtf8() + vote.timestamp.toString(Qt::ISODate).toUtf8() + vote.machineId.toUtf8();
    vote.voteHash = Security::HashProvider::sha256(dataToHash);

    try {
        // TODO: Key generation should be managed securely, not generated on the fly for each vote.
        // For now, using a placeholder for demonstration.
        QPair<QByteArray, QByteArray> keys = Security::DigitalSignature::generateKeyPair(); // This should ideally be election-specific or system-wide
        vote.digitalSignature = Security::DigitalSignature::sign(
            vote.voteHash, keys.first); // Sign the hash of the vote data
        qDebug() << "VoteManager: Digital signature generated for vote" << vote.id;
    } catch (const std::exception& e) {
        qCritical() << "VoteManager: Failed to generate digital signature for vote" << vote.id << ". Error:" << e.what();
        storage->rollbackTransaction();
        Audit::AuditManager::instance().log(
            Core::AuditAction::VoteCompleted,
            QString("Vote failed: Digital signature error - Student: %1, Election: %2, Error: %3").arg(studentId, electionId, e.what()),
            studentId);
        return false;
    } catch (...) {
        qCritical() << "VoteManager: Unknown error during digital signature generation for vote" << vote.id;
        storage->rollbackTransaction();
        Audit::AuditManager::instance().log(
            Core::AuditAction::VoteCompleted,
            QString("Vote failed: Digital signature unknown error - Student: %1, Election: %2").arg(studentId, electionId),
            studentId);
        return false;
    }

    // --- Store Vote ---
    if (!storage->castVote(vote)) {
        qCritical() << "VoteManager: Failed to store vote in database for Student:" << studentId << "Election:" << electionId;
        storage->rollbackTransaction();
        Audit::AuditManager::instance().log(
            Core::AuditAction::VoteCompleted,
            QString("Vote failed: Database storage error - Student: %1, Election: %2").arg(studentId, electionId),
            studentId);
        return false;
    }

    // --- Update student voting status ---
    student->hasVoted = true;
    if (!storage->updateStudent(*student)) {
        qCritical() << "VoteManager: Failed to update student voting status for Student:" << studentId << ". Rolling back transaction.";
        storage->rollbackTransaction();
        Audit::AuditManager::instance().log(
            Core::AuditAction::VoteCompleted,
            QString("Vote failed: Student status update error - Student: %1, Election: %2").arg(studentId, electionId),
            studentId);
        return false;
    }

    // --- Commit Transaction ---
    if (!storage->commitTransaction()) {
        qCritical() << "VoteManager: Failed to commit transaction for vote casting. Rolling back.";
        storage->rollbackTransaction(); // Attempt to rollback if commit fails
        Audit::AuditManager::instance().log(
            Core::AuditAction::VoteCompleted,
            QString("Vote failed: Failed to commit transaction - Student: %1, Election: %2").arg(studentId, electionId),
            studentId);
        return false;
    }

    qInfo() << "VoteManager: Vote successfully cast for Student:" << studentId << "in Election:" << electionId;
    Audit::AuditManager::instance().log(
        Core::AuditAction::VoteCompleted,
        QString("Vote recorded successfully - Student: %1, Election: %2").arg(studentId, electionId),
        studentId);

    emit voteCast(electionId, studentId);
    emit resultsUpdated(electionId); // Signal for UI/dashboard to refresh
    return true;
}

bool VoteManager::hasStudentVoted(const QString& studentId, const QString& electionId) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "VoteManager: Storage not available for hasStudentVoted check.";
        return false;
    }
    return storage->hasStudentVoted(studentId, electionId);
}

int VoteManager::getVoteCount(const QString& electionId) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "VoteManager: Storage not available for getVoteCount.";
        return 0;
    }
    return storage->getVoteCount(electionId);
}

int VoteManager::getTotalVoters(const QString& electionId) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "VoteManager: Storage not available for getTotalVoters.";
        return 0;
    }
    return storage->getVoterCount(electionId);
}

QList<Core::ElectionResult> VoteManager::getResults(const QString& electionId) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "VoteManager: Storage not available for getResults.";
        return {};
    }
    return storage->getResults(electionId);
}

QList<Core::ElectionResult> VoteManager::getResultsByClass(const QString& electionId, const QString& className) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "VoteManager: Storage not available for getResultsByClass.";
        return {};
    }
    return storage->getResultsByClass(electionId, className);
}

QList<Core::ElectionResult> VoteManager::getResultsByDepartment(const QString& electionId, const QString& department) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "VoteManager: Storage not available for getResultsByDepartment.";
        return {};
    }
    return storage->getResultsByDepartment(electionId, department);
}

QList<Core::ElectionResult> VoteManager::getResultsByGender(const QString& electionId, const QString& gender) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "VoteManager: Storage not available for getResultsByGender.";
        return {};
    }
    return storage->getResultsByGender(electionId, gender);
}

double VoteManager::getTurnout(const QString& electionId) const {
    int votes = getVoteCount(electionId);
    int total = getTotalVoters(electionId);
    if (total == 0) return 0.0;
    return (static_cast<double>(votes) / total) * 100.0;
}

// --- TEMPORARY AUDITABLE ENCRYPTION (for vote hash, not candidate ID in DB) ---
// TODO: Replace with a secure, election-specific key management system.
// This hardcoded key is for auditability during development/testing ONLY.
static const QByteArray TEMP_ENCRYPTION_KEY = QByteArrayLiteral("ThisIsATempSecretKeyForAuditing1234567890"); // 32 bytes for AES256

// This function is now effectively unused for storing candidateId in the DB.
// It could be repurposed to encrypt the entire vote object if needed for end-to-end encryption.
QByteArray VoteManager::encryptVote(const QString& data) const {
    Security::AES256Provider crypto;
    try {
        QByteArray encrypted = crypto.encrypt(data.toUtf8(), TEMP_ENCRYPTION_KEY);
        qDebug() << "VoteManager: Encrypted data. Original size:" << data.toUtf8().size() << "Encrypted size:" << encrypted.size();
        return encrypted;
    } catch (const std::exception& e) {
        qCritical() << "VoteManager: Encryption failed for data. Error:" << e.what();
        return data.toUtf8(); // Fallback to unencrypted if encryption fails (CRITICAL, should ideally prevent vote)
    } catch (...) {
        qCritical() << "VoteManager: Unknown encryption failed for data.";
        return data.toUtf8(); // Fallback to unencrypted
    }
}

// This function is now effectively unused for decrypting candidateId from the DB.
// It could be repurposed to decrypt the entire vote object if needed for end-to-end encryption.
QString VoteManager::decryptVote(const QByteArray& encrypted) const {
    Security::AES256Provider crypto;
    try {
        QByteArray decrypted = crypto.decrypt(encrypted, TEMP_ENCRYPTION_KEY);
        qDebug() << "VoteManager: Decrypted data. Encrypted size:" << encrypted.size() << "Decrypted size:" << decrypted.size();
        return QString::fromUtf8(decrypted);
    } catch (const std::exception& e) {
        qCritical() << "VoteManager: Decryption failed. Error:" << e.what();
        return QString("DECRYPTION_FAILED: %1").arg(e.what());
    } catch (...) {
        qCritical() << "VoteManager: Unknown decryption failed.";
        return "DECRYPTION_FAILED: Unknown error";
    }
}

} // namespace Ballot::Election