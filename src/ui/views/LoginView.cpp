#include "LoginView.h"
#include "src/core/SystemManager.h"
#include "src/modules/audit/AuditManager.h" // For audit logging
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QInputDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QDebug> // For logging

namespace Ballot::UI {

LoginView::LoginView(QWidget *parent) : QWidget(parent) {
    setupUi();
    qDebug() << "LoginView: Initialized.";
}

/**
 * @brief Sets the AuthViewModel for this view.
 * Connects signals from the ViewModel to update the UI.
 * @param vm The AuthViewModel instance.
 */
void LoginView::setViewModel(ViewModels::AuthViewModel* vm) {
    if (m_viewModel == vm) return; // Avoid reconnecting if same VM
    if (m_viewModel) {
        // Disconnect old connections if a ViewModel was already set
        disconnect(m_viewModel, nullptr, this, nullptr);
    }

    m_viewModel = vm;
    if (m_viewModel) {
        connect(m_viewModel, &ViewModels::AuthViewModel::loginError, this, [this](const QString& reason) {
            m_errorLabel->setText(reason);
            m_errorLabel->setVisible(true);
            qWarning() << "LoginView: Displaying login error:" << reason;
        });
        connect(m_viewModel, &ViewModels::AuthViewModel::authStateChanged, this, [this]() {
            // Clear error message on any auth state change (e.g., successful login/logout)
            m_errorLabel->setVisible(false);
            m_passwordEdit->clear(); // Clear password field for security
            qDebug() << "LoginView: Auth state changed. Clearing error and password.";
        });
        qDebug() << "LoginView: ViewModel set and signals connected.";
    } else {
        qWarning() << "LoginView: Attempted to set null ViewModel.";
    }
}

/**
 * @brief Sets up the user interface for the login view.
 */
void LoginView::setupUi() {
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setAlignment(Qt::AlignCenter);
    setObjectName("authView");
    setStyleSheet(R"(
        QWidget#authView {
            background-image: url(:/assets/backgrounds/auth-backdrop.svg);
            background-position: center;
            background-repeat: no-repeat;
            background-color: #111827;
        }
    )");

    auto *loginCard = new QFrame(this);
    loginCard->setObjectName("loginCard");
    loginCard->setMinimumSize(440, 500); // Set a minimum size
    loginCard->setStyleSheet(R"(
        QFrame#loginCard {
            background-color: rgba(18, 27, 46, 238);
            border: 1px solid rgba(148, 163, 184, 80);
            border-radius: 16px;
        }
    )");

    auto *shadow = new QGraphicsDropShadowEffect(loginCard);
    shadow->setBlurRadius(60);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 8);
    loginCard->setGraphicsEffect(shadow);

    auto *layout = new QVBoxLayout(loginCard);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(16);

    // Logo
    auto *logo = new QLabel(loginCard);
    logo->setFixedSize(72, 72);
    logo->setPixmap(QPixmap(":/assets/brand/app-mark.svg").scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logo->setStyleSheet("background: transparent;");
    logo->setAlignment(Qt::AlignCenter);
    layout->addWidget(logo, 0, Qt::AlignCenter);

    layout->addSpacing(8);

    auto *title = new QLabel("Campus Ballot", loginCard);
    title->setStyleSheet("font-size: 24px; font-weight: 700; color: #ffffff; background: transparent;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *subtitle = new QLabel("Secure election administration", loginCard);
    subtitle->setStyleSheet("font-size: 14px; color: #9a9ab0; margin-bottom: 16px; background: transparent;");
    subtitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitle);

    layout->addSpacing(16);

    // Auth Type Selector
    auto *authTypeLabel = new QLabel("Authentication Type", loginCard);
    authTypeLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #e0e0e0; background: transparent;");
    layout->addWidget(authTypeLabel);

    m_authTypeComboBox = new QComboBox(loginCard);
    m_authTypeComboBox->addItem("Local");
    m_authTypeComboBox->addItem("Firebase"); // Placeholder for future implementation
    m_authTypeComboBox->setFixedHeight(44);
    m_authTypeComboBox->setStyleSheet(R"(
        QComboBox {
            background-color: #374151; color: #ffffff; border: 1px solid #4b5563;
            border-radius: 8px; padding: 8px 12px; font-size: 15px; min-height: 24px;
        }
        QComboBox::drop-down { border: 0px; }
        QComboBox::down-arrow { width: 16px; height: 16px; }
        QComboBox:hover { border: 1px solid #6b7280; }
        QComboBox:on { border: 1px solid #38bdf8; }
        QComboBox QAbstractItemView {
            background-color: #374151; color: #ffffff; selection-background-color: #38bdf8;
            border: 1px solid #4b5563; border-radius: 8px;
        }
    )");
    layout->addWidget(m_authTypeComboBox);

    layout->addSpacing(16);

    // Email
    auto *emailLabel = new QLabel("Email", loginCard);
    emailLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #e0e0e0; background: transparent;");
    layout->addWidget(emailLabel);

    m_emailEdit = new QLineEdit(loginCard);
    m_emailEdit->setPlaceholderText("Enter your email");
    m_emailEdit->setFixedHeight(44);
    layout->addWidget(m_emailEdit);

    layout->addSpacing(8);

    // Password
    auto *passLabel = new QLabel("Password", loginCard);
    passLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #e0e0e0; background: transparent;");
    layout->addWidget(passLabel);

    m_passwordEdit = new QLineEdit(loginCard);
    m_passwordEdit->setPlaceholderText("Enter your password");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setFixedHeight(44);
    layout->addWidget(m_passwordEdit);

    auto *recoveryLayout = new QHBoxLayout();
    recoveryLayout->setContentsMargins(0, 0, 0, 0);
    recoveryLayout->setSpacing(8);

    auto *recoverEmailBtn = new QPushButton("Forgot email?", loginCard);
    recoverEmailBtn->setObjectName("linkButton");
    recoverEmailBtn->setCursor(Qt::PointingHandCursor);
    recoverEmailBtn->setFlat(true);

    auto *recoverPasswordBtn = new QPushButton("Forgot password?", loginCard);
    recoverPasswordBtn->setObjectName("linkButton");
    recoverPasswordBtn->setCursor(Qt::PointingHandCursor);
    recoverPasswordBtn->setFlat(true);

    const QString linkStyle = R"(
        QPushButton#linkButton {
            background: transparent; border: none; color: #7dd3fc;
            font-size: 12px; font-weight: 600; padding: 0; min-height: 18px;
        }
        QPushButton#linkButton:hover { color: #bae6fd; text-decoration: underline; }
        QPushButton#linkButton:pressed { color: #38bdf8; }
    )";
    recoverEmailBtn->setStyleSheet(linkStyle);
    recoverPasswordBtn->setStyleSheet(linkStyle);

    recoveryLayout->addWidget(recoverEmailBtn);
    recoveryLayout->addStretch();
    recoveryLayout->addWidget(recoverPasswordBtn);
    layout->addLayout(recoveryLayout);

    // Error label
    m_errorLabel = new QLabel(loginCard);
    m_errorLabel->setStyleSheet("font-size: 13px; color: #f44336; background: transparent;");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setVisible(false);
    layout->addWidget(m_errorLabel);

    layout->addSpacing(16);

    // Login button
    m_loginButton = new QPushButton("Sign In", loginCard);
    m_loginButton->setFixedHeight(44);
    m_loginButton->setStyleSheet(R"(
        QPushButton {
            background-color: #0078d4; color: white; border: none; border-radius: 8px;
            font-size: 15px; font-weight: 600;
        }
        QPushButton:hover { background-color: #1a8ae8; }
        QPushButton:pressed { background-color: #006cbd; }
    )");
    layout->addWidget(m_loginButton);

    // Sign Up button
    m_signupButton = new QPushButton("Sign Up", loginCard);
    m_signupButton->setFixedHeight(44);
    m_signupButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4CAF50; /* Green color for sign up */
            color: white; border: none; border-radius: 8px;
            font-size: 15px; font-weight: 600;
        }
        QPushButton:hover { background-color: #66BB6A; }
        QPushButton:pressed { background-color: #388E3C; }
    )");
    layout->addWidget(m_signupButton);

    // --- Connect Signals ---
    connect(m_loginButton, &QPushButton::clicked, this, [this]() {
        qInfo() << "LoginView: Login button clicked.";
        emit loginRequested(m_emailEdit->text(), m_passwordEdit->text(), m_authTypeComboBox->currentText());
    });

    connect(m_signupButton, &QPushButton::clicked, this, [this]() {
        qInfo() << "LoginView: Signup button clicked.";
        emit signupRequested();
    });

    connect(m_passwordEdit, &QLineEdit::returnPressed, m_loginButton, &QPushButton::click);

    connect(recoverEmailBtn, &QPushButton::clicked, this, [this]() {
        qInfo() << "LoginView: Recover email requested.";
        QMessageBox::information(this,
                                 "Recover Email",
                                 "Please contact your election administrator to recover your email address. They can look up your account using your student ID, employee ID, department, or registered phone number.");
        Audit::AuditManager::instance().log(Core::AuditAction::FailedLogin, "User requested email recovery.", "UI");
    });

    connect(recoverPasswordBtn, &QPushButton::clicked, this, [this]() {
        qInfo() << "LoginView: Recover password requested.";
        bool ok = false;
        const QString email = QInputDialog::getText(this,
                                                    "Recover Password",
                                                    "Enter your registered email:",
                                                    QLineEdit::Normal,
                                                    m_emailEdit->text(),
                                                    &ok).trimmed();
        if (!ok || email.isEmpty()) {
            qDebug() << "LoginView: Password recovery cancelled or empty email provided.";
            return;
        }

        auto* storage = Core::SystemManager::instance().storage();
        if (!storage) {
            qCritical() << "LoginView: Storage not available for password recovery.";
            QMessageBox::warning(this,
                                 "Recover Password",
                                 "Password recovery is currently unavailable because the storage system is not initialized. Please try again later or contact support.");
            Audit::AuditManager::instance().log(Core::AuditAction::FailedLogin, "Password recovery failed: Storage unavailable.", "UI");
            return;
        }

        const auto user = storage->getUserByEmail(email);
        if (!user.has_value()) {
            qWarning() << "LoginView: Password recovery attempt for non-existent email:" << email;
            QMessageBox::information(this,
                                     "Recover Password",
                                     "No active account was found for that email. Please check the spelling or contact your election administrator for assistance.");
            Audit::AuditManager::instance().log(Core::AuditAction::FailedLogin, QString("Password recovery failed: User %1 not found.").arg(email), "UI");
            return;
        }

        qInfo() << "LoginView: Password recovery initiated for user:" << email;
        QMessageBox::information(this,
                                 "Recover Password",
                                 "A password reset for your account (" + email + ") must be completed by an election administrator. Please share this email with them to securely reset your account.");
        Audit::AuditManager::instance().log(Core::AuditAction::FailedLogin, QString("Password recovery initiated for user: %1 (Admin action required).").arg(email), user->id);
    });

    layout->addStretch(); // Pushes content to the top

    outerLayout->addWidget(loginCard, 0, Qt::AlignCenter);
    qDebug() << "LoginView: UI setup complete.";
}

} // namespace Ballot::UI