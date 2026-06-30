#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox> // Include QComboBox header
#include "src/ui/viewmodels/AuthViewModel.h"

namespace Ballot::UI {

class LoginView : public QWidget {
    Q_OBJECT
public:
    explicit LoginView(QWidget *parent = nullptr);
    void setViewModel(ViewModels::AuthViewModel* vm);

signals:
    void loginRequested(const QString& email, const QString& password, const QString& authType); // Modified signal
    void signupRequested();

private:
    void setupUi();

    QLineEdit *m_emailEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_loginButton;
    QPushButton *m_signupButton;
    QLabel *m_errorLabel;
    QComboBox *m_authTypeComboBox; // New QComboBox for auth type
    ViewModels::AuthViewModel* m_viewModel = nullptr;
};

} // namespace Ballot::UI