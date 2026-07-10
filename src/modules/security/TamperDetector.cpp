#include "TamperDetector.h"
#include "HashProvider.h"
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QTimer>
#include <QCryptographicHash>
#include <QProcess>
#include <QDebug>
#include <stdexcept> // For std::runtime_error
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace Ballot::Security {

/**
 * @brief Private implementation class for TamperDetector.
 * This uses the PIMPL idiom to hide implementation details and reduce compilation dependencies.
 */
class TamperDetector::Impl {
public:
    QTimer* monitorTimer = nullptr; ///< Timer for periodic integrity checks.
    QString monitoredDir; ///< The directory currently being monitored.
    QHash<QString, QByteArray> fileChecksums; ///< Stored checksums of monitored files.
    bool monitoring = false; ///< Flag indicating if monitoring is active.

    /**
     * @brief Performs a periodic check of monitored files for tampering.
     * @param q Pointer to the public TamperDetector instance to emit signals.
     */
    void checkFiles(TamperDetector* q) {
        if (!monitoring) return; // Ensure monitoring is still active

        qDebug() << "TamperDetector: Performing periodic file integrity check in" << monitoredDir;
        QDirIterator it(monitoredDir, QDir::Files, QDirIterator::Subdirectories);
        bool allOk = true;
        while (it.hasNext()) {
            QString path = it.next();
            if (fileChecksums.contains(path)) {
                QByteArray current = calculateChecksum(path);
                if (current.isEmpty()) {
                    qWarning() << "TamperDetector: Could not calculate checksum for" << path << ". Skipping.";
                    continue;
                }
                if (current != fileChecksums[path]) {
                    qCritical() << "TamperDetector: Tamper detected! File modified:" << path;
                    emit q->tamperDetected(QString("File modified: %1").arg(path));
                    allOk = false;
                    // Optionally, stop monitoring immediately on first detection
                    // q->stopIntegrityMonitoring();
                    // break;
                }
            } else {
                // New file detected in monitored directory
                qWarning() << "TamperDetector: New file detected in monitored directory:" << path << ". Consider updating baseline.";
                // Depending on policy, this could also be a tamper detection
            }
        }
        if (allOk) {
            qDebug() << "TamperDetector: File integrity check passed.";
            emit q->integrityCheckPassed();
        } else {
            qCritical() << "TamperDetector: File integrity check FAILED.";
        }
    }

    /**
     * @brief Calculates the SHA256 checksum of a file.
     * @param path The path to the file.
     * @return The SHA256 checksum as a QByteArray, or empty if file cannot be read.
     */
    QByteArray calculateChecksum(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical() << "TamperDetector: Failed to open file for checksum calculation:" << path << "-" << file.errorString();
            return {};
        }
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (!hash.addData(&file)) {
            qCritical() << "TamperDetector: Failed to add data to hash for file:" << path;
            file.close();
            return {};
        }
        file.close();
        return hash.result();
    }
};

TamperDetector::TamperDetector(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>()) {
    qDebug() << "TamperDetector: Instance created.";
}

TamperDetector::~TamperDetector() {
    stopIntegrityMonitoring();
    qDebug() << "TamperDetector: Instance destroyed.";
}

/**
 * @brief Verifies the integrity of the SQLite database file using a stored SHA256 checksum.
 * @param dbPath The path to the database file.
 * @return True if the database file exists and its checksum matches the stored one, false otherwise.
 * @note This assumes a .sha256 file exists alongside the database with its checksum.
 */
bool TamperDetector::verifyDatabaseIntegrity(const QString& dbPath) {
    qDebug() << "TamperDetector: Verifying database integrity for" << dbPath;
    QFile file(dbPath);
    if (!file.exists()) {
        qWarning() << "TamperDetector: Database file does not exist:" << dbPath;
        return false;
    }

    QByteArray currentChecksum = d->calculateChecksum(dbPath);
    if (currentChecksum.isEmpty()) {
        qCritical() << "TamperDetector: Failed to calculate checksum for database:" << dbPath;
        return false;
    }

    QFile integrityFile(dbPath + ".sha256");
    if (!integrityFile.exists()) {
        qWarning() << "TamperDetector: Database integrity checksum file not found:" << dbPath + ".sha256";
        // Depending on policy, this might be an initial run, or a tamper.
        // For now, if no checksum file, we can't verify, so return false.
        return false;
    }

    if (!integrityFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "TamperDetector: Failed to open integrity checksum file:" << integrityFile.fileName() << "-" << integrityFile.errorString();
        return false;
    }
    QByteArray storedChecksum = integrityFile.readAll().trimmed();
    integrityFile.close();

    if (storedChecksum == currentChecksum.toHex()) { // Compare hex representations
        qInfo() << "TamperDetector: Database integrity verified for" << dbPath;
        return true;
    } else {
        qCritical() << "TamperDetector: Database integrity check FAILED for" << dbPath << ". Stored:" << storedChecksum.toHex() << ", Current:" << currentChecksum.toHex();
        emit tamperDetected(QString("Database file modified: %1").arg(dbPath));
        return false;
    }
}

/**
 * @brief Verifies the integrity of a single file against an expected checksum.
 * @param filePath The path to the file.
 * @param expectedChecksum The expected SHA256 checksum (raw bytes).
 * @return True if the file's checksum matches the expected one, false otherwise.
 */
bool TamperDetector::verifyFileIntegrity(const QString& filePath, const QByteArray& expectedChecksum) {
    qDebug() << "TamperDetector: Verifying file integrity for" << filePath;
    QByteArray actual = d->calculateChecksum(filePath);
    if (actual.isEmpty()) {
        qCritical() << "TamperDetector: Failed to calculate checksum for file:" << filePath;
        return false;
    }
    if (actual == expectedChecksum) {
        qDebug() << "TamperDetector: File integrity verified for" << filePath;
        return true;
    } else {
        qWarning() << "TamperDetector: File integrity check FAILED for" << filePath;
        emit tamperDetected(QString("File integrity mismatch: %1").arg(filePath));
        return false;
    }
}

/**
 * @brief Verifies a chain of checksums, typically for audit logs.
 * This method is a placeholder and needs a more robust implementation for true chain verification.
 * @param logFiles A list of file paths representing the log chain.
 * @return True if the chain appears intact, false otherwise.
 * @warning This implementation is simplistic and needs to be aligned with AuditManager's chain hashing logic.
 */
bool TamperDetector::verifyChecksumChain(const QStringList& logFiles) {
    qWarning() << "TamperDetector: verifyChecksumChain is a simplistic placeholder and needs robust implementation aligned with AuditManager's chain hashing.";
    QByteArray previousHash;
    for (const QString& file : logFiles) {
        QByteArray currentHash = d->calculateChecksum(file);
        if (currentHash.isEmpty()) {
            qCritical() << "TamperDetector: Failed to calculate checksum for log file:" << file;
            return false;
        }
        // This logic is flawed for a true chain. It should check if the *content* of the current file
        // contains the *hash of the previous file's content*.
        // The current implementation checks if the current file's content contains the *hash of the previous file itself*.
        // This needs to be re-evaluated based on how the audit log chain is actually constructed.
        if (!previousHash.isEmpty()) {
            // Placeholder logic: This is not how a true hash chain works.
            // A true chain would involve the current log entry's hash being calculated
            // using the previous log entry's hash as part of its input.
            // The AuditManager::verifyLogIntegrity is the correct place for this.
            qDebug() << "TamperDetector: Skipping detailed chain verification in this placeholder method.";
        }
        previousHash = currentHash;
    }
    qInfo() << "TamperDetector: Simplistic checksum chain verification completed.";
    return true;
}

/**
 * @brief Calculates the SHA256 checksum of a file.
 * @param filePath The path to the file.
 * @return The SHA256 checksum as a QByteArray, or empty if file cannot be read.
 */
QByteArray TamperDetector::calculateChecksum(const QString& filePath) {
    return d->calculateChecksum(filePath);
}

/**
 * @brief Starts periodic integrity monitoring of a specified directory.
 * @param directory The absolute path to the directory to monitor.
 * @return True if monitoring started successfully, false if already monitoring or directory is invalid.
 */
bool TamperDetector::startIntegrityMonitoring(const QString& directory) {
    if (d->monitoring) {
        qWarning() << "TamperDetector: Integrity monitoring already active.";
        return false;
    }
    if (!QDir(directory).exists()) {
        qCritical() << "TamperDetector: Monitored directory does not exist:" << directory;
        return false;
    }

    d->monitoredDir = directory;
    d->fileChecksums.clear();

    qInfo() << "TamperDetector: Starting integrity monitoring for directory:" << directory;
    QDirIterator it(directory, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString path = it.next();
        QByteArray checksum = d->calculateChecksum(path);
        if (!checksum.isEmpty()) {
            d->fileChecksums[path] = checksum;
            qDebug() << "TamperDetector: Baseline checksum for" << path << ":" << checksum.toHex();
        }
    }

    d->monitorTimer = new QTimer(this);
    connect(d->monitorTimer, &QTimer::timeout, this, [this]() {
        d->checkFiles(this);
    });
    d->monitorTimer->start(30000); // Check every 30 seconds
    d->monitoring = true;
    qInfo() << "TamperDetector: Integrity monitoring started successfully.";
    return true;
}

/**
 * @brief Stops the periodic integrity monitoring.
 */
void TamperDetector::stopIntegrityMonitoring() {
    if (d->monitorTimer) {
        d->monitorTimer->stop();
        delete d->monitorTimer;
        d->monitorTimer = nullptr;
        qInfo() << "TamperDetector: Integrity monitoring stopped.";
    }
    d->monitoring = false;
    d->fileChecksums.clear();
}

/**
 * @brief Checks if integrity monitoring is currently active.
 * @return True if monitoring is active, false otherwise.
 */
bool TamperDetector::isMonitoring() const {
    return d->monitoring;
}

/**
 * @brief Attempts to detect if a debugger is attached to the process.
 * @return True if a debugger is detected, false otherwise.
 */
bool TamperDetector::detectDebugger() {
    qDebug() << "TamperDetector: Attempting to detect debugger.";
#ifdef Q_OS_WIN
    BOOL debuggerPresent = IsDebuggerPresent();
    if (debuggerPresent) {
        qWarning() << "TamperDetector: Debugger detected!";
    }
    return debuggerPresent;
#else
    // On Linux, check /proc/self/status for TracerPid
    QFile status("/proc/self/status");
    if (status.open(QIODevice::ReadOnly)) {
        QByteArray content = status.readAll();
        status.close();
        if (content.contains("TracerPid:\t0")) {
            qDebug() << "TamperDetector: No debugger (TracerPid: 0).";
            return false;
        }
        qWarning() << "TamperDetector: Debugger detected (TracerPid != 0)!";
        return true;
    }
    qWarning() << "TamperDetector: Could not read /proc/self/status to detect debugger.";
    return false; // Cannot determine
#endif
}

/**
 * @brief Attempts to detect if the application is running in a sandbox or virtual machine environment.
 * @return True if a sandbox/VM is detected, false otherwise.
 * @note This is a heuristic-based detection and may not be foolproof.
 */
bool TamperDetector::detectSandbox() {
    qDebug() << "TamperDetector: Attempting to detect sandbox/VM.";
    QProcess proc;
    proc.start("systeminfo"); // Windows command
    proc.waitForFinished(3000); // Wait up to 3 seconds
    QString output = proc.readAllStandardOutput();

    if (output.isEmpty()) {
        // Try on Linux/macOS
        proc.start("dmidecode -s system-manufacturer");
        proc.waitForFinished(3000);
        output = proc.readAllStandardOutput();
        if (output.isEmpty()) {
            proc.start("sysctl -n hw.model"); // macOS
            proc.waitForFinished(3000);
            output = proc.readAllStandardOutput();
        }
    }

    if (output.contains("VirtualBox", Qt::CaseInsensitive) ||
        output.contains("VMware", Qt::CaseInsensitive) ||
        output.contains("Hyper-V", Qt::CaseInsensitive) ||
        output.contains("VBox", Qt::CaseInsensitive) ||
        output.contains("Parallels", Qt::CaseInsensitive) ||
        output.contains("QEMU", Qt::CaseInsensitive)) {
        qWarning() << "TamperDetector: Sandbox/Virtual Machine detected!";
        return true;
    }
    qDebug() << "TamperDetector: No obvious sandbox/VM detected.";
    return false;
}

/**
 * @brief Alias for detectSandbox().
 * @return True if a virtual machine is detected, false otherwise.
 */
bool TamperDetector::detectVirtualMachine() {
    return detectSandbox();
}

} // namespace Ballot::Security