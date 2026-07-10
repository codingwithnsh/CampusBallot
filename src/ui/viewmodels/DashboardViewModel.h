#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
// #include "src/modules/storage/interfaces/IStorageProvider.h" // Removed direct dependency
#include "src/core/Models.h" // Still needed for Core::VotingState and other models

namespace Ballot::ViewModels {

/**
 * @brief The DashboardViewModel class is responsible for providing data and status
 * updates to the main dashboard UI.
 *
 * This ViewModel aggregates information from various parts of the system, including:
 * - Election statistics (total students, votes cast, turnout).
 * - System health indicators (master role, voting status, database status, storage type).
 * - Status of audit and backup systems.
 *
 * It also provides methods to trigger high-level actions like starting and ending voting.
 *
 * @note This class currently has a broad set of responsibilities. In future refactoring,
 * consider breaking it down or having it compose smaller, more focused view models
 * or services to adhere better to the Single Responsibility Principle.
 */
class DashboardViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int totalStudents READ totalStudents NOTIFY totalStudentsChanged)
    Q_PROPERTY(int votesCast READ votesCast NOTIFY votesCastChanged)
    Q_PROPERTY(double turnout READ turnout NOTIFY turnoutChanged)
    Q_PROPERTY(bool isMaster READ isMaster NOTIFY roleChanged)
    Q_PROPERTY(QString votingStatus READ votingStatus NOTIFY votingStatusChanged)
    Q_PROPERTY(QString currentElectionTitle READ currentElectionTitle NOTIFY currentElectionTitleChanged)
    Q_PROPERTY(QString dbStatus READ dbStatus NOTIFY dbStatusChanged)
    Q_PROPERTY(QString storageType READ storageType NOTIFY storageTypeChanged)
    Q_PROPERTY(QString serverStatus READ serverStatus NOTIFY serverStatusChanged)
    Q_PROPERTY(QString auditStatus READ auditStatus NOTIFY auditStatusChanged)
    Q_PROPERTY(QString backupStatus READ backupStatus NOTIFY backupStatusChanged)

public:
    /**
     * @brief Constructs a DashboardViewModel.
     * @param parent The parent QObject.
     */
    explicit DashboardViewModel(QObject *parent = nullptr); // Removed direct IStorageProvider dependency

    /**
     * @brief Returns the total number of registered students.
     */
    int totalStudents() const { return m_totalStudents; }

    /**
     * @brief Returns the total number of votes cast.
     */
    int votesCast() const { return m_votesCast; }

    /**
     * @brief Returns the number of students who have not yet voted.
     */
    int remainingStudents() const { return m_totalStudents - m_votesCast; }

    /**
     * @brief Calculates and returns the voter turnout percentage.
     */
    double turnout() const;

    /**
     * @brief Checks if the current machine is designated as the master.
     */
    bool isMaster() const;

    /**
     * @brief Returns the current voting status as a human-readable string.
     */
    QString votingStatus() const;

    /**
     * @brief Returns the title of the current active election.
     */
    QString currentElectionTitle() const { return m_currentElectionTitle; }

    /**
     * @brief Returns the status of the database connection.
     */
    QString dbStatus() const { return m_dbStatus; }

    /**
     * @brief Returns the type of storage provider being used.
     */
    QString storageType() const { return m_storageType; }

    /**
     * @brief Returns the status of the server connection (if applicable).
     */
    QString serverStatus() const { return m_serverStatus; }

    /**
     * @brief Returns the status of the audit system.
     */
    QString auditStatus() const { return m_auditStatus; }

    /**
     * @brief Returns the status of the backup system.
     */
    QString backupStatus() const { return m_backupStatus; }

    /**
     * @brief Refreshes all data and status properties.
     */
    void refresh();

    /**
     * @brief Initiates the start of the current active election.
     * @note This method should delegate to ElectionManager.
     */
    Q_INVOKABLE void startVoting(); // Q_INVOKABLE for QML/UI interaction

    /**
     * @brief Initiates the end of the current active election.
     * @note This method should delegate to ElectionManager.
     */
    Q_INVOKABLE void endVoting(); // Q_INVOKABLE for QML/UI interaction

signals:
    void totalStudentsChanged();
    void votesCastChanged();
    void turnoutChanged();
    void roleChanged();
    void votingStatusChanged();
    void currentElectionTitleChanged();
    void dbStatusChanged();
    void storageTypeChanged();
    void serverStatusChanged();
    void auditStatusChanged();
    void backupStatusChanged();
    void errorOccurred(const QString& error);

private:
    // Removed direct IStorageProvider pointer. Access managers via singletons.
    int m_totalStudents = 0;
    int m_votesCast = 0;
    QString m_currentElectionTitle;
    Core::VotingState m_status = Core::VotingState::Idle;

    QString m_dbStatus;
    QString m_storageType;
    QString m_serverStatus;
    QString m_auditStatus;
    QString m_backupStatus;

    QTimer m_refreshTimer; // Timer for periodic refresh
};

} // namespace Ballot::ViewModels