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
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace Ballot::Security {

class TamperDetector::Impl {
public:
    QTimer* monitorTimer = nullptr;
    QString monitoredDir;
    QHash<QString, QByteArray> fileChecksums;
    bool monitoring = false;

    void checkFiles(TamperDetector* q) {
        QDirIterator it(monitoredDir, QDir::Files, QDirIterator::Subdirectories);
        bool allOk = true;
        while (it.hasNext()) {
            QString path = it.next();
            if (fileChecksums.contains(path)) {
                QByteArray current = calculateChecksum(path);
                if (current != fileChecksums[path]) {
                    emit q->tamperDetected(QString("File modified: %1").arg(path));
                    allOk = false;
                }
            }
        }
        if (allOk) {
            emit q->integrityCheckPassed();
        }
    }

    QByteArray calculateChecksum(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return {};
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (hash.addData(&file)) {
            return hash.result();
        }
        return {};
    }
};

TamperDetector::TamperDetector(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>()) {}
TamperDetector::~TamperDetector() = default;

bool TamperDetector::verifyDatabaseIntegrity(const QString& dbPath) {
    QFile file(dbPath);
    if (!file.exists()) return false;

    if (!file.open(QIODevice::ReadOnly)) return false;
    QByteArray data = file.readAll();
    file.close();

    QByteArray integrity = HashProvider::sha256Hex(data);
    QFile integrityFile(dbPath + ".sha256");
    if (integrityFile.exists()) {
        if (!integrityFile.open(QIODevice::ReadOnly)) return false;
        QByteArray stored = integrityFile.readAll().trimmed();
        integrityFile.close();
        return stored == integrity;
    }
    return true;
}

bool TamperDetector::verifyFileIntegrity(const QString& filePath, const QByteArray& expectedChecksum) {
    QByteArray actual = calculateChecksum(filePath);
    return actual == expectedChecksum;
}

bool TamperDetector::verifyChecksumChain(const QStringList& logFiles) {
    QByteArray previousHash;
    for (const QString& file : logFiles) {
        QByteArray currentHash = calculateChecksum(file);
        if (!previousHash.isEmpty()) {
            QFile f(file);
            if (!f.open(QIODevice::ReadOnly)) return false;
            QByteArray content = f.readAll();
            f.close();
            if (!content.contains(previousHash.toHex())) return false;
        }
        previousHash = currentHash;
    }
    return true;
}

QByteArray TamperDetector::calculateChecksum(const QString& filePath) {
    return d->calculateChecksum(filePath);
}

bool TamperDetector::startIntegrityMonitoring(const QString& directory) {
    if (d->monitoring) return false;

    d->monitoredDir = directory;
    d->fileChecksums.clear();

    QDirIterator it(directory, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString path = it.next();
        d->fileChecksums[path] = d->calculateChecksum(path);
    }

    d->monitorTimer = new QTimer(this);
    connect(d->monitorTimer, &QTimer::timeout, this, [this]() {
        d->checkFiles(this);
    });
    d->monitorTimer->start(30000);
    d->monitoring = true;
    return true;
}

void TamperDetector::stopIntegrityMonitoring() {
    if (d->monitorTimer) {
        d->monitorTimer->stop();
        delete d->monitorTimer;
        d->monitorTimer = nullptr;
    }
    d->monitoring = false;
}

bool TamperDetector::isMonitoring() const {
    return d->monitoring;
}

bool TamperDetector::detectDebugger() {
#ifdef Q_OS_WIN
    return IsDebuggerPresent();
#else
    QFile status("/proc/self/status");
    if (status.open(QIODevice::ReadOnly)) {
        QByteArray content = status.readAll();
        status.close();
        if (content.contains("TracerPid:\t0")) return false;
        return true;
    }
    return false;
#endif
}

bool TamperDetector::detectSandbox() {
    QProcess proc;
    proc.start("systeminfo");
    proc.waitForFinished(3000);
    QString output = proc.readAll();
    if (output.contains("VirtualBox") || output.contains("VMware") || 
        output.contains("Hyper-V") || output.contains("VBox")) {
        return true;
    }
    return false;
}

bool TamperDetector::detectVirtualMachine() {
    return detectSandbox();
}

} // namespace Ballot::Security
