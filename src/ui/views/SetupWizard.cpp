#include "SetupWizard.h"
#include "src/core/SystemManager.h"
#include "src/ui/components/ToastNotification.h"
#include "src/modules/security/HashProvider.h" // Include HashProvider
#include "src/modules/auth/RBACManager.h" // Include RBACManager for roles
#include "src/core/Utils.h" // For FileUtil::appDataPath
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager> // For Firebase config validation (if needed)
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QLineEdit>
#include <QFileInfo>
#include <QGroupBox>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QGridLayout>
#include <QFormLayout>
#include <QFile>
#include <QApplication>
#include <QPixmap>
#include <QStyle>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>
#include <QUuid> // Still included for QJsonDocument::Indented or if needed elsewhere
#include <QDebug> // For logging

namespace Ballot::UI {

namespace {

bool isValidEmail(const QString& email) {
    static const QRegularExpression emailPattern(
        R"(^[A-Z0-9._%+\-]+@[A-Z0-9.\-]+\.[A-Z]{2,}$)",
        QRegularExpression::CaseInsensitiveOption);
    return emailPattern.match(email.trimmed()).hasMatch();
}

QString passwordValidationError(const QString& password) {
    if (password.length() < 12) {
        return "Password must be at least 12 characters long";
    }

    const bool hasLower = password.contains(QRegularExpression("[a-z]"));
    const bool hasUpper = password.contains(QRegularExpression("[A-Z]"));
    const bool hasDigit = password.contains(QRegularExpression("[0-9]"));
    const bool hasSymbol = password.contains(QRegularExpression(R"([^A-Za-z0-9])"));

    if (!hasLower || !hasUpper || !hasDigit || !hasSymbol) {
        return "Password must include uppercase, lowercase, number, and symbol characters";
    }

    return {};
}

} // namespace

SetupWizard::SetupWizard(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Campus Ballot - Set Up Wizard");
    setFixedSize(820, 620);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint); // Frameless dialog
    setupUi();
    qDebug() << "SetupWizard: Initialized.";
}

/**
 * @brief Sets up the user interface for the setup wizard.
 */
void SetupWizard::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- Header ---
    auto *header = new QFrame(this);
    header->setFixedHeight(80);
    header->setStyleSheet("background-color: #1a1a2e; border-bottom: 1px solid #2d2d44;");
    auto *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(20, 10, 20, 10);

    auto *title = new QLabel("Set Up Wizard", header);
    title->setStyleSheet("font-size: 24px; font-weight: 700; color: #ffffff; background: transparent;");
    headerLayout->addWidget(title);

    m_stepIndicator = new QLabel("Step 1 of 6", header); // Total steps might be dynamic
    m_stepIndicator->setStyleSheet("font-size: 13px; color: #9a9ab0; background: transparent;");
    headerLayout->addWidget(m_stepIndicator);

    mainLayout->addWidget(header);

    // --- Pages Stack ---
    m_pages = new QStackedWidget(this);
    m_pages->setStyleSheet("background-color: #1e1e34;");
    m_pages->addWidget(createWelcomePage());            // Index 0
    m_pages->addWidget(createStorageSelectionPage());   // Index 1
    m_pages->addWidget(createFirebaseConfigPage());     // Index 2
    m_pages->addWidget(createLocalConfigPage());        // Index 3
    m_pages->addWidget(createAdminAccountPage());       // Index 4
    m_pages->addWidget(createSummaryPage());            // Index 5

    mainLayout->addWidget(m_pages, 1);

    // --- Footer with Navigation Buttons ---
    auto *footer = new QFrame(this);
    footer->setFixedHeight(64);
    footer->setStyleSheet("background-color: #1a1a2e; border-top: 1px solid #2d2d44;");
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(20, 10, 20, 10);

    m_backButton = new QPushButton("Back", footer);
    m_backButton->setObjectName("ghostButton");
    m_backButton->setFixedWidth(100);
    m_backButton->setStyleSheet(R"(
        QPushButton#ghostButton { background-color: transparent; color: #9a9ab0; border: 1px solid #3d3d5c; border-radius: 8px; padding: 8px 16px; font-size: 15px; font-weight: 600; }
        QPushButton#ghostButton:hover { background-color: #2a2a42; color: #e0e0e0; }
        QPushButton#ghostButton:pressed { background-color: #1a1a2e; }
    )");
    footerLayout->addWidget(m_backButton);
    footerLayout->addStretch();

    auto *cancelBtn = new QPushButton("Cancel", footer);
    cancelBtn->setObjectName("ghostButton");
    cancelBtn->setFixedWidth(100);
    cancelBtn->setStyleSheet(R"(
        QPushButton#ghostButton { background-color: transparent; color: #9a9ab0; border: 1px solid #3d3d5c; border-radius: 8px; padding: 8px 16px; font-size: 15px; font-weight: 600; }
        QPushButton#ghostButton:hover { background-color: #2a2a42; color: #e0e0e0; }
        QPushButton#ghostButton:pressed { background-color: #1a1a2e; }
    )");
    connect(cancelBtn, &QPushButton::clicked, this, &QWidget::close);
    footerLayout->addWidget(cancelBtn);

    m_nextButton = new QPushButton("Next", footer);
    m_nextButton->setFixedWidth(120);
    m_nextButton->setStyleSheet(R"(
        QPushButton { background-color: #0078d4; color: white; border: none; border-radius: 8px; font-size: 15px; font-weight: 600; padding: 10px 24px; }
        QPushButton:hover { background-color: #1a8ae8; }
        QPushButton:pressed { background-color: #006cbd; }
    )");
    footerLayout->addWidget(m_nextButton);

    mainLayout->addWidget(footer);

    // --- Overall Styling ---
    setStyleSheet(R"(
        SetupWizard {
            background-color: #1a1a2e;
            border: 1px solid #3d3d5c;
            border-radius: 16px;
        }
        QLineEdit,
        QTextEdit,
        QPlainTextEdit,
        QSpinBox,
        QDoubleSpinBox,
        QDateEdit,
        QTimeEdit,
        QDateTimeEdit,
        QComboBox {
            background-color: #17172a;
            color: #ffffff;
            border: 2px solid #5a5a7a;
            border-radius: 8px;
            padding: 0 12px;
            selection-background-color: #0078d4;
        }
        QLineEdit:hover,
        QTextEdit:hover,
        QPlainTextEdit:hover,
        QSpinBox:hover,
        QDoubleSpinBox:hover,
        QDateEdit:hover,
        QTimeEdit:hover,
        QDateTimeEdit:hover,
        QComboBox:hover {
            border-color: #7a7aa0;
        }
        QLineEdit:focus,
        QTextEdit:focus,
        QPlainTextEdit:focus,
        QSpinBox:focus,
        QDoubleSpinBox:focus,
        QDateEdit:focus,
        QTimeEdit:focus,
        QDateTimeEdit:focus,
        QComboBox:focus {
            border: 2px solid #0078d4;
            background-color: #25253a;
        }
        QLineEdit:disabled,
        QTextEdit:disabled,
        QPlainTextEdit:disabled,
        QSpinBox:disabled,
        QDoubleSpinBox:disabled,
        QDateEdit:disabled,
        QTimeEdit:disabled,
        QDateTimeEdit:disabled,
        QComboBox:disabled {
            color: #6f6f86;
            border-color: #3d3d5c;
            background-color: #19192b;
        }
        QComboBox::drop-down {
            border: none;
            width: 28px;
        }
    )");

    connect(m_nextButton, &QPushButton::clicked, this, &SetupWizard::nextStep);
    connect(m_backButton, &QPushButton::clicked, this, &SetupWizard::prevStep);

    updateNavigation(); // Set initial button states and step indicator
    qDebug() << "SetupWizard: UI setup complete.";
}

/**
 * @brief Advances the wizard to the next step.
 * Performs validation for the current page before proceeding.
 */
void SetupWizard::nextStep() {
    qInfo() << "SetupWizard: Moving to next step from index" << m_currentIndex;
    
    int nextIndex = m_currentIndex + 1;

    // Page-specific validation and navigation logic
    if (m_currentIndex == 1) { // Storage/Auth Selection Page
        int id = m_storageGroup->checkedId();
        if (id < 0) {
            ToastNotification::show(this, "Please select an authentication method", ToastNotification::Warning);
            return;
        }
        if (id == 1) { // Firebase
            m_config["auth_type"] = "firebase";
            m_config["storage_type"] = "firebase";
            nextIndex = 2; // Go to Firebase Config
        } else { // Local
            m_config["auth_type"] = "local";
            m_config["storage_type"] = "sqlite";
            if (!m_config.contains("db_path")) {
                m_config["db_path"] = Core::FileUtil::appDataPath() + "/" + Core::Constants::DB_FILENAME;
            }
            nextIndex = 3; // Go to Local Config
        }
    } else if (m_currentIndex == 2) { // Firebase Config Page
        if (m_firebaseApiKey.isEmpty() || m_firebaseProjectId.isEmpty()) {
            ToastNotification::show(this, "Please upload a valid Firebase configuration file", ToastNotification::Warning);
            qWarning() << "SetupWizard: Firebase config incomplete.";
            return;
        }
        nextIndex = 4; // Skip Local Config page
    } else if (m_currentIndex == 3) { // Local Config Page
        if (!validateLocalConfiguration()) {
            return;
        }
        nextIndex = 4; // Go to Admin Account Page
    } else if (m_currentIndex == 4) { // Admin Account Page
        if (!m_adminNameEdit || m_adminNameEdit->text().isEmpty()) {
            ToastNotification::show(this, "Please enter admin name", ToastNotification::Warning);
            m_adminNameEdit->setFocus();
            return;
        }
        if (!m_adminEmailEdit || m_adminEmailEdit->text().isEmpty()) {
            ToastNotification::show(this, "Please enter admin email", ToastNotification::Warning);
            m_adminEmailEdit->setFocus();
            return;
        }
        if (!isValidEmail(m_adminEmailEdit->text())) {
            ToastNotification::show(this, "Please enter a valid email address", ToastNotification::Warning);
            m_adminEmailEdit->setFocus();
            return;
        }
        if (!m_adminPasswordEdit || m_adminPasswordEdit->text().isEmpty()) {
            ToastNotification::show(this, "Please enter admin password", ToastNotification::Warning);
            m_adminPasswordEdit->setFocus();
            return;
        }
        if (!m_adminConfirmEdit || m_adminConfirmEdit->text().isEmpty()) {
            ToastNotification::show(this, "Please confirm admin password", ToastNotification::Warning);
            m_adminConfirmEdit->setFocus();
            return;
        }
        if (m_adminPasswordEdit->text() != m_adminConfirmEdit->text()) {
            ToastNotification::show(this, "Passwords do not match", ToastNotification::Warning);
            m_adminPasswordEdit->setFocus();
            return;
        }
        const QString passwordError = passwordValidationError(m_adminPasswordEdit->text());
        if (!passwordError.isEmpty()) {
            ToastNotification::show(this, passwordError, ToastNotification::Warning);
            m_adminPasswordEdit->setFocus();
            return;
        }

        // Store admin details in config
        const QByteArray salt = Security::HashProvider::generateSalt();
        const QByteArray hashedPassword = Security::HashProvider::argon2Hash(m_adminPasswordEdit->text(), salt);
        m_config["admin_name"] = m_adminNameEdit->text().trimmed();
        m_config["admin_email"] = m_adminEmailEdit->text().trimmed().toLower();
        m_config["admin_password_hash"] = QString::fromUtf8((salt + hashedPassword).toHex());
        m_config.remove("admin_password");
        m_adminPasswordEdit->clear();
        m_adminConfirmEdit->clear();
        refreshSummary();
        nextIndex = 5;
    }

    if (nextIndex < m_pages->count()) {
        m_currentIndex = nextIndex;
        m_pages->setCurrentIndex(m_currentIndex);
        updateNavigation();
    } else {
        handleFinish();
    }
}

/**
 * @brief Moves the wizard to the previous step.
 * Handles custom back navigation for skipped pages.
 */
void SetupWizard::prevStep() {
    qInfo() << "SetupWizard: Moving to previous step from index" << m_currentIndex;
    if (m_currentIndex > 0) {
        int prevIndex = m_currentIndex - 1;
        
        if (m_currentIndex == 4) { // From Admin Account
            if (m_config.value("storage_type").toString() == "firebase") {
                prevIndex = 2; // Back to Firebase Config
            } else {
                prevIndex = 3; // Back to Local Config
            }
        } else if (m_currentIndex == 2 || m_currentIndex == 3) {
            prevIndex = 1; // Back to Storage Selection
        }
        
        m_currentIndex = prevIndex;
        m_pages->setCurrentIndex(m_currentIndex);
        updateNavigation();
    } else {
        qWarning() << "SetupWizard: Attempted to go before first step.";
    }
}

/**
 * @brief Handles the completion of the wizard.
 * Emits the setupCompleted signal with the collected configuration.
 */
void SetupWizard::handleFinish() {
    qInfo() << "SetupWizard: Setup wizard finished. Emitting setupCompleted signal.";
    emit setupCompleted(m_config);
    close();
}

/**
 * @brief Updates the navigation buttons (Next/Back) and step indicator based on the current page.
 */
void SetupWizard::updateNavigation() {
    m_backButton->setEnabled(m_currentIndex > 0);
    m_nextButton->setText(m_currentIndex == m_pages->count() - 1 ? "Finish" : "Next");

    int totalSteps = 5;
    int currentStepNum = 1;

    if (m_currentIndex == 0) currentStepNum = 1;
    else if (m_currentIndex == 1) currentStepNum = 2;
    else if (m_currentIndex == 2 || m_currentIndex == 3) currentStepNum = 3;
    else if (m_currentIndex == 4) currentStepNum = 4;
    else if (m_currentIndex == 5) currentStepNum = 5;

    m_stepIndicator->setText(QString("Step %1 of %2").arg(currentStepNum).arg(totalSteps));
    qDebug() << "SetupWizard: Navigation updated. Current step:" << currentStepNum << "of" << totalSteps;
}

bool SetupWizard::validateLocalConfiguration() {
    const QString configuredPath = m_config.value("db_path").toString().trimmed();
    const QString fallbackPath = Core::FileUtil::appDataPath() + "/" + Core::Constants::DB_FILENAME;
    const QString dbPath = configuredPath.isEmpty() ? fallbackPath : configuredPath;

    const QFileInfo dbInfo(dbPath);
    const QDir parentDir = dbInfo.dir();
    if (!parentDir.exists() && !QDir().mkpath(parentDir.absolutePath())) {
        ToastNotification::show(this, "The selected database folder could not be created", ToastNotification::Error);
        qWarning() << "SetupWizard: Failed to create database directory:" << parentDir.absolutePath();
        return false;
    }

    if (!dbInfo.fileName().endsWith(".db", Qt::CaseInsensitive)) {
        ToastNotification::show(this, "Please select a database file ending in .db", ToastNotification::Warning);
        qWarning() << "SetupWizard: Invalid database file extension:" << dbPath;
        return false;
    }

    m_config["db_path"] = QDir::toNativeSeparators(dbInfo.absoluteFilePath());
    return true;
}

void SetupWizard::refreshSummary() {
    if (!m_summaryDetails) {
        return;
    }

    const QString storageType = m_config.value("storage_type", "sqlite").toString();
    QStringList lines;
    lines << "Review the setup configuration before finishing."
          << ""
          << QString("Authentication: %1").arg(storageType == "firebase" ? "Firebase" : "Local SQLite")
          << QString("Admin name: %1").arg(m_config.value("admin_name").toString())
          << QString("Admin email: %1").arg(m_config.value("admin_email").toString());

    if (storageType == "firebase") {
        lines << QString("Project ID: %1").arg(m_config.value("project_id").toString())
              << QString("Database URL: %1").arg(m_config.value("database_url").toString().isEmpty() ? "(not set)" : m_config.value("database_url").toString())
              << ""
              << "Note: Firebase was selected, but the current application backend only initializes SQLite storage at startup.";
    } else {
        lines << QString("Database path: %1").arg(m_config.value("db_path").toString());
    }

    lines << ""
          << "The administrator password has been hashed and will not be shown here.";

    m_summaryDetails->setPlainText(lines.join('\n'));
}

// ---- Pages Creation ----

/**
 * @brief Creates the welcome page for the wizard.
 * @return The welcome page widget.
 */
QWidget* SetupWizard::createWelcomePage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(60, 40, 60, 40);
    layout->setSpacing(20);
    layout->setAlignment(Qt::AlignCenter);

    auto *icon = new QLabel(page);
    icon->setFixedSize(88, 88);
    icon->setPixmap(QPixmap(":/assets/brand/app-logo.svg").scaled(88, 88, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    icon->setStyleSheet("background: transparent;");
    icon->setAlignment(Qt::AlignCenter);
    layout->addWidget(icon);

    auto *welcome = new QLabel("Welcome to Campus Ballot", page);
    welcome->setStyleSheet("font-size: 28px; font-weight: 700; color: #ffffff; background: transparent;");
    welcome->setAlignment(Qt::AlignCenter);
    layout->addWidget(welcome);

    auto *desc = new QLabel("This wizard will help you configure your election system.\n"
                            "Choose where to store votes, set up security, and create your first admin account.",
                            page);
    desc->setStyleSheet("font-size: 14px; color: #9a9ab0; line-height: 1.6; background: transparent;");
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    layout->addStretch();
    return page;
}

/**
 * @brief Creates the storage selection page for the wizard.
 * @return The storage selection page widget.
 */
QWidget* SetupWizard::createStorageSelectionPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 30, 40, 30);
    layout->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel("Choose Authentication Method", page);
    title->setStyleSheet("font-size: 20px; font-weight: 600; color: #ffffff; margin-bottom: 20px; background: transparent;");
    layout->addWidget(title);

    auto *desc = new QLabel("Select how you want to authenticate users:", page);
    desc->setStyleSheet("font-size: 14px; color: #9a9ab0; margin-bottom: 20px; background: transparent;");
    layout->addWidget(desc);

    m_storageGroup = new QButtonGroup(page);

    QList<QPair<QString, QString>> storageOptions = {
        {"Local Authentication", "Authenticate users using local database. All data stays on this device."},
        {"Firebase Authentication", "Authenticate users using Firebase. Requires internet connection."}
    };

    auto *grid = new QGridLayout();
    grid->setSpacing(12);
    grid->setAlignment(Qt::AlignCenter);

    for (int i = 0; i < storageOptions.size(); ++i) {
        auto *card = new QFrame(page);
        card->setObjectName("storageCard");
        card->setFixedSize(220, 112);
        card->setCursor(Qt::PointingHandCursor);
        card->setStyleSheet(R"(
            QFrame#storageCard {
                background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 12px;
            }
            QFrame#storageCard:hover {
                border-color: #0078d4; background-color: rgba(0, 120, 212, 0.08);
            }
            QFrame#storageCard[checked="true"] {
                border-color: #0078d4; background-color: rgba(0, 120, 212, 0.15);
            }
        )");
        const bool selectedByDefault = (i == 0);
        card->setProperty("checked", selectedByDefault); // Custom property for QSS

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(0,0,0,0);
        cardLayout->setSpacing(4);

        auto *nameLabel = new QLabel(storageOptions[i].first, card);
        nameLabel->setStyleSheet("font-size: 14px; font-weight: 600; color: #e0e0e0; background: transparent;");
        cardLayout->addWidget(nameLabel);

        auto *descLabel = new QLabel(storageOptions[i].second, card);
        descLabel->setStyleSheet("font-size: 11px; color: #9a9ab0; background: transparent;");
        descLabel->setWordWrap(true);
        cardLayout->addWidget(descLabel);

        auto *rb = new QRadioButton(card);
        rb->setText("");
        rb->setFixedSize(20, 20);
        rb->setStyleSheet("border: none;");
        m_storageGroup->addButton(rb, i);
        rb->setChecked(selectedByDefault);
        cardLayout->addWidget(rb, 0, Qt::AlignRight | Qt::AlignBottom); // Position radio button at bottom right

        // Connect card click to radio button toggle
        card->installEventFilter(this); // Install event filter to capture mouse clicks on the card
        // The eventFilter method will handle the click to toggle the radio button

        grid->addWidget(card, i / 3, i % 3);
    }

    layout->addLayout(grid);
    layout->addStretch();

    m_config["storage_type"] = "sqlite";
    if (!m_config.contains("db_path")) {
        m_config["db_path"] = Core::FileUtil::appDataPath() + "/" + Core::Constants::DB_FILENAME;
    }
    return page;
}

/**
 * @brief Event filter to make QFrame (storageCard) clickable for radio buttons.
 * @param obj The object that received the event.
 * @param event The event that occurred.
 * @return True if the event was handled, false otherwise.
 */
bool SetupWizard::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QFrame* card = qobject_cast<QFrame*>(obj);
        if (card && card->objectName() == "storageCard") {
            QRadioButton* rb = card->findChild<QRadioButton*>();
            if (rb) {
                rb->setChecked(true);
                // Update QSS property for visual feedback
                card->setProperty("checked", true);
                card->style()->polish(card); // Repolish to apply new style
                // Uncheck other cards
                for (QAbstractButton* button : m_storageGroup->buttons()) {
                    if (button != rb) {
                        QFrame* otherCard = qobject_cast<QFrame*>(button->parentWidget());
                        if (otherCard) {
                            otherCard->setProperty("checked", false);
                            otherCard->style()->polish(otherCard);
                        }
                    }
                }
                qDebug() << "SetupWizard: Storage card clicked for:" << rb->parentWidget()->findChild<QLabel*>()->text();
                return true; // Event handled
            }
        }
    }
    return QWidget::eventFilter(obj, event); // Pass unhandled events to base class
}

/**
 * @brief Creates the Firebase configuration page.
 * @return The Firebase configuration page widget.
 */
QWidget* SetupWizard::createFirebaseConfigPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 30, 40, 30);
    layout->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel("Firebase Configuration", page);
    title->setStyleSheet("font-size: 20px; font-weight: 600; color: #ffffff; background: transparent;");
    layout->addWidget(title);

    auto *desc = new QLabel("Upload your Firebase service account JSON file.\n"
                            "The system will automatically extract the required configuration.",
                            page);
    desc->setStyleSheet("font-size: 14px; color: #9a9ab0; line-height: 1.5; background: transparent;");
    desc->setWordWrap(true);
    layout->addWidget(desc);

    layout->addSpacing(20);

    auto *uploadFrame = new QFrame(page);
    uploadFrame->setObjectName("uploadFrame");
    uploadFrame->setFixedHeight(180);
    uploadFrame->setStyleSheet(R"(
        QFrame#uploadFrame {
            background-color: #25253a; border: 2px dashed #3d3d5c; border-radius: 16px;
        }
        QFrame#uploadFrame:hover {
            border-color: #0078d4; background-color: rgba(0, 120, 212, 0.05);
        }
    )");

    auto *uploadLayout = new QVBoxLayout(uploadFrame);
    uploadLayout->setAlignment(Qt::AlignCenter);
    auto *uploadIcon = new QLabel("JSON", uploadFrame);
    uploadIcon->setStyleSheet("font-size: 18px; font-weight: 800; color: #5eead4; background: transparent;");
    uploadLayout->addWidget(uploadIcon, 0, Qt::AlignCenter);

    auto *uploadText = new QLabel("Click to upload Firebase JSON config", uploadFrame);
    uploadText->setStyleSheet("font-size: 14px; color: #9a9ab0; background: transparent;");
    uploadText->setAlignment(Qt::AlignCenter);
    uploadLayout->addWidget(uploadText);

    auto *statusLabel = new QLabel("", uploadFrame);
    statusLabel->setObjectName("firebaseStatus");
    statusLabel->setStyleSheet("font-size: 13px; color: #4caf50; background: transparent;");
    statusLabel->setAlignment(Qt::AlignCenter);
    uploadLayout->addWidget(statusLabel);

    auto *uploadButton = new QPushButton("Upload JSON File", uploadFrame);
    uploadButton->setStyleSheet(R"(
        QPushButton {
            background-color: #0078d4; color: white; border: none; border-radius: 8px;
            font-size: 15px; font-weight: 600; padding: 10px 24px;
        }
        QPushButton:hover { background-color: #1a8ae8; }
        QPushButton:pressed { background-color: #006cbd; }
    )");
    uploadLayout->addWidget(uploadButton, 0, Qt::AlignCenter);


    connect(uploadButton, &QPushButton::clicked, this, [this, statusLabel]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Firebase Config", "", "JSON Files (*.json)");
        if (!path.isEmpty()) {
            if (processFirebaseConfig(path)) {
                statusLabel->setText("Configuration loaded successfully: " + QFileInfo(path).fileName());
                statusLabel->setStyleSheet("font-size: 13px; color: #4caf50; background: transparent;");
                qInfo() << "SetupWizard: Firebase config loaded from" << path;
            } else {
                statusLabel->setText("Invalid Firebase configuration file");
                statusLabel->setStyleSheet("font-size: 13px; color: #f44336; background: transparent;");
                qWarning() << "SetupWizard: Invalid Firebase config file:" << path;
            }
        } else {
            qDebug() << "SetupWizard: Firebase config upload cancelled.";
        }
    });

    layout->addWidget(uploadFrame);

    // Config display
    auto *configGroup = new QGroupBox("Detected Configuration", page);
    configGroup->setStyleSheet(R"(
        QGroupBox {
            background-color: #1e1e34; border: 1px solid #3d3d5c; border-radius: 8px;
            margin-top: 12px; padding: 16px; color: #e0e0e0; font-weight: 600;
        }
    )");

    auto *configLayout = new QVBoxLayout(configGroup);
    auto *apiLabel = new QLabel("API Key: -", configGroup);
    auto *projectLabel = new QLabel("Project ID: -", configGroup);
    auto *dbLabel = new QLabel("Database URL: -", configGroup);
    apiLabel->setObjectName("firebaseApiKey");
    projectLabel->setObjectName("firebaseProjectId");
    dbLabel->setObjectName("firebaseDbUrl");
    QString style = "font-size: 13px; color: #9a9ab0; padding: 4px 0; background: transparent;";
    apiLabel->setStyleSheet(style);
    projectLabel->setStyleSheet(style);
    dbLabel->setStyleSheet(style);
    configLayout->addWidget(apiLabel);
    configLayout->addWidget(projectLabel);
    configLayout->addWidget(dbLabel);
    layout->addWidget(configGroup);

    layout->addStretch();
    return page;
}

/**
 * @brief Processes the uploaded Firebase configuration JSON file.
 * @param filePath The path to the Firebase JSON file.
 * @return True if the configuration was successfully parsed, false otherwise.
 */
bool SetupWizard::processFirebaseConfig(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "SetupWizard: Failed to open Firebase config file:" << filePath << "-" << file.errorString();
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qCritical() << "SetupWizard: Invalid JSON in Firebase config file:" << filePath << "-" << parseError.errorString();
        return false;
    }

    m_firebaseConfig = doc.object();
    // Extract relevant fields (these might vary based on Firebase project setup)
    m_firebaseApiKey = m_firebaseConfig["api_key"].toString(); // Assuming 'api_key' or 'apiKey'
    if (m_firebaseApiKey.isEmpty()) m_firebaseApiKey = m_firebaseConfig["apiKey"].toString();

    m_firebaseProjectId = m_firebaseConfig["project_id"].toString();
    if (m_firebaseProjectId.isEmpty()) m_firebaseProjectId = m_firebaseConfig["projectId"].toString();

    m_firebaseDbUrl = m_firebaseConfig["databaseURL"].toString();

    if (m_firebaseApiKey.isEmpty() || m_firebaseProjectId.isEmpty()) {
        qWarning() << "SetupWizard: Missing API Key or Project ID in Firebase config.";
        return false;
    }

    m_config["storage_type"] = "firebase";
    m_config["api_key"] = m_firebaseApiKey;
    m_config["project_id"] = m_firebaseProjectId;
    m_config["database_url"] = m_firebaseDbUrl;
    // Add other Firebase specific configs if needed
    m_config["auth_domain"] = m_firebaseConfig["authDomain"].toString();
    m_config["storage_bucket"] = m_firebaseConfig["storageBucket"].toString();
    m_config["messaging_sender_id"] = m_firebaseConfig["messagingSenderId"].toString();
    m_config["app_id"] = m_firebaseConfig["appId"].toString();

    // Update UI labels
    auto* apiLabel = findChild<QLabel*>("firebaseApiKey");
    auto* projectLabel = findChild<QLabel*>("firebaseProjectId");
    auto* dbLabel = findChild<QLabel*>("firebaseDbUrl");
    if (apiLabel) apiLabel->setText("API Key: " + m_firebaseApiKey.left(5) + "..." + m_firebaseApiKey.right(5)); // Show partial key
    if (projectLabel) projectLabel->setText("Project ID: " + m_firebaseProjectId);
    if (dbLabel) dbLabel->setText("Database URL: " + (m_firebaseDbUrl.isEmpty() ? "(not set)" : m_firebaseDbUrl));

    qInfo() << "SetupWizard: Firebase configuration processed successfully.";
    return true;
}

/**
 * @brief Creates the local storage configuration page.
 * @return The local storage configuration page widget.
 */
QWidget* SetupWizard::createLocalConfigPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 30, 40, 30);
    layout->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel("Local Storage Configuration", page);
    title->setStyleSheet("font-size: 20px; font-weight: 600; color: #ffffff; background: transparent;");
    layout->addWidget(title);

    auto *group = new QGroupBox("Database Location", page);
    group->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; margin-top: 16px; color: #e0e0e0; }
    )");

    auto *groupLayout = new QVBoxLayout(group);

    auto *pathLabel = new QLabel("Default location: " + Core::FileUtil::appDataPath() + "/" + Core::Constants::DB_FILENAME, group);
    pathLabel->setStyleSheet("font-size: 14px; color: #9a9ab0; padding: 8px 0; background: transparent;");
    groupLayout->addWidget(pathLabel);

    auto *btnLayout = new QHBoxLayout();
    auto *browseBtn = new QPushButton("Browse...", group);
    browseBtn->setObjectName("ghostButton");
    browseBtn->setFixedWidth(120);
    btnLayout->addWidget(browseBtn);
    btnLayout->addStretch();
    groupLayout->addLayout(btnLayout);

    connect(browseBtn, &QPushButton::clicked, [this, pathLabel]() {
        QString defaultPath = Core::FileUtil::appDataPath() + "/" + Core::Constants::DB_FILENAME;
        QString path = QFileDialog::getSaveFileName(this, "Select Database Location",
                                                     defaultPath, "Database Files (*.db)");
        if (!path.isEmpty()) {
            m_config["db_path"] = path;
            pathLabel->setText("Selected: " + path);
            qInfo() << "SetupWizard: Local DB path set to:" << path;
        } else {
            qDebug() << "SetupWizard: Local DB path selection cancelled.";
        }
    });

    layout->addWidget(group);

    // Storage info
    auto *infoGroup = new QGroupBox("Storage Details", page);
    infoGroup->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; margin-top: 16px; color: #e0e0e0; }
    )");
    auto *infoLayout = new QVBoxLayout(infoGroup);
    auto *infoLabel = new QLabel("SQLite database stored on this device.\n"
                                  "- All data stays local\n"
                                  "- No internet required\n"
                                  "- Encrypted storage\n"
                                  "- Automatic backups available", infoGroup);
    infoLabel->setStyleSheet("font-size: 13px; color: #9a9ab0; line-height: 1.6; background: transparent;");
    infoLayout->addWidget(infoLabel);
    layout->addWidget(infoGroup);

    layout->addStretch();
    return page;
}

/**
 * @brief Creates the admin account creation page.
 * @return The admin account creation page widget.
 */
QWidget* SetupWizard::createAdminAccountPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 30, 40, 30);
    layout->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel("Create Admin Account", page);
    title->setStyleSheet("font-size: 20px; font-weight: 600; color: #ffffff; background: transparent;");
    layout->addWidget(title);

    auto *desc = new QLabel("Create the first Super Administrator account:", page);
    desc->setStyleSheet("font-size: 14px; color: #9a9ab0; margin-bottom: 20px; background: transparent;");
    layout->addWidget(desc);

    auto *formGroup = new QGroupBox("Account Details", page);
    formGroup->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 24px; margin-top: 16px; color: #e0e0e0; }
    )");

    auto *formLayout = new QFormLayout(formGroup);
    formLayout->setSpacing(16);
    formLayout->setLabelAlignment(Qt::AlignLeft);
    formLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    auto styleLabel = [](QLabel* label) {
        label->setStyleSheet("font-size: 14px; font-weight: 500; color: #e0e0e0; background: transparent;");
    };

    m_adminNameEdit = new QLineEdit(formGroup);
    m_adminNameEdit->setPlaceholderText("Enter full name");
    m_adminNameEdit->setFixedHeight(44);
    auto* nameLabel = new QLabel("Name *");
    styleLabel(nameLabel);
    formLayout->addRow(nameLabel, m_adminNameEdit);

    m_adminEmailEdit = new QLineEdit(formGroup);
    m_adminEmailEdit->setPlaceholderText("Enter email address");
    m_adminEmailEdit->setFixedHeight(44);
    auto* emailLabel = new QLabel("Email *");
    styleLabel(emailLabel);
    formLayout->addRow(emailLabel, m_adminEmailEdit);

    m_adminPasswordEdit = new QLineEdit(formGroup);
    m_adminPasswordEdit->setPlaceholderText("Minimum 12 chars with uppercase, lowercase, number, and symbol");
    m_adminPasswordEdit->setEchoMode(QLineEdit::Password);
    m_adminPasswordEdit->setFixedHeight(44);
    auto* passwordLabel = new QLabel("Password *");
    styleLabel(passwordLabel);
    formLayout->addRow(passwordLabel, m_adminPasswordEdit);

    m_adminConfirmEdit = new QLineEdit(formGroup);
    m_adminConfirmEdit->setPlaceholderText("Confirm password");
    m_adminConfirmEdit->setEchoMode(QLineEdit::Password);
    m_adminConfirmEdit->setFixedHeight(44);
    auto* confirmLabel = new QLabel("Confirm Password *");
    styleLabel(confirmLabel);
    formLayout->addRow(confirmLabel, m_adminConfirmEdit);

    auto* passwordHint = new QLabel("Use at least 12 characters, including uppercase, lowercase, number, and symbol.", formGroup);
    passwordHint->setWordWrap(true);
    passwordHint->setStyleSheet("font-size: 12px; color: #9a9ab0; background: transparent;");
    formLayout->addRow("", passwordHint);

    layout->addWidget(formGroup);
    layout->addStretch();
    return page;
}

/**
 * @brief Creates the summary page for the wizard.
 * @return The summary page widget.
 */
QWidget* SetupWizard::createSummaryPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 30, 40, 30);
    layout->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel("Setup Complete", page);
    title->setStyleSheet("font-size: 24px; font-weight: 700; color: #ffffff; background: transparent;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *checkIcon = new QLabel("Ready", page);
    checkIcon->setStyleSheet("font-size: 28px; font-weight: 800; color: #4caf50; background: transparent;");
    checkIcon->setAlignment(Qt::AlignCenter);
    layout->addWidget(checkIcon);

    auto *summary = new QLabel("Review the configuration below before finishing setup.", page);
    summary->setStyleSheet("font-size: 14px; color: #9a9ab0; line-height: 1.6; background: transparent;");
    summary->setAlignment(Qt::AlignCenter);
    summary->setWordWrap(true);
    layout->addWidget(summary);

    m_summaryDetails = new QPlainTextEdit(page);
    m_summaryDetails->setReadOnly(true);
    m_summaryDetails->setMinimumHeight(220);
    m_summaryDetails->setStyleSheet(R"(
        QPlainTextEdit {
            background-color: #17172a;
            color: #e0e0e0;
            border: 1px solid #3d3d5c;
            border-radius: 12px;
            padding: 16px;
            font-size: 13px;
        }
    )");
    layout->addWidget(m_summaryDetails);

    refreshSummary();

    layout->addStretch();
    return page;
}

} // namespace Ballot::UI
