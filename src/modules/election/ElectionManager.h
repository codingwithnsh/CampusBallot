#pragma once

#include <QObject>
#include <QTimer>
#include <optional>
#include "src/core/Models.h"

namespace Ballot::Election {

/**
 * @brief The ElectionManager class is a singleton responsible for managing the lifecycle
 * and state of elections within the application.
 *
 * This class handles:
 * - Creation, updating, and deletion of election configurations.
 * - Starting, ending, and pausing elections.
 * - Providing access to election and candidate data.
 * - Monitoring election timers to automatically change election states.
 *
 * @note This class currently also includes methods for candidate management. In a future
 * refactoring, these might be moved to a dedicated CandidateManager for better separation
 * of concerns. Voting actions are delegated to VoteManager.
 */
class ElectionManager : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Returns the singleton instance of the ElectionManager.
     * @return Reference to the ElectionManager instance.
     */
    static ElectionManager& instance();

    /**
     * @brief Destructor.
     */
    ~ElectionManager() override = default;

    /**
     * @brief Creates a new election.
     * @param election The Core::Election object to create.
     * @return True if the election was created successfully, false otherwise.
     */
    bool createElection(const Core::Election& election);

    /**
     * @brief Updates an existing election.
     * @param election The Core::Election object with updated data.
     * @return True if the election was updated successfully, false otherwise.
     */
    bool updateElection(const Core::Election& election);

    /**
     * @brief Deletes an election by its ID.
     * @param id The ID of the election to delete.
     * @return True if the election was deleted successfully, false otherwise.
     */
    bool deleteElection(const QString& id);

    /**
     * @brief Starts an election, changing its state to Core::VotingState::Voting.
     * @param id The ID of the election to start.
     * @return True if the election was started successfully, false otherwise.
     */
    bool startElection(const QString& id);

    /**
     * @brief Ends an election, changing its state to Core::VotingState::Ended.
     * @param id The ID of the election to end.
     * @return True if the election was ended successfully, false otherwise.
     */
    bool endElection(const QString& id);

    /**
     * @brief Pauses an election, changing its state to Core::VotingState::Paused.
     * @param id The ID of the election to pause.
     * @return True if the election was paused successfully, false otherwise.
     */
    bool pauseElection(const QString& id);

    /**
     * @brief Retrieves an election by its ID.
     * @param id The ID of the election.
     * @return An optional containing the Core::Election if found, std::nullopt otherwise.
     */
    std::optional<Core::Election> getElection(const QString& id) const;

    /**
     * @brief Retrieves the currently active election (if any).
     * @return An optional containing the active Core::Election if found, std::nullopt otherwise.
     */
    std::optional<Core::Election> getActiveElection() const;

    /**
     * @brief Retrieves all elections.
     * @return A list of all Core::Election objects.
     */
    QList<Core::Election> getElections() const;

    /**
     * @brief Retrieves all active elections.
     * @return A list of all active Core::Election objects.
     */
    QList<Core::Election> getActiveElections() const;

    /**
     * @brief Adds a candidate to an election.
     * @param candidate The Core::Candidate object to add.
     * @return True if the candidate was added successfully, false otherwise.
     */
    bool addCandidate(const Core::Candidate& candidate);

    /**
     * @brief Updates an existing candidate.
     * @param candidate The Core::Candidate object with updated data.
     * @return True if the candidate was updated successfully, false otherwise.
     */
    bool updateCandidate(const Core::Candidate& candidate);

    /**
     * @brief Deletes a candidate by its ID.
     * @param id The ID of the candidate to delete.
     * @return True if the candidate was deleted successfully, false otherwise.
     */
    bool deleteCandidate(const QString& id);

    /**
     * @brief Retrieves all candidates for a given election.
     * @param electionId The ID of the election.
     * @return A list of Core::Candidate objects.
     */
    QList<Core::Candidate> getCandidates(const QString& electionId) const;

    /**
     * @brief Retrieves the current voting state of an election.
     * @param electionId The ID of the election.
     * @return The Core::VotingState of the election.
     */
    Core::VotingState getElectionState(const QString& electionId) const;

    // Removed: bool canVote(const QString& studentId, const QString& electionId) const;
    // Removed: bool castVote(const QString& electionId, const QString& studentId, const QString& candidateId);

signals:
    /**
     * @brief Emitted when a new election is created.
     * @param id The ID of the created election.
     */
    void electionCreated(const QString& id);

    /**
     * @brief Emitted when an election is updated.
     * @param id The ID of the updated election.
     */
    void electionUpdated(const QString& id);

    /**
     * @brief Emitted when an election is deleted.
     * @param id The ID of the deleted election.
     */
    void electionDeleted(const QString& id);

    /**
     * @brief Emitted when an election starts.
     * @param id The ID of the started election.
     */
    void electionStarted(const QString& id);

    /**
     * @brief Emitted when an election ends.
     * @param id The ID of the ended election.
     */
    void electionEnded(const QString& id);

    /**
     * @brief Emitted when an election's state changes.
     * @param id The ID of the election.
     * @param state The new voting state.
     */
    void electionStateChanged(const QString& id, Core::VotingState state);

    // Removed: void voteCast(const QString& electionId, const QString& studentId, const QString& candidateId);

private:
    /**
     * @brief Private constructor to enforce singleton pattern.
     */
    ElectionManager();

    /**
     * @brief Checks the status of all elections and updates their states based on timers.
     */
    void checkElectionTimers();

    QTimer* m_timer; ///< Timer for periodically checking election states.

    // Private copy constructor and assignment operator to prevent copying
    ElectionManager(const ElectionManager&) = delete;
    ElectionManager& operator=(const ElectionManager&) = delete;
};

} // namespace Ballot::Election