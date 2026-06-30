#pragma once

#include <QObject>
#include <QTimer>
#include <optional> // Include for std::optional
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

    std::optional<Core::Election> getElection(const QString& id) const; // Changed return type
    std::optional<Core::Election> getActiveElection() const; // New method
    QList<Core::Election> getElections() const;
    QList<Core::Election> getActiveElections() const; // This method already exists, but its usage might need review

    bool addCandidate(const Core::Candidate& candidate);
    bool updateCandidate(const Core::Candidate& candidate);
    bool deleteCandidate(const QString& id);
    QList<Core::Candidate> getCandidates(const QString& electionId) const;

    Core::VotingState getElectionState(const QString& electionId) const;
    bool canVote(const QString& studentId, const QString& electionId) const;
    bool castVote(const QString& electionId, const QString& studentId, const QString& candidateId); // New method

signals:
    void electionCreated(const QString& id);
    void electionUpdated(const QString& id);
    void electionDeleted(const QString& id);
    void electionStarted(const QString& id);
    void electionEnded(const QString& id);
    void electionStateChanged(const QString& id, Core::VotingState state);
    void voteCast(const QString& electionId, const QString& studentId, const QString& candidateId); // New signal

private:
    ElectionManager();
    void checkElectionTimers();

    QTimer* m_timer;
};

} // namespace Ballot::Election