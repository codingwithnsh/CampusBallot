#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QSlider>

namespace Ballot::UI {

class SettingsView : public QWidget {
    Q_OBJECT
public:
    explicit SettingsView(QWidget *parent = nullptr);

private:
    void setupUi();
    void loadSettings();
    void saveSettings();

    QComboBox* m_themeCombo;
    QComboBox* m_languageCombo;
    QComboBox* m_storageCombo;
    QSpinBox* m_sessionTimeoutSpin;
    QSpinBox* m_backupIntervalSpin;
    QCheckBox* m_autoBackupCheck;
    QCheckBox* m_auditAllCheck;
    QCheckBox* m_encryptionCheck;
    QCheckBox* m_tamperCheck;
    QPushButton* m_saveBtn;
};

} // namespace Ballot::UI
