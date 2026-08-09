#include "AuthManager.h"
#include "RBACManager.h"
#include "src/modules/security/HashProvider.h"
#include "src/core/SystemManager.h"
#include "src/core/Utils.h" // Include for Core::SystemInfo and Core::IdGenerator
#include "src/modules/audit/AuditManager.h" // For AuditManager::instance().log
#include "src/modules/integration/FirebaseRealtimeSyncManager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
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
    Q_UNUSED(config);
    // Retrieve session timeout from SystemManager settings for centralized configuration
    // Never allow a malformed/legacy setting to disable session expiry or create
    // an immediately-expiring session.
    m_timeoutMinutes = qBound(1, Core::SystemManager::instance().settings().sessionTimeoutMinutes, 24 * 60);
    loadLockoutState();
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
        clearLockoutState(normalizedUsername);
    }

    auto recordFailedLogin = [&]() {
        const int attempts = m_failedLoginCounts.value(normalizedUsername, 0) + 1;
        m_failedLoginCounts.insert(normalizedUsername, attempts);
        if (attempts >= maxAttempts) {
            m_lockoutUntil.insert(normalizedUsername, now.addSecs(lockoutMinutes * 60));
            qWarning() << "AuthManager: Temporary lockout applied for user:" << normalizedUsername
                       << "after" << attempts << "failed attempts.";
        }
        persistLockoutState();
    };

    std::optional<Core::User> userOpt = resolveUserForLogin(normalizedUsername);
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
    clearLockoutState(normalizedUsername);
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

std::optional<Core::User> AuthManager::resolveUserForLogin(const QString& normalizedUsername) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        return std::nullopt;
    }

    std::optional<Core::User> localUser = storage->getUserByEmail(normalizedUsername);
    if (localUser || !Core::SystemManager::instance().firebaseSyncEnabled()) {
        return localUser;
    }

    QString error;
    const auto remoteUser = Integration::FirebaseRealtimeSyncManager::instance().fetchUserByEmail(normalizedUsername, &error);
    if (!remoteUser) {
        if (!error.isEmpty()) {
            qWarning() << "AuthManager: Firebase lookup failed for" << normalizedUsername << ":" << error;
        }
        return std::nullopt;
    }

    Core::User hydratedUser;
    hydratedUser.id = remoteUser->id;
    hydratedUser.name = remoteUser->name;
    hydratedUser.email = remoteUser->email;
    hydratedUser.role = remoteUser->role;
    hydratedUser.isActive = remoteUser->isActive;
    hydratedUser.passwordHashAndSalt = remoteUser->passwordHashAndSalt;
    hydratedUser.createdAt = QDateTime::currentDateTime();

    if (!syncRemoteUserToLocal(hydratedUser)) {
        qWarning() << "AuthManager: Failed to mirror Firebase user locally for" << normalizedUsername;
    }

    return hydratedUser;
}

bool AuthManager::syncRemoteUserToLocal(const Core::User& user) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        return false;
    }

    const auto existingUser = storage->getUserByEmail(user.email);
    if (existingUser) {
        Core::User updated = *existingUser;
        updated.id = user.id;
        updated.name = user.name;
        updated.email = user.email;
        updated.role = user.role;
        updated.isActive = user.isActive;
        updated.passwordHashAndSalt = user.passwordHashAndSalt;
        return storage->updateUser(updated) && storage->updateUserPassword(updated.id, updated.passwordHashAndSalt);
    }

    return storage->createUser(user);
}

bool AuthManager::loginByToken(const QString& token) {
    Q_UNUSED(token);
    qWarning() << "AuthManager: loginByToken is not yet implemented.";
    emit loginFailed("Login by token not implemented");
    return false;
}

bool AuthManager::loginByQR(const QByteArray& qrData) {
    Q_UNUSED(qrData);
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
    return changePasswordWithResult(oldPassword, newPassword).success;
}

PasswordChangeResult AuthManager::changePasswordWithResult(const QString& oldPassword, const QString& newPassword) {
    PasswordChangeResult result;
    if (!m_isAuthenticated) {
        qWarning() << "AuthManager: Cannot change password. User not authenticated.";
        result.errorMessage = "You must be logged in to change your password.";
        return result;
    }
    if (newPassword.isEmpty()) {
        qWarning() << "AuthManager: New password cannot be empty.";
        result.errorMessage = "New password cannot be empty.";
        return result;
    }
    if (oldPassword == newPassword) {
        qWarning() << "AuthManager: New password must differ from old password for user" << m_currentUser.id;
        Audit::AuditManager::instance().log(
            Core::AuditAction::UserModified,
            QString("Password change rejected because the new password matched the old password for user: %1").arg(m_currentUser.email),
            m_currentUser.id);
        result.errorMessage = "New password must be different from the current password.";
        return result;
    }

    if (Core::SystemManager::instance().settings().requireStrongPassword) {
        const QString validationError = passwordValidationError(newPassword);
        if (!validationError.isEmpty()) {
            qWarning() << "AuthManager: New password rejected by strength policy for user" << m_currentUser.id << "-" << validationError;
            Audit::AuditManager::instance().log(
                Core::AuditAction::UserModified,
                QString("Password change rejected by strength policy for user: %1").arg(m_currentUser.email),
                m_currentUser.id);
            result.errorMessage = validationError;
            return result;
        }
    }

    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "AuthManager: Storage not available. Cannot change password.";
        result.errorMessage = "Storage is unavailable. Please try again later.";
        return result;
    }

    // Verify old password
    QByteArray salt = m_currentUser.passwordHashAndSalt.left(16);
    QByteArray hash = m_currentUser.passwordHashAndSalt.mid(16);
    if (!Security::HashProvider::verifyArgon2(oldPassword, hash, salt)) {
        qWarning() << "AuthManager: Old password verification failed for user" << m_currentUser.id;
        result.errorMessage = "Current password is incorrect.";
        return result;
    }

    // Generate new hash and salt for the new password
    QByteArray newSalt = Security::HashProvider::generateSalt();
    QByteArray newHash = Security::HashProvider::argon2Hash(newPassword, newSalt);
    QByteArray newPasswordHashAndSalt = newSalt + newHash;

    if (!storage->updateUserPassword(m_currentUser.id, newPasswordHashAndSalt)) {
        qCritical() << "AuthManager: Failed to update password in storage for user" << m_currentUser.id;
        result.errorMessage = "The password could not be updated.";
        return result;
    }

    if (Core::SystemManager::instance().firebaseSyncEnabled()) {
        Core::User syncedUser = m_currentUser;
        syncedUser.passwordHashAndSalt = newPasswordHashAndSalt;
        QString syncError;
        if (!Integration::FirebaseRealtimeSyncManager::instance().syncUser(syncedUser, &syncError)) {
            qWarning() << "AuthManager: Firebase password sync failed for user" << m_currentUser.id << ":" << syncError;
            result.errorMessage = QString("Password updated locally, but Firebase sync failed: %1").arg(syncError);
            return result;
        }
    }

    // Update current user's password hash in memory
    m_currentUser.passwordHashAndSalt = newPasswordHashAndSalt;

    Audit::AuditManager::instance().log(
        Core::AuditAction::SettingsChanged, // Or a more specific password change action
        QString("User password changed for user: %1").arg(m_currentUser.email),
        m_currentUser.id);
    qInfo() << "AuthManager: Password changed successfully for user" << m_currentUser.email;
    result.success = true;
    return result;
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

void AuthManager::loadLockoutState() {
    m_failedLoginCounts.clear();
    m_lockoutUntil.clear();

    const auto settingsOpt = Core::SystemManager::instance().storage()->getSystemSettings();
    if (!settingsOpt || settingsOpt->lockoutStateJson.trimmed().isEmpty()) {
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(settingsOpt->lockoutStateJson.toUtf8());
    if (!document.isObject()) {
        return;
    }

    const QJsonObject root = document.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        const QJsonObject state = it.value().toObject();
        const int attempts = state.value("attempts").toInt(0);
        const QDateTime until = QDateTime::fromString(state.value("until").toString(), Qt::ISODate);
        if (attempts > 0) {
            m_failedLoginCounts.insert(it.key(), attempts);
        }
        if (until.isValid()) {
            m_lockoutUntil.insert(it.key(), until);
        }
    }
}

void AuthManager::persistLockoutState() const {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        return;
    }

    auto settingsOpt = storage->getSystemSettings();
    if (!settingsOpt) {
        return;
    }

    QJsonObject root;
    for (auto it = m_failedLoginCounts.begin(); it != m_failedLoginCounts.end(); ++it) {
        QJsonObject state;
        state["attempts"] = it.value();
        const QDateTime until = m_lockoutUntil.value(it.key());
        if (until.isValid()) {
            state["until"] = until.toString(Qt::ISODate);
        }
        root[it.key()] = state;
    }

    Core::SystemSettings settings = *settingsOpt;
    settings.lockoutStateJson = QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
    if (!storage->updateSystemSettings(settings)) {
        qWarning() << "AuthManager: Failed to persist lockout state.";
    }
}

void AuthManager::clearLockoutState(const QString& normalizedUsername) {
    m_failedLoginCounts.remove(normalizedUsername);
    m_lockoutUntil.remove(normalizedUsername);
    persistLockoutState();
}

} // namespace Ballot::Auth


