#pragma once

#include <QObject>
#include <QTimer>
#include <memory>
#include "src/core/Models.h"

namespace Ballot::Backup {

class BackupManager : public QObject {
    Q_OBJECT
public:
    static BackupManager& instance();

    void initialize();
    bool createBackup(const QString& name = QString());
    bool restoreBackup(const QString& backupId);
    bool deleteBackup(const QString& backupId);

    QList<Core::BackupEntry> getBackups() const;
    Core::BackupEntry getBackup(const QString& backupId) const;

    void setAutoBackup(bool enabled, int intervalHours = 24);
    bool isAutoBackupEnabled() const;
    int autoBackupInterval() const;

    bool exportBackup(const QString& backupId, const QString& targetPath);
    bool importBackup(const QString& filePath);

signals:
    void backupStarted(const QString& backupId);
    void backupCompleted(const QString& backupId, const QString& path);
    void backupFailed(const QString& error);
    void restoreStarted(const QString& backupId);
    void restoreCompleted(const QString& backupId);
    void restoreFailed(const QString& error);

private:
    BackupManager();
    bool encryptBackupFile(const QString& sourcePath, const QString& targetPath);
    bool decryptBackupFile(const QString& sourcePath, const QString& targetPath);

    QTimer* m_autoBackupTimer;
    bool m_autoBackupEnabled = true;
    int m_backupIntervalHours = 24;
};

} // namespace Ballot::Backup
