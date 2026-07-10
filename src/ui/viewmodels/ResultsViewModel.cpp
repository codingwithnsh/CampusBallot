#include "ResultsViewModel.h"
#include "src/core/SystemManager.h"
#include "src/modules/election/ElectionManager.h"
#include "src/modules/election/VoteManager.h"
#include "src/modules/audit/AuditManager.h" // For audit logging
#include "src/modules/auth/AuthManager.h"
#include "src/modules/auth/RBACManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTextStream>
#include <QTextDocument> // For PDF export
#include <QPrinter>      // For PDF export
#include <QDebug>        // For logging

namespace Ballot::ViewModels {

namespace {

QString escapeCsvCell(QString value) {
    if (!value.isEmpty() && QStringLiteral("=+-@").contains(value.front())) {
        value.prepend('\'');
    }
    value.replace("\"", "\"\"");
    return "\"" + value + "\"";
}

QString htmlEscaped(const QString& value) {
    return value.toHtmlEscaped();
}

} // namespace

ResultsViewModel::ResultsViewModel(QObject *parent)
    : QObject(parent) {
    qDebug() << "ResultsViewModel: Initializing...";
    // Connect to signals that might affect results
    connect(&Election::VoteManager::instance(), &Election::VoteManager::resultsUpdated, this, &ResultsViewModel::refresh);
    connect(&Election::ElectionManager::instance(), &Election::ElectionManager::electionUpdated, this, &ResultsViewModel::refresh);
    connect(&Election::ElectionManager::instance(), &Election::ElectionManager::electionDeleted, this, &ResultsViewModel::refresh);
    qDebug() << "ResultsViewModel: Initialized.";
}

/**
 * @brief Returns the ID of the current active election.
 * @return The ID of the current election, or an empty string if none is selected.
 */
QString ResultsViewModel::currentElectionId() const {
    if (m_election.has_value()) {
        return m_election->id;
    }
    return QString();
}

/**
 * @brief Returns the title of the current election.
 * @return The title of the current election, or an empty string if none is selected.
 */
QString ResultsViewModel::currentElectionTitle() const {
    if (m_election.has_value()) {
        return m_election->title;
    }
    return "No Election Selected";
}

/**
 * @brief Refreshes the election results data for the currently selected election.
 * If no election is selected, it attempts to select the first available election.
 */
void ResultsViewModel::refresh() {
    qDebug() << "ResultsViewModel: Refreshing data...";
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage || !storage->isConnected()) {
        qCritical() << "ResultsViewModel: Storage not available during refresh.";
        emit errorOccurred("Storage not available.");
        return;
    }

    QList<Core::Election> elections = Election::ElectionManager::instance().getElections();

    // Try to re-select the current election if it still exists
    bool currentElectionFound = false;
    if (m_election.has_value()) {
        for (const auto& e : elections) {
            if (e.id == m_election->id) {
                setElection(m_election->id); // Re-set to refresh data
                currentElectionFound = true;
                break;
            }
        }
    }

    // If no current election or it's no longer valid, try to select the first one
    if (!currentElectionFound && !elections.isEmpty()) {
        qDebug() << "ResultsViewModel: No current election or invalid. Selecting first available election.";
        setElection(elections.first().id);
    } else if (elections.isEmpty()) {
        // No elections available, clear current results
        qDebug() << "ResultsViewModel: No elections available. Clearing results.";
        m_results.clear();
        m_election = std::nullopt; // Reset optional
        m_turnout = 0.0;
        m_totalVotes = 0;
        emit resultsChanged();
    } else {
        // If an election was already selected and found, just re-emit resultsChanged
        // as setElection would have already done the work.
        qDebug() << "ResultsViewModel: Current election" << currentElectionId() << "still selected. Emitting resultsChanged.";
        emit resultsChanged();
    }
    qDebug() << "ResultsViewModel: Refresh complete.";
}

/**
 * @brief Sets the election for which results should be displayed.
 * @param electionId The ID of the election.
 */
void ResultsViewModel::setElection(const QString& electionId) {
    qInfo() << "ResultsViewModel: Setting election to ID:" << electionId;
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage || !storage->isConnected()) {
        qCritical() << "ResultsViewModel: Storage not available. Cannot set election.";
        emit errorOccurred("Storage not available.");
        return;
    }

    if (electionId.isEmpty()) {
        qDebug() << "ResultsViewModel: Election ID is empty. Clearing results.";
        if (!m_election.has_value() && m_results.isEmpty() && m_turnout == 0.0 && m_totalVotes == 0) {
            return;
        }
        m_results.clear();
        m_election = std::nullopt;
        m_turnout = 0.0;
        m_totalVotes = 0;
        emit resultsChanged();
        return;
    }

    std::optional<Core::Election> electionOpt = Election::ElectionManager::instance().getElection(electionId);
    if (electionOpt) {
        m_election = *electionOpt;
        m_results = Election::VoteManager::instance().getResults(electionId);
        m_totalVotes = Election::VoteManager::instance().getVoteCount(electionId);
        m_turnout = Election::VoteManager::instance().getTurnout(electionId);
        qInfo() << "ResultsViewModel: Election" << m_election->title << "selected. Total votes:" << m_totalVotes << ", Turnout:" << m_turnout << "%";
        emit resultsChanged();
    } else {
        qWarning() << "ResultsViewModel: Election with ID" << electionId << "not found.";
        // Election not found, clear current results
        m_results.clear();
        m_election = std::nullopt;
        m_turnout = 0.0;
        m_totalVotes = 0;
        emit resultsChanged();
        emit errorOccurred("Selected election not found.");
    }
}

/**
 * @brief Retrieves a list of all available elections.
 * @return A list of all Core::Election objects.
 */
QList<Core::Election> ResultsViewModel::getElections() const {
    return Election::ElectionManager::instance().getElections();
}

/**
 * @brief Exports the current election results to a file in the specified format.
 * @param filePath The path to save the exported file.
 * @param format The export format (e.g., "json", "csv", "pdf").
 * @return True if export is successful, false otherwise.
 */
bool ResultsViewModel::exportResults(const QString& filePath, const QString& format) {
    qInfo() << "ResultsViewModel: Exporting results to" << filePath << "in" << format << "format.";
    if (!Auth::AuthManager::instance().hasPermission(Auth::RBACManager::PERM_RESULTS_EXPORT)) {
        qWarning() << "ResultsViewModel: Results export denied by RBAC.";
        Audit::AuditManager::instance().log(Core::AuditAction::PermissionDenied, QString("Denied results export to %1 as %2.").arg(filePath, format), Auth::AuthManager::instance().currentUserId());
        emit errorOccurred("You do not have permission to export results.");
        return false;
    }

    if (m_results.isEmpty()) {
        qWarning() << "ResultsViewModel: No results to export.";
        emit errorOccurred("No results to export.");
        return false;
    }
    if (!m_election.has_value()) {
        qWarning() << "ResultsViewModel: No election selected for export.";
        emit errorOccurred("No election selected for export.");
        return false;
    }

    bool success = false;
    if (format.toLower() == "json") {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            qCritical() << "ResultsViewModel: Cannot open file for writing JSON:" << filePath << "-" << file.errorString();
            emit errorOccurred("Cannot open file for writing: " + file.errorString());
            return false;
        }
        QJsonArray arr;
        for (const auto& r : m_results) {
            arr.append(r.toJson()); // Use toJson from Core::ElectionResult
        }
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        file.close();
        success = true;
    } else if (format.toLower() == "csv") {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            qCritical() << "ResultsViewModel: Cannot open file for writing CSV:" << filePath << "-" << file.errorString();
            emit errorOccurred("Cannot open file for writing: " + file.errorString());
            return false;
        }
        QTextStream out(&file);
        out << "Candidate,Party,Votes,Percentage\n";
        for (const auto& r : m_results) {
            out << escapeCsvCell(r.candidateName) << ","
                << escapeCsvCell(r.party) << ","
                << r.voteCount << ","
                << escapeCsvCell(QString::number(r.percentage, 'f', 1) + "%") << "\n";
        }
        file.close();
        success = true;
    } else if (format.toLower() == "pdf") {
        QTextDocument document;
        QString htmlContent = "<h1>Election Results for " + htmlEscaped(m_election->title) + "</h1>";
        htmlContent += "<p>Total Votes: " + QString::number(m_totalVotes) + "</p>";
        htmlContent += "<p>Voter Turnout: " + QString::number(m_turnout, 'f', 1) + "%</p>";
        htmlContent += "<table border='1' cellpadding='5' cellspacing='0'>";
        htmlContent += "<thead><tr><th>Candidate</th><th>Party</th><th>Votes</th><th>Percentage</th></tr></thead>";
        htmlContent += "<tbody>";
        for (const auto& r : m_results) {
            htmlContent += "<tr><td>" + htmlEscaped(r.candidateName) + "</td><td>" + htmlEscaped(r.party) + "</td><td>" + QString::number(r.voteCount) + "</td><td>" + QString::number(r.percentage, 'f', 1) + "%</td></tr>";
        }
        htmlContent += "</tbody></table>";
        document.setHtml(htmlContent);

        QPrinter printer(QPrinter::PrinterResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filePath);
        document.print(&printer);
        success = true;
    } else {
        qWarning() << "ResultsViewModel: Unsupported export format:" << format;
        emit errorOccurred("Unsupported export format: " + format);
        return false;
    }

    if (success) {
        qInfo() << "ResultsViewModel: Results exported successfully to" << filePath;
        Audit::AuditManager::instance().log(Core::AuditAction::LogsExported, QString("Election results exported for %1 to %2").arg(m_election->title, filePath), "System");
        emit exportCompleted(filePath);
    }
    return success;
}

} // namespace Ballot::ViewModels
