#pragma once

#include <QByteArray>
#include <QString>
#include <QPair>
#include <QDebug> // For logging

namespace Ballot::Security {

/**
 * @brief The DigitalSignature class provides functionality for generating and verifying
 * digital signatures.
 *
 * @warning CURRENT IMPLEMENTATION USES SYMMETRIC HMAC-SHA256. This is NOT a true
 * asymmetric digital signature scheme and is NOT suitable for production environments
 * where non-repudiation and public key verification are required.
 * This is a placeholder for a future, proper asymmetric cryptography implementation (e.g., Ed25519).
 */
class DigitalSignature {
public:
    /**
     * @brief Generates a symmetric key pair for digital signatures.
     * @return A QPair where first is the private key and second is the public key.
     */
    static QPair<QByteArray, QByteArray> generateKeyPair();

    /**
     * @brief Signs data using the provided private key (HMAC-SHA256).
     * @param data The data to sign.
     * @param privateKey The symmetric key used as the private key.
     * @return The HMAC-SHA256 signature.
     */
    static QByteArray sign(const QByteArray& data, const QByteArray& privateKey);

    /**
     * @brief Verifies a signature using the provided public key (HMAC-SHA256).
     * @param data The original data.
     * @param signature The signature to verify.
     * @param publicKey The symmetric key used as the public key.
     * @return True if the signature is valid, false otherwise.
     */
    static bool verify(const QByteArray& data, const QByteArray& signature, const QByteArray& publicKey);

    /**
     * @brief Signs the content of a file using the provided private key.
     * @param filePath The path to the file to sign.
     * @param privateKey The symmetric key used as the private key.
     * @return The HMAC-SHA256 signature of the file content.
     * @throws std::runtime_error if the file cannot be opened.
     */
    static QByteArray signDocument(const QString& filePath, const QByteArray& privateKey);

    /**
     * @brief Verifies the signature of a file using the provided public key.
     * @param filePath The path to the file to verify.
     * @param signature The signature to verify against.
     * @param publicKey The symmetric key used as the public key.
     * @return True if the signature is valid, false otherwise.
     */
    static bool verifyDocument(const QString& filePath, const QByteArray& signature, const QByteArray& publicKey);
};

} // namespace Ballot::Security