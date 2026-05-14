#include "chainstate.h"
#include "src/btcwallet/crypto.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include "leveldb/db.h"

namespace fs = std::filesystem;

// ---- minimal test harness ----

struct TestCase { std::string name; std::function<void()> fn; };
static std::vector<TestCase>& tests() { static std::vector<TestCase> v; return v; }
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
#define EXPECT_TRUE(x)    check((x),      #x " is false",          __FILE__, __LINE__)
#define EXPECT_EQ(a, b)   check((a)==(b), #a " != " #b,            __FILE__, __LINE__)
#define EXPECT_NE(a, b)   check((a)!=(b), #a " == " #b,            __FILE__, __LINE__)
#define EXPECT_THROWS(x)  do { bool threw = false; \
    try { x; } catch (...) { threw = true; } \
    check(threw, #x " did not throw", __FILE__, __LINE__); } while(0)

// ---- Bitcoin Core encoding helpers ----

// CVarInt: big-endian, 7 bits per byte, MSB = more bytes follow.
// Mirrors Bitcoin Core's WriteVarInt()/serialize.h.
static std::vector<uint8_t> encode_cvarint(uint64_t n) {
    uint8_t tmp[10];
    int len = 0;
    while (true) {
        tmp[len] = static_cast<uint8_t>((n & 0x7f) | (len ? 0x80 : 0x00));
        if (n <= 0x7f) break;
        n = (n >> 7) - 1;
        ++len;
    }
    std::vector<uint8_t> out;
    for (int i = len; i >= 0; --i) out.push_back(tmp[i]);
    return out;
}

// CompressAmount: mirrors Bitcoin Core's serialize.h CompressAmount().
static uint64_t compress_amount(uint64_t n) {
    if (n == 0) return 0;
    int e = 0;
    while (n % 10 == 0 && e < 9) { n /= 10; ++e; }
    if (e < 9) {
        int d = static_cast<int>(n % 10);
        n /= 10;
        return 1 + (n * 9 + static_cast<uint64_t>(d) - 1) * 10 + static_cast<uint64_t>(e);
    } else {
        return 1 + (n - 1) * 10 + 9;
    }
}

static void sappend(std::string& s, const std::vector<uint8_t>& v) {
    s.append(reinterpret_cast<const char*>(v.data()), v.size());
}

// Build the LevelDB key for a coin entry.
// txid_display: 32-byte txid in standard display order (big-endian).
// Stored reversed (internal byte order) in the key.
static std::string make_coin_key(const std::vector<uint8_t>& txid_display, uint32_t vout) {
    std::string key;
    key += 'C';
    for (int i = 31; i >= 0; --i) key += static_cast<char>(txid_display[i]);
    sappend(key, encode_cvarint(vout));
    return key;
}

// Build the LevelDB value for a coin entry (unobfuscated).
static std::string make_coin_value(uint32_t height, bool coinbase,
                                    int64_t satoshis,
                                    uint64_t nsize,
                                    const std::vector<uint8_t>& script_extra) {
    std::string val;
    sappend(val, encode_cvarint(static_cast<uint64_t>(height) * 2 + (coinbase ? 1 : 0)));
    sappend(val, encode_cvarint(compress_amount(static_cast<uint64_t>(satoshis))));
    sappend(val, encode_cvarint(nsize));
    val.append(reinterpret_cast<const char*>(script_extra.data()), script_extra.size());
    return val;
}

static void xor_obfuscate(std::string& data, const std::vector<uint8_t>& key) {
    for (size_t i = 0; i < data.size(); ++i)
        data[i] ^= static_cast<char>(key[i % key.size()]);
}

// ---- TempDb: RAII wrapper for a test LevelDB directory ----

struct TempDb {
    std::string dir;
    leveldb::DB* db = nullptr;

    TempDb() {
        char tmpl[] = "/tmp/btcutxo_test_XXXXXX";
        if (!mkdtemp(tmpl)) throw std::runtime_error("mkdtemp failed");
        dir = tmpl;
        leveldb::Options opts;
        opts.create_if_missing = true;
        auto s = leveldb::DB::Open(opts, dir, &db);
        if (!s.ok()) throw std::runtime_error("cannot create test db: " + s.ToString());
    }

    ~TempDb() {
        delete db;
        fs::remove_all(dir);
    }

    void put(const std::string& key, const std::string& val) {
        auto s = db->Put(leveldb::WriteOptions(), key, val);
        if (!s.ok()) throw std::runtime_error("leveldb Put: " + s.ToString());
    }

    // Write a UTXO entry with a P2PKH-style script (nsize=0, 20-byte hash).
    void put_p2pkh(const std::vector<uint8_t>& txid, uint32_t vout,
                   uint32_t height, bool coinbase, int64_t satoshis,
                   const std::vector<uint8_t>& hash20,
                   const std::vector<uint8_t>& obf_key = {}) {
        std::string val = make_coin_value(height, coinbase, satoshis, 0, hash20);
        if (!obf_key.empty()) xor_obfuscate(val, obf_key);
        put(make_coin_key(txid, vout), val);
    }

    // Generic UTXO with any nsize and script payload.
    void put_utxo(const std::vector<uint8_t>& txid, uint32_t vout,
                  uint32_t height, bool coinbase, int64_t satoshis,
                  uint64_t nsize, const std::vector<uint8_t>& script_extra,
                  const std::vector<uint8_t>& obf_key = {}) {
        std::string val = make_coin_value(height, coinbase, satoshis, nsize, script_extra);
        if (!obf_key.empty()) xor_obfuscate(val, obf_key);
        put(make_coin_key(txid, vout), val);
    }

    // Set the chainstate obfuscation key entry.
    void set_obfuscation_key(const std::vector<uint8_t>& key) {
        std::string db_key = std::string("\x0e", 1) + "obfuscate_key";
        std::string val = std::string("\x08", 1);  // length prefix byte (ignored by parser)
        val.append(reinterpret_cast<const char*>(key.data()), key.size());
        put(db_key, val);
    }

    // Close DB and run scan_chainstate, returning all UTXOs found.
    // After this call, the TempDb is no longer writable.
    std::vector<btcutxo::Utxo> close_and_scan(bool mainnet = true) {
        delete db;
        db = nullptr;
        btcutxo::ScanOptions opts;
        opts.mainnet = mainnet;
        std::vector<btcutxo::Utxo> result;
        btcutxo::scan_chainstate(dir, opts, [&](const btcutxo::Utxo& u) {
            result.push_back(u);
        });
        return result;
    }
};

// ---- Standard test data ----

static const std::vector<uint8_t> TXID_A(32, 0x01);
static const std::vector<uint8_t> TXID_B(32, 0x02);
static const std::vector<uint8_t> HASH20_A(20, 0xaa);
static const std::vector<uint8_t> HASH20_B(20, 0xbb);
static const std::vector<uint8_t> HASH32_A(32, 0xcc);
static const std::vector<uint8_t> XCOORD_A(32, 0xdd);

// ---- Tests ----

// ------ CVarInt round-trip (via vout field) ------

TEST(CVarInt, VoutZero) {
    TempDb db;
    db.put_p2pkh(TXID_A, 0, 100, false, 1, HASH20_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);
    EXPECT_EQ(utxos[0].vout, 0u);
}

TEST(CVarInt, Vout127) {
    TempDb db;
    db.put_p2pkh(TXID_A, 127, 100, false, 1, HASH20_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);
    EXPECT_EQ(utxos[0].vout, 127u);
}

TEST(CVarInt, Vout128TwoBytes) {
    // 128 requires two CVarInt bytes [0x80, 0x00]
    TempDb db;
    db.put_p2pkh(TXID_A, 128, 100, false, 1, HASH20_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);
    EXPECT_EQ(utxos[0].vout, 128u);
}

TEST(CVarInt, Vout16383) {
    TempDb db;
    db.put_p2pkh(TXID_A, 16383, 100, false, 1, HASH20_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);
    EXPECT_EQ(utxos[0].vout, 16383u);
}

// ------ Amount round-trip ------

TEST(Amount, ZeroSatoshis) {
    TempDb db;
    db.put_p2pkh(TXID_A, 0, 1, false, 0, HASH20_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);
    EXPECT_EQ(utxos[0].satoshis, 0);
}

TEST(Amount, OneSatoshi) {
    TempDb db;
    db.put_p2pkh(TXID_A, 0, 1, false, 1, HASH20_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);
    EXPECT_EQ(utxos[0].satoshis, 1);
}

TEST(Amount, OneMBTC) {
    // 0.001 BTC = 100000 satoshis
    TempDb db;
    db.put_p2pkh(TXID_A, 0, 1, false, 100000, HASH20_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);
    EXPECT_EQ(utxos[0].satoshis, 100000);
}

TEST(Amount, OneBTC) {
    // 1 BTC = 100000000 satoshis
    TempDb db;
    db.put_p2pkh(TXID_A, 0, 1, false, 100000000, HASH20_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);
    EXPECT_EQ(utxos[0].satoshis, 100000000);
}

// ------ Script type → address ------

TEST(Script, P2PKH) {
    TempDb db;
    db.put_p2pkh(TXID_A, 0, 1, false, 1, HASH20_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);

    std::vector<uint8_t> payload = {0x00};
    payload.insert(payload.end(), HASH20_A.begin(), HASH20_A.end());
    std::string expected = btcwallet::base58check_encode(payload);
    EXPECT_EQ(utxos[0].address, expected);
    EXPECT_TRUE(utxos[0].address[0] == '1');  // mainnet P2PKH starts with '1'
}

TEST(Script, P2SH) {
    TempDb db;
    // nsize=1, 20-byte hash follows
    db.put_utxo(TXID_A, 0, 1, false, 1, 1, HASH20_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);

    std::vector<uint8_t> payload = {0x05};
    payload.insert(payload.end(), HASH20_A.begin(), HASH20_A.end());
    std::string expected = btcwallet::base58check_encode(payload);
    EXPECT_EQ(utxos[0].address, expected);
    EXPECT_TRUE(utxos[0].address[0] == '3');  // mainnet P2SH starts with '3'
}

TEST(Script, P2PKCompressedEven) {
    // nsize=2: 32-byte x-coord, prefix 0x02 → P2PKH of compressed pubkey
    TempDb db;
    db.put_utxo(TXID_A, 0, 1, false, 1, 2, XCOORD_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);

    std::vector<uint8_t> pubkey = {0x02};
    pubkey.insert(pubkey.end(), XCOORD_A.begin(), XCOORD_A.end());
    auto h = btcwallet::hash160(std::span<const uint8_t>(pubkey));
    std::vector<uint8_t> payload = {0x00};
    payload.insert(payload.end(), h.begin(), h.end());
    EXPECT_EQ(utxos[0].address, btcwallet::base58check_encode(payload));
}

TEST(Script, P2PKCompressedOdd) {
    // nsize=3: 32-byte x-coord, prefix 0x03 → P2PKH of compressed pubkey
    TempDb db;
    db.put_utxo(TXID_A, 0, 1, false, 1, 3, XCOORD_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);

    std::vector<uint8_t> pubkey = {0x03};
    pubkey.insert(pubkey.end(), XCOORD_A.begin(), XCOORD_A.end());
    auto h = btcwallet::hash160(std::span<const uint8_t>(pubkey));
    std::vector<uint8_t> payload = {0x00};
    payload.insert(payload.end(), h.begin(), h.end());
    EXPECT_EQ(utxos[0].address, btcwallet::base58check_encode(payload));
    // Even vs odd produce different addresses
    std::vector<uint8_t> pubkey2 = {0x02};
    pubkey2.insert(pubkey2.end(), XCOORD_A.begin(), XCOORD_A.end());
    auto h2 = btcwallet::hash160(std::span<const uint8_t>(pubkey2));
    std::vector<uint8_t> payload2 = {0x00};
    payload2.insert(payload2.end(), h2.begin(), h2.end());
    EXPECT_NE(utxos[0].address, btcwallet::base58check_encode(payload2));
}

TEST(Script, P2WPKHRawScript) {
    // P2WPKH: OP_0 <20>, stored as raw script with nsize=22+6=28
    std::vector<uint8_t> script = {0x00, 0x14};
    script.insert(script.end(), HASH20_A.begin(), HASH20_A.end());
    TempDb db;
    db.put_utxo(TXID_A, 0, 1, false, 1, 28, script);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);

    std::string expected = btcwallet::bech32_encode("bc", 0, HASH20_A);
    EXPECT_EQ(utxos[0].address, expected);
    EXPECT_TRUE(utxos[0].address.substr(0, 4) == "bc1q");
}

TEST(Script, P2WSHRawScript) {
    // P2WSH: OP_0 <32>, stored as raw script with nsize=34+6=40
    std::vector<uint8_t> script = {0x00, 0x20};
    script.insert(script.end(), HASH32_A.begin(), HASH32_A.end());
    TempDb db;
    db.put_utxo(TXID_A, 0, 1, false, 1, 40, script);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);

    std::string expected = btcwallet::bech32_encode("bc", 0, HASH32_A);
    EXPECT_EQ(utxos[0].address, expected);
    // P2WSH bc1q is 62 characters
    EXPECT_EQ(utxos[0].address.size(), 62u);
}

TEST(Script, P2TRRawScript) {
    // P2TR: OP_1 (0x51) <32>, nsize=34+6=40
    std::vector<uint8_t> script = {0x51, 0x20};
    script.insert(script.end(), HASH32_A.begin(), HASH32_A.end());
    TempDb db;
    db.put_utxo(TXID_A, 0, 1, false, 1, 40, script);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);

    std::string expected = btcwallet::bech32_encode("bc", 1, HASH32_A);
    EXPECT_EQ(utxos[0].address, expected);
    EXPECT_TRUE(utxos[0].address.substr(0, 4) == "bc1p");
}

TEST(Script, UnknownScriptEmptyAddress) {
    // An unrecognized raw script → empty address, but UTXO still emitted
    std::vector<uint8_t> script = {0xde, 0xad};  // 2 bytes: nsize=8
    TempDb db;
    db.put_utxo(TXID_A, 0, 1, false, 999, 8, script);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);
    EXPECT_TRUE(utxos[0].address.empty());
    EXPECT_EQ(utxos[0].satoshis, 999);
}

// ------ UTXO metadata ------

TEST(Meta, HeightDecoding) {
    TempDb db;
    db.put_p2pkh(TXID_A, 0, 840000, false, 1, HASH20_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);
    EXPECT_EQ(utxos[0].height, 840000u);
}

TEST(Meta, CoinbaseTrue) {
    TempDb db;
    db.put_p2pkh(TXID_A, 0, 100, true, 5000000000LL, HASH20_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);
    EXPECT_TRUE(utxos[0].coinbase);
}

TEST(Meta, CoinbaseFalse) {
    TempDb db;
    db.put_p2pkh(TXID_A, 0, 100, false, 1, HASH20_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);
    EXPECT_TRUE(!utxos[0].coinbase);
}

TEST(Meta, TxidByteReversal) {
    // txid in display order: first byte 0xab, rest zeros.
    // The key stores bytes reversed (internal order): last byte is 0xab.
    // Parser must reverse back → display hex must start with "ab".
    std::vector<uint8_t> txid(32, 0x00);
    txid[0] = 0xab;

    TempDb db;
    db.put_p2pkh(txid, 0, 1, false, 1, HASH20_A);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);

    EXPECT_EQ(utxos[0].txid_hex.substr(0, 2), "ab");
    EXPECT_EQ(utxos[0].txid_hex.substr(2), std::string(62, '0'));
}

// ------ Multiple UTXOs ------

TEST(Multi, TwoUtxosSameTxid) {
    TempDb db;
    db.put_p2pkh(TXID_A, 0, 100, false, 50000, HASH20_A);
    db.put_p2pkh(TXID_A, 1, 100, false, 30000, HASH20_B);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 2u);
}

TEST(Multi, TwoUtxosDifferentTxids) {
    TempDb db;
    db.put_p2pkh(TXID_A, 0, 200, false, 100000, HASH20_A);
    db.put_p2pkh(TXID_B, 0, 300, false, 200000, HASH20_B);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 2u);
}

TEST(Multi, NonCoinKeyIgnored) {
    TempDb db;
    db.put_p2pkh(TXID_A, 0, 1, false, 1, HASH20_A);
    // Write a non-'C' key that should be skipped
    db.put("obfuscate_key", "somevalue");
    db.put("B" + std::string(32, 0x00), "junk");
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);
}

// ------ Obfuscation ------

TEST(Obfuscation, XorKeyApplied) {
    std::vector<uint8_t> obf_key = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0};
    TempDb db;
    db.set_obfuscation_key(obf_key);
    db.put_p2pkh(TXID_A, 0, 500, false, 150000, HASH20_A, obf_key);
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);
    EXPECT_EQ(utxos[0].height, 500u);
    EXPECT_EQ(utxos[0].satoshis, 150000);

    std::vector<uint8_t> payload = {0x00};
    payload.insert(payload.end(), HASH20_A.begin(), HASH20_A.end());
    EXPECT_EQ(utxos[0].address, btcwallet::base58check_encode(payload));
}

TEST(Obfuscation, NoKeyIsIdentity) {
    // A database without an obfuscation key entry should also work
    TempDb db;
    db.put_p2pkh(TXID_A, 0, 1, false, 42, HASH20_A);
    // Deliberately do NOT call set_obfuscation_key
    auto utxos = db.close_and_scan();
    EXPECT_EQ(utxos.size(), 1u);
    EXPECT_EQ(utxos[0].satoshis, 42);
}

// ------ Network encoding ------

TEST(Network, TestnetP2PKH) {
    TempDb db;
    db.put_p2pkh(TXID_A, 0, 1, false, 1, HASH20_A);
    auto utxos = db.close_and_scan(/*mainnet=*/false);
    EXPECT_EQ(utxos.size(), 1u);

    std::vector<uint8_t> payload = {0x6f};  // testnet P2PKH version
    payload.insert(payload.end(), HASH20_A.begin(), HASH20_A.end());
    EXPECT_EQ(utxos[0].address, btcwallet::base58check_encode(payload));
    EXPECT_TRUE(utxos[0].address[0] == 'm' || utxos[0].address[0] == 'n');
}

TEST(Network, TestnetP2WPKH) {
    std::vector<uint8_t> script = {0x00, 0x14};
    script.insert(script.end(), HASH20_A.begin(), HASH20_A.end());
    TempDb db;
    db.put_utxo(TXID_A, 0, 1, false, 1, 28, script);
    auto utxos = db.close_and_scan(/*mainnet=*/false);
    EXPECT_EQ(utxos.size(), 1u);

    std::string expected = btcwallet::bech32_encode("tb", 0, HASH20_A);
    EXPECT_EQ(utxos[0].address, expected);
    EXPECT_TRUE(utxos[0].address.substr(0, 4) == "tb1q");
}

TEST(Network, MainnetTestnetDiffer) {
    TempDb db_main, db_test;
    db_main.put_p2pkh(TXID_A, 0, 1, false, 1, HASH20_A);
    db_test.put_p2pkh(TXID_A, 0, 1, false, 1, HASH20_A);
    auto main_utxos = db_main.close_and_scan(true);
    auto test_utxos = db_test.close_and_scan(false);
    EXPECT_NE(main_utxos[0].address, test_utxos[0].address);
}

// ------ Error handling ------

TEST(Error, BadDirectoryThrows) {
    btcutxo::ScanOptions opts;
    EXPECT_THROWS(btcutxo::scan_chainstate("/no/such/path", opts, [](const btcutxo::Utxo&){}));
}

// ---- main ----

int main() {
    int passed = 0, failed = 0;
    for (auto& tc : tests()) {
        try {
            tc.fn();
            fprintf(stdout, "PASS  %s\n", tc.name.c_str());
            ++passed;
        } catch (const std::exception& e) {
            fprintf(stdout, "FAIL  %s\n      %s\n", tc.name.c_str(), e.what());
            ++failed;
        }
    }
    fprintf(stdout, "\n%d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
