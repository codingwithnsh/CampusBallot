#include "ResultsViewModel.h"
#include "src/modules/election/VoteManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTextStream>
#include <QTextDocument> // For PDF export
#include <QPrinter>      // For PDF export

namespace Ballot::ViewModels {

ResultsViewModel::ResultsViewModel(Core::IStorageProvider* storage, QObject *parent)
    : QObject(parent), m_storage(storage) {}

// Getter for currentElectionId for ResultsView to re-select combo box
QString ResultsViewModel::currentElectionId() const {
    return m_election.id;
}

void ResultsViewModel::refresh() {
    if (!m_storage) {
        emit errorOccurred("Storage not available.");
        return;
    }

    QList<Core::Election> elections = m_storage->getElections();

    // Try to re-select the current election if it still exists
    bool currentElectionFound = false;
    if (!m_election.id.isEmpty()) {
        for (const auto& e : elections) {
            if (e.id == m_election.id) {
                setElection(m_election.id); // Re-set to refresh data
                currentElectionFound = true;
                break;
            }
        }
    }

    // If no current election or it's no longer valid, try to select the first one
    if (!currentElectionFound && !elections.isEmpty()) {
        setElection(elections.first().id);
    } else if (elections.isEmpty()) {
        // No elections available, clear current results
        m_results.clear();
        m_election = Core::Election(); // Reset election
        m_turnout = 0.0;
        m_totalVotes = 0;
        emit resultsChanged();
    }
}

void ResultsViewModel::setElection(const QString& electionId) {
    if (!m_storage) {
        emit errorOccurred("Storage not available.");
        return;
    }

    if (electionId.isEmpty()) {
        m_results.clear();
        m_election = Core::Election();
        m_turnout = 0.0;
        m_totalVotes = 0;
        emit resultsChanged();
        return;
    }

    auto election = m_storage->getElection(electionId);
    if (election) {
        m_election = *election;
        m_results = m_storage->getResults(electionId);
        m_totalVotes = m_storage->getTotalVotesCast(electionId);
        int totalVoters = m_storage->getVoterCount(electionId); // Assuming getVoterCount returns total eligible voters
        m_turnout = totalVoters > 0 ? (static_cast<double>(m_totalVotes) / totalVoters) * 100.0 : 0.0;
        emit resultsChanged();
    } else {
        // Election not found, clear current results
        m_results.clear();
        m_election = Core::Election();
        m_turnout = 0.0;
        m_totalVotes = 0;
        emit resultsChanged();
        emit errorOccurred("Selected election not found.");
    }
}

QList<Core::Election> ResultsViewModel::getElections() const {
    if (!m_storage) return {};
    return m_storage->getElections();
}

bool ResultsViewModel::exportResults(const QString& filePath, const QString& format) {
    if (m_results.isEmpty()) {
        emit errorOccurred("No results to export.");
        return false;
    }

    if (format == "json") {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            emit errorOccurred("Cannot open file for writing: " + file.errorString());
            return false;
        }
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
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            emit errorOccurred("Cannot open file for writing: " + file.errorString());
            return false;
        }
        QTextStream out(&file);
        out << "Candidate,Party,Votes,Percentage\n";
        for (const auto& r : m_results) {
            out << "\"" << r.candidateName << "\",\""
                << r.party << "\","
                << r.voteCount << ","
                << QString::number(r.percentage, 'f', 1) << "%\n";
        }
    } else if (format == "pdf") {
        QTextDocument document;
        QString htmlContent = "<h1>Election Results for " + m_election.title + "</h1>";
        htmlContent += "<p>Total Votes: " + QString::number(m_totalVotes) + "</p>";
        htmlContent += "<p>Voter Turnout: " + QString::number(m_turnout, 'f', 1) + "%</p>";
        htmlContent += "<table border='1' cellpadding='5' cellspacing='0'>";
        htmlContent += "<thead><tr><th>Candidate</th><th>Party</th><th>Votes</th><th>Percentage</th></tr></thead>";
        htmlContent += "<tbody>";
        for (const auto& r : m_results) {
            htmlContent += "<tr><td>" + r.candidateName + "</td><td>" + r.party + "</td><td>" + QString::number(r.voteCount) + "</td><td>" + QString::number(r.percentage, 'f', 1) + "%</td></tr>";
        }
        htmlContent += "</tbody></table>";
        document.setHtml(htmlContent);

        QPrinter printer(QPrinter::PrinterResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filePath);
        document.print(&printer);
    } else {
        emit errorOccurred("Unsupported export format: " + format);
        return false;
    }

    emit exportCompleted(filePath);
    return true;
}

} // namespace Ballot::ViewModels
