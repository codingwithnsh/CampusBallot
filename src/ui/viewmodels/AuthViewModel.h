#pragma once

#include <QObject>
#include <QString>
#include "src/core/Models.h"

namespace Ballot::ViewModels {

class AuthViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY authStateChanged)
    Q_PROPERTY(QString currentUser READ currentUser NOTIFY authStateChanged)
    Q_PROPERTY(QString currentRole READ currentRole NOTIFY authStateChanged)

public:
    explicit AuthViewModel(QObject *parent = nullptr);

    bool isAuthenticated() const;
    QString currentUser() const;
    QString currentRole() const;

    void login(const QString& email, const QString& password);
    void logout();

signals:
    void authStateChanged();
    void loginError(const QString& error);

private:
    QString m_userName;
    QString m_userRole;
};

} // namespace Ballot::ViewModels
