#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace btcwallet {

enum class KeyType {
    ECDSA_COMPRESSED,    // 33-byte compressed public key → P2WPKH bech32 address
    ECDSA_UNCOMPRESSED,  // 65-byte uncompressed public key → P2PKH base58 address
    SCHNORR,             // 32-byte x-only public key (BIP-340) → P2TR bech32m address
};

struct DerivationPath {
    std::vector<uint32_t> indices;
    static constexpr uint32_t HARDENED = 0x80000000u;

    // Parse "m/44'/0'/0'/0/0" into indices (apostrophe → HARDENED flag)
    static DerivationPath parse(const std::string& path);
    bool empty() const { return indices.empty(); }
};

struct KeyGenParams {
    KeyType key_type = KeyType::ECDSA_COMPRESSED;

    // Custom 32-byte private key seed. If absent, OS randomness is used.
    std::optional<std::vector<uint8_t>> entropy;

    // BIP32 HD derivation path. Requires hd_seed or entropy as master material.
    // If hd_path is set but hd_seed is absent, 64 random bytes are used as master seed.
    std::optional<DerivationPath>       hd_path;
    std::optional<std::vector<uint8_t>> hd_seed; // 64-byte BIP32 master seed
};

struct KeyPair {
    std::vector<uint8_t> private_key_bytes; // always 32 bytes
    std::vector<uint8_t> public_key_bytes;  // 33 / 65 / 32 bytes
    KeyType              type;
    bool                 mainnet = true;

    std::string private_key_hex() const;
    std::string public_key_hex()  const;
    std::string private_key_wif() const; // base58check WIF
    std::string address()         const; // P2WPKH / P2PKH / P2TR
};

// Generate a key pair according to params.
// Throws std::runtime_error on invalid input or secp256k1 failure.
KeyPair generateKeyPair(const KeyGenParams& params, bool mainnet = true);

} // namespace btcwallet
