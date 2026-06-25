#pragma once

#include <QString>
#include <QVariantMap>
#include <optional>
#include "src/core/Models.h"

namespace Ballot::Auth {

class IAuthProvider {
public:
    virtual ~IAuthProvider() = default;

    virtual QString providerName() const = 0;
    virtual bool initialize(const QVariantMap& config) = 0;
    virtual bool isInitialized() const = 0;

    virtual std::optional<Core::User> authenticate(const QString& username, const QString& password) = 0;
    virtual std::optional<Core::User> authenticateByToken(const QString& token) = 0;
    virtual std::optional<Core::User> authenticateByQR(const QByteArray& qrData) = 0;
    virtual std::optional<Core::User> authenticateByBiometric(const QByteArray& biometricData) = 0;

    virtual bool changePassword(const QString& userId, const QString& oldPassword, const QString& newPassword) = 0;
    virtual bool resetPassword(const QString& userId, const QString& newPassword) = 0;
    virtual bool validateToken(const QString& token) = 0;
    virtual QString generateToken(const QString& userId) = 0;
    virtual bool revokeToken(const QString& token) = 0;

    virtual bool hasPermission(const QString& userId, const QString& permission) = 0;
    virtual QStringList getPermissions(const QString& userId) = 0;
    virtual Core::UserRole getUserRole(const QString& userId) = 0;
};

} // namespace Ballot::Auth
