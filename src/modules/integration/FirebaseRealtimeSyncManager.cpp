#include "FirebaseRealtimeSyncManager.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QTimer>
#include <QUrlQuery>

namespace Ballot::Integration {

FirebaseRealtimeSyncManager& FirebaseRealtimeSyncManager::instance() {
    static FirebaseRealtimeSyncManager manager;
    return manager;
}

void FirebaseRealtimeSyncManager::configure(const QVariantMap& config) {
    QSettings settings;
    m_databaseUrl = config.value("database_url", settings.value("firebase_database_url")).toString().trimmed();
    m_projectId = config.value("project_id", settings.value("firebase_project_id")).toString().trimmed();
    m_apiKey = config.value("api_key", settings.value("firebase_api_key")).toString().trimmed();
    m_databaseSecret = config.value("database_secret", settings.value("firebase_database_secret")).toString().trimmed();
}

bool FirebaseRealtimeSyncManager::isConfigured() const {
    return !m_databaseUrl.isEmpty() && m_databaseUrl.startsWith("http", Qt::CaseInsensitive);
}

bool FirebaseRealtimeSyncManager::testConnectionAndSeed(QString* errorMessage) {
    if (!isConfigured()) {
        setError("Firebase Realtime Database URL is missing or invalid.", errorMessage);
        return false;
    }

    const QJsonObject testPayload{
        {"project_id", m_projectId},
        {"connected_at_utc", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"client", "TheRealCampusBallot"}
    };
    const HttpResult testResult = request("PUT", "campus_ballot/connection_test", QJsonDocument(testPayload).toJson(QJsonDocument::Compact));
    if (!testResult.success) {
        setError(QString("Firebase connection test failed (%1): %2").arg(testResult.statusCode).arg(testResult.error), errorMessage);
        return false;
    }

    const QJsonObject controlPayload{
        {"election_id", ""},
        {"voting_state", votingStateToWire(Core::VotingState::Idle)},
        {"updated_at_utc", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    };
    const HttpResult controlResult = request("PATCH", "campus_ballot/control", QJsonDocument(controlPayload).toJson(QJsonDocument::Compact));
    if (!controlResult.success) {
        setError(QString("Firebase seed write failed (%1): %2").arg(controlResult.statusCode).arg(controlResult.error), errorMessage);
        return false;
    }

    return true;
}

bool FirebaseRealtimeSyncManager::syncUser(const Core::User& user, QString* errorMessage) {
    if (!isConfigured()) {
        return true;
    }

    const QString normalizedEmail = user.email.trimmed().toLower();
    const QJsonObject payload{
        {"id", user.id},
        {"name", user.name},
        {"email", normalizedEmail},
        {"role", roleToWire(user.role)},
        {"is_active", user.isActive},
        {"password_hash_hex", QString::fromLatin1(user.passwordHashAndSalt.toHex())},
        {"updated_at_utc", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    };

    const HttpResult userResult = request(
        "PUT",
        QString("campus_ballot/users/%1").arg(sanitizeKey(user.id)),
        QJsonDocument(payload).toJson(QJsonDocument::Compact));
    if (!userResult.success) {
        setError(QString("Failed to sync user to Firebase (%1): %2").arg(userResult.statusCode).arg(userResult.error), errorMessage);
        return false;
    }

    if (!normalizedEmail.isEmpty()) {
        const HttpResult indexResult = request(
            "PUT",
            QString("campus_ballot/user_index/%1").arg(sanitizeKey(normalizedEmail)),
            QByteArray("\"") + user.id.toUtf8() + QByteArray("\""));
        if (!indexResult.success) {
            setError(QString("Failed to sync user index to Firebase (%1): %2").arg(indexResult.statusCode).arg(indexResult.error), errorMessage);
            return false;
        }
    }

    return true;
}

bool FirebaseRealtimeSyncManager::removeUser(const QString& userId, const QString& email, QString* errorMessage) {
    if (!isConfigured()) {
        return true;
    }

    const HttpResult userResult = request("DELETE", QString("campus_ballot/users/%1").arg(sanitizeKey(userId)));
    if (!userResult.success) {
        setError(QString("Failed to remove user from Firebase (%1): %2").arg(userResult.statusCode).arg(userResult.error), errorMessage);
        return false;
    }

    const QString normalizedEmail = email.trimmed().toLower();
    if (!normalizedEmail.isEmpty()) {
        const HttpResult indexResult = request("DELETE", QString("campus_ballot/user_index/%1").arg(sanitizeKey(normalizedEmail)));
        if (!indexResult.success) {
            setError(QString("Failed to remove user index from Firebase (%1): %2").arg(indexResult.statusCode).arg(indexResult.error), errorMessage);
            return false;
        }
    }

    return true;
}

std::optional<FirebaseRemoteUser> FirebaseRealtimeSyncManager::fetchUserByEmail(const QString& email, QString* errorMessage) {
    if (!isConfigured()) {
        return std::nullopt;
    }

    const QString normalizedEmail = email.trimmed().toLower();
    if (normalizedEmail.isEmpty()) {
        return std::nullopt;
    }

    const HttpResult indexResult = request("GET", QString("campus_ballot/user_index/%1").arg(sanitizeKey(normalizedEmail)));
    if (!indexResult.success) {
        setError(QString("Failed to read Firebase user index (%1): %2").arg(indexResult.statusCode).arg(indexResult.error), errorMessage);
        return std::nullopt;
    }

    const QString userId = QString::fromUtf8(indexResult.body).trimmed().remove('"');
    if (userId.isEmpty() || userId == "null") {
        return std::nullopt;
    }

    const HttpResult userResult = request("GET", QString("campus_ballot/users/%1").arg(sanitizeKey(userId)));
    if (!userResult.success) {
        setError(QString("Failed to read Firebase user payload (%1): %2").arg(userResult.statusCode).arg(userResult.error), errorMessage);
        return std::nullopt;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(userResult.body);
    if (!doc.isObject()) {
        setError("Firebase returned an invalid user payload.", errorMessage);
        return std::nullopt;
    }

    const QJsonObject obj = doc.object();
    FirebaseRemoteUser user;
    user.id = obj.value("id").toString(userId);
    user.name = obj.value("name").toString();
    user.email = obj.value("email").toString(normalizedEmail);
    user.role = roleFromWire(obj.value("role"));
    user.isActive = obj.value("is_active").toBool(true);
    user.passwordHashAndSalt = QByteArray::fromHex(obj.value("password_hash_hex").toString().toUtf8());
    if (user.passwordHashAndSalt.isEmpty()) {
        setError("Firebase user record is missing password hash.", errorMessage);
        return std::nullopt;
    }
    return user;
}

bool FirebaseRealtimeSyncManager::syncVote(const Core::Vote& vote, QString* errorMessage) {
    if (!isConfigured()) {
        return true;
    }

    const QJsonObject payload{
        {"id", vote.id},
        {"election_id", vote.electionId},
        {"student_id", vote.studentId},
        {"candidate_id", vote.candidateId},
        {"machine_id", vote.machineId},
        {"timestamp_utc", vote.timestamp.toUTC().toString(Qt::ISODate)}
    };
    const HttpResult voteResult = request(
        "PUT",
        QString("campus_ballot/votes/%1/%2")
            .arg(sanitizeKey(vote.electionId), sanitizeKey(vote.id)),
        QJsonDocument(payload).toJson(QJsonDocument::Compact));
    if (!voteResult.success) {
        setError(QString("Failed to sync vote to Firebase (%1): %2").arg(voteResult.statusCode).arg(voteResult.error), errorMessage);
        return false;
    }
    return true;
}

bool FirebaseRealtimeSyncManager::publishElectionState(const QString& electionId, Core::VotingState state, QString* errorMessage) {
    if (!isConfigured()) {
        return true;
    }

    const QJsonObject payload{
        {"election_id", electionId},
        {"voting_state", votingStateToWire(state)},
        {"updated_at_utc", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    };
    const HttpResult result = request("PATCH", "campus_ballot/control", QJsonDocument(payload).toJson(QJsonDocument::Compact));
    if (!result.success) {
        setError(QString("Failed to publish election state to Firebase (%1): %2").arg(result.statusCode).arg(result.error), errorMessage);
        return false;
    }

    FirebaseControlState cached;
    cached.electionId = electionId;
    cached.state = state;
    cached.valid = true;
    m_cachedControlState = cached;
    m_lastControlFetchUtc = QDateTime::currentDateTimeUtc();
    return true;
}

std::optional<FirebaseControlState> FirebaseRealtimeSyncManager::fetchControlState(QString* errorMessage) {
    if (!isConfigured()) {
        return std::nullopt;
    }

    const QDateTime now = QDateTime::currentDateTimeUtc();
    if (m_cachedControlState && m_lastControlFetchUtc.isValid() && m_lastControlFetchUtc.msecsTo(now) < 1500) {
        return m_cachedControlState;
    }

    const HttpResult result = request("GET", "campus_ballot/control");
    if (!result.success) {
        setError(QString("Failed to fetch Firebase control state (%1): %2").arg(result.statusCode).arg(result.error), errorMessage);
        return std::nullopt;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(result.body);
    if (!doc.isObject()) {
        return std::nullopt;
    }

    const QJsonObject controlObject = doc.object();
    FirebaseControlState control;
    control.electionId = controlObject.value("election_id").toString();
    control.state = votingStateFromWire(controlObject.value("voting_state"));
    control.valid = control.state != Core::VotingState::Unknown;

    m_cachedControlState = control;
    m_lastControlFetchUtc = now;
    return m_cachedControlState;
}

FirebaseRealtimeSyncManager::HttpResult FirebaseRealtimeSyncManager::request(const QString& method,
                                                                             const QString& path,
                                                                             const QByteArray& payload,
                                                                             int timeoutMs) {
    HttpResult result;
    if (!isConfigured()) {
        result.error = "Firebase is not configured.";
        return result;
    }

    const QUrl url = buildUrl(path);
    if (!url.isValid()) {
        result.error = "Invalid Firebase URL.";
        return result;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = nullptr;
    const QByteArray upperMethod = method.trimmed().toUpper().toUtf8();
    if (upperMethod == "GET") {
        reply = m_network.get(request);
    } else if (upperMethod == "PUT") {
        reply = m_network.put(request, payload);
    } else if (upperMethod == "PATCH") {
        reply = m_network.sendCustomRequest(request, "PATCH", payload);
    } else if (upperMethod == "DELETE") {
        reply = m_network.deleteResource(request);
    } else if (upperMethod == "POST") {
        reply = m_network.post(request, payload);
    } else {
        result.error = "Unsupported HTTP method.";
        return result;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (timer.isActive()) {
        timer.stop();
    } else {
        reply->abort();
        result.error = "Request timed out.";
    }

    result.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.body = reply->readAll();
    if (result.error.isEmpty() && reply->error() != QNetworkReply::NoError) {
        result.error = reply->errorString();
    }
    result.success = result.error.isEmpty() && result.statusCode >= 200 && result.statusCode < 300;

    reply->deleteLater();
    return result;
}

QUrl FirebaseRealtimeSyncManager::buildUrl(const QString& path) const {
    QString normalizedBase = m_databaseUrl.trimmed();
    while (normalizedBase.endsWith('/')) {
        normalizedBase.chop(1);
    }

    QString normalizedPath = path.trimmed();
    while (normalizedPath.startsWith('/')) {
        normalizedPath.remove(0, 1);
    }

    QUrl url(normalizedBase + "/" + normalizedPath + ".json");
    if (!m_databaseSecret.isEmpty()) {
        QUrlQuery query(url);
        query.addQueryItem("auth", m_databaseSecret);
        url.setQuery(query);
    }
    return url;
}

QString FirebaseRealtimeSyncManager::sanitizeKey(const QString& key) {
    QString normalized = key.trimmed().toLower();
    normalized.replace('.', '_');
    normalized.replace('#', '_');
    normalized.replace('$', '_');
    normalized.replace('[', '_');
    normalized.replace(']', '_');
    normalized.replace('/', '_');
    return normalized;
}

QString FirebaseRealtimeSyncManager::votingStateToWire(Core::VotingState state) {
    switch (state) {
        case Core::VotingState::Idle: return "idle";
        case Core::VotingState::Voting: return "voting";
        case Core::VotingState::Ended: return "ended";
        case Core::VotingState::Paused: return "paused";
        default: return "unknown";
    }
}

Core::VotingState FirebaseRealtimeSyncManager::votingStateFromWire(const QJsonValue& value) {
    const QString encoded = value.toString().trimmed().toLower();
    if (encoded == "idle") return Core::VotingState::Idle;
    if (encoded == "voting") return Core::VotingState::Voting;
    if (encoded == "ended") return Core::VotingState::Ended;
    if (encoded == "paused") return Core::VotingState::Paused;
    return Core::VotingState::Unknown;
}

QString FirebaseRealtimeSyncManager::roleToWire(Core::UserRole role) {
    return QString::number(static_cast<int>(role));
}

Core::UserRole FirebaseRealtimeSyncManager::roleFromWire(const QJsonValue& value) {
    bool ok = false;
    const int roleValue = value.toVariant().toInt(&ok);
    if (!ok) {
        return Core::UserRole::Observer;
    }
    if (roleValue < 0 || roleValue >= static_cast<int>(Core::UserRole::Count)) {
        return Core::UserRole::Observer;
    }
    return static_cast<Core::UserRole>(roleValue);
}

void FirebaseRealtimeSyncManager::setError(const QString& errorMessage, QString* outputErrorMessage) const {
    m_lastError = errorMessage;
    if (outputErrorMessage) {
        *outputErrorMessage = errorMessage;
    }
}

} // namespace Ballot::Integration
