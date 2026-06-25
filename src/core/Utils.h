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

namespace Ballot::Core {

class Utils {
public:
    static QString getMachineId() {
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

    static QString getMachineName() {
        return QHostInfo::localHostName();
    }

    static QString getIpAddress() {
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

    static QString generateId() {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    static QString generateVotingId() {
        const QString chars = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
        QString id;
        for (int i = 0; i < 8; ++i) {
            id += chars.at(QRandomGenerator::global()->bounded(chars.size()));
        }
        return id;
    }

    static QJsonObject parseJsonFile(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return {};
        QByteArray data = file.readAll();
        file.close();
        return QJsonDocument::fromJson(data).object();
    }

    static QString appDataPath() {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(path);
        return path;
    }

    static QString formatBytes(qint64 bytes) {
        if (bytes < 1024) return QString::number(bytes) + " B";
        if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
        if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
        return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    }

    static QString truncateText(const QString& text, int maxLen) {
        if (text.length() <= maxLen) return text;
        return text.left(maxLen - 3) + "...";
    }
};

} // namespace Ballot::Core
