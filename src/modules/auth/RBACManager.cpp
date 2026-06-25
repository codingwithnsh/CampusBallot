#include "RBACManager.h"
#include "src/modules/storage/interfaces/IStorageProvider.h"
#include "src/core/SystemManager.h"

namespace Ballot::Auth {

const QString RBACManager::PERM_ELECTION_CREATE   = "election.create";
const QString RBACManager::PERM_ELECTION_DELETE   = "election.delete";
const QString RBACManager::PERM_ELECTION_MODIFY   = "election.modify";
const QString RBACManager::PERM_ELECTION_VIEW     = "election.view";
const QString RBACManager::PERM_CANDIDATE_MANAGE  = "candidate.manage";
const QString RBACManager::PERM_STUDENT_MANAGE    = "student.manage";
const QString RBACManager::PERM_USER_MANAGE       = "user.manage";
const QString RBACManager::PERM_USER_CREATE       = "user.create";
const QString RBACManager::PERM_USER_DELETE       = "user.delete";
const QString RBACManager::PERM_USER_MODIFY       = "user.modify";
const QString RBACManager::PERM_VOTE_CAST         = "vote.cast";
const QString RBACManager::PERM_VOTE_VERIFY       = "vote.verify";
const QString RBACManager::PERM_VOTE_START        = "vote.start";
const QString RBACManager::PERM_VOTE_END          = "vote.end";
const QString RBACManager::PERM_RESULTS_VIEW      = "results.view";
const QString RBACManager::PERM_RESULTS_EXPORT    = "results.export";
const QString RBACManager::PERM_AUDIT_VIEW        = "audit.view";
const QString RBACManager::PERM_AUDIT_EXPORT      = "audit.export";
const QString RBACManager::PERM_SETTINGS_MODIFY   = "settings.modify";
const QString RBACManager::PERM_SECURITY_MODIFY   = "security.modify";
const QString RBACManager::PERM_BACKUP_CREATE     = "backup.create";
const QString RBACManager::PERM_BACKUP_RESTORE    = "backup.restore";
const QString RBACManager::PERM_SYSTEM_UPDATE     = "system.update";
const QString RBACManager::PERM_LOGS_VIEW         = "logs.view";
const QString RBACManager::PERM_REPORTS_GENERATE  = "reports.generate";
const QString RBACManager::PERM_PLUGIN_MANAGE     = "plugin.manage";

static const QSet<QString> ALL_PERMS = {
    RBACManager::PERM_ELECTION_CREATE,
    RBACManager::PERM_ELECTION_DELETE,
    RBACManager::PERM_ELECTION_MODIFY,
    RBACManager::PERM_ELECTION_VIEW,
    RBACManager::PERM_CANDIDATE_MANAGE,
    RBACManager::PERM_STUDENT_MANAGE,
    RBACManager::PERM_USER_MANAGE,
    RBACManager::PERM_USER_CREATE,
    RBACManager::PERM_USER_DELETE,
    RBACManager::PERM_USER_MODIFY,
    RBACManager::PERM_VOTE_CAST,
    RBACManager::PERM_VOTE_VERIFY,
    RBACManager::PERM_VOTE_START,
    RBACManager::PERM_VOTE_END,
    RBACManager::PERM_RESULTS_VIEW,
    RBACManager::PERM_RESULTS_EXPORT,
    RBACManager::PERM_AUDIT_VIEW,
    RBACManager::PERM_AUDIT_EXPORT,
    RBACManager::PERM_SETTINGS_MODIFY,
    RBACManager::PERM_SECURITY_MODIFY,
    RBACManager::PERM_BACKUP_CREATE,
    RBACManager::PERM_BACKUP_RESTORE,
    RBACManager::PERM_SYSTEM_UPDATE,
    RBACManager::PERM_LOGS_VIEW,
    RBACManager::PERM_REPORTS_GENERATE,
    RBACManager::PERM_PLUGIN_MANAGE
};

static const QSet<QString> ADMIN_PERMS = ALL_PERMS;
static const QSet<QString> ELECTION_ADMIN_PERMS = {
    RBACManager::PERM_ELECTION_CREATE,
    RBACManager::PERM_ELECTION_DELETE,
    RBACManager::PERM_ELECTION_MODIFY,
    RBACManager::PERM_ELECTION_VIEW,
    RBACManager::PERM_CANDIDATE_MANAGE,
    RBACManager::PERM_STUDENT_MANAGE,
    RBACManager::PERM_USER_MANAGE,
    RBACManager::PERM_USER_CREATE,
    RBACManager::PERM_USER_MODIFY,
    RBACManager::PERM_VOTE_START,
    RBACManager::PERM_VOTE_END,
    RBACManager::PERM_RESULTS_VIEW,
    RBACManager::PERM_RESULTS_EXPORT,
    RBACManager::PERM_REPORTS_GENERATE
};
static const QSet<QString> TEACHER_PERMS = {
    RBACManager::PERM_VOTE_VERIFY,
    RBACManager::PERM_VOTE_START,
    RBACManager::PERM_ELECTION_VIEW,
    RBACManager::PERM_RESULTS_VIEW
};
static const QSet<QString> VOLUNTEER_PERMS = {
    RBACManager::PERM_STUDENT_MANAGE,
    RBACManager::PERM_VOTE_VERIFY,
    RBACManager::PERM_ELECTION_VIEW
};
static const QSet<QString> OBSERVER_PERMS = {
    RBACManager::PERM_ELECTION_VIEW,
    RBACManager::PERM_RESULTS_VIEW
};
static const QSet<QString> AUDITOR_PERMS = {
    RBACManager::PERM_AUDIT_VIEW,
    RBACManager::PERM_AUDIT_EXPORT,
    RBACManager::PERM_LOGS_VIEW,
    RBACManager::PERM_ELECTION_VIEW,
    RBACManager::PERM_RESULTS_VIEW
};

RBACManager& RBACManager::instance() {
    static RBACManager inst;
    return inst;
}

RBACManager::RBACManager() {
    initPermissions();
}

void RBACManager::initPermissions() {
    m_rolePermissions[Core::UserRole::SuperAdministrator] = ADMIN_PERMS;
    m_rolePermissions[Core::UserRole::ElectionAdministrator] = ELECTION_ADMIN_PERMS;
    m_rolePermissions[Core::UserRole::Teacher] = TEACHER_PERMS;
    m_rolePermissions[Core::UserRole::StudentVolunteer] = VOLUNTEER_PERMS;
    m_rolePermissions[Core::UserRole::Observer] = OBSERVER_PERMS;
    m_rolePermissions[Core::UserRole::ResultAuditor] = AUDITOR_PERMS;
}

bool RBACManager::hasPermission(Core::UserRole role, const QString& permission) const {
    return m_rolePermissions.value(role).contains(permission);
}

bool RBACManager::hasPermission(const QString& userId, const QString& permission) const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return false;
    auto user = storage->getUser(userId);
    if (!user) return false;
    return hasPermission(user->role, permission);
}

QStringList RBACManager::getPermissionsForRole(Core::UserRole role) const {
    return QStringList(m_rolePermissions.value(role).begin(), m_rolePermissions.value(role).end());
}

QStringList RBACManager::getAllPermissions() const {
    return QStringList(ALL_PERMS.begin(), ALL_PERMS.end());
}

Core::UserRole RBACManager::roleFromString(const QString& role) const {
    static const QHash<QString, Core::UserRole> map = {
        {"super_administrator", Core::UserRole::SuperAdministrator},
        {"election_administrator", Core::UserRole::ElectionAdministrator},
        {"teacher", Core::UserRole::Teacher},
        {"student_volunteer", Core::UserRole::StudentVolunteer},
        {"observer", Core::UserRole::Observer},
        {"result_auditor", Core::UserRole::ResultAuditor}
    };
    return map.value(role.toLower(), Core::UserRole::Observer);
}

QString RBACManager::roleToString(Core::UserRole role) const {
    static const QHash<Core::UserRole, QString> map = {
        {Core::UserRole::SuperAdministrator, "Super Administrator"},
        {Core::UserRole::ElectionAdministrator, "Election Administrator"},
        {Core::UserRole::Teacher, "Teacher"},
        {Core::UserRole::StudentVolunteer, "Student Volunteer"},
        {Core::UserRole::Observer, "Observer"},
        {Core::UserRole::ResultAuditor, "Result Auditor"}
    };
    return map.value(role, "Unknown");
}

bool RBACManager::canPerformAction(Core::UserRole role, Core::UserRole targetRole) const {
    if (role == Core::UserRole::SuperAdministrator) return true;
    if (role == Core::UserRole::ElectionAdministrator && targetRole != Core::UserRole::SuperAdministrator) return true;
    return false;
}

} // namespace Ballot::Auth
