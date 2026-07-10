#include "AuthManager.h"
#include "RBACManager.h"
#include "src/modules/security/HashProvider.h"
#include "src/core/SystemManager.h"
#include "src/core/Utils.h" // Include for Core::SystemInfo and Core::IdGenerator
#include "src/modules/audit/AuditManager.h" // For AuditManager::instance().log
#include <QDebug>
#include <QRegularExpression>

namespace Ballot::Auth {

namespace {

QString passwordValidationError(const QString& password) {
    if (password.length() < 12) {
        return "Password must be at least 12 characters long.";
    }

    const bool hasLower = password.contains(QRegularExpression("[a-z]"));
    const bool hasUpper = password.contains(QRegularExpression("[A-Z]"));
    const bool hasDigit = password.contains(QRegularExpression("[0-9]"));
    const bool hasSymbol = password.contains(QRegularExpression(R"([^A-Za-z0-9])"));

    if (!hasLower || !hasUpper || !hasDigit || !hasSymbol) {
        return "Password must include uppercase, lowercase, number, and symbol characters.";
    }

    return {};
}

} // namespace

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
    // Never allow a malformed/legacy setting to disable session expiry or create
    // an immediately-expiring session.
    m_timeoutMinutes = qBound(1, Core::SystemManager::instance().settings().sessionTimeoutMinutes, 24 * 60);
    qInfo() << "AuthManager: Initialized. Session timeout set to" << m_timeoutMinutes << "minutes.";
    return true;
}

// Removed IAuthProvider related methods as they are not currently implemented or used.
// void AuthManager::registerProvider(IAuthProvider* provider) { /* ... */ }
// void AuthManager::setActiveProvider(const QString& providerName) { /* ... */ }

bool AuthManager::login(const QString& username, const QString& password) {
    const QString normalizedUsername = username.trimmed().toLower();
    if (normalizedUsername.isEmpty() || password.isEmpty()) {
        emit loginFailed("Invalid credentials");
        return false;
    }

    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "AuthManager: Storage not available. Login failed for" << username;
        emit loginFailed("Storage not available");
        return false;
    }

    const Core::SystemSettings settings = Core::SystemManager::instance().settings();
    const int maxAttempts = qBound(1, settings.failedLoginAttempts, 25);
    const int lockoutMinutes = qBound(1, settings.lockoutDurationMinutes, 24 * 60);
    const QDateTime now = QDateTime::currentDateTimeUtc();

    const QDateTime lockoutExpiry = m_lockoutUntil.value(normalizedUsername);
    if (lockoutExpiry.isValid() && now < lockoutExpiry) {
        qWarning() << "AuthManager: Login blocked by temporary lockout for user:" << normalizedUsername;
        emit loginFailed(QString("Too many failed attempts. Try again after %1.")
            .arg(lockoutExpiry.toLocalTime().toString("yyyy-MM-dd HH:mm")));
        Audit::AuditManager::instance().log(
            Core::AuditAction::FailedLogin,
            QString("Failed login blocked by temporary lockout for user: %1").arg(normalizedUsername),
            "Unknown");
        return false;
    }

    if (lockoutExpiry.isValid() && now >= lockoutExpiry) {
        m_lockoutUntil.remove(normalizedUsername);
        m_failedLoginCounts.remove(normalizedUsername);
    }

    auto recordFailedLogin = [&]() {
        const int attempts = m_failedLoginCounts.value(normalizedUsername, 0) + 1;
        m_failedLoginCounts.insert(normalizedUsername, attempts);
        if (attempts >= maxAttempts) {
            m_lockoutUntil.insert(normalizedUsername, now.addSecs(lockoutMinutes * 60));
            qWarning() << "AuthManager: Temporary lockout applied for user:" << normalizedUsername
                       << "after" << attempts << "failed attempts.";
        }
    };

    std::optional<Core::User> userOpt = storage->getUserByEmail(normalizedUsername);
    if (!userOpt) {
        qWarning() << "AuthManager: Login failed. User not found for email:" << username;
        recordFailedLogin();
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
        recordFailedLogin();
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
        recordFailedLogin();
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
        recordFailedLogin();
        emit loginFailed("Invalid credentials");
        Audit::AuditManager::instance().log(
            Core::AuditAction::FailedLogin,
            QString("Failed login attempt for user: %1").arg(username),
            user.id);
        return false;
    }

    m_currentUser = user;
    m_isAuthenticated = true;
    m_failedLoginCounts.remove(normalizedUsername);
    m_lockoutUntil.remove(normalizedUsername);
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
    if (oldPassword == newPassword) {
        qWarning() << "AuthManager: New password must differ from old password for user" << m_currentUser.id;
        Audit::AuditManager::instance().log(
            Core::AuditAction::UserModified,
            QString("Password change rejected because the new password matched the old password for user: %1").arg(m_currentUser.email),
            m_currentUser.id);
        return false;
    }

    if (Core::SystemManager::instance().settings().requireStrongPassword) {
        const QString validationError = passwordValidationError(newPassword);
        if (!validationError.isEmpty()) {
            qWarning() << "AuthManager: New password rejected by strength policy for user" << m_currentUser.id << "-" << validationError;
            Audit::AuditManager::instance().log(
                Core::AuditAction::UserModified,
                QString("Password change rejected by strength policy for user: %1").arg(m_currentUser.email),
                m_currentUser.id);
            return false;
        }
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
