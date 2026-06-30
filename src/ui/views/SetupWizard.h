#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QButtonGroup>
#include <QRadioButton>
#include <QJsonObject>
#include <QLineEdit> // Include QLineEdit header

namespace Ballot::UI {

class SetupWizard : public QWidget {
    Q_OBJECT
public:
    explicit SetupWizard(QWidget *parent = nullptr);
    bool eventFilter(QObject *obj, QEvent *event) override; // Declare eventFilter

signals:
    void setupCompleted(const QVariantMap& config);

private:
    void setupUi();
    void nextStep();
    void prevStep();
    void handleFinish();
    void updateNavigation();

    QWidget* createWelcomePage();
    QWidget* createStorageSelectionPage();
    QWidget* createFirebaseConfigPage();
    QWidget* createLocalConfigPage();
    QWidget* createAdminAccountPage();
    QWidget* createSummaryPage();

    bool processFirebaseConfig(const QString& filePath);

    QStackedWidget *m_pages;
    QPushButton *m_nextButton;
    QPushButton *m_backButton;
    QLabel *m_stepIndicator;
    QButtonGroup *m_storageGroup;
    QVariantMap m_config;
    int m_currentIndex = 0;

    // Firebase
    QString m_firebaseApiKey;
    QString m_firebaseProjectId;
    QString m_firebaseDbUrl;
    QJsonObject m_firebaseConfig;

    // Admin Account fields
    QLineEdit *m_adminNameEdit = nullptr;
    QLineEdit *m_adminEmailEdit = nullptr;
    QLineEdit *m_adminPasswordEdit = nullptr;
    QLineEdit *m_adminConfirmEdit = nullptr;
};

} // namespace Ballot::UI