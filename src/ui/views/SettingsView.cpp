#include "SettingsView.h"
#include "src/core/SystemManager.h"
#include "src/ui/components/ToastNotification.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QGroupBox>
#include <QFormLayout>
#include <QFile>
#include <QApplication>

namespace Ballot::UI {

SettingsView::SettingsView(QWidget *parent) : QWidget(parent) {
    setupUi();
    loadSettings();
}

void SettingsView::setupUi() {
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    auto *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(20);

    auto *title = new QLabel("Settings");
    title->setObjectName("title");
    mainLayout->addWidget(title);

    // Appearance
    auto *appearanceGroup = new QGroupBox("Appearance");
    appearanceGroup->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; margin-top: 16px; color: #e0e0e0; }
    )");
    auto *appearanceLayout = new QFormLayout(appearanceGroup);
    appearanceLayout->setSpacing(12);

    m_themeCombo = new QComboBox(appearanceGroup);
    m_themeCombo->addItems({"Dark", "Light"});
    appearanceLayout->addRow("Theme:", m_themeCombo);

    m_languageCombo = new QComboBox(appearanceGroup);
    m_languageCombo->addItems({"English", "Spanish", "French", "Arabic", "Hindi"});
    appearanceLayout->addRow("Language:", m_languageCombo);

    mainLayout->addWidget(appearanceGroup);

    // Storage
    auto *storageGroup = new QGroupBox("Storage");
    storageGroup->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; margin-top: 16px; color: #e0e0e0; }
    )");
    auto *storageLayout = new QFormLayout(storageGroup);
    storageLayout->setSpacing(12);

    m_storageCombo = new QComboBox(storageGroup);
    m_storageCombo->addItems({"Local Device", "Firebase", "PostgreSQL", "MySQL", "SQL Server", "REST API", "Custom Server"});
    m_storageCombo->setEnabled(false); // Make it non-editable as changing storage dynamically is complex
    storageLayout->addRow("Storage Provider:", m_storageCombo);

    mainLayout->addWidget(storageGroup);

    // Security
    auto *securityGroup = new QGroupBox("Security");
    securityGroup->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; margin-top: 16px; color: #e0e0e0; }
    )");
    auto *securityLayout = new QFormLayout(securityGroup);
    securityLayout->setSpacing(12);

    m_encryptionCheck = new QCheckBox("Enable database encryption");
    m_encryptionCheck->setChecked(true);
    securityLayout->addRow("", m_encryptionCheck);

    m_tamperCheck = new QCheckBox("Enable tamper detection");
    m_tamperCheck->setChecked(true);
    securityLayout->addRow("", m_tamperCheck);

    m_auditAllCheck = new QCheckBox("Audit all actions");
    m_auditAllCheck->setChecked(true);
    securityLayout->addRow("", m_auditAllCheck);

    m_sessionTimeoutSpin = new QSpinBox(securityGroup);
    m_sessionTimeoutSpin->setRange(1, 480);
    m_sessionTimeoutSpin->setSuffix(" minutes");
    securityLayout->addRow("Session timeout:", m_sessionTimeoutSpin);

    mainLayout->addWidget(securityGroup);

    // Backup
    auto *backupGroup = new QGroupBox("Backup");
    backupGroup->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; margin-top: 16px; color: #e0e0e0; }
    )");
    auto *backupLayout = new QFormLayout(backupGroup);
    backupLayout->setSpacing(12);

    m_autoBackupCheck = new QCheckBox("Enable automatic backups");
    m_autoBackupCheck->setChecked(true);
    backupLayout->addRow("", m_autoBackupCheck);

    m_backupIntervalSpin = new QSpinBox(backupGroup);
    m_backupIntervalSpin->setRange(1, 168);
    m_backupIntervalSpin->setSuffix(" hours");
    m_backupIntervalSpin->setValue(24);
    backupLayout->addRow("Backup interval:", m_backupIntervalSpin);

    mainLayout->addWidget(backupGroup);

    // Save button
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    m_saveBtn = new QPushButton("Save Settings");
    m_saveBtn->setFixedSize(200, 44);
    m_saveBtn->setStyleSheet("font-weight: 600; font-size: 15px;");
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);

    mainLayout->addStretch();

    scrollArea->setWidget(content);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);

    connect(m_saveBtn, &QPushButton::clicked, this, &SettingsView::saveSettings);
}

void SettingsView::loadSettings() {
    Core::SystemSettings settings = Core::SystemManager::instance().settings();
    m_themeCombo->setCurrentText(settings.theme == "dark" ? "Dark" : "Light");

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
}

void SettingsView::saveSettings() {
    Core::SystemSettings settings = Core::SystemManager::instance().settings(); // Get current settings to modify

    settings.theme = m_themeCombo->currentText().toLower();

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
    } else {
        ToastNotification::show(this, "Failed to save settings", ToastNotification::Error);
    }
}

} // namespace Ballot::UI
