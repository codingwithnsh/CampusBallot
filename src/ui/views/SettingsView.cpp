#include "SettingsView.h"
#include "src/core/SystemManager.h"
#include "src/modules/audit/AuditManager.h" // For audit logging
#include "src/modules/auth/AuthManager.h"
#include "src/modules/backup/BackupManager.h" // For auto-backup settings
#include "src/ui/components/ToastNotification.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QGroupBox>
#include <QFormLayout>
#include <QFile>
#include <QApplication>
#include <QDebug> // For logging
#include <QMetaEnum> // For QMetaEnum
#include <QSignalBlocker>

namespace Ballot::UI {

SettingsView::SettingsView(QWidget *parent) : QWidget(parent) {
    setupUi();
    loadSettings();
    qDebug() << "SettingsView: Initialized.";
}

/**
 * @brief Sets up the user interface for the settings view.
 */
void SettingsView::setupUi() {
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    auto *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(20);

    // --- Title ---
    auto *title = new QLabel("Settings", this);
    title->setObjectName("title");
    title->setStyleSheet("font-size: 32px; font-weight: 700; color: #e0e0e0;");
    mainLayout->addWidget(title);

    // --- Appearance Group ---
    auto *appearanceGroup = new QGroupBox("Appearance", this);
    appearanceGroup->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; margin-top: 16px; color: #e0e0e0; font-weight: 600; }
        QGroupBox::title { subcontrol-origin: margin; left: 16px; padding: 0 8px; }
    )");
    auto *appearanceLayout = new QFormLayout(appearanceGroup);
    appearanceLayout->setSpacing(12);

    m_themeCombo = new QComboBox(appearanceGroup);
    // Populate theme combo box from ThemeManager enum
    QMetaEnum metaEnum = QMetaEnum::fromType<Core::ThemeManager::Theme>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_themeCombo->addItem(metaEnum.key(i));
    }
    m_themeCombo->setStyleSheet(R"(
        QComboBox { background-color: #1e1e34; color: #e0e0e0; border: 1px solid #3d3d5c; border-radius: 4px; padding: 5px; }
        QComboBox::drop-down { border: 0px; }
        QComboBox::down-arrow { width: 12px; height: 12px; }
        QComboBox QAbstractItemView { background-color: #1e1e34; color: #e0e0e0; selection-background-color: #0078d4; }
    )");
    appearanceLayout->addRow("Theme:", m_themeCombo);
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsView::onThemeChanged);


    m_languageCombo = new QComboBox(appearanceGroup);
    m_languageCombo->addItems({"English", "Spanish", "French", "Arabic", "Hindi"});
    m_languageCombo->setStyleSheet(R"(
        QComboBox { background-color: #1e1e34; color: #e0e0e0; border: 1px solid #3d3d5c; border-radius: 4px; padding: 5px; }
        QComboBox::drop-down { border: 0px; }
        QComboBox::down-arrow { width: 12px; height: 12px; }
        QComboBox QAbstractItemView { background-color: #1e1e34; color: #e0e0e0; selection-background-color: #0078d4; }
    )");
    appearanceLayout->addRow("Language:", m_languageCombo);

    mainLayout->addWidget(appearanceGroup);

    // --- Storage Group ---
    auto *storageGroup = new QGroupBox("Storage", this);
    storageGroup->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; margin-top: 16px; color: #e0e0e0; font-weight: 600; }
        QGroupBox::title { subcontrol-origin: margin; left: 16px; padding: 0 8px; }
    )");
    auto *storageLayout = new QFormLayout(storageGroup);
    storageLayout->setSpacing(12);

    m_storageCombo = new QComboBox(storageGroup);
    m_storageCombo->addItems({"Local Device", "Firebase", "PostgreSQL", "MySQL", "SQL Server", "REST API", "Custom Server"});
    m_storageCombo->setEnabled(false); // Make it non-editable as changing storage dynamically is complex
    m_storageCombo->setStyleSheet(R"(
        QComboBox { background-color: #1e1e34; color: #e0e0e0; border: 1px solid #3d3d5c; border-radius: 4px; padding: 5px; }
        QComboBox::drop-down { border: 0px; }
        QComboBox::down-arrow { width: 12px; height: 12px; }
        QComboBox QAbstractItemView { background-color: #1e1e34; color: #e0e0e0; selection-background-color: #0078d4; }
    )");
    storageLayout->addRow("Storage Provider:", m_storageCombo);

    mainLayout->addWidget(storageGroup);

    // --- Security Group ---
    auto *securityGroup = new QGroupBox("Security", this);
    securityGroup->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; margin-top: 16px; color: #e0e0e0; font-weight: 600; }
        QGroupBox::title { subcontrol-origin: margin; left: 16px; padding: 0 8px; }
    )");
    auto *securityLayout = new QFormLayout(securityGroup);
    securityLayout->setSpacing(12);

    m_encryptionCheck = new QCheckBox("Enable database encryption", securityGroup);
    m_encryptionCheck->setChecked(true);
    m_encryptionCheck->setStyleSheet("color: #e0e0e0; font-size: 14px;");
    securityLayout->addRow("", m_encryptionCheck);

    m_tamperCheck = new QCheckBox("Enable tamper detection", securityGroup);
    m_tamperCheck->setChecked(true);
    m_tamperCheck->setStyleSheet("color: #e0e0e0; font-size: 14px;");
    securityLayout->addRow("", m_tamperCheck);

    m_auditAllCheck = new QCheckBox("Audit all actions", securityGroup);
    m_auditAllCheck->setChecked(true);
    m_auditAllCheck->setStyleSheet("color: #e0e0e0; font-size: 14px;");
    securityLayout->addRow("", m_auditAllCheck);

    m_sessionTimeoutSpin = new QSpinBox(securityGroup);
    m_sessionTimeoutSpin->setRange(1, 480); // 1 minute to 8 hours
    m_sessionTimeoutSpin->setSuffix(" minutes");
    m_sessionTimeoutSpin->setStyleSheet(R"(
        QSpinBox { background-color: #1e1e34; color: #e0e0e0; border: 1px solid #3d3d5c; border-radius: 4px; padding: 5px; }
        QSpinBox::up-button, QSpinBox::down-button { width: 20px; }
    )");
    securityLayout->addRow("Session timeout:", m_sessionTimeoutSpin);

    mainLayout->addWidget(securityGroup);

    // --- Backup Group ---
    auto *backupGroup = new QGroupBox("Backup", this);
    backupGroup->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; margin-top: 16px; color: #e0e0e0; font-weight: 600; }
        QGroupBox::title { subcontrol-origin: margin; left: 16px; padding: 0 8px; }
    )");
    auto *backupLayout = new QFormLayout(backupGroup);
    backupLayout->setSpacing(12);

    m_autoBackupCheck = new QCheckBox("Enable automatic backups", backupGroup);
    m_autoBackupCheck->setChecked(true);
    m_autoBackupCheck->setStyleSheet("color: #e0e0e0; font-size: 14px;");
    backupLayout->addRow("", m_autoBackupCheck);

    m_backupIntervalSpin = new QSpinBox(backupGroup);
    m_backupIntervalSpin->setRange(1, 168); // 1 hour to 7 days
    m_backupIntervalSpin->setSuffix(" hours");
    m_backupIntervalSpin->setValue(24);
    m_backupIntervalSpin->setStyleSheet(R"(
        QSpinBox { background-color: #1e1e34; color: #e0e0e0; border: 1px solid #3d3d5c; border-radius: 4px; padding: 5px; }
        QSpinBox::up-button, QSpinBox::down-button { width: 20px; }
    )");
    backupLayout->addRow("Backup interval:", m_backupIntervalSpin);

    mainLayout->addWidget(backupGroup);

    // --- Save button ---
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    m_saveBtn = new QPushButton("Save Settings", this);
    m_saveBtn->setFixedSize(200, 44);
    m_saveBtn->setStyleSheet(R"(
        QPushButton { background-color: #0078d4; color: white; border: none; border-radius: 8px; font-size: 15px; font-weight: 600; }
        QPushButton:hover { background-color: #1a8ae8; }
        QPushButton:pressed { background-color: #006cbd; }
    )");
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);

    mainLayout->addStretch(); // Pushes content to the top

    scrollArea->setWidget(content);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);

    connect(m_saveBtn, &QPushButton::clicked, this, &SettingsView::saveSettings);
    qDebug() << "SettingsView: UI setup complete.";
}

/**
 * @brief Loads the current system settings into the UI controls.
 */
void SettingsView::loadSettings() {
    qInfo() << "SettingsView: Loading settings...";
    Core::SystemSettings settings = Core::SystemManager::instance().settings();

    // Set current theme in combo box
    QMetaEnum metaEnum = QMetaEnum::fromType<Core::ThemeManager::Theme>();
    QString currentThemeName = QString::fromUtf8(metaEnum.valueToKey(static_cast<int>(Core::ThemeManager::instance().currentTheme())));
    int index = m_themeCombo->findText(currentThemeName);
    if (index != -1) {
        const QSignalBlocker blocker(m_themeCombo);
        m_themeCombo->setCurrentIndex(index);
    }


    // Map language code to display text
    if (settings.language == "en") m_languageCombo->setCurrentText("English");
    else if (settings.language == "es") m_languageCombo->setCurrentText("Spanish");
    else if (settings.language == "fr") m_languageCombo->setCurrentText("French");
    else if (settings.language == "ar") m_languageCombo->setCurrentText("Arabic");
    else if (settings.language == "hi") m_languageCombo->setCurrentText("Hindi");
    else m_languageCombo->setCurrentText("English"); // Default

    // Load storage type
    auto* storage = Core::SystemManager::instance().storage();
    if (storage) {
        QString providerName = storage->providerName();
        if (providerName == "SQLite") m_storageCombo->setCurrentText("Local Device");
        else m_storageCombo->setCurrentText(providerName);
    } else {
        m_storageCombo->setCurrentText("Unknown");
    }

    m_sessionTimeoutSpin->setValue(settings.sessionTimeoutMinutes);
    m_backupIntervalSpin->setValue(settings.backupIntervalHours);
    m_autoBackupCheck->setChecked(settings.autoBackupEnabled);
    m_auditAllCheck->setChecked(settings.auditAllActions);
    m_encryptionCheck->setChecked(settings.encryptionEnabled);
    m_tamperCheck->setChecked(settings.tamperDetection);
    qDebug() << "SettingsView: Settings loaded.";
}

/**
 * @brief Saves the current UI control values to the system settings.
 */
void SettingsView::saveSettings() {
    qInfo() << "SettingsView: Saving settings...";
    Core::SystemSettings settings = Core::SystemManager::instance().settings(); // Get current settings to modify

    // Theme is now saved directly by onThemeChanged slot, no need to save here.
    // settings.theme = m_themeCombo->currentText().toLower();

    // Map display text to language code
    if (m_languageCombo->currentText() == "English") settings.language = "en";
    else if (m_languageCombo->currentText() == "Spanish") settings.language = "es";
    else if (m_languageCombo->currentText() == "French") settings.language = "fr";
    else if (m_languageCombo->currentText() == "Arabic") settings.language = "ar";
    else if (m_languageCombo->currentText() == "Hindi") settings.language = "hi";
    else settings.language = "en"; // Default

    settings.sessionTimeoutMinutes = m_sessionTimeoutSpin->value();
    settings.backupIntervalHours = m_backupIntervalSpin->value();
    settings.autoBackupEnabled = m_autoBackupCheck->isChecked();
    settings.auditAllActions = m_auditAllCheck->isChecked();
    settings.encryptionEnabled = m_encryptionCheck->isChecked();
    settings.tamperDetection = m_tamperCheck->isChecked();

    if (Core::SystemManager::instance().updateSettings(settings)) {
        ToastNotification::show(this, "Settings saved successfully", ToastNotification::Success);
        Audit::AuditManager::instance().log(Core::AuditAction::SettingsChanged, "System settings updated.", Auth::AuthManager::instance().currentUserId());
        qInfo() << "SettingsView: Settings saved successfully.";
        // Trigger a refresh of SystemManager's internal state if necessary
        Core::SystemManager::instance().initialize(QVariantMap()); // Re-initialize to apply new settings
    } else {
        ToastNotification::show(this, "Failed to save settings", ToastNotification::Error);
        Audit::AuditManager::instance().log(Core::AuditAction::SettingsChanged, "Failed to update system settings.", Auth::AuthManager::instance().currentUserId());
        qCritical() << "SettingsView: Failed to save settings.";
    }
}

void SettingsView::onThemeChanged(int index)
{
    QString themeName = m_themeCombo->itemText(index);
    Core::ThemeManager::instance().applyTheme(themeName);
    qInfo() << "SettingsView: Theme changed to" << themeName;
}

} // namespace Ballot::UI
