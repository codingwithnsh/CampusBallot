#include "AuthViewModel.h"
#include "src/modules/auth/AuthManager.h"
#include "src/modules/auth/RBACManager.h"

namespace Ballot::ViewModels {

AuthViewModel::AuthViewModel(QObject *parent) : QObject(parent) {
    auto& auth = Auth::AuthManager::instance();
    connect(&auth, &Auth::AuthManager::authStateChanged, this, [this]() {
        emit authStateChanged();
    });
    connect(&auth, &Auth::AuthManager::loginFailed, this, [this](const QString& reason) {
        emit loginError(reason);
    });
}

bool AuthViewModel::isAuthenticated() const {
    return Auth::AuthManager::instance().isAuthenticated();
}

QString AuthViewModel::currentUser() const {
    return Auth::AuthManager::instance().currentUser().name;
}

QString AuthViewModel::currentRole() const {
    auto user = Auth::AuthManager::instance().currentUser();
    return Auth::RBACManager::instance().roleToString(user.role);
}

void AuthViewModel::login(const QString& email, const QString& password, const QString& authType) {
    if (email.isEmpty() || password.isEmpty()) {
        emit loginError("Please enter email and password");
        return;
    }

    if (authType == "Local") {
        Auth::AuthManager::instance().login(email, password);
    } else if (authType == "Firebase") {
        // Placeholder for Firebase authentication
        emit loginError("Firebase authentication is not yet implemented.");
    } else {
        emit loginError("Unknown authentication type.");
    }
}

void AuthViewModel::logout() {
    Auth::AuthManager::instance().logout();
}

} // namespace Ballot::ViewModels