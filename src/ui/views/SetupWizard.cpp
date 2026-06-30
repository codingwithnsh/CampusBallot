#include "SetupWizard.h"
#include "src/core/SystemManager.h"
#include "src/ui/components/ToastNotification.h"
#include "src/security/HashProvider.h" // Include HashProvider
#include "src/modules/auth/RBACManager.h" // Include RBACManager for roles
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
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
#include <QUuid> // Include QUuid for user ID

namespace Ballot::UI {

SetupWizard::SetupWizard(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Campus Ballot - Setup Wizard");
    setFixedSize(820, 620);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setupUi();
}

void SetupWizard::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header
    auto *header = new QFrame(this);
    header->setFixedHeight(80);
    header->setStyleSheet("background-color: #1a1a2e; border-bottom: 1px solid #2d2d44;");
    auto *headerLayout = new QVBoxLayout(header);

    auto *title = new QLabel("Setup Wizard", header);
    title->setStyleSheet("font-size: 24px; font-weight: 700; color: #ffffff; background: transparent;");
    headerLayout->addWidget(title);

    m_stepIndicator = new QLabel("Step 1 of 6", header);
    m_stepIndicator->setStyleSheet("font-size: 13px; color: #9a9ab0; background: transparent;");
    headerLayout->addWidget(m_stepIndicator);

    mainLayout->addWidget(header);

    // Pages
    m_pages = new QStackedWidget(this);
    m_pages->setStyleSheet("background-color: #1e1e34;");
    m_pages->addWidget(createWelcomePage());
    m_pages->addWidget(createStorageSelectionPage());
    m_pages->addWidget(createFirebaseConfigPage());
    m_pages->addWidget(createLocalConfigPage());
    m_pages->addWidget(createAdminAccountPage());
    m_pages->addWidget(createSummaryPage());

    mainLayout->addWidget(m_pages, 1);

    // Footer with buttons
    auto *footer = new QFrame(this);
    footer->setFixedHeight(64);
    footer->setStyleSheet("background-color: #1a1a2e; border-top: 1px solid #2d2d44;");
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(20, 10, 20, 10);

    m_backButton = new QPushButton("Back", footer);
    m_backButton->setObjectName("ghostButton");
    m_backButton->setFixedWidth(100);

    footerLayout->addWidget(m_backButton);
    footerLayout->addStretch();

    auto *cancelBtn = new QPushButton("Cancel", footer);
    cancelBtn->setObjectName("ghostButton");
    cancelBtn->setFixedWidth(100);
    connect(cancelBtn, &QPushButton::clicked, this, &QWidget::close);
    footerLayout->addWidget(cancelBtn);

    m_nextButton = new QPushButton("Next", footer);
    m_nextButton->setFixedWidth(120);
    m_nextButton->setStyleSheet("background-color: #0078d4; color: white; font-weight: 600; padding: 10px 24px;");
    footerLayout->addWidget(m_nextButton);

    mainLayout->addWidget(footer);

    // Background
    setStyleSheet(R"(
        SetupWizard {
            background-color: #1a1a2e;
            border: 1px solid #3d3d5c;
            border-radius: 16px;
        }
    )");

    connect(m_nextButton, &QPushButton::clicked, this, &SetupWizard::nextStep);
    connect(m_backButton, &QPushButton::clicked, this, &SetupWizard::prevStep);

    m_backButton->setEnabled(false);
}

void SetupWizard::nextStep() {
    // Page-specific validation and navigation logic
    if (m_currentIndex == 1) { // Storage Selection Page
        int id = m_storageGroup->checkedId();
        if (id < 0) {
            ToastNotification::show(this, "Please select a storage provider", ToastNotification::Warning);
            return;
        }
        QRadioButton* rb = qobject_cast<QRadioButton*>(m_storageGroup->button(id));
        if (rb) {
            QString selected = rb->text();
            if (selected == "Firebase") {
                m_config["storage_type"] = "firebase";
                m_currentIndex = 2; // Go to Firebase Config
            } else { // Local Device, PostgreSQL, MySQL, etc.
                m_config["storage_type"] = selected.toLower().replace(" ", "_");
                m_currentIndex = 3; // Go to Local Config (or other specific config page if implemented)
            }
        }
    } else if (m_currentIndex == 2) { // Firebase Config Page
        if (m_firebaseApiKey.isEmpty() || m_firebaseProjectId.isEmpty()) {
            ToastNotification::show(this, "Please upload a valid Firebase configuration file", ToastNotification::Warning);
            return;
        }
    } else if (m_currentIndex == 4) { // Admin Account Page
        if (!m_adminNameEdit || m_adminNameEdit->text().isEmpty()) {
            ToastNotification::show(this, "Please enter admin name", ToastNotification::Warning);
            return;
        }
        if (!m_adminEmailEdit || m_adminEmailEdit->text().isEmpty()) {
            ToastNotification::show(this, "Please enter admin email", ToastNotification::Warning);
            return;
        }
        if (!m_adminPasswordEdit || m_adminPasswordEdit->text().isEmpty()) {
            ToastNotification::show(this, "Please enter admin password", ToastNotification::Warning);
            return;
        }
        if (!m_adminConfirmEdit || m_adminConfirmEdit->text().isEmpty()) {
            ToastNotification::show(this, "Please confirm admin password", ToastNotification::Warning);
            return;
        }
        if (m_adminPasswordEdit->text() != m_adminConfirmEdit->text()) {
            ToastNotification::show(this, "Passwords do not match", ToastNotification::Warning);
            return;
        }
        if (m_adminPasswordEdit->text().length() < 8) {
            ToastNotification::show(this, "Password must be at least 8 characters long", ToastNotification::Warning);
            return;
        }
    }

    // Default navigation
    if (m_currentIndex < m_pages->count() - 1) {
        m_currentIndex++;
        m_pages->setCurrentIndex(m_currentIndex);
        updateNavigation();
    } else {
        handleFinish();
    }
}

void SetupWizard::prevStep() {
    if (m_currentIndex > 0) {
        // Custom back navigation for skipped pages
        if (m_currentIndex == 2 && m_config.value("storage_type").toString() == "firebase") {
            m_currentIndex = 1; // From Firebase Config back to Storage Selection
        } else if (m_currentIndex == 3 && m_config.value("storage_type").toString() != "firebase") { // Assuming local config is at index 3
            m_currentIndex = 1; // From Local Config back to Storage Selection
        } else {
            m_currentIndex--;
        }
        m_pages->setCurrentIndex(m_currentIndex);
        updateNavigation();
    }
}

void SetupWizard::handleFinish() {
    // Collect admin account details
    if (m_adminNameEdit && m_adminEmailEdit && m_adminPasswordEdit) {
        QString name = m_adminNameEdit->text();
        QString email = m_adminEmailEdit->text();
        QString password = m_adminPasswordEdit->text();

        // Generate salt and hash the password
        QByteArray salt = Security::HashProvider::generateSalt();
        QByteArray hashedPassword = Security::HashProvider::argon2Hash(password, salt);
        QByteArray combinedHash = salt + hashedPassword; // Combine salt and hash

        m_config["admin_name"] = name;
        m_config["admin_email"] = email;
        m_config["admin_password_hash"] = QString(combinedHash.toHex()); // Store combined hash as hex string
    }

    // Ensure db_path is set for local storage if not already set by browse button
    if (m_config.value("storage_type").toString() == "local_device" && !m_config.contains("db_path")) {
        m_config["db_path"] = "ballot.db";
    }

    emit setupCompleted(m_config);
    close();
}

void SetupWizard::updateNavigation() {
    m_backButton->setEnabled(m_currentIndex > 0);
    m_nextButton->setText(m_currentIndex == m_pages->count() - 1 ? "Finish" : "Next");
    // The total number of steps should be dynamic if pages are skipped.
    // For now, let's assume 6 steps as per original design, but this might need adjustment.
    m_stepIndicator->setText(QString("Step %1 of %2").arg(m_currentIndex + 1).arg(m_pages->count()));
}

// ---- Pages ----

QWidget* SetupWizard::createWelcomePage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(60, 40, 60, 40);
    layout->setSpacing(20);

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

QWidget* SetupWizard::createStorageSelectionPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 30, 40, 30);

    auto *title = new QLabel("Where should votes be stored?", page);
    title->setStyleSheet("font-size: 20px; font-weight: 600; color: #ffffff; margin-bottom: 20px; background: transparent;");
    layout->addWidget(title);

    auto *desc = new QLabel("Select a storage provider for election data:", page);
    desc->setStyleSheet("font-size: 14px; color: #9a9ab0; margin-bottom: 20px; background: transparent;");
    layout->addWidget(desc);

    m_storageGroup = new QButtonGroup(page);

    // SQLite is the only storage provider currently implemented by SystemManager.
    QStringList options = {"Local Device"};
    QStringList icons = {"LOCAL"};

    auto *grid = new QGridLayout();
    grid->setSpacing(12);

    for (int i = 0; i < options.size(); ++i) {
        auto *card = new QFrame(page);
        card->setObjectName("storageCard");
        card->setFixedSize(220, 112);
        card->setCursor(Qt::PointingHandCursor);
        card->setStyleSheet(R"(
            QFrame#storageCard {
                background-color: #25253a;
                border: 1px solid #3d3d5c;
                border-radius: 12px;
                padding: 12px;
            }
            QFrame#storageCard:hover {
                border-color: #5a5a7a;
                background-color: #2a2a42;
            }
        )");

        auto *cardLayout = new QVBoxLayout(card);
        auto *iconLabel = new QLabel(icons[i], card);
        iconLabel->setStyleSheet("font-size: 11px; font-weight: 800; color: #5eead4; background: transparent; letter-spacing: 1px;");
        cardLayout->addWidget(iconLabel);

        auto *nameLabel = new QLabel(options[i], card);
        nameLabel->setStyleSheet("font-size: 14px; font-weight: 600; color: #e0e0e0; background: transparent;");
        cardLayout->addWidget(nameLabel);

        auto *rb = new QRadioButton(card);
        rb->setText("");
        rb->setFixedSize(20, 20);
        m_storageGroup->addButton(rb, i);
        cardLayout->addWidget(rb, 0, Qt::AlignRight);

        // Make the whole card clickable by connecting a lambda to the card's mouse press event
        // This is a more robust way than customContextMenuRequested
        card->installEventFilter(this); // Install event filter to capture mouse clicks on the card
        // The eventFilter method will handle the click to toggle the radio button

        grid->addWidget(card, i / 3, i % 3);
    }

    layout->addLayout(grid);
    layout->addStretch();
    return page;
}

// Event filter to make QFrame clickable for radio buttons
bool SetupWizard::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QFrame* card = qobject_cast<QFrame*>(obj);
        if (card && card->objectName() == "storageCard") {
            // Find the radio button within this card
            QRadioButton* rb = card->findChild<QRadioButton*>();
            if (rb) {
                rb->setChecked(true);
                return true; // Event handled
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}


QWidget* SetupWizard::createFirebaseConfigPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 30, 40, 30);

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
            background-color: #25253a;
            border: 2px dashed #3d3d5c;
            border-radius: 16px;
        }
        QFrame#uploadFrame:hover {
            border-color: #0078d4;
            background-color: rgba(0, 120, 212, 0.05);
        }
    )");

    auto *uploadLayout = new QVBoxLayout(uploadFrame);
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

    // Use a QPushButton directly for upload functionality
    auto *uploadButton = new QPushButton("Upload JSON File", uploadFrame);
    uploadButton->setStyleSheet(R"(
        QPushButton {
            background-color: #0078d4;
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 15px;
            font-weight: 600;
            padding: 10px 24px;
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
            } else {
                statusLabel->setText("Invalid Firebase configuration file");
                statusLabel->setStyleSheet("font-size: 13px; color: #f44336; background: transparent;");
            }
        }
    });

    layout->addWidget(uploadFrame);

    // Config display
    auto *configGroup = new QGroupBox("Detected Configuration", page);
    configGroup->setStyleSheet(R"(
        QGroupBox {
            background-color: #1e1e34;
            border: 1px solid #3d3d5c;
            border-radius: 8px;
            margin-top: 12px;
            padding: 16px;
            color: #e0e0e0;
            font-weight: 600;
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

bool SetupWizard::processFirebaseConfig(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull() || !doc.isObject()) return false;

    m_firebaseConfig = doc.object();
    m_firebaseApiKey = m_firebaseConfig["apiKey"].toString();
    m_firebaseProjectId = m_firebaseConfig["project_id"].toString();
    m_firebaseDbUrl = m_firebaseConfig["databaseURL"].toString();

    if (m_firebaseApiKey.isEmpty() || m_firebaseProjectId.isEmpty()) return false;

    m_config["storage_type"] = "firebase";
    m_config["api_key"] = m_firebaseApiKey;
    m_config["project_id"] = m_firebaseProjectId;
    m_config["database_url"] = m_firebaseDbUrl;
    m_config["auth_domain"] = m_firebaseConfig["authDomain"].toString();
    m_config["storage_bucket"] = m_firebaseConfig["storageBucket"].toString();
    m_config["messaging_sender_id"] = m_firebaseConfig["messagingSenderId"].toString();
    m_config["app_id"] = m_firebaseConfig["appId"].toString();

    // Update UI labels
    auto* apiLabel = findChild<QLabel*>("firebaseApiKey");
    auto* projectLabel = findChild<QLabel*>("firebaseProjectId");
    auto* dbLabel = findChild<QLabel*>("firebaseDbUrl");
    if (apiLabel) apiLabel->setText("API Key: " + m_firebaseApiKey.left(20) + "...");
    if (projectLabel) projectLabel->setText("Project ID: " + m_firebaseProjectId);
    if (dbLabel) dbLabel->setText("Database URL: " + (m_firebaseDbUrl.isEmpty() ? "(not set)" : m_firebaseDbUrl));

    return true;
}

QWidget* SetupWizard::createLocalConfigPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 30, 40, 30);

    auto *title = new QLabel("Local Storage Configuration", page);
    title->setStyleSheet("font-size: 20px; font-weight: 600; color: #ffffff; background: transparent;");
    layout->addWidget(title);

    auto *group = new QGroupBox("Database Location", page);
    group->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; margin-top: 16px; color: #e0e0e0; }
    )");

    auto *groupLayout = new QVBoxLayout(group);

    auto *pathLabel = new QLabel("Default location: ballot.db", group);
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
        QString path = QFileDialog::getSaveFileName(this, "Select Database Location",
                                                     "ballot.db", "Database Files (*.db)");
        if (!path.isEmpty()) {
            m_config["db_path"] = path;
            pathLabel->setText("Selected: " + path);
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

QWidget* SetupWizard::createAdminAccountPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 30, 40, 30);

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

    m_adminNameEdit = new QLineEdit(formGroup);
    m_adminNameEdit->setPlaceholderText("Enter full name");
    formLayout->addRow("Name:", m_adminNameEdit);

    m_adminEmailEdit = new QLineEdit(formGroup);
    m_adminEmailEdit->setPlaceholderText("Enter email address");
    formLayout->addRow("Email:", m_adminEmailEdit);

    m_adminPasswordEdit = new QLineEdit(formGroup);
    m_adminPasswordEdit->setPlaceholderText("Enter password (min 8 characters)");
    m_adminPasswordEdit->setEchoMode(QLineEdit::Password);
    formLayout->addRow("Password:", m_adminPasswordEdit);

    m_adminConfirmEdit = new QLineEdit(formGroup);
    m_adminConfirmEdit->setPlaceholderText("Confirm password");
    m_adminConfirmEdit->setEchoMode(QLineEdit::Password);
    formLayout->addRow("Confirm:", m_adminConfirmEdit);

    layout->addWidget(formGroup);
    layout->addStretch();
    return page;
}

QWidget* SetupWizard::createSummaryPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 30, 40, 30);

    auto *title = new QLabel("Setup Complete", page);
    title->setStyleSheet("font-size: 24px; font-weight: 700; color: #ffffff; background: transparent;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *checkIcon = new QLabel("OK", page);
    checkIcon->setStyleSheet("font-size: 64px; color: #4caf50; background: transparent;");
    checkIcon->setAlignment(Qt::AlignCenter);
    layout->addWidget(checkIcon);

    auto *summary = new QLabel("Your system is ready to use.\n"
                                "You can change these settings later in the admin panel.", page);
    summary->setStyleSheet("font-size: 14px; color: #9a9ab0; line-height: 1.6; background: transparent;");
    summary->setAlignment(Qt::AlignCenter);
    summary->setWordWrap(true);
    layout->addWidget(summary);

    layout->addStretch();
    return page;
}

} // namespace Ballot::UI
