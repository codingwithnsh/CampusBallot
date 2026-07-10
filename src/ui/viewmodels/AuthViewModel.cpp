#include "AuthViewModel.h"
#include "src/modules/auth/AuthManager.h"
#include "src/modules/auth/RBACManager.h"
#include <QDebug> // For logging

namespace Ballot::ViewModels {

AuthViewModel::AuthViewModel(QObject *parent) : QObject(parent) {
    auto& auth = Auth::AuthManager::instance();
    // Connect to AuthManager's state changes to update UI properties
    connect(&auth, &Auth::AuthManager::authStateChanged, this, [this]() {
        qDebug() << "AuthViewModel: AuthManager::authStateChanged signal received. Re-emitting authStateChanged.";
        emit authStateChanged();
    });
    // Connect to AuthManager's login failure signal to propagate errors to UI
    connect(&auth, &Auth::AuthManager::loginFailed, this, [this](const QString& reason) {
        qWarning() << "AuthViewModel: Login failed with reason:" << reason;
        emit loginError(reason);
    });
    qDebug() << "AuthViewModel: Initialized.";
}

/**
 * @brief Checks if a user is currently authenticated.
 * @return True if authenticated, false otherwise.
 */
bool AuthViewModel::isAuthenticated() const {
    return Auth::AuthManager::instance().isAuthenticated();
}

/**
 * @brief Gets the name of the current authenticated user.
 * @return The user's name, or an empty string if not authenticated.
 */
QString AuthViewModel::currentUser() const {
    if (isAuthenticated()) {
        return Auth::AuthManager::instance().currentUser().name;
    }
    return QString();
}

/**
 * @brief Gets the role of the current authenticated user.
 * @return The user's role as a string, or an empty string if not authenticated.
 */
QString AuthViewModel::currentRole() const {
    if (isAuthenticated()) {
        auto user = Auth::AuthManager::instance().currentUser();
        return Auth::RBACManager::instance().roleToString(user.role);
    }
    return QString();
}

/**
 * @brief Attempts to log in a user.
 * @param email The user's email (username).
 * @param password The user's password.
 * @param authType The type of authentication to use (e.g., "Local", "Firebase").
 */
void AuthViewModel::login(const QString& email, const QString& password, const QString& authType) {
    qInfo() << "AuthViewModel: Login attempt for email:" << email << "with authType:" << authType;
    if (email.isEmpty() || password.isEmpty()) {
        qWarning() << "AuthViewModel: Login failed - email or password empty.";
        emit loginError("Please enter both email and password.");
        return;
    }

    // Currently, AuthManager only supports local authentication.
    // The authType parameter is a placeholder for future expansion with different providers.
    if (authType == "Local") {
        if (!Auth::AuthManager::instance().login(email, password)) {
            // AuthManager::login already emits loginFailed signal, so no need to emit here again
            qDebug() << "AuthViewModel: AuthManager::login returned false for local authentication.";
        } else {
            qInfo() << "AuthViewModel: Local login successful for" << email;
        }
    } else if (authType == "Firebase") {
        qWarning() << "AuthViewModel: Firebase authentication is not yet implemented.";
        emit loginError("Firebase authentication is not yet implemented.");
    } else {
        qWarning() << "AuthViewModel: Unknown authentication type specified:" << authType;
        emit loginError("Unknown authentication type. Please contact support.");
    }
}

/**
 * @brief Logs out the current user.
 */
void AuthViewModel::logout() {
    qInfo() << "AuthViewModel: User logout initiated.";
    Auth::AuthManager::instance().logout();
}

} // namespace Ballot::ViewModels