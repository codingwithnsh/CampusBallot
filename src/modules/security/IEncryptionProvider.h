#pragma once

#include <QByteArray>
#include <QString>

namespace Ballot::Security {

class IEncryptionProvider {
public:
    virtual ~IEncryptionProvider() = default;

    virtual QByteArray encrypt(const QByteArray& data, const QByteArray& key) = 0;
    virtual QByteArray decrypt(const QByteArray& encryptedData, const QByteArray& key) = 0;
    virtual QByteArray generateKey(int size = 32) = 0;
    virtual QByteArray generateIv(int size = 16) = 0;
    virtual QString algorithm() const = 0;
};

} // namespace Ballot::Security
