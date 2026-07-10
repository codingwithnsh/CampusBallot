#pragma once

#include <QString>
#include <QDateTime>
#include <QImage> // Keep for potential conversion utilities, but not for direct storage
#include <QByteArray>
#include <QVariantMap>
#include <QUuid>
#include <QJsonObject>
#include <optional>
#include <QJsonArray> // Added for QStringList serialization

namespace Ballot::Core {

enum class UserRole {
    SuperAdministrator,
    ElectionAdministrator,
    Teacher,
    StudentVolunteer,
    Observer,
    ResultAuditor,
    Count
};

enum class VotingState {
    Idle,
    Voting,
    Ended,
    Paused,
    Unknown
};

enum class StorageProviderType {
    SQLite,
    FirebaseFirestore,
    FirebaseRealtime,
    PostgreSQL,
    MySQL,
    MSSQL,
    CustomRestApi,
    CustomSchoolServer,
    Plugin
};

enum class AuditAction {
    Login,
    Logout,
    VoteStarted,
    VoteCompleted,
    VoteVerified,
    ElectionCreated,
    ElectionDeleted,
    ElectionModified,
    DatabaseModified,
    SettingsChanged,
    FailedLogin,
    PermissionDenied,
    UserCreated,
    UserDeleted,
    UserModified,
    BackupCreated,
    BackupRestored,
    LogsExported,
    SystemUpdate,
    TamperDetected,
    IntegrityCheckPassed,
    IntegrityCheckFailed,
    ElectionStarted,
    ElectionEnded,
    ElectionPaused,
    CandidateAdded,
    CandidateModified,
    CandidateDeleted,
    VoteCast
};

// Struct to hold theme-related colors for UI components
struct ThemeColors {
    QString surface;            // Background color for cards/surfaces
    QString outline;            // Border color for cards/elements
    QString accent;             // Primary accent color
    QString onSurface;          // Text color on surface backgrounds
    QString onSurfaceVariant;   // Secondary text color on surface backgrounds
    QString onPrimary;          // Text color on primary/accent buttons
    QString secondary;          // Secondary accent color (e.g., for hover states)
    QString surfaceVariant;     // Background color for photo placeholders
    // Add other relevant colors as needed
};


struct User {
    QString id;
    QString name;
    QByteArray photoData;
    QString idCardNumber;
    QString department;
    QString className;
    QString section;
    QString phone;
    QString email;
    QByteArray passwordHashAndSalt; // Renamed from digitalSignature for password storage
    UserRole role = UserRole::Observer;
    QStringList permissions;
    QByteArray digitalSignature; // Re-added for actual digital signature if needed
    QByteArray qrCode;
    bool isActive = true;
    QDateTime createdAt;
    QDateTime lastLogin;

    QJsonObject toJson() const {
        QJsonObject o;
        o["id"] = id;
        o["name"] = name;
        o["photoData"] = QString(photoData.toBase64());
        o["idCardNumber"] = idCardNumber;
        o["department"] = department;
        o["class"] = className;
        o["section"] = section;
        o["phone"] = phone;
        o["email"] = email;
        // Do NOT serialize passwordHashAndSalt for security reasons
        o["role"] = static_cast<int>(role);
        o["permissions"] = QJsonArray::fromStringList(permissions);
        o["digitalSignature"] = QString(digitalSignature.toBase64());
        o["qrCode"] = QString(qrCode.toBase64());
        o["isActive"] = isActive;
        o["createdAt"] = createdAt.toString(Qt::ISODate);
        o["lastLogin"] = lastLogin.toString(Qt::ISODate);
        return o;
    }
};

struct Student {
    QString id;
    QString name;
    QByteArray photoData;
    QString admissionNumber;
    QString rollNumber;
    QString department;
    QString className;
    QString section;
    int age = 0;
    QString gender;
    QString email;
    QString phone;
    QString parentName;
    QByteArray qrCode;
    QString rfidTag;
    QByteArray barcode;
    QString uniqueVotingId;
    bool hasVoted = false;
    bool isVerified = false;
    QDateTime verifiedAt;
    QString verifiedBy;
    QDateTime registeredAt;

    QJsonObject toJson() const {
        QJsonObject o;
        o["id"] = id;
        o["name"] = name;
        o["photoData"] = QString(photoData.toBase64());
        o["admissionNumber"] = admissionNumber;
        o["rollNumber"] = rollNumber;
        o["department"] = department;
        o["class"] = className;
        o["section"] = section;
        o["age"] = age;
        o["gender"] = gender;
        o["email"] = email;
        o["phone"] = phone;
        o["parentName"] = parentName;
        o["qrCode"] = QString(qrCode.toBase64());
        o["rfidTag"] = rfidTag;
        o["barcode"] = QString(barcode.toBase64());
        o["uniqueVotingId"] = uniqueVotingId;
        o["hasVoted"] = hasVoted;
        o["isVerified"] = isVerified;
        o["verifiedAt"] = verifiedAt.toString(Qt::ISODate);
        o["verifiedBy"] = verifiedBy;
        o["registeredAt"] = registeredAt.toString(Qt::ISODate);
        return o;
    }
};

struct Candidate {
    QString id;
    QString electionId;
    QString name;
    QByteArray photoData;
    QString manifesto;
    QString party;
    QString className;
    QString section;
    QString symbol;
    QString videoUrl;
    QByteArray campaignPosterData;
    bool isApproved = false;
    QDateTime registeredAt;

    QJsonObject toJson() const {
        QJsonObject o;
        o["id"] = id;
        o["electionId"] = electionId;
        o["name"] = name;
        o["photoData"] = QString(photoData.toBase64());
        o["manifesto"] = manifesto;
        o["party"] = party;
        o["class"] = className;
        o["section"] = section;
        o["symbol"] = symbol;
        o["videoUrl"] = videoUrl;
        o["campaignPosterData"] = QString(campaignPosterData.toBase64());
        o["isApproved"] = isApproved;
        o["registeredAt"] = registeredAt.toString(Qt::ISODate);
        return o;
    }
};

struct Election {
    QString id;
    QString title;
    QString description;
    QDateTime startDate;
    QDateTime endDate;
    VotingState state = VotingState::Idle;
    bool isActive = false;
    QString createdBy;
    QDateTime createdAt;
    QStringList eligibleClasses;
    QStringList eligibleDepartments;
    int maxVotesPerStudent = 1;
    bool requireVerification = true;

    QJsonObject toJson() const {
        QJsonObject o;
        o["id"] = id;
        o["title"] = title;
        o["description"] = description;
        o["startDate"] = startDate.toString(Qt::ISODate);
        o["endDate"] = endDate.toString(Qt::ISODate);
        o["state"] = static_cast<int>(state);
        o["isActive"] = isActive;
        o["createdBy"] = createdBy;
        o["createdAt"] = createdAt.toString(Qt::ISODate);
        o["eligibleClasses"] = QJsonArray::fromStringList(eligibleClasses);
        o["eligibleDepartments"] = QJsonArray::fromStringList(eligibleDepartments);
        o["maxVotesPerStudent"] = maxVotesPerStudent;
        o["requireVerification"] = requireVerification;
        return o;
    }
};

struct Vote {
    QString id;
    QString electionId;
    QString studentId;
    QString candidateId;
    QByteArray voteHash;
    QByteArray digitalSignature;
    QDateTime timestamp;
    QString machineId;
    bool isAudited = false;

    QJsonObject toJson() const {
        QJsonObject o;
        o["id"] = id;
        o["electionId"] = electionId;
        o["studentId"] = studentId;
        o["candidateId"] = candidateId;
        o["voteHash"] = QString(voteHash.toBase64());
        o["digitalSignature"] = QString(digitalSignature.toBase64());
        o["timestamp"] = timestamp.toString(Qt::ISODate);
        o["machineId"] = machineId;
        o["isAudited"] = isAudited;
        return o;
    }
};

struct AuditLogEntry {
    QString id;
    QDateTime timestamp;
    QString userId;
    QString userName;
    AuditAction action;
    QString details;
    QString ipAddress;
    QString machineId;
    QByteArray hash;
    bool isImmutable = false;

    QJsonObject toJson() const {
        QJsonObject o;
        o["id"] = id;
        o["timestamp"] = timestamp.toString(Qt::ISODate);
        o["userId"] = userId;
        o["userName"] = userName;
        o["action"] = static_cast<int>(action);
        o["details"] = details;
        o["ipAddress"] = ipAddress;
        o["machineId"] = machineId;
        o["hash"] = QString(hash.toBase64());
        o["isImmutable"] = isImmutable;
        return o;
    }
};

struct MachineInfo {
    QString id;
    QString name;
    bool isMaster = false;
    QDateTime lastSeen;
    QString ipAddress;
    QString osVersion;
    QString appVersion;
    bool isOnline = false;

    QJsonObject toJson() const {
        QJsonObject o;
        o["id"] = id;
        o["name"] = name;
        o["isMaster"] = isMaster;
        o["lastSeen"] = lastSeen.toString(Qt::ISODate);
        o["ipAddress"] = ipAddress;
        o["osVersion"] = osVersion;
        o["appVersion"] = appVersion;
        o["isOnline"] = isOnline;
        return o;
    }
};

struct SystemSettings {
    QString masterMachineId;
    VotingState votingStatus = VotingState::Idle;
    bool allowResultsPreview = false;
    bool autoBackupEnabled = true;
    int backupIntervalHours = 24;
    int sessionTimeoutMinutes = 30;
    int failedLoginAttempts = 5;
    int lockoutDurationMinutes = 15;
    bool requireStrongPassword = true;
    bool auditAllActions = true;
    bool encryptionEnabled = true;
    bool tamperDetection = true;
    QString theme = "Modern"; // Changed default to "Modern"
    QString accentColor = "#0078d4";
    QString language = "en";

    QJsonObject toJson() const {
        QJsonObject o;
        o["masterMachineId"] = masterMachineId;
        o["votingStatus"] = static_cast<int>(votingStatus);
        o["allowResultsPreview"] = allowResultsPreview;
        o["autoBackupEnabled"] = autoBackupEnabled;
        o["backupIntervalHours"] = backupIntervalHours;
        o["sessionTimeoutMinutes"] = sessionTimeoutMinutes;
        o["failedLoginAttempts"] = failedLoginAttempts;
        o["lockoutDurationMinutes"] = lockoutDurationMinutes;
        o["requireStrongPassword"] = requireStrongPassword;
        o["auditAllActions"] = auditAllActions;
        o["encryptionEnabled"] = encryptionEnabled;
        o["tamperDetection"] = tamperDetection;
        o["theme"] = theme;
        o["accentColor"] = accentColor;
        o["language"] = language;
        return o;
    }
};

struct ElectionResult {
    QString electionId;
    QString candidateId;
    QString candidateName;
    QString party;
    int voteCount = 0;
    double percentage = 0.0;

    QJsonObject toJson() const {
        QJsonObject o;
        o["electionId"] = electionId;
        o["candidateId"] = candidateId;
        o["candidateName"] = candidateName;
        o["party"] = party;
        o["voteCount"] = voteCount;
        o["percentage"] = percentage;
        return o;
    }
};

struct BackupEntry {
    QString id;
    QString name;
    QDateTime createdAt;
    qint64 sizeBytes = 0;
    QString type; // "full", "incremental"
    QByteArray checksum;
    QString storagePath;
    bool isEncrypted = true;

    QJsonObject toJson() const {
        QJsonObject o;
        o["id"] = id;
        o["name"] = name;
        o["createdAt"] = createdAt.toString(Qt::ISODate);
        o["sizeBytes"] = sizeBytes;
        o["type"] = type;
        o["checksum"] = QString(checksum.toBase64());
        o["storagePath"] = storagePath;
        o["isEncrypted"] = isEncrypted;
        return o;
    }
};

} // namespace Ballot::Core
