#include "LoginView.h"
#include "src/core/SystemManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QInputDialog>
#include <QMessageBox>
#include <QPixmap>

namespace Ballot::UI {

LoginView::LoginView(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void LoginView::setViewModel(ViewModels::AuthViewModel* vm) {
    m_viewModel = vm;
    if (vm) {
        connect(vm, &ViewModels::AuthViewModel::loginError, this, [this](const QString& error) {
            m_errorLabel->setText(error);
            m_errorLabel->setVisible(true);
        });
        connect(vm, &ViewModels::AuthViewModel::authStateChanged, this, [this]() {
            m_errorLabel->setVisible(false);
        });
    }
}

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
    // Removed fixed size to allow layout to manage it, or set a minimum size if desired
    loginCard->setMinimumSize(440, 500); // Set a minimum size instead of fixed
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
    m_authTypeComboBox->addItem("Firebase");
    m_authTypeComboBox->setFixedHeight(44); // Consistent height with buttons
    m_authTypeComboBox->setStyleSheet(R"(
        QComboBox {
            background-color: #374151;
            color: #ffffff;
            border: 1px solid #4b5563;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 15px;
            min-height: 24px;
        }
        QComboBox::drop-down {
            border: 0px;
        }
        QComboBox::down-arrow {
            width: 16px;
            height: 16px;
        }
        QComboBox:hover {
            border: 1px solid #6b7280;
        }
        QComboBox:on {
            border: 1px solid #38bdf8;
        }
        QComboBox QAbstractItemView {
            background-color: #374151;
            color: #ffffff;
            selection-background-color: #38bdf8;
            border: 1px solid #4b5563;
            border-radius: 8px;
        }
    )");
    layout->addWidget(m_authTypeComboBox);

    layout->addSpacing(16); // Increased spacing for better separation

    // Email
    auto *emailLabel = new QLabel("Email", loginCard);
    emailLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #e0e0e0; background: transparent;");
    layout->addWidget(emailLabel);

    m_emailEdit = new QLineEdit(loginCard);
    m_emailEdit->setPlaceholderText("Enter your email");
    m_emailEdit->setFixedHeight(44); // Set fixed height
    layout->addWidget(m_emailEdit);

    layout->addSpacing(8);

    // Password
    auto *passLabel = new QLabel("Password", loginCard);
    passLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #e0e0e0; background: transparent;");
    layout->addWidget(passLabel);

    m_passwordEdit = new QLineEdit(loginCard);
    m_passwordEdit->setPlaceholderText("Enter your password");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setFixedHeight(44); // Set fixed height
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
            background: transparent;
            border: none;
            color: #7dd3fc;
            font-size: 12px;
            font-weight: 600;
            padding: 0;
            min-height: 18px;
        }
        QPushButton#linkButton:hover {
            color: #bae6fd;
            text-decoration: underline;
        }
        QPushButton#linkButton:pressed {
            color: #38bdf8;
        }
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
            background-color: #0078d4;
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 15px;
            font-weight: 600;
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
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 15px;
            font-weight: 600;
        }
        QPushButton:hover { background-color: #66BB6A; }
        QPushButton:pressed { background-color: #388E3C; }
    )");
    layout->addWidget(m_signupButton);

    connect(m_loginButton, &QPushButton::clicked, this, [this]() {
        emit loginRequested(m_emailEdit->text(), m_passwordEdit->text(), m_authTypeComboBox->currentText());
    });

    connect(m_signupButton, &QPushButton::clicked, this, [this]() {
        emit signupRequested();
    });

    connect(m_passwordEdit, &QLineEdit::returnPressed, m_loginButton, &QPushButton::click);

    connect(recoverEmailBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this,
                                 "Recover email",
                                 "Ask your election administrator to look up your account using your student ID, employee ID, department, or registered phone number.");
    });

    connect(recoverPasswordBtn, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        const QString email = QInputDialog::getText(this,
                                                    "Recover password",
                                                    "Enter your registered email:",
                                                    QLineEdit::Normal,
                                                    m_emailEdit->text(),
                                                    &ok).trimmed();
        if (!ok || email.isEmpty()) {
            return;
        }

        auto* storage = Core::SystemManager::instance().storage();
        if (!storage) {
            QMessageBox::warning(this,
                                 "Recover password",
                                 "Password recovery is unavailable because storage is not initialized.");
            return;
        }

        const auto user = storage->getUserByEmail(email);
        if (!user.has_value()) {
            QMessageBox::information(this,
                                     "Recover password",
                                     "No active account was found for that email. Check the spelling or contact your election administrator.");
            return;
        }

        QMessageBox::information(this,
                                 "Recover password",
                                 "A password reset must be completed by an election administrator. Share this email with them to reset your account securely.");
    });

    layout->addStretch();

    outerLayout->addWidget(loginCard, 0, Qt::AlignCenter);
}

} // namespace Ballot::UI