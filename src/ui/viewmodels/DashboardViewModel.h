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
    Q_PROPERTY(QString currentElection READ currentElection NOTIFY electionChanged)

public:
    explicit DashboardViewModel(Core::IStorageProvider* storage, QObject *parent = nullptr);

    int totalStudents() const { return m_totalStudents; }
    int votesCast() const { return m_votesCast; }
    int remainingStudents() const { return m_totalStudents - m_votesCast; }
    double turnout() const;
    bool isMaster() const;
    QString votingStatus() const;
    QString currentElection() const { return m_currentElection; }

    void refresh();
    void startVoting();
    void endVoting();

signals:
    void totalStudentsChanged();
    void votesCastChanged();
    void turnoutChanged();
    void roleChanged();
    void votingStatusChanged();
    void electionChanged();
    void errorOccurred(const QString& error);

private:
    Core::IStorageProvider* m_storage;
    int m_totalStudents = 0;
    int m_votesCast = 0;
    QString m_currentElection;
    Core::VotingState m_status = Core::VotingState::Idle;
};

} // namespace Ballot::ViewModels
