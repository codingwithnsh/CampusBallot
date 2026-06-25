#pragma once

#include "src/modules/storage/interfaces/IStorageProvider.h"
#include <QSqlDatabase>

namespace Ballot::Storage {

class SQLiteStorageProvider : public Core::IStorageProvider {
public:
    SQLiteStorageProvider() = default;
    ~SQLiteStorageProvider() override;

    bool connect(const QVariantMap& config) override;
    bool disconnect() override;
    bool isConnected() const override;
    QString providerName() const override { return "SQLite"; }
    Core::StorageProviderType providerType() const override { return Core::StorageProviderType::SQLite; }
    bool testConnection() override;

    bool createElection(const Core::Election& election) override;
    bool updateElection(const Core::Election& election) override;
    bool deleteElection(const QString& id) override;
    std::optional<Core::Election> getElection(const QString& id) override;
    QList<Core::Election> getElections() override;
    QList<Core::Election> getActiveElections() override;

    bool addCandidate(const Core::Candidate& candidate) override;
    bool updateCandidate(const Core::Candidate& candidate) override;
    bool deleteCandidate(const QString& id) override;
    std::optional<Core::Candidate> getCandidate(const QString& id) override;
    QList<Core::Candidate> getCandidates(const QString& electionId) override;

    bool addStudent(const Core::Student& student) override;
    bool updateStudent(const Core::Student& student) override;
    bool deleteStudent(const QString& id) override;
    std::optional<Core::Student> getStudent(const QString& id) override;
    std::optional<Core::Student> getStudentByAdmission(const QString& admissionNumber) override;
    std::optional<Core::Student> getStudentByVotingId(const QString& votingId) override;
    QList<Core::Student> getStudents() override;
    QList<Core::Student> getStudentsByClass(const QString& className) override;
    QList<Core::Student> getEligibleVoters(const QString& electionId) override;
    int getStudentCount() override;
    int getVoterCount(const QString& electionId) override;

    bool createUser(const Core::User& user) override;
    bool updateUser(const Core::User& user) override;
    bool deleteUser(const QString& id) override;
    std::optional<Core::User> getUser(const QString& id) override;
    std::optional<Core::User> getUserByEmail(const QString& email) override;
    QList<Core::User> getUsers() override;
    QList<Core::User> getUsersByRole(Core::UserRole role) override;

    bool castVote(const Core::Vote& vote) override;
    bool hasStudentVoted(const QString& studentId, const QString& electionId) override;
    int getVoteCount(const QString& electionId) override;
    QList<Core::ElectionResult> getResults(const QString& electionId) override;
    QList<Core::ElectionResult> getResultsByClass(const QString& electionId, const QString& className) override;
    QList<Core::ElectionResult> getResultsByDepartment(const QString& electionId, const QString& department) override;
    QList<Core::ElectionResult> getResultsByGender(const QString& electionId, const QString& gender) override;
    int getTotalVotesCast(const QString& electionId) override;

    bool logAction(const Core::AuditLogEntry& log) override;
    QList<Core::AuditLogEntry> getAuditLogs(const QDateTime& from, const QDateTime& to) override;
    QList<Core::AuditLogEntry> getAuditLogsByUser(const QString& userId) override;
    QList<Core::AuditLogEntry> getAuditLogsByAction(Core::AuditAction action) override;
    int getAuditLogCount() override;

    bool registerMachine(const Core::MachineInfo& machine) override;
    bool updateMachine(const Core::MachineInfo& machine) override;
    QList<Core::MachineInfo> getMachines() override;
    std::optional<Core::MachineInfo> getMachine(const QString& id) override;
    std::optional<Core::SystemSettings> getSystemSettings() override;
    bool updateSystemSettings(const Core::SystemSettings& settings) override;

    bool saveBackupRecord(const Core::BackupEntry& backup) override;
    QList<Core::BackupEntry> getBackupHistory() override;
    bool deleteBackupRecord(const QString& id) override;

    bool bulkAddStudents(const QList<Core::Student>& students) override;
    bool bulkAddCandidates(const QList<Core::Candidate>& candidates) override;
    bool clearElectionData(const QString& electionId) override;

private:
    QSqlDatabase m_db;
    bool initSchema();
    bool execQuery(const QString& sql);
};

} // namespace Ballot::Storage
