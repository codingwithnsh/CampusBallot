#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include "src/core/Models.h"
#include "src/modules/storage/interfaces/IStorageProvider.h"

namespace Ballot::ViewModels {

class ResultsViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool hasResults READ hasResults NOTIFY resultsChanged)
    Q_PROPERTY(double turnout READ turnout NOTIFY resultsChanged)

public:
    explicit ResultsViewModel(Core::IStorageProvider* storage, QObject *parent = nullptr);

    bool hasResults() const { return !m_results.isEmpty(); }
    double turnout() const { return m_turnout; }
    QList<Core::ElectionResult> results() const { return m_results; }
    Core::Election currentElection() const { return m_election; }
    QString currentElectionId() const;
    int totalVotes() const { return m_totalVotes; }

    void refresh();
    void setElection(const QString& electionId);
    QList<Core::Election> getElections() const;

    bool exportResults(const QString& filePath, const QString& format = "pdf");

signals:
    void resultsChanged();
    void exportCompleted(const QString& path);
    void errorOccurred(const QString& error);

private:
    Core::IStorageProvider* m_storage;
    QList<Core::ElectionResult> m_results;
    Core::Election m_election;
    double m_turnout = 0.0;
    int m_totalVotes = 0;
};

} // namespace Ballot::ViewModels
