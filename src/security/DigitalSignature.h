#pragma once

#include <QByteArray>
#include <QString>
#include <QPair>

namespace Ballot::Security {

class DigitalSignature {
public:
    static QPair<QByteArray, QByteArray> generateKeyPair();
    static QByteArray sign(const QByteArray& data, const QByteArray& privateKey);
    static bool verify(const QByteArray& data, const QByteArray& signature, const QByteArray& publicKey);
    static QByteArray signDocument(const QString& filePath, const QByteArray& privateKey);
    static bool verifyDocument(const QString& filePath, const QByteArray& signature, const QByteArray& publicKey);
};

} // namespace Ballot::Security
