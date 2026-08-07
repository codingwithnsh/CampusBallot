#pragma once

#include <QObject>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <optional>

#include "src/core/Models.h"

namespace Ballot::Integration {

struct FirebaseRemoteUser {
    QString id;
    QString name;
    QString email;
    Core::UserRole role = Core::UserRole::Observer;
    bool isActive = true;
    QByteArray passwordHashAndSalt;
};

struct FirebaseControlState {
    QString electionId;
    Core::VotingState state = Core::VotingState::Unknown;
    bool valid = false;
};

class FirebaseRealtimeSyncManager {
public:
    static FirebaseRealtimeSyncManager& instance();

    void configure(const QVariantMap& config);
    bool isConfigured() const;
    QString databaseUrl() const { return m_databaseUrl; }
    QString lastError() const { return m_lastError; }

    bool testConnectionAndSeed(QString* errorMessage = nullptr);

    bool syncUser(const Core::User& user, QString* errorMessage = nullptr);
    bool removeUser(const QString& userId, const QString& email, QString* errorMessage = nullptr);
    std::optional<FirebaseRemoteUser> fetchUserByEmail(const QString& email, QString* errorMessage = nullptr);

    bool syncVote(const Core::Vote& vote, QString* errorMessage = nullptr);
    bool publishElectionState(const QString& electionId, Core::VotingState state, QString* errorMessage = nullptr);
    std::optional<FirebaseControlState> fetchControlState(QString* errorMessage = nullptr);

private:
    FirebaseRealtimeSyncManager() = default;

    struct HttpResult {
        bool success = false;
        int statusCode = 0;
        QByteArray body;
        QString error;
    };

    HttpResult request(const QString& method,
                       const QString& path,
                       const QByteArray& payload = {},
                       int timeoutMs = 5000);
    QUrl buildUrl(const QString& path) const;
    static QString sanitizeKey(const QString& key);
    static QString votingStateToWire(Core::VotingState state);
    static Core::VotingState votingStateFromWire(const QJsonValue& value);
    static QString roleToWire(Core::UserRole role);
    static Core::UserRole roleFromWire(const QJsonValue& value);
    void setError(const QString& errorMessage, QString* outputErrorMessage) const;

    QNetworkAccessManager m_network;
    QString m_databaseUrl;
    QString m_projectId;
    QString m_apiKey;
    QString m_databaseSecret;
    mutable QString m_lastError;

    QDateTime m_lastControlFetchUtc;
    std::optional<FirebaseControlState> m_cachedControlState;
};

} // namespace Ballot::Integration
