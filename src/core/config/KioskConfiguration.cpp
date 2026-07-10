#include "KioskConfiguration.h"
#include <QDebug>
#include <QCoreApplication>

namespace Ballot::Core {

KioskConfigurationManager& KioskConfigurationManager::instance() {
    static KioskConfigurationManager instance;
    return instance;
}

KioskConfigurationManager::KioskConfigurationManager()
    : m_config(KioskConfiguration::defaultConfig())
    , m_currentConfigId("default")
{
}

bool KioskConfigurationManager::initialize() {
    ensureConfigDirectory();
    return loadConfiguration();
}

void KioskConfigurationManager::shutdown() {
    saveConfiguration();
}

void KioskConfigurationManager::ensureConfigDirectory() {
    m_configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/kiosk";
    QDir dir;
    if (!dir.exists(m_configDir)) {
        dir.mkpath(m_configDir);
    }
}

QString KioskConfigurationManager::getConfigPath(const QString& configId) const {
    return m_configDir + "/" + configId + ".json";
}

QString KioskConfigurationManager::getOverridePath(const QString& electionId) const {
    return m_configDir + "/overrides/" + electionId + ".json";
}

bool KioskConfigurationManager::loadConfiguration(const QString& configId) {
    QMutexLocker locker(&m_mutex);
    
    QString path = getConfigPath(configId);
    QFile file(path);
    
    if (!file.exists()) {
        qInfo() << "KioskConfigurationManager: Config file not found, using defaults:" << path;
        m_config = KioskConfiguration::defaultConfig();
        m_currentConfigId = configId;
        return saveConfiguration(configId);
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "KioskConfigurationManager: Failed to open config file:" << path;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "KioskConfigurationManager: JSON parse error:" << error.errorString();
        return false;
    }
    
    m_config = KioskConfiguration::fromJson(doc.object());
    m_currentConfigId = configId;
    m_hasOverride = false;
    
    emit configurationChanged();
    qInfo() << "KioskConfigurationManager: Loaded configuration:" << configId;
    return true;
}

bool KioskConfigurationManager::saveConfiguration(const QString& configId) {
    QMutexLocker locker(&m_mutex);
    
    QString path = getConfigPath(configId);
    QFile file(path);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "KioskConfigurationManager: Failed to open config file for writing:" << path;
        return false;
    }
    
    QJsonDocument doc(m_config.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    qInfo() << "KioskConfigurationManager: Saved configuration:" << configId;
    return true;
}

bool KioskConfigurationManager::resetToDefaults() {
    QMutexLocker locker(&m_mutex);
    m_config = KioskConfiguration::defaultConfig();
    m_hasOverride = false;
    emit configurationChanged();
    return saveConfiguration();
}

bool KioskConfigurationManager::loadElectionOverride(const QString& electionId) {
    QMutexLocker locker(&m_mutex);
    
    QString path = getOverridePath(electionId);
    QFile file(path);
    
    if (!file.exists()) {
        m_hasOverride = false;
        m_currentElectionId.clear();
        emit configurationChanged();
        return true;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "KioskConfigurationManager: Failed to open override file:" << path;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "KioskConfigurationManager: JSON parse error in override:" << error.errorString();
        return false;
    }
    
    KioskConfiguration override = KioskConfiguration::fromJson(doc.object());
    m_config = override;
    m_currentElectionId = electionId;
    m_hasOverride = true;
    
    emit configurationChanged();
    qInfo() << "KioskConfigurationManager: Loaded election override for:" << electionId;
    return true;
}

bool KioskConfigurationManager::saveElectionOverride(const QString& electionId, const KioskConfiguration& config) {
    QMutexLocker locker(&m_mutex);
    
    QString overrideDir = m_configDir + "/overrides";
    QDir dir;
    if (!dir.exists(overrideDir)) {
        dir.mkpath(overrideDir);
    }
    
    QString path = getOverridePath(electionId);
    QFile file(path);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "KioskConfigurationManager: Failed to open override file for writing:" << path;
        return false;
    }
    
    QJsonDocument doc(config.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    qInfo() << "KioskConfigurationManager: Saved election override for:" << electionId;
    return true;
}

void KioskConfigurationManager::clearElectionOverride() {
    QMutexLocker locker(&m_mutex);
    if (m_hasOverride) {
        m_config = KioskConfiguration::defaultConfig();
        m_hasOverride = false;
        m_currentElectionId.clear();
        emit configurationChanged();
    }
}

void KioskConfigurationManager::applyTheme(const Core::Models::ThemeConfig& theme) {
    QMutexLocker locker(&m_mutex);
    
    // Apply theme colors to kiosk configuration
    m_config.themeId = theme.id;
    m_config.accentColor = theme.darkColors.primary; // Use dark theme as default for kiosk
    m_config.backgroundColor = theme.darkColors.background;
    m_config.fontFamily = theme.fontFamily;
    m_config.baseFontSize = theme.baseFontSize;
    m_config.headingFontSize = theme.headingFontSize;
    m_config.borderRadius = theme.borderRadius;
    m_config.animationsEnabled = theme.animationsEnabled && !theme.reducedMotion;
    m_config.reducedMotion = theme.reducedMotion;
    
    emit configurationChanged();
    emit themeChanged(theme.id);
}

QJsonObject KioskConfiguration::toJson() const {
    QJsonObject o;
    o["themeId"] = themeId;
    o["accentColor"] = accentColor;
    o["backgroundColor"] = backgroundColor;
    o["fontFamily"] = fontFamily;
    o["baseFontSize"] = baseFontSize;
    o["headingFontSize"] = headingFontSize;
    o["borderRadius"] = borderRadius;
    o["animationsEnabled"] = animationsEnabled;
    o["reducedMotion"] = reducedMotion;
    
    o["stepCount"] = stepCount;
    o["requirePhotoVerification"] = requirePhotoVerification;
    o["allowSkipVerification"] = allowSkipVerification;
    o["autoAdvanceDelayMs"] = autoAdvanceDelayMs;
    o["showProgressIndicator"] = showProgressIndicator;
    o["showStepNumbers"] = showStepNumbers;
    
    o["candidatesPerRow"] = candidatesPerRow;
    o["candidateCardWidth"] = candidateCardWidth;
    o["candidateCardHeight"] = candidateCardHeight;
    o["showCandidatePhotos"] = showCandidatePhotos;
    o["showCandidateParty"] = showCandidateParty;
    o["showCandidateManifesto"] = showCandidateManifesto;
    o["randomizeCandidateOrder"] = randomizeCandidateOrder;
    o["showCandidateDetailsOnHover"] = showCandidateDetailsOnHover;
    
    o["confirmBeforeVote"] = confirmBeforeVote;
    o["requireFinalConfirmation"] = requireFinalConfirmation;
    o["confirmationTimeoutSeconds"] = confirmationTimeoutSeconds;
    o["showVoteReceipt"] = showVoteReceipt;
    o["receiptFormat"] = receiptFormat;
    
    o["enabledAuthMethods"] = QJsonArray::fromStringList(enabledAuthMethods);
    o["primaryAuthMethod"] = primaryAuthMethod;
    o["allowManualEntry"] = allowManualEntry;
    o["autoSubmitOnScan"] = autoSubmitOnScan;
    o["scanDebounceMs"] = scanDebounceMs;
    
    o["highContrastMode"] = highContrastMode;
    o["largeTextMode"] = largeTextMode;
    o["screenReaderOptimized"] = screenReaderOptimized;
    o["touchTargetSize"] = touchTargetSize;
    
    o["institutionName"] = institutionName;
    o["institutionLogoPath"] = institutionLogoPath;
    o["welcomeMessage"] = welcomeMessage;
    o["waitingMessage"] = waitingMessage;
    o["votingMessage"] = votingMessage;
    
    o["testModeEnabled"] = testModeEnabled;
    o["auditLoggingEnabled"] = auditLoggingEnabled;
    o["sessionTimeoutSeconds"] = sessionTimeoutSeconds;
    o["requireAdminUnlock"] = requireAdminUnlock;
    
    o["statePollIntervalMs"] = statePollIntervalMs;
    o["enableCaching"] = enableCaching;
    o["candidateImageCacheSize"] = candidateImageCacheSize;
    
    return o;
}

KioskConfiguration KioskConfiguration::fromJson(const QJsonObject& obj) {
    KioskConfiguration config;
    
    config.themeId = obj["themeId"].toString("default-dark");
    config.accentColor = obj["accentColor"].toString("#0078d4");
    config.backgroundColor = obj["backgroundColor"].toString("#1a1a2e");
    config.fontFamily = obj["fontFamily"].toString("Segoe UI");
    config.baseFontSize = obj["baseFontSize"].toInt(14);
    config.headingFontSize = obj["headingFontSize"].toInt(24);
    config.borderRadius = obj["borderRadius"].toDouble(12.0);
    config.animationsEnabled = obj["animationsEnabled"].toBool(true);
    config.reducedMotion = obj["reducedMotion"].toBool(false);
    
    config.stepCount = obj["stepCount"].toInt(4);
    config.requirePhotoVerification = obj["requirePhotoVerification"].toBool(true);
    config.allowSkipVerification = obj["allowSkipVerification"].toBool(false);
    config.autoAdvanceDelayMs = obj["autoAdvanceDelayMs"].toInt(2000);
    config.showProgressIndicator = obj["showProgressIndicator"].toBool(true);
    config.showStepNumbers = obj["showStepNumbers"].toBool(true);
    
    config.candidatesPerRow = obj["candidatesPerRow"].toInt(3);
    config.candidateCardWidth = obj["candidateCardWidth"].toInt(280);
    config.candidateCardHeight = obj["candidateCardHeight"].toInt(380);
    config.showCandidatePhotos = obj["showCandidatePhotos"].toBool(true);
    config.showCandidateParty = obj["showCandidateParty"].toBool(true);
    config.showCandidateManifesto = obj["showCandidateManifesto"].toBool(false);
    config.randomizeCandidateOrder = obj["randomizeCandidateOrder"].toBool(false);
    config.showCandidateDetailsOnHover = obj["showCandidateDetailsOnHover"].toBool(true);
    
    config.confirmBeforeVote = obj["confirmBeforeVote"].toBool(true);
    config.requireFinalConfirmation = obj["requireFinalConfirmation"].toBool(true);
    config.confirmationTimeoutSeconds = obj["confirmationTimeoutSeconds"].toInt(30);
    config.showVoteReceipt = obj["showVoteReceipt"].toBool(false);
    config.receiptFormat = obj["receiptFormat"].toString("text");
    
    QJsonArray authArray = obj["enabledAuthMethods"].toArray();
    for (const auto& val : authArray) {
        config.enabledAuthMethods.append(val.toString());
    }
    config.primaryAuthMethod = obj["primaryAuthMethod"].toString("AdmissionNumber");
    config.allowManualEntry = obj["allowManualEntry"].toBool(true);
    config.autoSubmitOnScan = obj["autoSubmitOnScan"].toBool(true);
    config.scanDebounceMs = obj["scanDebounceMs"].toInt(500);
    
    config.highContrastMode = obj["highContrastMode"].toBool(false);
    config.largeTextMode = obj["largeTextMode"].toBool(false);
    config.screenReaderOptimized = obj["screenReaderOptimized"].toBool(false);
    config.touchTargetSize = obj["touchTargetSize"].toInt(48);
    
    config.institutionName = obj["institutionName"].toString("Campus Ballot");
    config.institutionLogoPath = obj["institutionLogoPath"].toString();
    config.welcomeMessage = obj["welcomeMessage"].toString("Welcome to Campus Ballot");
    config.waitingMessage = obj["waitingMessage"].toString("Please wait for the election session to start.");
    config.votingMessage = obj["votingMessage"].toString("An election is currently in progress.");
    
    config.testModeEnabled = obj["testModeEnabled"].toBool(false);
    config.auditLoggingEnabled = obj["auditLoggingEnabled"].toBool(true);
    config.sessionTimeoutSeconds = obj["sessionTimeoutSeconds"].toInt(300);
    config.requireAdminUnlock = obj["requireAdminUnlock"].toBool(false);
    
    config.statePollIntervalMs = obj["statePollIntervalMs"].toInt(2000);
    config.enableCaching = obj["enableCaching"].toBool(true);
    config.candidateImageCacheSize = obj["candidateImageCacheSize"].toInt(50);
    
    return config;
}

KioskConfiguration KioskConfiguration::defaultConfig() {
    return KioskConfiguration();
}

KioskConfiguration KioskConfiguration::kioskModeConfig() {
    KioskConfiguration config = defaultConfig();
    config.stepCount = 4;
    config.requirePhotoVerification = true;
    config.autoAdvanceDelayMs = 3000;
    config.confirmBeforeVote = true;
    config.requireFinalConfirmation = true;
    config.confirmationTimeoutSeconds = 60;
    config.sessionTimeoutSeconds = 600; // 10 minutes
    config.touchTargetSize = 56;
    return config;
}

KioskConfiguration KioskConfiguration::accessibilityConfig() {
    KioskConfiguration config = defaultConfig();
    config.highContrastMode = true;
    config.largeTextMode = true;
    config.screenReaderOptimized = true;
    config.baseFontSize = 18;
    config.headingFontSize = 32;
    config.touchTargetSize = 64;
    config.animationsEnabled = false;
    config.reducedMotion = true;
    config.confirmationTimeoutSeconds = 120;
    config.sessionTimeoutSeconds = 1200; // 20 minutes
    return config;
}

} // namespace Ballot::Core