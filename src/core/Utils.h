#pragma once

#include <QString>
#include <QSysInfo>
#include <QHostInfo>
#include <QCryptographicHash>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QRandomGenerator>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QFile>
#include <QDebug> // For logging errors

namespace Ballot::Core {

// Implementations for the new namespaces
namespace SystemInfo {
    inline QString getMachineId() {
        QByteArray id = QSysInfo::machineUniqueId();
        if (id.isEmpty()) {
            id = QHostInfo::localHostName().toUtf8();
            auto interfaces = QNetworkInterface::allInterfaces();
            for (const auto& iface : interfaces) {
                if (!iface.hardwareAddress().isEmpty()) {
                    id.append(iface.hardwareAddress().toUtf8());
                    break;
                }
            }
        }
        return QString(QCryptographicHash::hash(id, QCryptographicHash::Sha256).toHex());
    }

    inline QString getMachineName() {
        return QHostInfo::localHostName();
    }

    inline QString getIpAddress() {
        auto interfaces = QNetworkInterface::allInterfaces();
        for (const auto& iface : interfaces) {
            if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
                !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
                auto entries = iface.addressEntries();
                for (const auto& entry : entries) {
                    if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                        return entry.ip().toString();
                    }
                }
            }
        }
        return "127.0.0.1";
    }
} // namespace SystemInfo

namespace IdGenerator {
    inline QString generateId() {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    inline QString generateVotingId() {
        const QString chars = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
        QString id;
        for (int i = 0; i < 8; ++i) {
            id += chars.at(QRandomGenerator::global()->bounded(chars.size()));
        }
        return id;
    }
} // namespace IdGenerator

namespace FileUtil {
    inline QJsonObject parseJsonFile(const QString& path, QString* error = nullptr) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            if (error) *error = QString("Failed to open file: %1. Error: %2").arg(path, file.errorString());
            qWarning() << "FileUtil::parseJsonFile - Failed to open file:" << path << "Error:" << file.errorString();
            return {};
        }
        QByteArray data = file.readAll();
        file.close();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            if (error) *error = QString("Failed to parse JSON from file: %1. Error: %2").arg(path, parseError.errorString());
            qWarning() << "FileUtil::parseJsonFile - Failed to parse JSON from file:" << path << "Error:" << parseError.errorString();
            return {};
        }

        if (!doc.isObject()) {
            if (error) *error = QString("JSON document is not an object in file: %1").arg(path);
            qWarning() << "FileUtil::parseJsonFile - JSON document is not an object in file:" << path;
            return {};
        }

        return doc.object();
    }

    inline QString appDataPath() {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(path);
        return path;
    }
} // namespace FileUtil

namespace StringUtil {
    inline QString formatBytes(qint64 bytes) {
        if (bytes < 1024) return QString::number(bytes) + " B";
        if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
        if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
        return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    }

    inline QString truncateText(const QString& text, int maxLen) {
        if (text.length() <= maxLen) return text;
        return text.left(maxLen - 3) + "...";
    }
} // namespace StringUtil

// The original Utils class can remain for backward compatibility during refactoring,
// or be removed if all calls are updated to the new namespaces.
// For now, it acts as a facade.
class Utils {
public:
    static QString getMachineId() { return SystemInfo::getMachineId(); }
    static QString getMachineName() { return SystemInfo::getMachineName(); }
    static QString getIpAddress() { return SystemInfo::getIpAddress(); }
    static QString generateId() { return IdGenerator::generateId(); }
    static QString generateVotingId() { return IdGenerator::generateVotingId(); }
    static QJsonObject parseJsonFile(const QString& path) { return FileUtil::parseJsonFile(path); } // Simplified for backward compatibility
    static QString appDataPath() { return FileUtil::appDataPath(); }
    static QString formatBytes(qint64 bytes) { return StringUtil::formatBytes(bytes); }
    static QString truncateText(const QString& text, int maxLen) { return StringUtil::truncateText(text, maxLen); }
};

} // namespace Ballot::Core