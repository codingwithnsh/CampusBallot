#pragma once

#include <QObject>
#include "src/core/Models.h"

namespace Ballot::Election {

class VoteManager : public QObject {
    Q_OBJECT
public:
    static VoteManager& instance();

    bool castVote(const QString& electionId, const QString& studentId, const QString& candidateId);
    bool hasStudentVoted(const QString& studentId, const QString& electionId) const;
    int getVoteCount(const QString& electionId) const;
    int getTotalVoters(const QString& electionId) const;

    QList<Core::ElectionResult> getResults(const QString& electionId) const;
    QList<Core::ElectionResult> getResultsByClass(const QString& electionId, const QString& className) const;
    QList<Core::ElectionResult> getResultsByDepartment(const QString& electionId, const QString& department) const;
    QList<Core::ElectionResult> getResultsByGender(const QString& electionId, const QString& gender) const;

    double getTurnout(const QString& electionId) const;

signals:
    void voteCast(const QString& electionId, const QString& studentId);
    void duplicateVoteAttempt(const QString& studentId, const QString& electionId);
    void resultsUpdated(const QString& electionId);

private:
    VoteManager();
    QByteArray encryptVote(const QString& candidateId) const;
    QString decryptVote(const QByteArray& encrypted) const;
};

} // namespace Ballot::Election
