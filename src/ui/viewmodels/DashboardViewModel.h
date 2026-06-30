#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include "src/modules/storage/interfaces/IStorageProvider.h"
#include "src/core/Models.h"

namespace Ballot::ViewModels {

class DashboardViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int totalStudents READ totalStudents NOTIFY totalStudentsChanged)
    Q_PROPERTY(int votesCast READ votesCast NOTIFY votesCastChanged)
    Q_PROPERTY(double turnout READ turnout NOTIFY turnoutChanged)
    Q_PROPERTY(bool isMaster READ isMaster NOTIFY roleChanged)
    Q_PROPERTY(QString votingStatus READ votingStatus NOTIFY votingStatusChanged)
    Q_PROPERTY(QString currentElectionTitle READ currentElectionTitle NOTIFY currentElectionTitleChanged) // Changed from currentElection
    Q_PROPERTY(QString dbStatus READ dbStatus NOTIFY dbStatusChanged)
    Q_PROPERTY(QString storageType READ storageType NOTIFY storageTypeChanged)
    Q_PROPERTY(QString serverStatus READ serverStatus NOTIFY serverStatusChanged)
    Q_PROPERTY(QString auditStatus READ auditStatus NOTIFY auditStatusChanged)
    Q_PROPERTY(QString backupStatus READ backupStatus NOTIFY backupStatusChanged)

public:
    explicit DashboardViewModel(Core::IStorageProvider* storage, QObject *parent = nullptr);

    int totalStudents() const { return m_totalStudents; }
    int votesCast() const { return m_votesCast; }
    int remainingStudents() const { return m_totalStudents - m_votesCast; }
    double turnout() const;
    bool isMaster() const;
    QString votingStatus() const;
    QString currentElectionTitle() const { return m_currentElectionTitle; } // Changed from currentElection
    QString dbStatus() const { return m_dbStatus; }
    QString storageType() const { return m_storageType; }
    QString serverStatus() const { return m_serverStatus; }
    QString auditStatus() const { return m_auditStatus; }
    QString backupStatus() const { return m_backupStatus; }


    void refresh();
    void startVoting();
    void endVoting();

signals:
    void totalStudentsChanged();
    void votesCastChanged();
    void turnoutChanged();
    void roleChanged();
    void votingStatusChanged();
    void currentElectionTitleChanged(); // Changed from electionChanged
    void dbStatusChanged();
    void storageTypeChanged();
    void serverStatusChanged();
    void auditStatusChanged();
    void backupStatusChanged();
    void errorOccurred(const QString& error);

private:
    Core::IStorageProvider* m_storage;
    int m_totalStudents = 0;
    int m_votesCast = 0;
    QString m_currentElectionTitle; // Changed from m_currentElection
    Core::VotingState m_status = Core::VotingState::Idle;

    // New member variables for system health
    QString m_dbStatus;
    QString m_storageType;
    QString m_serverStatus;
    QString m_auditStatus;
    QString m_backupStatus;
};

} // namespace Ballot::ViewModels