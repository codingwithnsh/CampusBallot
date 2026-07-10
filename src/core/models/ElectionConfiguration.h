#pragma once

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include <QUuid>
#include <QImage>
#include <optional>
#include "FieldDefinition.h"

namespace Ballot::Core::Models {

enum class ElectionType {
    StudentCouncil,
    HouseCaptain,
    ClubElection,
    ClassRepresentative,
    DepartmentRepresentative,
    SportsCaptain,
    CulturalSecretary,
    Custom
};

enum class VotingMethod {
    FirstPastThePost,
    RankedChoice,
    ApprovalVoting,
    SingleTransferableVote,
    BordaCount,
    Condorcet,
    Custom
};

enum class ResultVisibility {
    RealTime,
    Delayed,
    Hidden,
    AdminOnly,
    AfterVotingEnds,
    AfterCertification
};

enum class AuthenticationMethod {
    AdmissionNumber,
    RollNumber,
    StudentID,
    RFID,
    QRCode,
    Biometric,
    Email,
    PhoneNumber,
    CustomField,
    TeacherApproval,
    OTP,
    PIN
};

enum class VoteConfirmation {
    None,
    Simple,
    WithPIN,
    WithOTP,
    WithTeacherApproval,
    WithBiometric
};

struct ThemeColors {
    QString primary = "#0078d4";
    QString secondary = "#106ebe";
    QString accent = "#0078d4";
    QString background = "#1a1a2e";
    QString surface = "#25253a";
    QString surfaceVariant = "#2d2d44";
    QString onPrimary = "#ffffff";
    QString onSecondary = "#ffffff";
    QString onBackground = "#e0e0e0";
    QString onSurface = "#ffffff";
    QString error = "#f44336";
    QString warning = "#ffb300";
    QString success = "#4caf50";
    QString info = "#2196f3";
    QString outline = "#3d3d5c";
    QString shadow = "#000000";
    
    QJsonObject toJson() const;
    static ThemeColors fromJson(const QJsonObject& obj);
    static ThemeColors lightTheme();
    static ThemeColors darkTheme();
    static ThemeColors schoolTheme(const QString& primaryColor, const QString& secondaryColor);
};

struct ThemeConfig {
    QString id;
    QString name;
    QString description;
    ThemeColors lightColors;
    ThemeColors darkColors;
    QString fontFamily = "Segoe UI";
    QString fontFamilyHeading = "Segoe UI";
    int baseFontSize = 14;
    int headingFontSize = 24;
    double borderRadius = 12.0;
    double elevation = 4.0;
    bool animationsEnabled = true;
    bool reducedMotion = false;
    QString logoPath;
    QString backgroundImagePath;
    QString faviconPath;
    QVariantMap customProperties;
    
    QJsonObject toJson() const;
    static ThemeConfig fromJson(const QJsonObject& obj);
    static ThemeConfig defaultLight();
    static ThemeConfig defaultDark();
    static ThemeConfig schoolTheme(const QString& primaryColor, const QString& secondaryColor);
};

struct StudentIdentificationConfig {
    QList<AuthenticationMethod> enabledMethods;
    AuthenticationMethod primaryMethod = AuthenticationMethod::AdmissionNumber;
    bool allowMultipleMethods = true;
    bool requireAllEnabled = false;
    QString customFieldId;
    bool showStudentPhoto = true;
    bool showStudentDetails = true;
    bool confirmIdentityBeforeVote = true;
    int autoAdvanceDelayMs = 2000;
    
    QJsonObject toJson() const;
    static StudentIdentificationConfig fromJson(const QJsonObject& obj);
};

struct StudentFieldConfig {
    FieldDefinitionList enabledFields;
    FieldDefinitionList visibleDuringVoting;
    FieldDefinitionList requiredForRegistration;
    FieldDefinitionList searchableFields;
    QString uniqueIdentifierField = "admissionNumber";
    QList<QString> displayFieldsForVerification = {"name", "photo", "className", "section", "house"};
    
    QJsonObject toJson() const;
    static StudentFieldConfig fromJson(const QJsonObject& obj);
    static StudentFieldConfig defaultConfig();
};

struct CandidateRulesConfig {
    int minCandidates = 2;
    int maxCandidates = 50;
    int maxCandidatesPerParty = 10;
    bool allowParties = true;
    bool allowIndependents = true;
    bool requireParty = false;
    bool allowCandidatePhotos = true;
    bool allowPartyLogos = true;
    bool allowManifestos = true;
    bool allowSocialLinks = true;
    bool allowVideoIntro = false;
    bool allowCampaignPosters = false;
    int minAge = 0;
    int maxAge = 100;
    QList<QString> allowedClasses;
    QList<QString> allowedSections;
    QList<QString> allowedHouses;
    QList<QString> allowedGenders;
    QString customEligibilityScript;
    bool requireApproval = true;
    int maxManifestoLength = 5000;
    int maxBioLength = 2000;
    
    QJsonObject toJson() const;
    static CandidateRulesConfig fromJson(const QJsonObject& obj);
    static CandidateRulesConfig defaultConfig();
};

struct VotingRulesConfig {
    VotingMethod method = VotingMethod::FirstPastThePost;
    int maxVotesPerVoter = 1;
    bool allowMultipleVotes = false;
    bool allowRankedChoice = false;
    bool allowApprovalVoting = false;
    bool allowNOTA = true;
    bool secretBallot = true;
    bool anonymousVoting = true;
    VoteConfirmation confirmation = VoteConfirmation::Simple;
    bool requireOTP = false;
    bool requirePIN = false;
    bool requireTeacherApproval = false;
    bool allowVoteEditing = false;
    bool allowVoteCancellation = false;
    bool allowSkip = true;
    bool allowBlankVote = true;
    int votingTimeLimitMinutes = 0;
    bool showCandidatePhotos = true;
    bool showCandidateParty = true;
    bool showCandidateManifesto = true;
    bool randomizeCandidateOrder = false;
    bool showVoterReceipt = false;
    QString receiptFormat = "text"; // text, qr, pdf
    
    QJsonObject toJson() const;
    static VotingRulesConfig fromJson(const QJsonObject& obj);
    static VotingRulesConfig defaultConfig();
};

struct ResultSettingsConfig {
    ResultVisibility visibility = ResultVisibility::RealTime;
    int delayMinutes = 0;
    bool showGraphs = true;
    bool showCharts = true;
    bool showWinnerAnimation = true;
    bool allowExportPDF = true;
    bool allowExportExcel = true;
    bool allowExportCSV = true;
    bool generateCertificates = true;
    QString certificateTemplate;
    bool showVoteCount = true;
    bool showPercentages = true;
    bool showTurnout = true;
    bool showByClass = true;
    bool showBySection = true;
    bool showByHouse = true;
    bool showByGender = true;
    bool showHourlyTrend = true;
    bool showHeatmap = true;
    int resultRefreshIntervalSeconds = 30;
    bool requireCertification = false;
    QString certificationAuthority;
    
    QJsonObject toJson() const;
    static ResultSettingsConfig fromJson(const QJsonObject& obj);
    static ResultSettingsConfig defaultConfig();
};

struct ElectionConfiguration {
    QString id;
    QString title;
    QString description;
    QString shortDescription;
    ElectionType type = ElectionType::Custom;
    QString customTypeName;
    
    QDateTime startDate;
    QDateTime endDate;
    QString academicYear;
    QString electionCode;
    
    QImage logo;
    QString logoPath;
    ThemeConfig theme;
    
    StudentIdentificationConfig studentIdentification;
    StudentFieldConfig studentFields;
    CandidateRulesConfig candidateRules;
    VotingRulesConfig votingRules;
    ResultSettingsConfig resultSettings;
    
    QList<QString> eligibleClasses;
    QList<QString> eligibleSections;
    QList<QString> eligibleHouses;
    QList<QString> eligibleDepartments;
    QList<QString> eligibleGrades;
    QVariantMap customEligibilityRules;
    
    bool isActive = false;
    bool isPublished = false;
    bool allowRegistration = true;
    QDateTime registrationStartDate;
    QDateTime registrationEndDate;
    int maxRegistrations = 0;
    
    QString createdBy;
    QDateTime createdAt;
    QDateTime updatedAt;
    int version = 1;
    QVariantMap metadata;
    
    QJsonObject toJson() const;
    static ElectionConfiguration fromJson(const QJsonObject& obj);
    static ElectionConfiguration createDefault(const QString& title, ElectionType type);
    
    bool isFieldEnabled(const QString& fieldId) const;
    bool isFieldRequired(const QString& fieldId) const;
    bool isFieldVisibleDuringVoting(const QString& fieldId) const;
    FieldDefinition getFieldDefinition(const QString& fieldId) const;
    FieldDefinitionList getEnabledFields() const;
    FieldDefinitionList getRequiredFields() const;
    FieldDefinitionList getVotingDisplayFields() const;
};

struct Party {
    QString id;
    QString electionId;
    QString name;
    QString shortName;
    QString description;
    QString motto;
    QString symbol;
    QImage logo;
    QString logoPath;
    QString primaryColor;
    QString secondaryColor;
    QString accentColor;
    QString presidentStudentId;
    QList<QString> memberStudentIds;
    bool isActive = true;
    int sortOrder = 0;
    QDateTime createdAt;
    QDateTime updatedAt;
    QVariantMap metadata;
    
    QJsonObject toJson() const;
    static Party fromJson(const QJsonObject& obj);
};

struct Candidate {
    QString id;
    QString electionId;
    QString partyId;
    QString studentId;
    QString name;
    QImage photo;
    QString photoPath;
    QString biography;
    QString manifesto;
    QString position;
    QString className;
    QString section;
    QString house;
    QString admissionNumber;
    QString rollNumber;
    QString videoUrl;
    QImage campaignPoster;
    QString campaignPosterPath;
    QVariantMap socialLinks;
    QList<QString> documentPaths;
    bool isApproved = false;
    bool isIndependent = false;
    int sortOrder = 0;
    QDateTime registeredAt;
    QDateTime approvedAt;
    QString approvedBy;
    QVariantMap customFields;
    
    QJsonObject toJson() const;
    static Candidate fromJson(const QJsonObject& obj);
};

struct Student {
    QString id;
    QString electionId;
    QString name;
    QImage photo;
    QString photoPath;
    QString admissionNumber;
    QString rollNumber;
    QString studentId;
    QString rfidTag;
    QString qrCode;
    QString email;
    QString phone;
    QString className;
    QString section;
    QString house;
    QString department;
    QString grade;
    QString gender;
    QDateTime dateOfBirth;
    QString parentName;
    QString parentPhone;
    QString parentEmail;
    QString address;
    QString uniqueVotingId;
    bool hasVoted = false;
    bool isVerified = false;
    QDateTime verifiedAt;
    QString verifiedBy;
    QDateTime registeredAt;
    QVariantMap customFields;
    QVariantMap importedData;
    
    QJsonObject toJson() const;
    static Student fromJson(const QJsonObject& obj);
    
    QString getIdentifier(AuthenticationMethod method) const;
    QVariant getFieldValue(const QString& fieldId) const;
    void setFieldValue(const QString& fieldId, const QVariant& value);
};

struct Vote {
    QString id;
    QString electionId;
    QString studentId;
    QVariant candidateData; // Can be single ID, list of IDs, or ranked choices
    QByteArray encryptedData;
    QByteArray voteHash;
    QByteArray digitalSignature;
    QDateTime timestamp;
    QString machineId;
    QString ipAddress;
    bool isAudited = false;
    QDateTime auditedAt;
    QString auditedBy;
    QVariantMap metadata;
    
    QJsonObject toJson() const;
    static Vote fromJson(const QJsonObject& obj);
    
    QList<QString> getCandidateIds() const;
    void setCandidateIds(const QList<QString>& ids);
    QList<int> getRankedChoices() const;
    void setRankedChoices(const QList<int>& choices);
};

struct ElectionResult {
    QString electionId;
    QString candidateId;
    QString candidateName;
    QString partyId;
    QString partyName;
    int voteCount = 0;
    double percentage = 0.0;
    int rank = 0;
    bool isWinner = false;
    QVariantMap breakdown;
    
    QJsonObject toJson() const;
    static ElectionResult fromJson(const QJsonObject& obj);
};

struct AnalyticsData {
    QString electionId;
    int totalRegistered = 0;
    int totalVoted = 0;
    double turnoutPercentage = 0.0;
    
    QMap<QString, int> votesByClass;
    QMap<QString, int> votesBySection;
    QMap<QString, int> votesByHouse;
    QMap<QString, int> votesByGender;
    QMap<QString, int> votesByDepartment;
    QMap<QString, int> votesByGrade;
    QMap<QDateTime, int> hourlyTrend;
    QMap<QString, int> votesByCandidate;
    QMap<QString, double> turnoutByClass;
    QMap<QString, double> turnoutBySection;
    QMap<QString, double> turnoutByHouse;
    QMap<QString, double> turnoutByGender;
    
    QJsonObject toJson() const;
    static AnalyticsData fromJson(const QJsonObject& obj);
};

} // namespace Ballot::Core::Models