#include "AuthManager.h"
#include "RBACManager.h"
#include "src/security/HashProvider.h"
#include "src/core/SystemManager.h"
#include "src/core/Utils.h"
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
            logout();
            emit sessionTimedOut();
        }
    });
}

bool AuthManager::initialize(const QVariantMap& config) {
    m_timeoutMinutes = config.value("session_timeout", 30).toInt();
    return true;
}

void AuthManager::registerProvider(IAuthProvider* provider) {
    // Transfers ownership
}

void AuthManager::setActiveProvider(const QString& providerName) {
    for (auto& p : m_providers) {
        if (p->providerName() == providerName) {
            m_activeProvider = p.get();
            return;
        }
    }
}

bool AuthManager::login(const QString& username, const QString& password) {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        emit loginFailed("Storage not available");
        return false;
    }

    auto user = storage->getUserByEmail(username);
    if (!user) {
        emit loginFailed("Invalid credentials");
        return false;
    }

    if (!user->isActive) {
        emit loginFailed("Account is disabled");
        return false;
    }

    const QByteArray storedHash = user->digitalSignature;
    QByteArray salt = storedHash.left(16);
    QByteArray hash = storedHash.mid(16);

    if (!Security::HashProvider::verifyArgon2(password, hash, salt)) {
        emit loginFailed("Invalid credentials");
        Core::AuditLogEntry log;
        log.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        log.timestamp = QDateTime::currentDateTime();
        log.userId = user->id;
        log.userName = user->name;
        log.action = Core::AuditAction::FailedLogin;
        log.details = "Failed login attempt for " + username;
        log.machineId = Core::Utils::getMachineId();
        storage->logAction(log);
        return false;
    }

    m_currentUser = *user;
    m_isAuthenticated = true;
    startSessionTimer();

    Core::AuditLogEntry log;
    log.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    log.timestamp = QDateTime::currentDateTime();
    log.userId = user->id;
    log.userName = user->name;
    log.action = Core::AuditAction::Login;
    log.details = "User logged in";
    log.machineId = Core::Utils::getMachineId();
    storage->logAction(log);

    emit loginSuccessful(user->id);
    emit authStateChanged();
    return true;
}

bool AuthManager::loginByToken(const QString& token) {
    return false;
}

bool AuthManager::loginByQR(const QByteArray& qrData) {
    return false;
}

void AuthManager::logout() {
    if (m_isAuthenticated) {
        auto* storage = Core::SystemManager::instance().storage();
        if (storage) {
            Core::AuditLogEntry log;
            log.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            log.timestamp = QDateTime::currentDateTime();
            log.userId = m_currentUser.id;
            log.userName = m_currentUser.name;
            log.action = Core::AuditAction::Logout;
            log.details = "User logged out";
            log.machineId = Core::Utils::getMachineId();
            storage->logAction(log);
        }
    }
    m_isAuthenticated = false;
    m_currentUser = {};
    m_sessionTimer->stop();
    emit logoutOccurred();
    emit authStateChanged();
}

bool AuthManager::isAuthenticated() const { return m_isAuthenticated; }
Core::UserRole AuthManager::currentRole() const { return m_currentUser.role; }
QString AuthManager::currentUserId() const { return m_currentUser.id; }
Core::User AuthManager::currentUser() const { return m_currentUser; }

bool AuthManager::hasPermission(const QString& permission) const {
    return RBACManager::instance().hasPermission(m_currentUser.role, permission);
}

bool AuthManager::changePassword(const QString& oldPassword, const QString& newPassword) {
    return false;
}

QStringList AuthManager::availableProviders() const {
    QStringList names;
    for (const auto& p : m_providers) {
        names.append(p->providerName());
    }
    return names;
}

IAuthProvider* AuthManager::activeProvider() const { return m_activeProvider; }

void AuthManager::startSessionTimer() {
    m_sessionTimer->start(m_timeoutMinutes * 60000);
}

void AuthManager::resetSessionTimer() {
    if (m_isAuthenticated) {
        m_sessionTimer->start(m_timeoutMinutes * 60000);
    }
}

} // namespace Ballot::Auth
