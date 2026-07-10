#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QMutex>
#include <optional>
#include "src/core/models/ElectionConfiguration.h"

namespace Ballot::Core {

struct KioskConfiguration {
    // Visual settings
    QString themeId = "default-dark";
    QString accentColor = "#0078d4";
    QString backgroundColor = "#1a1a2e";
    QString fontFamily = "Segoe UI";
    int baseFontSize = 14;
    int headingFontSize = 24;
    double borderRadius = 12.0;
    bool animationsEnabled = true;
    bool reducedMotion = false;
    
    // Workflow settings
    int stepCount = 4;
    bool requirePhotoVerification = true;
    bool allowSkipVerification = false;
    int autoAdvanceDelayMs = 2000;
    bool showProgressIndicator = true;
    bool showStepNumbers = true;
    
    // Candidate display settings
    int candidatesPerRow = 3;
    int candidateCardWidth = 280;
    int candidateCardHeight = 380;
    bool showCandidatePhotos = true;
    bool showCandidateParty = true;
    bool showCandidateManifesto = false;
    bool randomizeCandidateOrder = false;
    bool showCandidateDetailsOnHover = true;
    
    // Voting settings
    bool confirmBeforeVote = true;
    bool requireFinalConfirmation = true;
    int confirmationTimeoutSeconds = 30;
    bool showVoteReceipt = false;
    QString receiptFormat = "text"; // text, qr, pdf
    
    // Input settings
    QStringList enabledAuthMethods = {"AdmissionNumber", "QRCode", "RFID"};
    QString primaryAuthMethod = "AdmissionNumber";
    bool allowManualEntry = true;
    bool autoSubmitOnScan = true;
    int scanDebounceMs = 500;
    
    // Accessibility
    bool highContrastMode = false;
    bool largeTextMode = false;
    bool screenReaderOptimized = false;
    int touchTargetSize = 48; // minimum touch target in pixels
    
    // Branding
    QString institutionName = "Campus Ballot";
    QString institutionLogoPath;
    QString welcomeMessage = "Welcome to Campus Ballot";
    QString waitingMessage = "Please wait for the election session to start.";
    QString votingMessage = "An election is currently in progress.";
    
    // Security
    bool testModeEnabled = false;
    bool auditLoggingEnabled = true;
    int sessionTimeoutSeconds = 300; // 5 minutes
    bool requireAdminUnlock = false;
    
    // Network/Performance
    int statePollIntervalMs = 2000;
    bool enableCaching = true;
    int candidateImageCacheSize = 50;
    
    QJsonObject toJson() const;
    static KioskConfiguration fromJson(const QJsonObject& obj);
    static KioskConfiguration defaultConfig();
    static KioskConfiguration kioskModeConfig();
    static KioskConfiguration accessibilityConfig();
};

class KioskConfigurationManager : public QObject {
    Q_OBJECT
public:
    static KioskConfigurationManager& instance();
    
    bool initialize();
    void shutdown();
    
    const KioskConfiguration& configuration() const { return m_config; }
    bool loadConfiguration(const QString& configId = "default");
    bool saveConfiguration(const QString& configId = "default");
    bool resetToDefaults();
    
    // Per-election configuration overrides
    bool loadElectionOverride(const QString& electionId);
    bool saveElectionOverride(const QString& electionId, const KioskConfiguration& config);
    void clearElectionOverride();
    
    // Theme integration
    void applyTheme(const Core::Models::ThemeConfig& theme);
    
signals:
    void configurationChanged();
    void themeChanged(const QString& themeId);
    
private:
    KioskConfigurationManager();
    ~KioskConfigurationManager() override = default;
    
    QString getConfigPath(const QString& configId) const;
    QString getOverridePath(const QString& electionId) const;
    void ensureConfigDirectory();
    
    KioskConfiguration m_config;
    QString m_configDir;
    QString m_currentConfigId;
    QString m_currentElectionId;
    bool m_hasOverride = false;
    mutable QMutex m_mutex;
    
    // Private copy constructor and assignment operator to prevent copying
    KioskConfigurationManager(const KioskConfigurationManager&) = delete;
    KioskConfigurationManager& operator=(const KioskConfigurationManager&) = delete;
};

} // namespace Ballot::Core
