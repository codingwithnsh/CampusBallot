#pragma once

#include <QByteArray>
#include <QString>
#include <QDebug> // For logging

namespace Ballot::Security {

/**
 * @brief The HashProvider class provides various cryptographic hashing functionalities.
 *
 * This includes SHA256, HMAC-SHA256, and password hashing using PBKDF2 (as a fallback for Argon2).
 */
class HashProvider {
public:
    /**
     * @brief Computes the SHA256 hash of the given QByteArray data.
     * @param data The data to hash.
     * @return The SHA256 hash as a QByteArray.
     */
    static QByteArray sha256(const QByteArray& data);

    /**
     * @brief Computes the SHA256 hash of the given QString data.
     * @param data The data to hash.
     * @return The SHA256 hash as a QByteArray.
     */
    static QByteArray sha256(const QString& data);

    /**
     * @brief Computes the SHA256 hash of the file at the given path.
     * @param filePath The path to the file.
     * @return The SHA256 hash as a QByteArray, or empty if file cannot be read.
     */
    static QByteArray sha256File(const QString& filePath);

    /**
     * @brief Computes the SHA256 hash of the given QByteArray data and returns it as a hexadecimal string.
     * @param data The data to hash.
     * @return The SHA256 hash as a hexadecimal QByteArray.
     */
    static QByteArray sha256Hex(const QByteArray& data);

    /**
     * @brief Computes the SHA256 hash of the given QString data and returns it as a hexadecimal string.
     * @param data The data to hash.
     * @return The SHA256 hash as a hexadecimal QByteArray.
     */
    static QByteArray sha256Hex(const QString& data);

    /**
     * @brief Computes the HMAC-SHA256 of the given data using the provided key.
     * @param key The secret key.
     * @param data The data to hash.
     * @return The HMAC-SHA256 as a QByteArray.
     */
    static QByteArray hmacSha256(const QByteArray& key, const QByteArray& data);

    /**
     * @brief Hashes a password using PBKDF2-HMAC-SHA256 (as a fallback for Argon2).
     * @warning This implementation uses PBKDF2 as a fallback because Argon2 is not directly
     * available in Qt's standard library. For production, consider integrating a dedicated
     * Argon2 library for stronger password hashing.
     * @param password The password to hash.
     * @param salt The salt to use.
     * @return The hashed password as a QByteArray.
     */
    static QByteArray argon2Hash(const QString& password, const QByteArray& salt);

    /**
     * @brief Generates a cryptographically secure random salt.
     * @param size The desired size of the salt in bytes.
     * @return A QByteArray containing the random salt.
     */
    static QByteArray generateSalt(int size = 16);

    /**
     * @brief Verifies a password against a stored hash using PBKDF2-HMAC-SHA256 (as a fallback for Argon2).
     * @param password The password to verify.
     * @param hash The stored hash.
     * @param salt The salt used during hashing.
     * @return True if the password matches the hash, false otherwise.
     */
    static bool verifyArgon2(const QString& password, const QByteArray& hash, const QByteArray& salt);

    /**
     * @brief Implements PBKDF2-HMAC-SHA256 key derivation function.
     * @param password The master password.
     * @param salt The salt.
     * @param iterations The number of iterations.
     * @param keyLength The desired length of the derived key.
     * @return The derived key as a QByteArray.
     */
    static QByteArray pbkdf2(const QString& password, const QByteArray& salt, int iterations = 100000, int keyLength = 32);
};

} // namespace Ballot::Security