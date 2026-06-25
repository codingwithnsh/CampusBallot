#pragma once

#include <QByteArray>
#include <QString>

namespace Ballot::Security {

class HashProvider {
public:
    static QByteArray sha256(const QByteArray& data);
    static QByteArray sha256(const QString& data);
    static QByteArray sha256Hex(const QByteArray& data);
    static QByteArray sha256Hex(const QString& data);
    static QByteArray hmacSha256(const QByteArray& key, const QByteArray& data);
    static QByteArray argon2Hash(const QString& password, const QByteArray& salt);
    static QByteArray generateSalt(int size = 16);
    static bool verifyArgon2(const QString& password, const QByteArray& hash, const QByteArray& salt);
    static QByteArray pbkdf2(const QString& password, const QByteArray& salt, int iterations = 100000, int keyLength = 32);
};

} // namespace Ballot::Security
