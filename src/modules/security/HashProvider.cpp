#include "HashProvider.h"
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QFile>
#include <stdexcept>
#include <cstring> // For std::memcpy
#include <QDebug> // For logging

namespace Ballot::Security {

/**
 * @brief Computes the SHA256 hash of the given QByteArray data.
 * @param data The data to hash.
 * @return The SHA256 hash as a QByteArray.
 */
QByteArray HashProvider::sha256(const QByteArray& data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

/**
 * @brief Computes the SHA256 hash of the given QString data.
 * @param data The data to hash.
 * @return The SHA256 hash as a QByteArray.
 */
QByteArray HashProvider::sha256(const QString& data) {
    return sha256(data.toUtf8());
}

/**
 * @brief Computes the SHA256 hash of the file at the given path.
 * @param filePath The path to the file.
 * @return The SHA256 hash as a QByteArray, or empty if file cannot be read.
 */
QByteArray HashProvider::sha256File(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "HashProvider: Failed to open file for hashing:" << filePath << "-" << file.errorString();
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        qCritical() << "HashProvider: Failed to add file data to hash:" << filePath;
        file.close();
        return {};
    }
    file.close();
    return hash.result();
}

/**
 * @brief Computes the SHA256 hash of the given QByteArray data and returns it as a hexadecimal string.
 * @param data The data to hash.
 * @return The SHA256 hash as a hexadecimal QByteArray.
 */
QByteArray HashProvider::sha256Hex(const QByteArray& data) {
    return sha256(data).toHex();
}

/**
 * @brief Computes the SHA256 hash of the given QString data and returns it as a hexadecimal string.
 * @param data The data to hash.
 * @return The SHA256 hash as a hexadecimal QByteArray.
 */
QByteArray HashProvider::sha256Hex(const QString& data) {
    return sha256Hex(data.toUtf8());
}

/**
 * @brief Computes the HMAC-SHA256 of the given data using the provided key.
 * @param key The secret key.
 * @param data The data to hash.
 * @return The HMAC-SHA256 as a QByteArray.
 */
QByteArray HashProvider::hmacSha256(const QByteArray& key, const QByteArray& data) {
    return QMessageAuthenticationCode::hash(data, key, QCryptographicHash::Sha256);
}

/**
 * @brief Generates a cryptographically secure random salt.
 * @param size The desired size of the salt in bytes.
 * @return A QByteArray containing the random salt.
 */
QByteArray HashProvider::generateSalt(int size) {
    if (size <= 0) {
        qWarning() << "HashProvider: generateSalt called with non-positive size. Defaulting to 16 bytes.";
        size = 16; // Default salt size
    }
    QByteArray salt(size, 0);
    auto* rng = QRandomGenerator::system();
    for (int i = 0; i < size; ++i) {
        salt[i] = static_cast<char>(rng->bounded(256));
    }
    qDebug() << "HashProvider: Generated salt of size" << size;
    return salt;
}

/**
 * @brief Hashes a password using PBKDF2-HMAC-SHA256.
 * @warning This implementation uses PBKDF2 as a fallback because Argon2 is not directly
 * available in Qt's standard library. For production, consider integrating a dedicated
 * Argon2 library for stronger password hashing.
 * @param password The password to hash.
 * @param salt The salt to use.
 * @return The hashed password as a QByteArray.
 */
QByteArray HashProvider::argon2Hash(const QString& password, const QByteArray& salt) {
    qWarning() << "HashProvider: Using PBKDF2-HMAC-SHA256 as a fallback for Argon2. Consider integrating a dedicated Argon2 library for production.";
    // Fall back to PBKDF2-HMAC-SHA256 with high iteration count.
    // Iteration count and key length are hardcoded for consistency.
    return pbkdf2(password, salt, 100000, 32); // 100,000 iterations, 32-byte key
}

/**
 * @brief Verifies a password against a stored hash using PBKDF2-HMAC-SHA256.
 * @param password The password to verify.
 * @param hash The stored hash.
 * @param salt The salt used during hashing.
 * @return True if the password matches the hash, false otherwise.
 */
bool HashProvider::verifyArgon2(const QString& password, const QByteArray& hash, const QByteArray& salt) {
    qDebug() << "HashProvider: Verifying password using PBKDF2-HMAC-SHA256 fallback.";
    QByteArray computed = pbkdf2(password, salt, 100000, 32);
    // Use constant-time comparison to prevent timing attacks
    if (computed.size() != hash.size()) {
        return false;
    }
    volatile int result = 0;
    for (int i = 0; i < computed.size(); ++i) {
        result |= computed[i] ^ hash[i];
    }
    return result == 0;
}

/**
 * @brief Implements PBKDF2-HMAC-SHA256 key derivation function.
 * @param password The master password.
 * @param salt The salt.
 * @param iterations The number of iterations.
 * @param keyLength The desired length of the derived key.
 * @return The derived key as a QByteArray.
 */
QByteArray HashProvider::pbkdf2(const QString& password, const QByteArray& salt,
                                 int iterations, int keyLength) {
    if (password.isEmpty() || salt.isEmpty() || iterations <= 0 || keyLength <= 0) {
        qCritical() << "HashProvider: Invalid parameters for PBKDF2. Password empty:" << password.isEmpty()
                    << "Salt empty:" << salt.isEmpty() << "Iterations:" << iterations << "KeyLength:" << keyLength;
        throw std::invalid_argument("Invalid parameters for PBKDF2");
    }

    QByteArray derivedKey(keyLength, 0);
    QByteArray passBytes = password.toUtf8();
    int hLen = 32; // SHA-256 output length in bytes

    for (int block = 1; keyLength > 0; ++block) {
        // U1 = PRF(Password, Salt || INT(block))
        QByteArray blockBytes;
        blockBytes.append(salt);
        // Append 4-byte big-endian representation of block number
        blockBytes.append(static_cast<char>((block >> 24) & 0xFF));
        blockBytes.append(static_cast<char>((block >> 16) & 0xFF));
        blockBytes.append(static_cast<char>((block >> 8) & 0xFF));
        blockBytes.append(static_cast<char>(block & 0xFF));

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
        std::memcpy(derivedKey.data() + (block - 1) * hLen, t.constData(), static_cast<size_t>(copyLen));
        keyLength -= copyLen;
    }
    qDebug() << "HashProvider: PBKDF2 derived key of length" << derivedKey.size();
    return derivedKey;
}

} // namespace Ballot::Security