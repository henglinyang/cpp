#include "keygen.h"
#include "crypto.h"

#include <cassert>
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

// ---- minimal test harness ----

struct TestCase { std::string name; std::function<void()> fn; };
static std::vector<TestCase>& tests() {
    static std::vector<TestCase> v;
    return v;
}
struct RegisterTest {
    RegisterTest(const char* n, std::function<void()> f) { tests().push_back({n, f}); }
};
#define TEST(suite, name) \
    static void test_##suite##_##name(); \
    static RegisterTest reg_##suite##_##name(#suite "." #name, test_##suite##_##name); \
    static void test_##suite##_##name()

static void check(bool cond, const std::string& msg, const char* file, int line) {
    if (!cond) throw std::runtime_error(std::string(file) + ":" + std::to_string(line) + ": " + msg);
}
#define EXPECT_TRUE(x)     check((x),        #x " is false",      __FILE__, __LINE__)
#define EXPECT_FALSE(x)    check(!(x),        #x " is true",       __FILE__, __LINE__)
#define EXPECT_EQ(a, b)    check((a) == (b), #a " != " #b,        __FILE__, __LINE__)
#define EXPECT_THROW(expr) do { \
    bool _threw = false; \
    try { expr; } catch (const std::exception&) { _threw = true; } \
    check(_threw, #expr " did not throw", __FILE__, __LINE__); \
} while (0)

using namespace btcwallet;

static std::vector<uint8_t> seed32(uint8_t fill = 0x01) {
    return std::vector<uint8_t>(32, fill);
}

// ---- KeyGen tests ----

TEST(KeyGen, DeterministicFromEntropy) {
    KeyGenParams p;
    p.entropy = seed32(0x42);
    auto kp1 = generateKeyPair(p);
    auto kp2 = generateKeyPair(p);
    EXPECT_EQ(kp1.private_key_hex(), kp2.private_key_hex());
    EXPECT_EQ(kp1.public_key_hex(),  kp2.public_key_hex());
}

TEST(KeyGen, RandomIsUnique) {
    auto kp1 = generateKeyPair({});
    auto kp2 = generateKeyPair({});
    EXPECT_FALSE(kp1.private_key_hex() == kp2.private_key_hex());
}

TEST(KeyGen, CompressedPubkeyLength) {
    KeyGenParams p;
    p.key_type = KeyType::ECDSA_COMPRESSED;
    p.entropy = seed32();
    auto kp = generateKeyPair(p);
    EXPECT_EQ(kp.public_key_bytes.size(), size_t(33));
}

TEST(KeyGen, UncompressedPubkeyLength) {
    KeyGenParams p;
    p.key_type = KeyType::ECDSA_UNCOMPRESSED;
    p.entropy = seed32();
    auto kp = generateKeyPair(p);
    EXPECT_EQ(kp.public_key_bytes.size(), size_t(65));
}

TEST(KeyGen, SchnorrPubkeyLength) {
    KeyGenParams p;
    p.key_type = KeyType::SCHNORR;
    p.entropy = seed32();
    auto kp = generateKeyPair(p);
    EXPECT_EQ(kp.public_key_bytes.size(), size_t(32));
}

TEST(KeyGen, WifMainnetCompressedPrefix) {
    KeyGenParams p;
    p.key_type = KeyType::ECDSA_COMPRESSED;
    p.entropy = seed32();
    auto kp = generateKeyPair(p, true);
    char first = kp.private_key_wif()[0];
    EXPECT_TRUE(first == 'K' || first == 'L');
}

TEST(KeyGen, WifMainnetUncompressedPrefix) {
    KeyGenParams p;
    p.key_type = KeyType::ECDSA_UNCOMPRESSED;
    p.entropy = seed32();
    auto kp = generateKeyPair(p, true);
    EXPECT_EQ(kp.private_key_wif()[0], '5');
}

TEST(KeyGen, P2WPKHAddress) {
    KeyGenParams p;
    p.key_type = KeyType::ECDSA_COMPRESSED;
    p.entropy = seed32();
    auto kp = generateKeyPair(p, true);
    auto addr = kp.address();
    // P2WPKH bech32 mainnet: "bc1q..."
    EXPECT_TRUE(addr.substr(0, 4) == "bc1q");
}

TEST(KeyGen, P2PKHAddress) {
    KeyGenParams p;
    p.key_type = KeyType::ECDSA_UNCOMPRESSED;
    p.entropy = seed32();
    auto kp = generateKeyPair(p, true);
    auto addr = kp.address();
    // P2PKH base58check mainnet: starts with '1'
    EXPECT_EQ(addr[0], '1');
}

TEST(KeyGen, P2TRAddress) {
    KeyGenParams p;
    p.key_type = KeyType::SCHNORR;
    p.entropy = seed32();
    auto kp = generateKeyPair(p, true);
    auto addr = kp.address();
    // P2TR bech32m mainnet: "bc1p..."
    EXPECT_TRUE(addr.substr(0, 4) == "bc1p");
}

TEST(KeyGen, BIP32DeterministicPath) {
    std::vector<uint8_t> hd_seed(64, 0xab);
    KeyGenParams p;
    p.hd_seed = hd_seed;
    p.hd_path = DerivationPath::parse("m/44'/0'/0'/0/0");

    auto kp1 = generateKeyPair(p);
    auto kp2 = generateKeyPair(p);
    EXPECT_EQ(kp1.private_key_hex(), kp2.private_key_hex());
}

TEST(KeyGen, BIP32DifferentPaths) {
    std::vector<uint8_t> hd_seed(64, 0xcd);
    KeyGenParams p1, p2;
    p1.hd_seed = hd_seed;
    p2.hd_seed = hd_seed;
    p1.hd_path = DerivationPath::parse("m/44'/0'/0'/0/0");
    p2.hd_path = DerivationPath::parse("m/44'/0'/0'/0/1");

    auto kp1 = generateKeyPair(p1);
    auto kp2 = generateKeyPair(p2);
    EXPECT_FALSE(kp1.private_key_hex() == kp2.private_key_hex());
}

TEST(KeyGen, BIP32HardenedVsNormal) {
    std::vector<uint8_t> hd_seed(64, 0xef);
    KeyGenParams p1, p2;
    p1.hd_seed = hd_seed;
    p2.hd_seed = hd_seed;
    p1.hd_path = DerivationPath::parse("m/44'");
    p2.hd_path = DerivationPath::parse("m/44");

    auto kp1 = generateKeyPair(p1);
    auto kp2 = generateKeyPair(p2);
    EXPECT_FALSE(kp1.private_key_hex() == kp2.private_key_hex());
}

TEST(KeyGen, InvalidEntropyTooShort) {
    KeyGenParams p;
    p.entropy = std::vector<uint8_t>(16, 0x01);  // only 16 bytes, not 32
    EXPECT_THROW(generateKeyPair(p));
}

// ---- Crypto tests ----

TEST(Crypto, SHA256KnownVector) {
    // SHA256("abc") = ba7816bf8f01cfea414140de5dae2ec73b00361bbef0469432f1c138580f4ed
    std::vector<uint8_t> data = {'a', 'b', 'c'};
    auto h = btcwallet::sha256(data);
    EXPECT_EQ(h.size(), size_t(32));
    // first 4 bytes of known hash
    EXPECT_EQ(h[0], uint8_t(0xba));
    EXPECT_EQ(h[1], uint8_t(0x78));
    EXPECT_EQ(h[2], uint8_t(0x16));
    EXPECT_EQ(h[3], uint8_t(0xbf));
}

TEST(Crypto, RIPEMD160KnownVector) {
    // RIPEMD160("abc") = 8eb208f7e05d987a9b044a8e98c6b087f15a0bfc
    std::vector<uint8_t> data = {'a', 'b', 'c'};
    auto h = btcwallet::ripemd160(data);
    EXPECT_EQ(h.size(), size_t(20));
    EXPECT_EQ(h[0], uint8_t(0x8e));
    EXPECT_EQ(h[1], uint8_t(0xb2));
    EXPECT_EQ(h[2], uint8_t(0x08));
    EXPECT_EQ(h[3], uint8_t(0xf7));
}

TEST(Crypto, Base58CheckRoundTrip) {
    // encode a known payload and verify the result is non-empty and stable
    std::vector<uint8_t> payload = {0x00, 0x01, 0x02, 0x03, 0x04};
    std::string enc1 = btcwallet::base58check_encode(payload);
    std::string enc2 = btcwallet::base58check_encode(payload);
    EXPECT_FALSE(enc1.empty());
    EXPECT_EQ(enc1, enc2);
}

TEST(Crypto, Bech32ValidP2WPKH) {
    // Encode a 20-byte payload with version 0 → "bc1q..." address
    std::vector<uint8_t> hash20(20, 0x00);
    std::string addr = btcwallet::bech32_encode("bc", 0, hash20);
    EXPECT_TRUE(addr.substr(0, 4) == "bc1q");
    EXPECT_EQ(addr.size(), size_t(42));  // bc1q + 38 chars for 20-byte payload
}

TEST(Crypto, ToHexFromHexRoundTrip) {
    std::vector<uint8_t> orig = {0xde, 0xad, 0xbe, 0xef, 0x00, 0xff};
    std::string hex = btcwallet::to_hex(orig);
    EXPECT_EQ(hex, std::string("deadbeef00ff"));
    auto back = btcwallet::from_hex(hex);
    EXPECT_EQ(back, orig);
}

// ---- main ----

int main() {
    int passed = 0, failed = 0;
    for (auto& tc : tests()) {
        try {
            tc.fn();
            printf("PASS  %s\n", tc.name.c_str());
            ++passed;
        } catch (const std::exception& e) {
            printf("FAIL  %s: %s\n", tc.name.c_str(), e.what());
            ++failed;
        }
    }
    printf("\n%d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
