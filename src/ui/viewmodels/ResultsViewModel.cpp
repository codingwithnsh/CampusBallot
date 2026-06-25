#include "ResultsViewModel.h"
#include "src/modules/election/VoteManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTextStream>

namespace Ballot::ViewModels {

ResultsViewModel::ResultsViewModel(Core::IStorageProvider* storage, QObject *parent)
    : QObject(parent), m_storage(storage) {}

void ResultsViewModel::refresh() {
    if (!m_storage) return;
    auto elections = m_storage->getElections();
    if (!elections.isEmpty()) {
        setElection(elections.first().id);
    }
}

void ResultsViewModel::setElection(const QString& electionId) {
    if (!m_storage) return;

    auto election = m_storage->getElection(electionId);
    if (election) {
        m_election = *election;
        m_results = m_storage->getResults(electionId);
        m_totalVotes = m_storage->getTotalVotesCast(electionId);
        int totalVoters = m_storage->getVoterCount(electionId);
        m_turnout = totalVoters > 0 ? (static_cast<double>(m_totalVotes) / totalVoters) * 100.0 : 0.0;
        emit resultsChanged();
    }
}

QList<Core::Election> ResultsViewModel::getElections() const {
    if (!m_storage) return {};
    return m_storage->getElections();
}

bool ResultsViewModel::exportResults(const QString& filePath, const QString& format) {
    if (m_results.isEmpty()) {
        emit exportError("No results to export");
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit exportError("Cannot open file for writing");
        return false;
    }

    if (format == "json") {
        QJsonArray arr;
        for (const auto& r : m_results) {
            QJsonObject o;
            o["candidate"] = r.candidateName;
            o["party"] = r.party;
            o["votes"] = r.voteCount;
            o["percentage"] = r.percentage;
            arr.append(o);
        }
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    } else if (format == "csv") {
        QTextStream out(&file);
        out << "Candidate,Party,Votes,Percentage\n";
        for (const auto& r : m_results) {
            out << "\"" << r.candidateName << "\",\""
                << r.party << "\","
                << r.voteCount << ","
                << QString::number(r.percentage, 'f', 1) << "%\n";
        }
    }

    file.close();
    emit exportCompleted(filePath);
    return true;
}

} // namespace Ballot::ViewModels
