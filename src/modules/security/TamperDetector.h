#pragma once

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QDateTime>
#include <QObject>
#include <memory> // For std::unique_ptr
#include <QDebug> // For logging

namespace Ballot::Security {

/**
 * @brief The TamperDetector class provides functionalities to detect tampering
 * with application files, databases, and runtime environment.
 *
 * It includes methods for:
 * - Verifying checksums of files and databases.
 * - Monitoring directories for file changes.
 * - Detecting debuggers and virtual machines.
 *
 * @note Some detection methods are heuristic and platform-dependent.
 */
class TamperDetector : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructs a TamperDetector object.
     * @param parent The parent QObject.
     */
    explicit TamperDetector(QObject* parent = nullptr);

    /**
     * @brief Destructor. Stops any active monitoring.
     */
    ~TamperDetector() override;

    /**
     * @brief Verifies the integrity of the SQLite database file using a stored SHA256 checksum.
     * @param dbPath The path to the database file.
     * @return True if the database file exists and its checksum matches the stored one, false otherwise.
     * @note This assumes a .sha256 file exists alongside the database with its checksum.
     */
    bool verifyDatabaseIntegrity(const QString& dbPath);

    /**
     * @brief Verifies the integrity of a single file against an expected checksum.
     * @param filePath The path to the file.
     * @param expectedChecksum The expected SHA256 checksum (raw bytes).
     * @return True if the file's checksum matches the expected one, false otherwise.
     */
    bool verifyFileIntegrity(const QString& filePath, const QByteArray& expectedChecksum);

    /**
     * @brief Verifies a chain of checksums, typically for audit logs.
     * @param logFiles A list of file paths representing the log chain.
     * @return True if the chain appears intact, false otherwise.
     * @warning This implementation is simplistic and needs to be aligned with AuditManager's chain hashing logic.
     */
    bool verifyChecksumChain(const QStringList& logFiles);

    /**
     * @brief Calculates the SHA256 checksum of a file.
     * @param filePath The path to the file.
     * @return The SHA256 checksum as a QByteArray, or empty if file cannot be read.
     */
    QByteArray calculateChecksum(const QString& filePath);

    /**
     * @brief Starts periodic integrity monitoring of a specified directory.
     * @param directory The absolute path to the directory to monitor.
     * @return True if monitoring started successfully, false if already monitoring or directory is invalid.
     */
    bool startIntegrityMonitoring(const QString& directory);

    /**
     * @brief Stops the periodic integrity monitoring.
     */
    void stopIntegrityMonitoring();

    /**
     * @brief Checks if integrity monitoring is currently active.
     * @return True if monitoring is active, false otherwise.
     */
    bool isMonitoring() const;

    /**
     * @brief Attempts to detect if a debugger is attached to the process.
     * @return True if a debugger is detected, false otherwise.
     */
    static bool detectDebugger();

    /**
     * @brief Attempts to detect if the application is running in a sandbox or virtual machine environment.
     * @return True if a sandbox/VM is detected, false otherwise.
     * @note This is a heuristic-based detection and may not be foolproof.
     */
    static bool detectSandbox();

    /**
     * @brief Alias for detectSandbox().
     * @return True if a virtual machine is detected, false otherwise.
     */
    static bool detectVirtualMachine();

signals:
    /**
     * @brief Emitted when tampering is detected.
     * @param details A string describing the detected tampering.
     */
    void tamperDetected(const QString& details);

    /**
     * @brief Emitted when an integrity check passes.
     */
    void integrityCheckPassed();

    /**
     * @brief Emitted when an integrity check fails.
     * @param reason A string describing the reason for failure.
     */
    void integrityCheckFailed(const QString& reason);

private:
    class Impl; ///< Private implementation class (PIMPL idiom).
    std::unique_ptr<Impl> d; ///< Pointer to the private implementation.
};

} // namespace Ballot::Security