#include "HashProvider.h"
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <stdexcept>

namespace Ballot::Security {

QByteArray HashProvider::sha256(const QByteArray& data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

QByteArray HashProvider::sha256(const QString& data) {
    return sha256(data.toUtf8());
}

QByteArray HashProvider::sha256Hex(const QByteArray& data) {
    return sha256(data).toHex();
}

QByteArray HashProvider::sha256Hex(const QString& data) {
    return sha256Hex(data.toUtf8());
}

QByteArray HashProvider::hmacSha256(const QByteArray& key, const QByteArray& data) {
    return QMessageAuthenticationCode::hash(data, key, QCryptographicHash::Sha256);
}

QByteArray HashProvider::generateSalt(int size) {
    QByteArray salt(size, 0);
    auto* rng = QRandomGenerator::system();
    for (int i = 0; i < size; ++i) {
        salt[i] = static_cast<char>(rng->bounded(256));
    }
    return salt;
}

QByteArray HashProvider::argon2Hash(const QString& password, const QByteArray& salt) {
    // Argon2 is not available. Fall back to PBKDF2-HMAC-SHA256 with high iteration count.
    return pbkdf2(password, salt, 100000, 32);
}

bool HashProvider::verifyArgon2(const QString& password, const QByteArray& hash, const QByteArray& salt) {
    QByteArray computed = pbkdf2(password, salt, 100000, 32);
    return computed == hash;
}

QByteArray HashProvider::pbkdf2(const QString& password, const QByteArray& salt,
                                 int iterations, int keyLength) {
    // PBKDF2-HMAC-SHA256 per RFC 2898
    QByteArray key(keyLength, 0);
    QByteArray passBytes = password.toUtf8();
    int hLen = 32; // SHA-256 output length

    for (int block = 1; keyLength > 0; ++block) {
        // U1 = PRF(Password, Salt || INT(block))
        QByteArray blockBytes;
        blockBytes.append(salt);
        blockBytes.append(static_cast<char>((block >> 24) & 0xff));
        blockBytes.append(static_cast<char>((block >> 16) & 0xff));
        blockBytes.append(static_cast<char>((block >> 8) & 0xff));
        blockBytes.append(static_cast<char>(block & 0xff));

        QByteArray u = QMessageAuthenticationCode::hash(blockBytes, passBytes, QCryptographicHash::Sha256);
        QByteArray t = u;

        // U2..Uc
        for (int i = 1; i < iterations; ++i) {
            u = QMessageAuthenticationCode::hash(u, passBytes, QCryptographicHash::Sha256);
            for (int j = 0; j < hLen; ++j) {
                t[j] = t[j] ^ u[j];
            }
        }

        // Append T to derived key
        int copyLen = qMin(hLen, keyLength);
        std::memcpy(key.data() + (block - 1) * hLen, t.constData(), static_cast<size_t>(copyLen));
        keyLength -= copyLen;
    }

    return key;
}

} // namespace Ballot::Security
