#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace btcwallet {

// ---- hash primitives ----

std::array<uint8_t, 32> sha256(std::span<const uint8_t> data);
std::array<uint8_t, 32> sha256d(std::span<const uint8_t> data);
std::array<uint8_t, 64> sha512(std::span<const uint8_t> data);
std::array<uint8_t, 32> sha512_half(std::span<const uint8_t> data); // first 32 of SHA512
std::array<uint8_t, 20> ripemd160(std::span<const uint8_t> data);
std::array<uint8_t, 20> hash160(std::span<const uint8_t> data); // RIPEMD160(SHA256(data))

// HMAC-SHA512 — returns 64 bytes
std::array<uint8_t, 64> hmac_sha512(std::span<const uint8_t> key,
                                     std::span<const uint8_t> data);

// ---- encoding ----

// Base58Check encode: base58(payload + sha256d(payload)[0..4])
std::string base58check_encode(std::span<const uint8_t> payload);

// Bech32 (witness v0) and Bech32m (witness v1+)
// data must already be the witness program bytes (not converted to 5-bit groups)
std::string bech32_encode(const std::string& hrp, int version,
                           std::span<const uint8_t> data);

// ---- hex utilities ----

std::string to_hex(std::span<const uint8_t> data);
std::vector<uint8_t> from_hex(const std::string& hex);

} // namespace btcwallet
