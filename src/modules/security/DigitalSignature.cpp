#include "DigitalSignature.h"
#include "HashProvider.h"
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QFile>
#include <stdexcept>
#include <QDebug> // For logging

namespace Ballot::Security {

/**
 * @brief Generates a symmetric key pair for digital signatures.
 * @note In this implementation, it generates a single symmetric key (HMAC-SHA256)
 * and returns it as both the "private" and "public" key.
 * For production, this should be replaced with an asymmetric (e.g., Ed25519) key pair.
 * @return A QPair where first is the private key and second is the public key.
 */
QPair<QByteArray, QByteArray> DigitalSignature::generateKeyPair() {
    qWarning() << "DigitalSignature: Using symmetric HMAC-SHA256 for key pair generation. This is NOT suitable for true digital signatures in a production environment. Replace with asymmetric cryptography (e.g., Ed25519).";
    auto* rng = QRandomGenerator::system();
    QByteArray key(32, 0); // 32 bytes for HMAC-SHA256 key
    for (int i = 0; i < 32; ++i) {
        key[i] = static_cast<char>(rng->bounded(256));
    }
    // Return same key as both "private" and "public" (symmetric)
    return {key, key};
}

/**
 * @brief Signs data using HMAC-SHA256.
 * @param data The data to sign.
 * @param key The symmetric key for signing.
 * @return The HMAC-SHA256 signature.
 */
QByteArray DigitalSignature::sign(const QByteArray& data, const QByteArray& key) {
    if (key.size() != 32) {
        qCritical() << "DigitalSignature: Invalid key size for signing. Expected 32 bytes.";
        throw std::runtime_error("Invalid key size for signing");
    }
    return QMessageAuthenticationCode::hash(data, key, QCryptographicHash::Sha256);
}

/**
 * @brief Verifies a signature using HMAC-SHA256.
 * @param data The original data.
 * @param signature The signature to verify.
 * @param key The symmetric key for verification.
 * @return True if the signature is valid, false otherwise.
 */
bool DigitalSignature::verify(const QByteArray& data, const QByteArray& signature,
                               const QByteArray& key) {
    if (key.size() != 32) {
        qCritical() << "DigitalSignature: Invalid key size for verification. Expected 32 bytes.";
        return false;
    }
    QByteArray expected = sign(data, key);
    // Constant-time comparison to prevent timing attacks
    if (expected.size() != signature.size()) {
        qWarning() << "DigitalSignature: Signature size mismatch during verification.";
        return false;
    }
    int result = 0;
    for (int i = 0; i < expected.size(); ++i) {
        result |= static_cast<uint8_t>(expected[i]) ^ static_cast<uint8_t>(signature[i]);
    }
    if (result == 0) {
        qDebug() << "DigitalSignature: Signature verified successfully.";
    } else {
        qWarning() << "DigitalSignature: Signature verification failed.";
    }
    return result == 0;
}

/**
 * @brief Signs the content of a file using HMAC-SHA256.
 * @param filePath The path to the file to sign.
 * @param key The symmetric key for signing.
 * @return The HMAC-SHA256 signature of the file content.
 * @throws std::runtime_error if the file cannot be opened.
 */
QByteArray DigitalSignature::signDocument(const QString& filePath, const QByteArray& key) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "DigitalSignature: Failed to open file for signing:" << filePath << "-" << file.errorString();
        throw std::runtime_error("Failed to open file for signing");
    }
    QByteArray content = file.readAll();
    file.close();
    qDebug() << "DigitalSignature: Signing document:" << filePath;
    return sign(content, key);
}

/**
 * @brief Verifies the signature of a file using HMAC-SHA256.
 * @param filePath The path to the file to verify.
 * @param signature The signature to verify against.
 * @param key The symmetric key for verification.
 * @return True if the signature is valid, false otherwise.
 */
bool DigitalSignature::verifyDocument(const QString& filePath, const QByteArray& signature,
                                       const QByteArray& key) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "DigitalSignature: Failed to open file for verification:" << filePath << "-" << file.errorString();
        return false;
    }
    QByteArray content = file.readAll();
    file.close();
    qDebug() << "DigitalSignature: Verifying document:" << filePath;
    return verify(content, signature, key);
}

} // namespace Ballot::Security