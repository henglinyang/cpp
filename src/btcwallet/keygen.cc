#include "keygen.h"
#include "crypto.h"

#include "secp256k1.h"
#include "secp256k1_extrakeys.h"
#include "secp256k1_schnorrsig.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace btcwallet {

// ---- OS entropy ----

static void os_random(uint8_t* buf, size_t len) {
    std::ifstream f("/dev/urandom", std::ios::binary);
    if (!f) throw std::runtime_error("cannot open /dev/urandom");
    f.read(reinterpret_cast<char*>(buf), static_cast<std::streamsize>(len));
    if (!f) throw std::runtime_error("failed to read /dev/urandom");
}

// ---- secp256k1 context (one per process, randomised once) ----

static secp256k1_context* ctx() {
    static secp256k1_context* s = []() -> secp256k1_context* {
        auto* c = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
        uint8_t seed[32];
        os_random(seed, 32);
        secp256k1_context_randomize(c, seed);
        return c;
    }();
    return s;
}

// ---- BN mod-n addition for BIP32 child key tweak ----
// We need (a + b) mod n where n is the secp256k1 group order.
// secp256k1_ec_seckey_tweak_add does exactly that.
static bool seckey_add_tweak(uint8_t key[32], const uint8_t tweak[32]) {
    return secp256k1_ec_seckey_tweak_add(ctx(), key, tweak) == 1;
}

// ---- BIP32 derivation ----

struct ExtKey {
    uint8_t privkey[32];
    uint8_t chaincode[32];
    uint8_t depth;
    uint8_t fingerprint[4];
    uint32_t child_index;

    static ExtKey from_seed(const uint8_t* seed, size_t seed_len) {
        static const uint8_t label[] = "Bitcoin seed";
        auto hmac = hmac_sha512({label, sizeof(label)-1}, {seed, seed_len});
        ExtKey k;
        memcpy(k.privkey,   hmac.data(),    32);
        memcpy(k.chaincode, hmac.data()+32, 32);
        k.depth = 0;
        memset(k.fingerprint, 0, 4);
        k.child_index = 0;
        // validate
        if (!secp256k1_ec_seckey_verify(ctx(), k.privkey))
            throw std::runtime_error("BIP32 master key invalid");
        return k;
    }

    ExtKey derive(uint32_t index) const {
        bool hardened = (index >= DerivationPath::HARDENED);

        // compute parent pubkey fingerprint
        secp256k1_pubkey pub;
        secp256k1_ec_pubkey_create(ctx(), &pub, privkey);
        uint8_t pubser[33]; size_t publen = 33;
        secp256k1_ec_pubkey_serialize(ctx(), pubser, &publen, &pub, SECP256K1_EC_COMPRESSED);

        // data = 0x00||privkey||index_BE  (hardened)  or  pubkey||index_BE (normal)
        uint8_t data[37];
        if (hardened) {
            data[0] = 0x00;
            memcpy(data+1, privkey, 32);
        } else {
            memcpy(data, pubser, 33);
        }
        data[33] = (index >> 24) & 0xff;
        data[34] = (index >> 16) & 0xff;
        data[35] = (index >>  8) & 0xff;
        data[36] = (index      ) & 0xff;

        auto hmac = hmac_sha512({chaincode, 32}, {data, 37});

        ExtKey child;
        memcpy(child.privkey, hmac.data(), 32);
        if (!seckey_add_tweak(child.privkey, privkey))
            throw std::runtime_error("BIP32 child key tweak failed");
        memcpy(child.chaincode, hmac.data()+32, 32);
        child.depth = depth + 1;
        // fingerprint = first 4 bytes of HASH160(parent_pubkey)
        auto h160 = hash160({pubser, 33});
        memcpy(child.fingerprint, h160.data(), 4);
        child.child_index = index;

        if (!secp256k1_ec_seckey_verify(ctx(), child.privkey))
            throw std::runtime_error("BIP32 derived key invalid");
        return child;
    }
};

// ---- DerivationPath::parse ----

DerivationPath DerivationPath::parse(const std::string& path) {
    DerivationPath dp;
    std::string s = path;
    if (s.substr(0,2) == "m/") s = s.substr(2);
    else if (s == "m")         return dp;

    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, '/')) {
        if (token.empty()) throw std::invalid_argument("empty path component");
        bool hardened = !token.empty() && token.back() == '\'';
        if (hardened) token.pop_back();
        uint32_t idx = static_cast<uint32_t>(std::stoul(token));
        if (hardened) idx |= HARDENED;
        dp.indices.push_back(idx);
    }
    return dp;
}

// ---- KeyPair methods ----

std::string KeyPair::private_key_hex() const { return to_hex(private_key_bytes); }
std::string KeyPair::public_key_hex()  const { return to_hex(public_key_bytes); }

std::string KeyPair::private_key_wif() const {
    std::vector<uint8_t> payload;
    payload.push_back(mainnet ? 0x80 : 0xef);
    payload.insert(payload.end(), private_key_bytes.begin(), private_key_bytes.end());
    if (type != KeyType::ECDSA_UNCOMPRESSED) payload.push_back(0x01);
    return base58check_encode(payload);
}

std::string KeyPair::address() const {
    const std::string hrp = mainnet ? "bc" : "tb";
    if (type == KeyType::ECDSA_COMPRESSED) {
        auto h = hash160(public_key_bytes);
        return bech32_encode(hrp, 0, h);       // P2WPKH
    } else if (type == KeyType::ECDSA_UNCOMPRESSED) {
        auto h = hash160(public_key_bytes);
        std::vector<uint8_t> payload;
        payload.push_back(mainnet ? 0x00 : 0x6f);
        payload.insert(payload.end(), h.begin(), h.end());
        return base58check_encode(payload);    // P2PKH
    } else {
        // SCHNORR: P2TR — witness version 1, 32-byte x-only key
        return bech32_encode(hrp, 1, public_key_bytes);
    }
}

// ---- generateKeyPair ----

KeyPair generateKeyPair(const KeyGenParams& params, bool mainnet) {
    std::vector<uint8_t> privkey_bytes(32);

    if (params.hd_path && !params.hd_path->empty()) {
        // BIP32 path derivation
        std::vector<uint8_t> seed(64);
        if (params.hd_seed) {
            if (params.hd_seed->size() != 64)
                throw std::runtime_error("hd_seed must be 64 bytes");
            seed = *params.hd_seed;
        } else if (params.entropy) {
            if (params.entropy->size() != 32)
                throw std::runtime_error("entropy must be 32 bytes");
            // stretch 32-byte entropy to 64-byte seed via SHA-512
            auto h = sha512(*params.entropy);
            seed = std::vector<uint8_t>(h.begin(), h.end());
        } else {
            os_random(seed.data(), 64);
        }

        ExtKey ext = ExtKey::from_seed(seed.data(), seed.size());
        for (uint32_t idx : params.hd_path->indices)
            ext = ext.derive(idx);
        memcpy(privkey_bytes.data(), ext.privkey, 32);

    } else if (params.entropy) {
        if (params.entropy->size() != 32)
            throw std::runtime_error("entropy must be exactly 32 bytes");
        privkey_bytes = *params.entropy;
        if (!secp256k1_ec_seckey_verify(ctx(), privkey_bytes.data()))
            throw std::runtime_error("provided entropy is not a valid secp256k1 private key");

    } else {
        // OS random, retry until valid (astronomically rare to need retry)
        for (int tries = 0; tries < 100; ++tries) {
            os_random(privkey_bytes.data(), 32);
            if (secp256k1_ec_seckey_verify(ctx(), privkey_bytes.data())) break;
            if (tries == 99) throw std::runtime_error("key generation failed after 100 tries");
        }
    }

    // Derive public key
    KeyPair kp;
    kp.private_key_bytes = privkey_bytes;
    kp.type    = params.key_type;
    kp.mainnet = mainnet;

    if (params.key_type == KeyType::ECDSA_COMPRESSED ||
        params.key_type == KeyType::ECDSA_UNCOMPRESSED) {
        secp256k1_pubkey pub;
        if (!secp256k1_ec_pubkey_create(ctx(), &pub, privkey_bytes.data()))
            throw std::runtime_error("secp256k1_ec_pubkey_create failed");
        unsigned int flags = (params.key_type == KeyType::ECDSA_COMPRESSED)
                             ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED;
        size_t publen = (flags == SECP256K1_EC_COMPRESSED) ? 33 : 65;
        kp.public_key_bytes.resize(publen);
        secp256k1_ec_pubkey_serialize(ctx(), kp.public_key_bytes.data(), &publen, &pub, flags);
    } else {
        // Schnorr: x-only pubkey
        secp256k1_keypair keypair;
        if (!secp256k1_keypair_create(ctx(), &keypair, privkey_bytes.data()))
            throw std::runtime_error("secp256k1_keypair_create failed");
        secp256k1_xonly_pubkey xonly;
        secp256k1_keypair_xonly_pub(ctx(), &xonly, nullptr, &keypair);
        kp.public_key_bytes.resize(32);
        secp256k1_xonly_pubkey_serialize(ctx(), kp.public_key_bytes.data(), &xonly);
    }

    return kp;
}

} // namespace btcwallet
