#pragma once

#include <QObject>
#include <memory>
#include <QTimer>
#include "IAuthProvider.h"
#include "src/core/Models.h"

namespace Ballot::Auth {

class AuthManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY authStateChanged)
    Q_PROPERTY(Core::UserRole currentRole READ currentRole NOTIFY authStateChanged)
    Q_PROPERTY(QString currentUserId READ currentUserId NOTIFY authStateChanged)

public:
    static AuthManager& instance();

    bool initialize(const QVariantMap& config);
    void registerProvider(IAuthProvider* provider);
    void setActiveProvider(const QString& providerName);

    bool login(const QString& username, const QString& password);
    bool loginByToken(const QString& token);
    bool loginByQR(const QByteArray& qrData);
    void logout();

    bool isAuthenticated() const;
    Core::UserRole currentRole() const;
    QString currentUserId() const;
    Core::User currentUser() const;

    bool hasPermission(const QString& permission) const;
    bool changePassword(const QString& oldPassword, const QString& newPassword);

    QStringList availableProviders() const;
    IAuthProvider* activeProvider() const;

signals:
    void authStateChanged();
    void loginSuccessful(const QString& userId);
    void loginFailed(const QString& reason);
    void logoutOccurred();
    void sessionTimedOut();
    void permissionDenied(const QString& permission);

private:
    AuthManager();
    void startSessionTimer();
    void resetSessionTimer();

    std::vector<std::unique_ptr<IAuthProvider>> m_providers;
    IAuthProvider* m_activeProvider = nullptr;
    Core::User m_currentUser;
    bool m_isAuthenticated = false;
    QTimer* m_sessionTimer;
    int m_timeoutMinutes = 30;
};

} // namespace Ballot::Auth
