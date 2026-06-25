#pragma once

#include <QObject>
#include <QTimer>
#include "src/core/Models.h"

namespace Ballot::Election {

class ElectionManager : public QObject {
    Q_OBJECT
public:
    static ElectionManager& instance();

    bool createElection(const Core::Election& election);
    bool updateElection(const Core::Election& election);
    bool deleteElection(const QString& id);
    bool startElection(const QString& id);
    bool endElection(const QString& id);
    bool pauseElection(const QString& id);

    Core::Election getElection(const QString& id) const;
    QList<Core::Election> getElections() const;
    QList<Core::Election> getActiveElections() const;

    bool addCandidate(const Core::Candidate& candidate);
    bool updateCandidate(const Core::Candidate& candidate);
    bool deleteCandidate(const QString& id);
    QList<Core::Candidate> getCandidates(const QString& electionId) const;

    Core::VotingState getElectionState(const QString& electionId) const;
    bool canVote(const QString& studentId, const QString& electionId) const;

signals:
    void electionCreated(const QString& id);
    void electionUpdated(const QString& id);
    void electionDeleted(const QString& id);
    void electionStarted(const QString& id);
    void electionEnded(const QString& id);
    void electionStateChanged(const QString& id, Core::VotingState state);

private:
    ElectionManager();
    void checkElectionTimers();

    QTimer* m_timer;
};

} // namespace Ballot::Election
