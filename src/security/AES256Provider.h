#pragma once

#include "IEncryptionProvider.h"
#include <memory>

namespace Ballot::Security {

class AES256Provider : public IEncryptionProvider {
public:
    AES256Provider();
    ~AES256Provider() override;

    QByteArray encrypt(const QByteArray& data, const QByteArray& key) override;
    QByteArray decrypt(const QByteArray& encryptedData, const QByteArray& key) override;
    QByteArray generateKey(int size = 32) override;
    QByteArray generateIv(int size = 16) override;
    QString algorithm() const override { return "AES-256-CBC+HMAC"; }

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace Ballot::Security
