#include "BackupManager.h"
#include "src/core/SystemManager.h"
#include "src/core/Constants.h" // For DB_FILENAME
#include "src/core/Utils.h"
#include "src/modules/security/AES256Provider.h"
#include "src/modules/security/HashProvider.h"
#include "src/modules/audit/AuditManager.h" // For audit logging
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QUuid>
#include <QStandardPaths>
#include <QDebug>
#include <stdexcept> // For std::runtime_error

namespace Ballot::Backup {
namespace {

const QByteArray kBackupFormatMagic = "CBALLOT_BACKUP_V2\n";

QByteArray deriveBackupEncryptionKey() {
    QByteArray material;
    material.append("CampusBallot::BackupEncryption::v2::");
    material.append(Core::SystemInfo::getMachineId().toUtf8());
    material.append("::");
    material.append(Core::SystemInfo::getMachineName().toUtf8());
    material.append("::");
    material.append(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toUtf8());

    return Security::HashProvider::sha256(material);
}

} // namespace

BackupManager& BackupManager::instance() {
    static BackupManager inst;
    return inst;
}

BackupManager::BackupManager() : m_initialized(false) {
    m_autoBackupTimer = new QTimer(this);
    connect(m_autoBackupTimer, &QTimer::timeout, this, [this]() {
        qInfo() << "BackupManager: Initiating automatic backup.";
        createBackup("Auto-Backup " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    });
    qDebug() << "BackupManager: Instance created.";
}

/**
 * @brief Initializes the BackupManager, loading auto-backup settings from SystemManager.
 */
void BackupManager::initialize() {
    if (m_initialized) {
        qDebug() << "BackupManager: Already initialized.";
        return;
    }

    qDebug() << "BackupManager: Initializing...";
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "BackupManager: Storage not available during initialization. Auto-backup will not start.";
        m_initialized = false;
        return;
    }

    auto settingsOpt = storage->getSystemSettings();
    if (settingsOpt) {
        m_autoBackupEnabled = settingsOpt->autoBackupEnabled;
        m_backupIntervalHours = settingsOpt->backupIntervalHours;
        qInfo() << "BackupManager: Auto-backup settings loaded. Enabled:" << m_autoBackupEnabled << ", Interval:" << m_backupIntervalHours << "hours.";
        if (m_autoBackupEnabled && m_backupIntervalHours > 0) {
            m_autoBackupTimer->start(m_backupIntervalHours * 3600 * 1000); // Convert hours to milliseconds
            qInfo() << "BackupManager: Auto-backup timer started for every" << m_backupIntervalHours << "hours.";
        } else {
            qInfo() << "BackupManager: Auto-backup is disabled or interval is invalid. Timer not started.";
        }
    } else {
        qWarning() << "BackupManager: Could not load system settings for auto-backup. Using defaults.";
        // Defaults are already set in member variables
    }

    m_initialized = true;
    qInfo() << "BackupManager: Initialization complete.";
}

bool BackupManager::isInitialized() const {
    return m_initialized;
}

/**
 * @brief Creates a new full backup of the database.
 * The backup file is encrypted and stored in the application's data directory.
 * @param name An optional name for the backup. If empty, a default name is generated.
 * @return True if the backup was created successfully, false otherwise.
 */
bool BackupManager::createBackup(const QString& name) {
    qInfo() << "BackupManager: Starting backup creation...";
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage || !storage->isConnected()) {
        qCritical() << "BackupManager: Storage not available or not connected. Cannot create backup.";
        emit backupFailed("Storage not available or not connected.");
        Audit::AuditManager::instance().log(Core::AuditAction::BackupCreated, "Backup failed: Storage not available.", "System");
        return false;
    }

    Core::BackupEntry entry;
    entry.id = Core::IdGenerator::generateId();
    entry.name = name.isEmpty() ? ("Backup " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")) : name;
    entry.createdAt = QDateTime::currentDateTime();
    entry.type = "full";
    entry.isEncrypted = true; // Always encrypt backups

    QString backupDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups/";
    QDir dir(backupDir);
    if (!dir.mkpath(".")) { // Ensure the directory exists
        qCritical() << "BackupManager: Failed to create backup directory:" << backupDir;
        emit backupFailed("Failed to create backup directory.");
        Audit::AuditManager::instance().log(Core::AuditAction::BackupCreated, "Backup failed: Directory creation error.", "System");
        return false;
    }

    QString sourceDbPath = Core::Constants::DB_FILENAME; // Get actual DB file name
    QString targetEncryptedPath = backupDir + entry.id + ".enc";
    QString checksumFilePath = backupDir + entry.id + ".sha256"; // Checksum of the original DB

    emit backupStarted(entry.id);

    // Calculate checksum of the ORIGINAL database file for integrity verification after decryption
    QByteArray originalDbChecksum;
    try {
        originalDbChecksum = Security::HashProvider::sha256File(sourceDbPath);
        if (originalDbChecksum.isEmpty()) {
            qCritical() << "BackupManager: Failed to calculate checksum for original database:" << sourceDbPath;
            emit backupFailed("Failed to calculate original database checksum.");
            Audit::AuditManager::instance().log(Core::AuditAction::BackupCreated, "Backup failed: Checksum calculation error.", "System");
            return false;
        }
    } catch (const std::exception& e) {
        qCritical() << "BackupManager: Exception calculating checksum for original database:" << e.what();
        emit backupFailed("Exception calculating original database checksum.");
        Audit::AuditManager::instance().log(Core::AuditAction::BackupCreated, QString("Backup failed: Checksum exception - %1").arg(e.what()), "System");
        return false;
    }

    if (encryptBackupFile(sourceDbPath, targetEncryptedPath)) {
        entry.checksum = originalDbChecksum; // Store checksum of original DB
        entry.sizeBytes = QFileInfo(targetEncryptedPath).size();
        entry.storagePath = targetEncryptedPath;

        // Save the original DB checksum to a separate file for verification
        QFile checksumFile(checksumFilePath);
        if (checksumFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            checksumFile.write(entry.checksum.toHex()); // Store hex representation
            checksumFile.close();
            qDebug() << "BackupManager: Saved original DB checksum to" << checksumFilePath;
        } else {
            qWarning() << "BackupManager: Failed to save original DB checksum file:" << checksumFilePath << "-" << checksumFile.errorString();
            // This is a warning, as the backup itself is encrypted and recorded.
        }

        if (storage->saveBackupRecord(entry)) {
            qInfo() << "BackupManager: Backup created successfully:" << entry.name << "(" << entry.id << ") at" << entry.storagePath;
            Audit::AuditManager::instance().log(Core::AuditAction::BackupCreated, QString("Backup created: %1").arg(entry.name), "System");
            emit backupCompleted(entry.id, entry.storagePath);
            return true;
        } else {
            qCritical() << "BackupManager: Failed to save backup record to storage for" << entry.name;
            emit backupFailed("Failed to save backup record to storage.");
            Audit::AuditManager::instance().log(Core::AuditAction::BackupCreated, "Backup failed: Storage record error.", "System");
            // Clean up the created encrypted file if record fails
            QFile::remove(targetEncryptedPath);
            QFile::remove(checksumFilePath);
            return false;
        }
    }

    qCritical() << "BackupManager: Encryption failed for backup" << entry.name;
    emit backupFailed("Encryption failed during backup creation.");
    Audit::AuditManager::instance().log(Core::AuditAction::BackupCreated, "Backup failed: Encryption error.", "System");
    return false;
}

/**
 * @brief Restores a backup from a given backup ID.
 * This process involves decrypting the backup file and replacing the current database.
 * @param backupId The ID of the backup to restore.
 * @return True if the backup was restored successfully, false otherwise.
 */
bool BackupManager::restoreBackup(const QString& backupId) {
    qInfo() << "BackupManager: Starting restore for backup ID:" << backupId;
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "BackupManager: Storage not available. Cannot restore backup.";
        emit restoreFailed("Storage not available.");
        Audit::AuditManager::instance().log(Core::AuditAction::BackupRestored, "Restore failed: Storage not available.", "System");
        return false;
    }

    std::optional<Core::BackupEntry> backupOpt = storage->getBackup(backupId);
    if (!backupOpt) {
        qWarning() << "BackupManager: Backup with ID" << backupId << "not found.";
        emit restoreFailed("Backup not found.");
        Audit::AuditManager::instance().log(Core::AuditAction::BackupRestored, "Restore failed: Backup not found.", "System");
        return false;
    }
    Core::BackupEntry b = *backupOpt;

    emit restoreStarted(backupId);

    QString tempDecryptedPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".db";
    QString originalDbPath = Core::Constants::DB_FILENAME;
    QString rollbackDbPath = originalDbPath + ".restore-backup-" + QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmsszzz");

    if (decryptBackupFile(b.storagePath, tempDecryptedPath)) {
        // Verify checksum of the decrypted file against the stored original DB checksum
        QByteArray computedChecksum = Security::HashProvider::sha256File(tempDecryptedPath);
        if (computedChecksum.isEmpty()) {
            qCritical() << "BackupManager: Failed to calculate checksum for decrypted database:" << tempDecryptedPath;
            emit restoreFailed("Failed to calculate checksum for decrypted database.");
            Audit::AuditManager::instance().log(Core::AuditAction::BackupRestored, "Restore failed: Decrypted DB checksum error.", "System");
            QFile::remove(tempDecryptedPath);
            return false;
        }

        if (computedChecksum == b.checksum) {
            qInfo() << "BackupManager: Decrypted backup checksum verified successfully.";

            // Disconnect from the current database only after the backup has
            // decrypted and passed integrity validation.
            if (storage->isConnected()) {
                qInfo() << "BackupManager: Disconnecting from current database for restore operation.";
                storage->disconnect();
            }

            bool originalMoved = true;
            if (QFile::exists(originalDbPath)) {
                QFile::remove(rollbackDbPath);
                originalMoved = QFile::rename(originalDbPath, rollbackDbPath);
            }

            if (!originalMoved) {
                qCritical() << "BackupManager: Failed to move existing database to rollback path:" << rollbackDbPath;
                emit restoreFailed("Failed to prepare existing database for restore.");
                Audit::AuditManager::instance().log(Core::AuditAction::BackupRestored, "Restore failed: Existing DB rollback preparation error.", "System");
            } else if (QFile::copy(tempDecryptedPath, originalDbPath)) {
                qInfo() << "BackupManager: Database restored successfully from" << b.name;
                Audit::AuditManager::instance().log(Core::AuditAction::BackupRestored, QString("Database restored from backup: %1").arg(b.name), "System");
                emit restoreCompleted(backupId);
                QFile::remove(tempDecryptedPath);
                QFile::remove(rollbackDbPath);

                // Reconnect to the newly restored database
                if (!storage->connect(QVariantMap())) { // Reconnect with default config
                    qCritical() << "BackupManager: Failed to reconnect to database after restore!";
                    Audit::AuditManager::instance().log(Core::AuditAction::BackupRestored, "Restore completed but failed to reconnect to DB.", "System");
                    // This is a critical state, application might need to restart
                }
                return true;
            } else {
                qCritical() << "BackupManager: Failed to copy decrypted database to original path:" << originalDbPath;
                emit restoreFailed("Failed to copy decrypted database.");
                Audit::AuditManager::instance().log(Core::AuditAction::BackupRestored, "Restore failed: File copy error.", "System");

                if (QFile::exists(rollbackDbPath)) {
                    QFile::remove(originalDbPath);
                    if (!QFile::rename(rollbackDbPath, originalDbPath)) {
                        qCritical() << "BackupManager: Failed to roll back original database after restore copy failure.";
                        Audit::AuditManager::instance().log(Core::AuditAction::BackupRestored, "Restore failed: Original DB rollback failed.", "System");
                    }
                }
            }
        } else {
            qCritical() << "BackupManager: Checksum mismatch for decrypted backup" << b.name << ". Backup may be corrupted.";
            qCritical() << "  Expected checksum:" << b.checksum.toHex();
            qCritical() << "  Computed checksum:" << computedChecksum.toHex();
            emit restoreFailed("Checksum mismatch - backup may be corrupted.");
            Audit::AuditManager::instance().log(Core::AuditAction::BackupRestored, "Restore failed: Checksum mismatch.", "System");
        }
    } else {
        qCritical() << "BackupManager: Decryption failed for backup" << b.name << "at" << b.storagePath;
        emit restoreFailed("Decryption failed.");
        Audit::AuditManager::instance().log(Core::AuditAction::BackupRestored, "Restore failed: Decryption error.", "System");
    }

    QFile::remove(tempDecryptedPath); // Clean up temp file in case of failure
    // Attempt to reconnect to the original database if restore failed
    if (!storage->isConnected()) {
        qWarning() << "BackupManager: Attempting to reconnect to original database after failed restore.";
        if (!storage->connect(QVariantMap())) {
            qCritical() << "BackupManager: Failed to reconnect to database after failed restore!";
        }
    }
    return false;
}

/**
 * @brief Deletes a backup record and its associated files.
 * @param backupId The ID of the backup to delete.
 * @return True if the backup was deleted successfully, false otherwise.
 */
bool BackupManager::deleteBackup(const QString& backupId) {
    qInfo() << "BackupManager: Deleting backup with ID:" << backupId;
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "BackupManager: Storage not available. Cannot delete backup.";
        return false;
    }

    std::optional<Core::BackupEntry> backupOpt = storage->getBackup(backupId);
    if (!backupOpt) {
        qWarning() << "BackupManager: Backup with ID" << backupId << "not found for deletion.";
        return false;
    }
    Core::BackupEntry b = *backupOpt;

    bool fileRemoved = !QFile::exists(b.storagePath) || QFile::remove(b.storagePath);
    if (!fileRemoved) {
        qWarning() << "BackupManager: Failed to remove backup file:" << b.storagePath << ". It might not exist or permissions are an issue.";
    } else {
        qDebug() << "BackupManager: Backup file removed:" << b.storagePath;
    }

    // Also remove the checksum file
    QString checksumFilePath = QFileInfo(b.storagePath).path() + "/" + b.id + ".sha256";
    bool checksumFileRemoved = !QFile::exists(checksumFilePath) || QFile::remove(checksumFilePath);
    if (!checksumFileRemoved) {
        qWarning() << "BackupManager: Failed to remove backup checksum file:" << checksumFilePath << ". It might not exist or permissions are an issue.";
    } else {
        qDebug() << "BackupManager: Backup checksum file removed:" << checksumFilePath;
    }

    if (storage->deleteBackupRecord(backupId)) {
        qInfo() << "BackupManager: Backup record" << b.name << "(" << b.id << ") deleted successfully.";
        Audit::AuditManager::instance().log(Core::AuditAction::BackupDeleted, QString("Backup deleted: %1").arg(b.name), "System");
        return true;
    } else {
        qCritical() << "BackupManager: Failed to delete backup record from storage for" << b.name;
        Audit::AuditManager::instance().log(Core::AuditAction::BackupDeleted, "Backup deletion failed: Storage record error.", "System");
        return false;
    }
}

QList<Core::BackupEntry> BackupManager::getBackups() const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "BackupManager: Storage not available. Cannot retrieve backups.";
        return {};
    }
    return storage->getBackupHistory();
}

/**
 * @brief Retrieves a specific backup entry by its ID.
 * @param backupId The ID of the backup.
 * @return An optional containing the BackupEntry if found, std::nullopt otherwise.
 */
std::optional<Core::BackupEntry> BackupManager::getBackup(const QString& backupId) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "BackupManager: Storage not available. Cannot retrieve backup.";
        return std::nullopt;
    }
    return storage->getBackup(backupId);
}

/**
 * @brief Enables or disables automatic backups and sets the interval.
 * @param enabled True to enable, false to disable.
 * @param intervalHours The interval in hours for automatic backups.
 */
void BackupManager::setAutoBackup(bool enabled, int intervalHours) {
    m_autoBackupEnabled = enabled;
    m_backupIntervalHours = intervalHours;
    if (enabled && intervalHours > 0) {
        m_autoBackupTimer->start(intervalHours * 3600 * 1000);
        qInfo() << "BackupManager: Auto-backup enabled with interval of" << intervalHours << "hours.";
    } else {
        m_autoBackupTimer->stop();
        qInfo() << "BackupManager: Auto-backup disabled.";
    }
}

bool BackupManager::isAutoBackupEnabled() const { return m_autoBackupEnabled; }
int BackupManager::autoBackupInterval() const { return m_backupIntervalHours; }

/**
 * @brief Exports a specific backup to a target file path.
 * @param backupId The ID of the backup to export.
 * @param targetPath The destination file path for the exported backup.
 * @return True if the backup was exported successfully, false otherwise.
 */
bool BackupManager::exportBackup(const QString& backupId, const QString& targetPath) {
    qInfo() << "BackupManager: Exporting backup" << backupId << "to" << targetPath;
    std::optional<Core::BackupEntry> backupOpt = getBackup(backupId);
    if (!backupOpt) {
        qWarning() << "BackupManager: Backup with ID" << backupId << "not found for export.";
        return false;
    }
    Core::BackupEntry b = *backupOpt;

    if (QFile::copy(b.storagePath, targetPath)) {
        qInfo() << "BackupManager: Backup exported successfully.";
        Audit::AuditManager::instance().log(Core::AuditAction::LogsExported, QString("Backup exported: %1 to %2").arg(b.name, targetPath), "System"); // Using LogsExported as closest
        return true;
    } else {
        qCritical() << "BackupManager: Failed to export backup file from" << b.storagePath << "to" << targetPath;
        return false;
    }
}

/**
 * @brief Imports a backup file into the system.
 * This only records the backup entry; it does NOT restore the database.
 * @param filePath The path to the backup file to import.
 * @return True if the backup was successfully recorded, false otherwise.
 * @warning This function only registers the backup. A separate restore operation is needed.
 */
bool BackupManager::importBackup(const QString& filePath) {
    qInfo() << "BackupManager: Importing backup from" << filePath;
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "BackupManager: Storage not available. Cannot import backup.";
        return false;
    }

    QFileInfo info(filePath);
    if (!info.exists()) {
        qWarning() << "BackupManager: Imported file does not exist:" << filePath;
        return false;
    }

    // Copy the imported file to the internal backup directory
    QString backupDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups/";
    QDir dir(backupDir);
    if (!dir.mkpath(".")) {
        qCritical() << "BackupManager: Failed to create backup directory for import:" << backupDir;
        return false;
    }

    QString newBackupId = Core::IdGenerator::generateId();
    QString newStoragePath = backupDir + newBackupId + ".enc"; // Assuming imported backups are also encrypted

    if (!QFile::copy(filePath, newStoragePath)) {
        qCritical() << "BackupManager: Failed to copy imported backup file to internal storage:" << filePath << "to" << newStoragePath;
        return false;
    }

    QString tempDecryptedPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".db";
    QByteArray decryptedChecksum;
    if (decryptBackupFile(newStoragePath, tempDecryptedPath)) {
        decryptedChecksum = Security::HashProvider::sha256File(tempDecryptedPath);
        QFile::remove(tempDecryptedPath);
    }

    if (decryptedChecksum.isEmpty()) {
        qCritical() << "BackupManager: Imported backup failed decrypt/checksum validation:" << filePath;
        QFile::remove(newStoragePath);
        return false;
    }

    Core::BackupEntry entry;
    entry.id = newBackupId;
    entry.name = "Imported " + info.fileName();
    entry.createdAt = QDateTime::currentDateTime();
    entry.sizeBytes = info.size();
    entry.storagePath = newStoragePath;
    entry.type = "full"; // Assuming full backup
    entry.isEncrypted = true; // Assuming imported backups are encrypted

    // Store the decrypted database checksum. Restore validates the decrypted
    // payload, not the encrypted wrapper, so imported backups must use the same
    // checksum contract as locally created backups.
    entry.checksum = decryptedChecksum;

    if (storage->saveBackupRecord(entry)) {
        qInfo() << "BackupManager: Imported backup recorded successfully:" << entry.name << "(" << entry.id << ")";
        Audit::AuditManager::instance().log(Core::AuditAction::BackupCreated, QString("Backup imported: %1 from %2").arg(entry.name, filePath), "System");
        return true;
    } else {
        qCritical() << "BackupManager: Failed to save imported backup record to storage for" << entry.name;
        QFile::remove(newStoragePath); // Clean up copied file
        return false;
    }
}

/**
 * @brief Encrypts a source file to a target file.
 * @param sourcePath The path to the unencrypted source file (e.g., database).
 * @param targetPath The path where the encrypted file will be saved.
 * @return True if encryption is successful, false otherwise.
 * The encryption key is derived from machine/install scoped material and is not
 * stored with the ciphertext. Files created by older builds that prepended a
 * random key are still accepted by decryptBackupFile() for restore compatibility.
 */
bool BackupManager::encryptBackupFile(const QString& sourcePath, const QString& targetPath) {
    qDebug() << "BackupManager: Encrypting" << sourcePath << "to" << targetPath;
    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) {
        qCritical() << "BackupManager: Failed to open source file for encryption:" << sourcePath << "-" << source.errorString();
        return false;
    }
    QByteArray data = source.readAll();
    source.close();

    try {
        Security::AES256Provider crypto;
        QByteArray key = deriveBackupEncryptionKey();
        QByteArray encrypted = crypto.encrypt(data, key);

        QFile target(targetPath);
        if (!target.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCritical() << "BackupManager: Failed to open target file for encryption:" << targetPath << "-" << target.errorString();
            return false;
        }
        target.write(kBackupFormatMagic);
        target.write(encrypted);
        target.close();
        qInfo() << "BackupManager: File encrypted successfully using protected backup format.";
        return true;
    } catch (const std::exception& e) {
        qCritical() << "BackupManager: Encryption failed for" << sourcePath << ":" << e.what();
        return false;
    } catch (...) {
        qCritical() << "BackupManager: Unknown encryption error for" << sourcePath;
        return false;
    }
}

/**
 * @brief Decrypts an encrypted file to a target file.
 * @param sourcePath The path to the encrypted source file.
 * @param targetPath The path where the decrypted file will be saved.
 * @return True if decryption is successful, false otherwise.
 * Supports the current protected backup format and legacy backups whose random
 * key was prepended to the encrypted payload.
 */
bool BackupManager::decryptBackupFile(const QString& sourcePath, const QString& targetPath) {
    qDebug() << "BackupManager: Decrypting" << sourcePath << "to" << targetPath;
    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) {
        qCritical() << "BackupManager: Failed to open source file for decryption:" << sourcePath << "-" << source.errorString();
        return false;
    }
    QByteArray fileData = source.readAll();
    source.close();

    QByteArray key;
    QByteArray encrypted;
    if (fileData.startsWith(kBackupFormatMagic)) {
        key = deriveBackupEncryptionKey();
        encrypted = fileData.mid(kBackupFormatMagic.size());
    } else {
        // Legacy compatibility: older backups prepended the raw encryption key.
        if (fileData.size() <= 32) {
            qCritical() << "BackupManager: Invalid legacy backup size:" << sourcePath;
            return false;
        }
        key = fileData.left(32);
        encrypted = fileData.mid(32);
    }

    if (key.size() != 32) {
        qCritical() << "BackupManager: Invalid backup encryption key size for:" << sourcePath;
        return false;
    }

    try {
        Security::AES256Provider crypto;
        QByteArray decrypted = crypto.decrypt(encrypted, key);

        QFile target(targetPath);
        if (!target.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCritical() << "BackupManager: Failed to open target file for decryption:" << targetPath << "-" << target.errorString();
            return false;
        }
        target.write(decrypted);
        target.close();
        qInfo() << "BackupManager: File decrypted successfully.";
        return true;
    } catch (const std::exception& e) {
        qCritical() << "BackupManager: Decryption failed for" << sourcePath << ":" << e.what();
        return false;
    } catch (...) {
        qCritical() << "BackupManager: Unknown decryption error for" << sourcePath;
        return false;
    }
}

} // namespace Ballot::Backup
