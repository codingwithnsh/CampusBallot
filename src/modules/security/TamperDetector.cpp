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
#include <QSet>
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
        QSet<QString> seenFiles;
        QDirIterator it(monitoredDir, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        bool allOk = true;
        while (it.hasNext()) {
            QString path = it.next();
            seenFiles.insert(path);
            if (fileChecksums.contains(path)) {
                QByteArray current = calculateChecksum(path);
                if (current.isEmpty()) {
                    qWarning() << "TamperDetector: Could not calculate checksum for" << path << ". Skipping.";
                    allOk = false;
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
                qCritical() << "TamperDetector: Tamper detected! New file detected in monitored directory:" << path;
                emit q->tamperDetected(QString("Unexpected file created: %1").arg(path));
                allOk = false;
            }
        }

        for (auto it = fileChecksums.constBegin(); it != fileChecksums.constEnd(); ++it) {
            if (!seenFiles.contains(it.key())) {
                qCritical() << "TamperDetector: Tamper detected! Baseline file missing:" << it.key();
                emit q->tamperDetected(QString("Baseline file deleted: %1").arg(it.key()));
                allOk = false;
            }
        }

        if (allOk) {
            qDebug() << "TamperDetector: File integrity check passed.";
            emit q->integrityCheckPassed();
        } else {
            qCritical() << "TamperDetector: File integrity check FAILED.";
            emit q->integrityCheckFailed("Monitored directory integrity check failed.");
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
 * The method computes a deterministic chain digest where each link is:
 * SHA256(previous_chain_digest || current_file_checksum). If a sidecar file
 * named "<logfile>.chain" exists, it must contain the expected chain digest in
 * hex for that link. For multi-file chains without sidecars, each file after
 * the first must contain the previous chain digest or previous file checksum in
 * hex, which catches broken append-chain exports.
 * @param logFiles A list of file paths representing the log chain.
 * @return True if the chain appears intact, false otherwise.
 */
bool TamperDetector::verifyChecksumChain(const QStringList& logFiles) {
    if (logFiles.isEmpty()) {
        qWarning() << "TamperDetector: Cannot verify an empty checksum chain.";
        emit integrityCheckFailed("Checksum chain is empty.");
        return false;
    }

    QByteArray previousChainDigest;
    QByteArray previousFileChecksum;
    bool hasAnchors = false;

    for (const QString& file : logFiles) {
        QFileInfo info(file);
        if (!info.exists() || !info.isFile()) {
            qCritical() << "TamperDetector: Missing log file in checksum chain:" << file;
            emit tamperDetected(QString("Missing log file in checksum chain: %1").arg(file));
            return false;
        }

        QByteArray currentChecksum = d->calculateChecksum(file);
        if (currentChecksum.isEmpty()) {
            qCritical() << "TamperDetector: Failed to calculate checksum for log file:" << file;
            emit integrityCheckFailed(QString("Could not checksum log file: %1").arg(file));
            return false;
        }

        if (!previousChainDigest.isEmpty()) {
            QFile currentFile(file);
            if (!currentFile.open(QIODevice::ReadOnly)) {
                qCritical() << "TamperDetector: Failed to read chained log file:" << file << "-" << currentFile.errorString();
                emit integrityCheckFailed(QString("Could not read chained log file: %1").arg(file));
                return false;
            }
            const QByteArray content = currentFile.readAll();
            currentFile.close();

            if (!content.contains(previousChainDigest.toHex()) &&
                !content.contains(previousFileChecksum.toHex())) {
                qCritical() << "TamperDetector: Broken checksum chain link at" << file;
                emit tamperDetected(QString("Broken checksum chain link: %1").arg(file));
                return false;
            }
        }

        const QByteArray chainDigest = HashProvider::sha256(previousChainDigest + currentChecksum);
        const QString sidecarPath = file + ".chain";
        QFile sidecar(sidecarPath);
        if (sidecar.exists()) {
            hasAnchors = true;
            if (!sidecar.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qCritical() << "TamperDetector: Failed to open checksum chain sidecar:" << sidecarPath << "-" << sidecar.errorString();
                emit integrityCheckFailed(QString("Could not read checksum chain sidecar: %1").arg(sidecarPath));
                return false;
            }
            const QByteArray expected = sidecar.readAll().trimmed().toLower();
            sidecar.close();
            if (expected != chainDigest.toHex()) {
                qCritical() << "TamperDetector: Checksum chain sidecar mismatch for" << file;
                emit tamperDetected(QString("Checksum chain anchor mismatch: %1").arg(file));
                return false;
            }
        }

        previousFileChecksum = currentChecksum;
        previousChainDigest = chainDigest;
    }

    if (!hasAnchors && logFiles.size() == 1) {
        qWarning() << "TamperDetector: Single-file checksum chain has no sidecar anchor; integrity cannot be proven.";
        emit integrityCheckFailed("Single-file checksum chain has no trusted anchor.");
        return false;
    }

    qInfo() << "TamperDetector: Checksum chain verification completed. Final digest:" << previousChainDigest.toHex();
    emit integrityCheckPassed();
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
