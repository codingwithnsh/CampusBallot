#include "DigitalSignature.h"
#include "HashProvider.h"
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QFile>
#include <stdexcept>

namespace Ballot::Security {

QPair<QByteArray, QByteArray> DigitalSignature::generateKeyPair() {
    // Use HMAC-SHA256 as a symmetric signing mechanism.
    // In production, replace with Ed25519 (asymmetric) via a plugin.
    auto* rng = QRandomGenerator::system();
    QByteArray key(32, 0);
    for (int i = 0; i < 32; ++i) {
        key[i] = static_cast<char>(rng->bounded(256));
    }
    // Return same key as both "private" and "public" (symmetric)
    return {key, key};
}

QByteArray DigitalSignature::sign(const QByteArray& data, const QByteArray& key) {
    // HMAC-SHA256(data, key)
    return QMessageAuthenticationCode::hash(data, key, QCryptographicHash::Sha256);
}

bool DigitalSignature::verify(const QByteArray& data, const QByteArray& signature,
                               const QByteArray& key) {
    QByteArray expected = sign(data, key);
    // Constant-time comparison to prevent timing attacks
    if (expected.size() != signature.size()) return false;
    int result = 0;
    for (int i = 0; i < expected.size(); ++i) {
        result |= static_cast<uint8_t>(expected[i]) ^ static_cast<uint8_t>(signature[i]);
    }
    return result == 0;
}

QByteArray DigitalSignature::signDocument(const QString& filePath, const QByteArray& key) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Failed to open file for signing");
    }
    QByteArray content = file.readAll();
    file.close();
    return sign(content, key);
}

bool DigitalSignature::verifyDocument(const QString& filePath, const QByteArray& signature,
                                       const QByteArray& key) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray content = file.readAll();
    file.close();
    return verify(content, signature, key);
}

} // namespace Ballot::Security
