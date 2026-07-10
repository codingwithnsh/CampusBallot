#include "AuthManager.h"
#include "RBACManager.h"
#include "src/modules/security/HashProvider.h"
#include "src/core/SystemManager.h"
#include "src/core/Utils.h" // Include for Core::SystemInfo and Core::IdGenerator
#include "src/modules/audit/AuditManager.h" // For AuditManager::instance().log
#include <QDebug>

namespace Ballot::Auth {

AuthManager& AuthManager::instance() {
    static AuthManager inst;
    return inst;
}

AuthManager::AuthManager() : QObject(nullptr) {
    m_sessionTimer = new QTimer(this);
    m_sessionTimer->setSingleShot(true);
    connect(m_sessionTimer, &QTimer::timeout, this, [this]() {
        if (m_isAuthenticated) {
            qInfo() << "AuthManager: Session timed out for user" << m_currentUser.id;
            logout();
            emit sessionTimedOut();
        }
    });
}

bool AuthManager::initialize(const QVariantMap& config) {
    // Retrieve session timeout from SystemManager settings for centralized configuration
    m_timeoutMinutes = Core::SystemManager::instance().settings().sessionTimeoutMinutes;
    qInfo() << "AuthManager: Initialized. Session timeout set to" << m_timeoutMinutes << "minutes.";
    return true;
}

// Removed IAuthProvider related methods as they are not currently implemented or used.
// void AuthManager::registerProvider(IAuthProvider* provider) { /* ... */ }
// void AuthManager::setActiveProvider(const QString& providerName) { /* ... */ }

bool AuthManager::login(const QString& username, const QString& password) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "AuthManager: Storage not available. Login failed for" << username;
        emit loginFailed("Storage not available");
        return false;
    }

    std::optional<Core::User> userOpt = storage->getUserByEmail(username);
    if (!userOpt) {
        qWarning() << "AuthManager: Login failed. User not found for email:" << username;
        emit loginFailed("Invalid credentials");
        // Log failed attempt even if user not found to deter enumeration attacks
        Audit::AuditManager::instance().log(
            Core::AuditAction::FailedLogin,
            QString("Failed login attempt for unknown user: %1").arg(username),
            "Unknown");
        return false;
    }

    Core::User user = *userOpt; // Get the actual user object

    if (!user.isActive) {
        qWarning() << "AuthManager: Login failed. Account disabled for user:" << username;
        emit loginFailed("Account is disabled");
        Audit::AuditManager::instance().log(
            Core::AuditAction::FailedLogin,
            QString("Failed login attempt (account disabled) for user: %1").arg(username),
            user.id);
        return false;
    }

    // Use passwordHashAndSalt for verification
    const QByteArray storedHashAndSalt = user.passwordHashAndSalt;
    if (storedHashAndSalt.isEmpty()) {
        qCritical() << "AuthManager: User" << user.id << "has no password set. Login failed.";
        emit loginFailed("Account misconfigured");
        Audit::AuditManager::instance().log(
            Core::AuditAction::FailedLogin,
            QString("Failed login attempt (no password set) for user: %1").arg(username),
            user.id);
        return false;
    }

    // Extract salt (first 16 bytes) and hash from storedHashAndSalt
    QByteArray salt = storedHashAndSalt.left(16);
    QByteArray hash = storedHashAndSalt.mid(16);

    if (!Security::HashProvider::verifyArgon2(password, hash, salt)) {
        qWarning() << "AuthManager: Login failed. Invalid credentials for user:" << username;
        emit loginFailed("Invalid credentials");
        Audit::AuditManager::instance().log(
            Core::AuditAction::FailedLogin,
            QString("Failed login attempt for user: %1").arg(username),
            user.id);
        return false;
    }

    m_currentUser = user;
    m_isAuthenticated = true;
    resetSessionTimer(); // Start/reset session timer on successful login

    Audit::AuditManager::instance().log(
        Core::AuditAction::Login,
        QString("User logged in: %1").arg(username),
        user.id);

    emit loginSuccessful(user.id);
    emit authStateChanged();
    qInfo() << "AuthManager: User" << username << "logged in successfully.";
    return true;
}

bool AuthManager::loginByToken(const QString& token) {
    qWarning() << "AuthManager: loginByToken is not yet implemented.";
    emit loginFailed("Login by token not implemented");
    return false;
}

bool AuthManager::loginByQR(const QByteArray& qrData) {
    qWarning() << "AuthManager: loginByQR is not yet implemented.";
    emit loginFailed("Login by QR not implemented");
    return false;
}

void AuthManager::logout() {
    if (m_isAuthenticated) {
        auto* storage = Core::SystemManager::instance().storage();
        if (storage) {
            Audit::AuditManager::instance().log(
                Core::AuditAction::Logout,
                QString("User logged out: %1").arg(m_currentUser.email),
                m_currentUser.id);
        } else {
            qCritical() << "AuthManager: Storage not available during logout audit logging.";
        }
        qInfo() << "AuthManager: User" << m_currentUser.email << "logged out.";
    }
    m_isAuthenticated = false;
    m_currentUser = {}; // Clear current user data
    m_sessionTimer->stop();
    emit logoutOccurred();
    emit authStateChanged();
}

bool AuthManager::isAuthenticated() const { return m_isAuthenticated; }
Core::UserRole AuthManager::currentRole() const { return m_currentUser.role; }
QString AuthManager::currentUserId() const { return m_currentUser.id; }
Core::User AuthManager::currentUser() const { return m_currentUser; }

bool AuthManager::hasPermission(const QString& permission) const {
    if (!m_isAuthenticated) {
        qWarning() << "AuthManager: Permission check for" << permission << "failed. User not authenticated.";
        emit permissionDenied(permission);
        return false;
    }
    bool granted = RBACManager::instance().hasPermission(m_currentUser.role, permission);
    if (!granted) {
        qWarning() << "AuthManager: Permission" << permission << "denied for user" << m_currentUser.id << "with role" << static_cast<int>(m_currentUser.role);
        emit permissionDenied(permission);
    }
    return granted;
}

bool AuthManager::changePassword(const QString& oldPassword, const QString& newPassword) {
    if (!m_isAuthenticated) {
        qWarning() << "AuthManager: Cannot change password. User not authenticated.";
        return false;
    }
    if (newPassword.isEmpty()) {
        qWarning() << "AuthManager: New password cannot be empty.";
        return false;
    }

    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "AuthManager: Storage not available. Cannot change password.";
        return false;
    }

    // Verify old password
    QByteArray salt = m_currentUser.passwordHashAndSalt.left(16);
    QByteArray hash = m_currentUser.passwordHashAndSalt.mid(16);
    if (!Security::HashProvider::verifyArgon2(oldPassword, hash, salt)) {
        qWarning() << "AuthManager: Old password verification failed for user" << m_currentUser.id;
        return false;
    }

    // Generate new hash and salt for the new password
    QByteArray newSalt = Security::HashProvider::generateSalt();
    QByteArray newHash = Security::HashProvider::argon2Hash(newPassword, newSalt);
    QByteArray newPasswordHashAndSalt = newSalt + newHash;

    if (!storage->updateUserPassword(m_currentUser.id, newPasswordHashAndSalt)) {
        qCritical() << "AuthManager: Failed to update password in storage for user" << m_currentUser.id;
        return false;
    }

    // Update current user's password hash in memory
    m_currentUser.passwordHashAndSalt = newPasswordHashAndSalt;

    Audit::AuditManager::instance().log(
        Core::AuditAction::SettingsChanged, // Or a more specific password change action
        QString("User password changed for user: %1").arg(m_currentUser.email),
        m_currentUser.id);
    qInfo() << "AuthManager: Password changed successfully for user" << m_currentUser.email;
    return true;
}

// Removed IAuthProvider related methods as they are not currently implemented or used.
// QStringList AuthManager::availableProviders() const { /* ... */ }
// IAuthProvider* AuthManager::activeProvider() const { /* ... */ }

void AuthManager::startSessionTimer() {
    m_sessionTimer->start(m_timeoutMinutes * 60 * 1000); // Convert minutes to milliseconds
    qDebug() << "AuthManager: Session timer started for" << m_timeoutMinutes << "minutes.";
}

void AuthManager::resetSessionTimer() {
    if (m_isAuthenticated) {
        m_sessionTimer->stop(); // Stop existing timer
        m_sessionTimer->start(m_timeoutMinutes * 60 * 1000); // Restart timer
        qDebug() << "AuthManager: Session timer reset for" << m_timeoutMinutes << "minutes.";
    }
}

} // namespace Ballot::Auth