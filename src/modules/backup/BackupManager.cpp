#include "BackupManager.h"
#include "src/core/SystemManager.h"
#include "src/security/AES256Provider.h"
#include "src/security/HashProvider.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QUuid>
#include <QStandardPaths>
#include <QDebug>

namespace Ballot::Backup {

BackupManager& BackupManager::instance() {
    static BackupManager inst;
    return inst;
}

BackupManager::BackupManager() {
    m_autoBackupTimer = new QTimer(this);
    connect(m_autoBackupTimer, &QTimer::timeout, this, [this]() {
        createBackup("Auto-Backup " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"));
    });
}

void BackupManager::initialize() {
    auto* storage = Core::SystemManager::instance().storage();
    if (storage) {
        auto settings = storage->getSystemSettings();
        if (settings) {
            m_autoBackupEnabled = settings->autoBackupEnabled;
            m_backupIntervalHours = settings->backupIntervalHours;
            if (m_autoBackupEnabled) {
                m_autoBackupTimer->start(m_backupIntervalHours * 3600000);
            }
        }
    }
}

bool BackupManager::createBackup(const QString& name) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage || !storage->isConnected()) {
        emit backupFailed("Storage not available");
        return false;
    }

    Core::BackupEntry entry;
    entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.name = name.isEmpty() ? ("Backup " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm")) : name;
    entry.createdAt = QDateTime::currentDateTime();
    entry.type = "full";
    entry.isEncrypted = true;

    QString backupDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups/";
    QDir().mkpath(backupDir);

    QString sourcePath = "ballot.db";
    QString targetPath = backupDir + entry.id + ".enc";
    QString checksumPath = backupDir + entry.id + ".sha256";

    if (encryptBackupFile(sourcePath, targetPath)) {
        entry.checksum = Security::HashProvider::sha256Hex(sourcePath);
        entry.sizeBytes = QFileInfo(targetPath).size();
        entry.storagePath = targetPath;

        QFile checksumFile(checksumPath);
        if (checksumFile.open(QIODevice::WriteOnly)) {
            checksumFile.write(entry.checksum);
            checksumFile.close();
        }

        storage->saveBackupRecord(entry);
        emit backupCompleted(entry.id, targetPath);
        return true;
    }

    emit backupFailed("Encryption failed");
    return false;
}

bool BackupManager::restoreBackup(const QString& backupId) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        emit restoreFailed("Storage not available");
        return false;
    }

    auto backups = storage->getBackupHistory();
    for (const auto& b : backups) {
        if (b.id == backupId) {
            emit restoreStarted(backupId);

            QString decryptedPath = "restored_" + backupId + ".db";
            if (decryptBackupFile(b.storagePath, decryptedPath)) {
                QByteArray computedChecksum = Security::HashProvider::sha256Hex(decryptedPath);
                if (computedChecksum == b.checksum) {
                    emit restoreCompleted(backupId);
                    return true;
                } else {
                    emit restoreFailed("Checksum mismatch - backup may be corrupted");
                    return false;
                }
            }

            emit restoreFailed("Decryption failed");
            return false;
        }
    }

    emit restoreFailed("Backup not found");
    return false;
}

bool BackupManager::deleteBackup(const QString& backupId) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return false;

    auto backups = storage->getBackupHistory();
    for (const auto& b : backups) {
        if (b.id == backupId) {
            QFile::remove(b.storagePath);
            storage->deleteBackupRecord(backupId);
            return true;
        }
    }
    return false;
}

QList<Core::BackupEntry> BackupManager::getBackups() const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return {};
    return storage->getBackupHistory();
}

Core::BackupEntry BackupManager::getBackup(const QString& backupId) const {
    auto backups = getBackups();
    for (const auto& b : backups) {
        if (b.id == backupId) return b;
    }
    return {};
}

void BackupManager::setAutoBackup(bool enabled, int intervalHours) {
    m_autoBackupEnabled = enabled;
    m_backupIntervalHours = intervalHours;
    if (enabled) {
        m_autoBackupTimer->start(intervalHours * 3600000);
    } else {
        m_autoBackupTimer->stop();
    }
}

bool BackupManager::isAutoBackupEnabled() const { return m_autoBackupEnabled; }
int BackupManager::autoBackupInterval() const { return m_backupIntervalHours; }

bool BackupManager::exportBackup(const QString& backupId, const QString& targetPath) {
    auto backups = getBackups();
    for (const auto& b : backups) {
        if (b.id == backupId) {
            return QFile::copy(b.storagePath, targetPath);
        }
    }
    return false;
}

bool BackupManager::importBackup(const QString& filePath) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return false;

    QFileInfo info(filePath);
    if (!info.exists()) return false;

    Core::BackupEntry entry;
    entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.name = "Imported " + info.fileName();
    entry.createdAt = QDateTime::currentDateTime();
    entry.sizeBytes = info.size();
    entry.storagePath = filePath;
    entry.type = "full";
    entry.isEncrypted = true;
    entry.checksum = Security::HashProvider::sha256Hex(filePath);

    return storage->saveBackupRecord(entry);
}

bool BackupManager::encryptBackupFile(const QString& sourcePath, const QString& targetPath) {
    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) return false;
    QByteArray data = source.readAll();
    source.close();

    try {
        Security::AES256Provider crypto;
        QByteArray key = crypto.generateKey(32);
        QByteArray encrypted = crypto.encrypt(data, key);

        QFile target(targetPath);
        if (!target.open(QIODevice::WriteOnly)) return false;
        target.write(key);
        target.write(encrypted);
        target.close();
        return true;
    } catch (...) {
        return false;
    }
}

bool BackupManager::decryptBackupFile(const QString& sourcePath, const QString& targetPath) {
    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) return false;
    QByteArray key = source.read(32);
    QByteArray encrypted = source.readAll();
    source.close();

    try {
        Security::AES256Provider crypto;
        QByteArray decrypted = crypto.decrypt(encrypted, key);

        QFile target(targetPath);
        if (!target.open(QIODevice::WriteOnly)) return false;
        target.write(decrypted);
        target.close();
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace Ballot::Backup
