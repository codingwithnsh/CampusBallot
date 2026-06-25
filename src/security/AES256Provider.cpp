#include "AES256Provider.h"
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QDebug>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace Ballot::Security {

// ---------------------------------------------------------------------------
// AES-256 lookup tables (standard FIPS-197)
// ---------------------------------------------------------------------------
static const uint8_t sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static const uint8_t inv_sbox[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

static const uint8_t rcon[11] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

static const int BLOCK_SIZE = 16;   // 128-bit
static const int KEY_SIZE   = 32;   // AES-256
static const int ROUNDS     = 14;   // AES-256
static const int IV_SIZE    = 16;

// ---------------------------------------------------------------------------
// AES-256 core helpers
// ---------------------------------------------------------------------------
static uint8_t xtime(uint8_t x) {
    return static_cast<uint8_t>((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}

static void subWord(uint8_t* w) {
    for (int i = 0; i < 4; ++i) w[i] = sbox[w[i]];
}

static void rotWord(uint8_t* w) {
    uint8_t t = w[0]; w[0] = w[1]; w[1] = w[2]; w[2] = w[3]; w[3] = t;
}

static void keyExpansion256(const uint8_t* key, uint8_t* roundKeys) {
    // roundKeys holds 15 * 16 = 240 bytes (14 rounds + initial key)
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 4; ++j) {
            roundKeys[i * 4 + j] = key[i * 4 + j];
        }
    }

    for (int i = 8; i < 60; ++i) {
        uint8_t temp[4];
        for (int j = 0; j < 4; ++j) temp[j] = roundKeys[(i - 1) * 4 + j];

        if (i % 8 == 0) {
            rotWord(temp);
            subWord(temp);
            temp[0] ^= rcon[i / 8];
        } else if (i % 8 == 4) {
            subWord(temp);
        }

        for (int j = 0; j < 4; ++j) {
            roundKeys[i * 4 + j] = roundKeys[(i - 8) * 4 + j] ^ temp[j];
        }
    }
}

static void addRoundKey(uint8_t* state, const uint8_t* roundKey) {
    for (int i = 0; i < 16; ++i) state[i] ^= roundKey[i];
}

static void subBytes(uint8_t* state) {
    for (int i = 0; i < 16; ++i) state[i] = sbox[state[i]];
}

static void invSubBytes(uint8_t* state) {
    for (int i = 0; i < 16; ++i) state[i] = inv_sbox[state[i]];
}

static void shiftRows(uint8_t* state) {
    uint8_t t;
    // Row 1: shift left 1
    t = state[1]; state[1] = state[5]; state[5] = state[9];
    state[9] = state[13]; state[13] = t;
    // Row 2: shift left 2
    t = state[2]; state[2] = state[10]; state[10] = t;
    t = state[6]; state[6] = state[14]; state[14] = t;
    // Row 3: shift left 3
    t = state[15]; state[15] = state[11]; state[11] = state[7];
    state[7] = state[3]; state[3] = t;
}

static void invShiftRows(uint8_t* state) {
    uint8_t t;
    // Row 1: shift right 1
    t = state[13]; state[13] = state[9]; state[9] = state[5];
    state[5] = state[1]; state[1] = t;
    // Row 2: shift right 2
    t = state[2]; state[2] = state[10]; state[10] = t;
    t = state[6]; state[6] = state[14]; state[14] = t;
    // Row 3: shift right 3
    t = state[3]; state[3] = state[7]; state[7] = state[11];
    state[11] = state[15]; state[15] = t;
}

static void mixColumns(uint8_t* state) {
    for (int i = 0; i < 4; ++i) {
        int idx = i * 4;
        uint8_t a0 = state[idx], a1 = state[idx + 1];
        uint8_t a2 = state[idx + 2], a3 = state[idx + 3];
        uint8_t t0 = xtime(a0 ^ a1 ^ a2 ^ a3);
        state[idx]     = a0 ^ a1 ^ xtime(a0 ^ a1) ^ t0;
        state[idx + 1] = a1 ^ a2 ^ xtime(a1 ^ a2) ^ t0;
        state[idx + 2] = a2 ^ a3 ^ xtime(a2 ^ a3) ^ t0;
        state[idx + 3] = a3 ^ a0 ^ xtime(a3 ^ a0) ^ t0;
    }
}

static void invMixColumns(uint8_t* state) {
    for (int i = 0; i < 4; ++i) {
        int idx = i * 4;
        uint8_t a0 = state[idx], a1 = state[idx + 1];
        uint8_t a2 = state[idx + 2], a3 = state[idx + 3];
        uint8_t t0 = xtime(xtime(a0 ^ a2));
        uint8_t t1 = xtime(xtime(a1 ^ a3));
        state[idx]     = a0 ^ t0 ^ xtime(a0 ^ a1 ^ a2 ^ a3);
        state[idx + 1] = a1 ^ t1 ^ xtime(a0 ^ a1 ^ a2 ^ a3);
        state[idx + 2] = a2 ^ t0 ^ xtime(a0 ^ a1 ^ a2 ^ a3);
        state[idx + 3] = a3 ^ t1 ^ xtime(a0 ^ a1 ^ a2 ^ a3);
    }
}

static void aes256EncryptBlock(const uint8_t* roundKeys, uint8_t* state) {
    addRoundKey(state, roundKeys);
    for (int r = 1; r <= ROUNDS; ++r) {
        subBytes(state);
        shiftRows(state);
        if (r < ROUNDS) mixColumns(state);
        addRoundKey(state, roundKeys + r * BLOCK_SIZE);
    }
}

static void aes256DecryptBlock(const uint8_t* roundKeys, uint8_t* state) {
    addRoundKey(state, roundKeys + ROUNDS * BLOCK_SIZE);
    for (int r = ROUNDS - 1; r >= 0; --r) {
        invShiftRows(state);
        invSubBytes(state);
        addRoundKey(state, roundKeys + r * BLOCK_SIZE);
        if (r > 0) invMixColumns(state);
    }
}

// ---------------------------------------------------------------------------
// PKCS7 padding
// ---------------------------------------------------------------------------
static QByteArray pkcs7Pad(const QByteArray& data, int blockSize) {
    int padLen = blockSize - (data.size() % blockSize);
    QByteArray padded = data;
    padded.append(QByteArray(padLen, static_cast<char>(padLen)));
    return padded;
}

static QByteArray pkcs7Unpad(const QByteArray& data) {
    if (data.isEmpty()) return data;
    int padLen = static_cast<uint8_t>(data.at(data.size() - 1));
    if (padLen <= 0 || padLen > BLOCK_SIZE) throw std::runtime_error("Invalid padding");
    return data.left(data.size() - padLen);
}

// ---------------------------------------------------------------------------
// AES256Provider implementation
// ---------------------------------------------------------------------------
class AES256Provider::Impl {
public:
    static constexpr int KEY_SIZE_BYTES = 32;
    static constexpr int IV_SIZE_BYTES = 16;
    static constexpr int MAC_SIZE_BYTES = 32; // SHA-256

    QByteArray encrypt(const QByteArray& data, const QByteArray& key) {
        if (key.size() != KEY_SIZE_BYTES) throw std::runtime_error("Invalid key size");

        // Generate random IV
        QByteArray iv = generateIv(IV_SIZE_BYTES);

        // Expand key
        uint8_t roundKeys[240];
        keyExpansion256(reinterpret_cast<const uint8_t*>(key.constData()), roundKeys);

        // Pad and encrypt in CBC mode
        QByteArray padded = pkcs7Pad(data, BLOCK_SIZE);
        QByteArray ciphertext;
        ciphertext.reserve(padded.size());

        uint8_t prev[BLOCK_SIZE];
        std::memcpy(prev, iv.constData(), BLOCK_SIZE);

        for (int i = 0; i < padded.size(); i += BLOCK_SIZE) {
            uint8_t block[BLOCK_SIZE];
            std::memcpy(block, padded.constData() + i, BLOCK_SIZE);

            // CBC: XOR with previous ciphertext (or IV)
            for (int j = 0; j < BLOCK_SIZE; ++j) block[j] ^= prev[j];

            aes256EncryptBlock(roundKeys, block);
            ciphertext.append(reinterpret_cast<const char*>(block), BLOCK_SIZE);
            std::memcpy(prev, block, BLOCK_SIZE);
        }

        // Compute HMAC-SHA256 over IV + ciphertext (encrypt-then-MAC)
        QByteArray authData = iv + ciphertext;
        QByteArray mac = QMessageAuthenticationCode::hash(
            authData, key, QCryptographicHash::Sha256);

        // Output: IV (16) + ciphertext (N) + MAC (32)
        return authData + mac;
    }

    QByteArray decrypt(const QByteArray& encryptedData, const QByteArray& key) {
        if (key.size() != KEY_SIZE_BYTES) throw std::runtime_error("Invalid key size");
        if (encryptedData.size() < IV_SIZE_BYTES + MAC_SIZE_BYTES + BLOCK_SIZE)
            throw std::runtime_error("Invalid data size");

        // Split: IV | ciphertext | MAC
        QByteArray iv = encryptedData.left(IV_SIZE_BYTES);
        QByteArray mac = encryptedData.right(MAC_SIZE_BYTES);
        QByteArray ciphertext = encryptedData.mid(IV_SIZE_BYTES,
            encryptedData.size() - IV_SIZE_BYTES - MAC_SIZE_BYTES);

        // Verify MAC first
        QByteArray authData = iv + ciphertext;
        QByteArray computedMac = QMessageAuthenticationCode::hash(
            authData, key, QCryptographicHash::Sha256);

        if (computedMac != mac) {
            throw std::runtime_error("Decryption failed: authentication tag mismatch");
        }

        // Expand key
        uint8_t roundKeys[240];
        keyExpansion256(reinterpret_cast<const uint8_t*>(key.constData()), roundKeys);

        // Decrypt in CBC mode
        QByteArray plaintext;
        plaintext.reserve(ciphertext.size());

        uint8_t prev[BLOCK_SIZE];
        std::memcpy(prev, iv.constData(), BLOCK_SIZE);

        for (int i = 0; i < ciphertext.size(); i += BLOCK_SIZE) {
            uint8_t block[BLOCK_SIZE];
            std::memcpy(block, ciphertext.constData() + i, BLOCK_SIZE);

            uint8_t decrypted[BLOCK_SIZE];
            std::memcpy(decrypted, block, BLOCK_SIZE);
            aes256DecryptBlock(roundKeys, decrypted);

            // CBC: XOR with previous ciphertext (or IV)
            for (int j = 0; j < BLOCK_SIZE; ++j) decrypted[j] ^= prev[j];

            plaintext.append(reinterpret_cast<const char*>(decrypted), BLOCK_SIZE);
            std::memcpy(prev, block, BLOCK_SIZE);
        }

        // Remove PKCS7 padding
        return pkcs7Unpad(plaintext);
    }

    QByteArray generateKey(int size) {
        if (size <= 0) size = KEY_SIZE_BYTES;
        QByteArray key(size, 0);
        auto* rng = QRandomGenerator::system();
        for (int i = 0; i < size; ++i) {
            key[i] = static_cast<char>(rng->bounded(256));
        }
        return key;
    }

    QByteArray generateIv(int size) {
        return generateKey(size);
    }
};

AES256Provider::AES256Provider() : d(std::make_unique<Impl>()) {}
AES256Provider::~AES256Provider() = default;

QByteArray AES256Provider::encrypt(const QByteArray& data, const QByteArray& key) {
    return d->encrypt(data, key);
}

QByteArray AES256Provider::decrypt(const QByteArray& encryptedData, const QByteArray& key) {
    return d->decrypt(encryptedData, key);
}

QByteArray AES256Provider::generateKey(int size) {
    return d->generateKey(size);
}

QByteArray AES256Provider::generateIv(int size) {
    return d->generateIv(size);
}

} // namespace Ballot::Security
