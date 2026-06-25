#pragma once

#include <QObject>
#include <QHash>
#include <QSet>
#include <QString>
#include "src/core/Models.h"

namespace Ballot::Auth {

class RBACManager : public QObject {
    Q_OBJECT
public:
    static RBACManager& instance();

    bool hasPermission(Core::UserRole role, const QString& permission) const;
    bool hasPermission(const QString& userId, const QString& permission) const;
    QStringList getPermissionsForRole(Core::UserRole role) const;
    QStringList getAllPermissions() const;
    Core::UserRole roleFromString(const QString& role) const;
    QString roleToString(Core::UserRole role) const;
    bool canPerformAction(Core::UserRole role, Core::UserRole targetRole) const;

    static const QString PERM_ELECTION_CREATE;
    static const QString PERM_ELECTION_DELETE;
    static const QString PERM_ELECTION_MODIFY;
    static const QString PERM_ELECTION_VIEW;
    static const QString PERM_CANDIDATE_MANAGE;
    static const QString PERM_STUDENT_MANAGE;
    static const QString PERM_USER_MANAGE;
    static const QString PERM_USER_CREATE;
    static const QString PERM_USER_DELETE;
    static const QString PERM_USER_MODIFY;
    static const QString PERM_VOTE_CAST;
    static const QString PERM_VOTE_VERIFY;
    static const QString PERM_VOTE_START;
    static const QString PERM_VOTE_END;
    static const QString PERM_RESULTS_VIEW;
    static const QString PERM_RESULTS_EXPORT;
    static const QString PERM_AUDIT_VIEW;
    static const QString PERM_AUDIT_EXPORT;
    static const QString PERM_SETTINGS_MODIFY;
    static const QString PERM_SECURITY_MODIFY;
    static const QString PERM_BACKUP_CREATE;
    static const QString PERM_BACKUP_RESTORE;
    static const QString PERM_SYSTEM_UPDATE;
    static const QString PERM_LOGS_VIEW;
    static const QString PERM_REPORTS_GENERATE;
    static const QString PERM_PLUGIN_MANAGE;

signals:
    void permissionChanged(const QString& userId);

private:
    RBACManager();
    void initPermissions();
    QHash<Core::UserRole, QSet<QString>> m_rolePermissions;
};

} // namespace Ballot::Auth
