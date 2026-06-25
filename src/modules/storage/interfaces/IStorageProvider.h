#pragma once

#include <QString>
#include <QVariantMap>
#include <QList>
#include <optional>
#include "src/core/Models.h"

namespace Ballot::Core {

class IStorageProvider {
public:
    virtual ~IStorageProvider() = default;

    virtual bool connect(const QVariantMap& config) = 0;
    virtual bool disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual QString providerName() const = 0;
    virtual Core::StorageProviderType providerType() const = 0;
    virtual bool testConnection() = 0;

    // Election Management
    virtual bool createElection(const Election& election) = 0;
    virtual bool updateElection(const Election& election) = 0;
    virtual bool deleteElection(const QString& id) = 0;
    virtual std::optional<Election> getElection(const QString& id) = 0;
    virtual QList<Election> getElections() = 0;
    virtual QList<Election> getActiveElections() = 0;

    // Candidate Management
    virtual bool addCandidate(const Candidate& candidate) = 0;
    virtual bool updateCandidate(const Candidate& candidate) = 0;
    virtual bool deleteCandidate(const QString& id) = 0;
    virtual std::optional<Candidate> getCandidate(const QString& id) = 0;
    virtual QList<Candidate> getCandidates(const QString& electionId) = 0;

    // Student Management
    virtual bool addStudent(const Student& student) = 0;
    virtual bool updateStudent(const Student& student) = 0;
    virtual bool deleteStudent(const QString& id) = 0;
    virtual std::optional<Student> getStudent(const QString& id) = 0;
    virtual std::optional<Student> getStudentByAdmission(const QString& admissionNumber) = 0;
    virtual std::optional<Student> getStudentByVotingId(const QString& votingId) = 0;
    virtual QList<Student> getStudents() = 0;
    virtual QList<Student> getStudentsByClass(const QString& className) = 0;
    virtual QList<Student> getEligibleVoters(const QString& electionId) = 0;
    virtual int getStudentCount() = 0;
    virtual int getVoterCount(const QString& electionId) = 0;

    // User Management
    virtual bool createUser(const User& user) = 0;
    virtual bool updateUser(const User& user) = 0;
    virtual bool deleteUser(const QString& id) = 0;
    virtual std::optional<User> getUser(const QString& id) = 0;
    virtual std::optional<User> getUserByEmail(const QString& email) = 0;
    virtual QList<User> getUsers() = 0;
    virtual QList<User> getUsersByRole(UserRole role) = 0;

    // Voting
    virtual bool castVote(const Vote& vote) = 0;
    virtual bool hasStudentVoted(const QString& studentId, const QString& electionId) = 0;
    virtual int getVoteCount(const QString& electionId) = 0;
    virtual QList<ElectionResult> getResults(const QString& electionId) = 0;
    virtual QList<ElectionResult> getResultsByClass(const QString& electionId, const QString& className) = 0;
    virtual QList<ElectionResult> getResultsByDepartment(const QString& electionId, const QString& department) = 0;
    virtual QList<ElectionResult> getResultsByGender(const QString& electionId, const QString& gender) = 0;
    virtual int getTotalVotesCast(const QString& electionId) = 0;

    // Audit
    virtual bool logAction(const AuditLogEntry& log) = 0;
    virtual QList<AuditLogEntry> getAuditLogs(const QDateTime& from = QDateTime(), const QDateTime& to = QDateTime()) = 0;
    virtual QList<AuditLogEntry> getAuditLogsByUser(const QString& userId) = 0;
    virtual QList<AuditLogEntry> getAuditLogsByAction(AuditAction action) = 0;
    virtual int getAuditLogCount() = 0;

    // System & Machine Management
    virtual bool registerMachine(const MachineInfo& machine) = 0;
    virtual bool updateMachine(const MachineInfo& machine) = 0;
    virtual QList<MachineInfo> getMachines() = 0;
    virtual std::optional<MachineInfo> getMachine(const QString& id) = 0;
    virtual std::optional<SystemSettings> getSystemSettings() = 0;
    virtual bool updateSystemSettings(const SystemSettings& settings) = 0;

    // Backup
    virtual bool saveBackupRecord(const BackupEntry& backup) = 0;
    virtual QList<BackupEntry> getBackupHistory() = 0;
    virtual bool deleteBackupRecord(const QString& id) = 0;

    // Bulk operations
    virtual bool bulkAddStudents(const QList<Student>& students) = 0;
    virtual bool bulkAddCandidates(const QList<Candidate>& candidates) = 0;
    virtual bool clearElectionData(const QString& electionId) = 0;
};

} // namespace Ballot::Core
