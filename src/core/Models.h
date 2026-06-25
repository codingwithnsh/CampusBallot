#pragma once

#include <QString>
#include <QDateTime>
#include <QImage>
#include <QByteArray>
#include <QVariantMap>
#include <QUuid>
#include <QJsonObject>
#include <optional>

namespace Ballot::Core {

enum class UserRole {
    SuperAdministrator,
    ElectionAdministrator,
    Teacher,
    StudentVolunteer,
    Observer,
    ResultAuditor
};

enum class VotingState {
    Idle,
    Voting,
    Ended,
    Paused
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
    IntegrityCheckFailed
};

struct User {
    QString id;
    QString name;
    QImage photo;
    QString idCardNumber;
    QString department;
    QString className;
    QString section;
    QString phone;
    QString email;
    UserRole role = UserRole::Observer;
    QStringList permissions;
    QByteArray digitalSignature;
    QByteArray qrCode;
    bool isActive = true;
    QDateTime createdAt;
    QDateTime lastLogin;

    QJsonObject toJson() const {
        QJsonObject o;
        o["id"] = id;
        o["name"] = name;
        o["department"] = department;
        o["class"] = className;
        o["section"] = section;
        o["phone"] = phone;
        o["email"] = email;
        o["role"] = static_cast<int>(role);
        o["isActive"] = isActive;
        return o;
    }
};

struct Student {
    QString id;
    QString name;
    QImage photo;
    QString admissionNumber;
    QString rollNumber;
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
        o["admissionNumber"] = admissionNumber;
        o["rollNumber"] = rollNumber;
        o["class"] = className;
        o["section"] = section;
        o["age"] = age;
        o["gender"] = gender;
        o["email"] = email;
        o["phone"] = phone;
        o["parentName"] = parentName;
        o["uniqueVotingId"] = uniqueVotingId;
        o["hasVoted"] = hasVoted;
        o["isVerified"] = isVerified;
        return o;
    }
};

struct Candidate {
    QString id;
    QString electionId;
    QString name;
    QImage photo;
    QString manifesto;
    QString party;
    QString className;
    QString section;
    QString symbol;
    QString videoUrl;
    QImage campaignPoster;
    bool isApproved = false;
    QDateTime registeredAt;

    QJsonObject toJson() const {
        QJsonObject o;
        o["id"] = id;
        o["electionId"] = electionId;
        o["name"] = name;
        o["manifesto"] = manifesto;
        o["party"] = party;
        o["class"] = className;
        o["section"] = section;
        o["symbol"] = symbol;
        o["videoUrl"] = videoUrl;
        o["isApproved"] = isApproved;
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
        o["requireVerification"] = requireVerification;
        return o;
    }
};

struct Vote {
    QString id;
    QString electionId;
    QString studentId;
    QString encryptedCandidateId;
    QByteArray voteHash;
    QByteArray digitalSignature;
    QDateTime timestamp;
    QString machineId;
    bool isAudited = false;

    QJsonObject toJson() const {
        QJsonObject o;
        o["id"] = id;
        o["electionId"] = electionId;
        o["encryptedCandidateId"] = encryptedCandidateId;
        o["timestamp"] = timestamp.toString(Qt::ISODate);
        o["machineId"] = machineId;
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
    QString theme = "dark";
    QString accentColor = "#0078d4";
    QString language = "en";
};

struct ElectionResult {
    QString electionId;
    QString candidateId;
    QString candidateName;
    QString party;
    int voteCount = 0;
    double percentage = 0.0;
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
};

} // namespace Ballot::Core
