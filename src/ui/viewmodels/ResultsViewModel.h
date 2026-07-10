#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <optional> // For std::optional
#include "src/core/Models.h"
// #include "src/modules/storage/interfaces/IStorageProvider.h" // Removed direct dependency

namespace Ballot::ViewModels {

/**
 * @brief The ResultsViewModel class is responsible for providing election results data
 * to the UI and handling result-related actions like export.
 *
 * It aggregates results from the VoteManager and ElectionManager, and calculates turnout.
 */
class ResultsViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool hasResults READ hasResults NOTIFY resultsChanged)
    Q_PROPERTY(double turnout READ turnout NOTIFY resultsChanged)
    Q_PROPERTY(QString currentElectionTitle READ currentElectionTitle NOTIFY resultsChanged)
    Q_PROPERTY(int totalVotes READ totalVotes NOTIFY resultsChanged)

public:
    /**
     * @brief Constructs a ResultsViewModel.
     * @param parent The parent QObject.
     */
    explicit ResultsViewModel(QObject *parent = nullptr); // Removed direct IStorageProvider dependency

    /**
     * @brief Checks if there are any results to display.
     * @return True if results are available, false otherwise.
     */
    bool hasResults() const { return !m_results.isEmpty(); }

    /**
     * @brief Returns the voter turnout percentage for the current election.
     */
    double turnout() const { return m_turnout; }

    /**
     * @brief Returns the list of election results.
     */
    QList<Core::ElectionResult> results() const { return m_results; }

    /**
     * @brief Returns the current active election object.
     */
    std::optional<Core::Election> currentElection() const { return m_election; } // Changed to optional

    /**
     * @brief Returns the ID of the current active election.
     */
    QString currentElectionId() const;

    /**
     * @brief Returns the total number of votes cast in the current election.
     */
    int totalVotes() const { return m_totalVotes; }

    /**
     * @brief Refreshes the election results data.
     */
    void refresh();

    /**
     * @brief Sets the election for which results should be displayed.
     * @param electionId The ID of the election.
     */
    void setElection(const QString& electionId);

    /**
     * @brief Retrieves a list of all available elections.
     * @return A list of Core::Election objects.
     */
    QList<Core::Election> getElections() const;

    /**
     * @brief Exports the current election results to a file.
     * @param filePath The path to save the exported file.
     * @param format The export format (e.g., "pdf", "csv", "excel").
     * @return True if export is successful, false otherwise.
     */
    bool exportResults(const QString& filePath, const QString& format = "pdf");

    /**
     * @brief Returns the title of the current election.
     */
    QString currentElectionTitle() const;

signals:
    /**
     * @brief Emitted when the results data changes.
     */
    void resultsChanged();

    /**
     * @brief Emitted when the export operation is completed.
     * @param path The path to the exported file.
     */
    void exportCompleted(const QString& path);

    /**
     * @brief Emitted when an error occurs during results processing or export.
     * @param error A description of the error.
     */
    void errorOccurred(const QString& error);

private:
    // Removed direct IStorageProvider pointer. Access managers via singletons.
    QList<Core::ElectionResult> m_results; ///< List of election results.
    std::optional<Core::Election> m_election; ///< The currently selected election.
    double m_turnout = 0.0; ///< Voter turnout percentage.
    int m_totalVotes = 0; ///< Total votes cast.
};

} // namespace Ballot::ViewModels