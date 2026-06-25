#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "src/ui/viewmodels/AuthViewModel.h"

namespace Ballot::UI {

class LoginView : public QWidget {
    Q_OBJECT
public:
    explicit LoginView(QWidget *parent = nullptr);
    void setViewModel(ViewModels::AuthViewModel* vm);

signals:
    void loginRequested(const QString& email, const QString& password);

private:
    void setupUi();

    QLineEdit *m_emailEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_loginButton;
    QLabel *m_errorLabel;
    ViewModels::AuthViewModel* m_viewModel = nullptr;
};

} // namespace Ballot::UI
