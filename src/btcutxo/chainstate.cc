#include "chainstate.h"
#include "src/btcwallet/crypto.h"

#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "leveldb/db.h"
#include "leveldb/options.h"

namespace btcutxo {

// ---- CVarInt (Bitcoin Core's custom variable-length integer) ----
// Different from compact size / VarInt used in transactions.
// Encoding: each byte contributes 7 bits, MSB means "more bytes follow".
// Decode: n = 0; while (true) { b = next_byte; n = (n<<7)|(b&0x7f); if !(b&0x80) break; n++; }

struct SliceCursor {
    const uint8_t* p;
    size_t         remaining;

    uint8_t read_byte() {
        if (remaining == 0) throw std::runtime_error("unexpected end of UTXO data");
        --remaining;
        return *p++;
    }

    uint64_t read_cvarint() {
        uint64_t n = 0;
        while (true) {
            uint8_t b = read_byte();
            n = (n << 7) | (b & 0x7f);
            if (!(b & 0x80)) break;
            ++n;  // Bitcoin Core quirk
        }
        return n;
    }

    // Read exactly `len` bytes into a vector.
    std::vector<uint8_t> read_bytes(size_t len) {
        if (remaining < len) throw std::runtime_error("unexpected end of UTXO data");
        std::vector<uint8_t> out(p, p + len);
        p += len;
        remaining -= len;
        return out;
    }
};

// ---- Bitcoin Core compressed-amount decompression ----
// Reference: serialize.h DecompressAmount()
static int64_t decompress_amount(uint64_t x) {
    if (x == 0) return 0;
    x--;
    uint64_t e = x % 10;
    x /= 10;
    uint64_t n;
    if (e < 9) {
        uint64_t d = (x % 9) + 1;
        x /= 9;
        n = x * 10 + d;
    } else {
        n = x + 1;
    }
    while (e > 0) { n *= 10; --e; }
    return static_cast<int64_t>(n);
}

// ---- Compressed script → address ----
// Script compression types (Bitcoin Core compressor.cpp):
//   0  → P2PKH  20-byte hash follows
//   1  → P2SH   20-byte hash follows
//   2  → P2PK compressed pubkey, even y  (32-byte x-coord follows)
//   3  → P2PK compressed pubkey, odd  y  (32-byte x-coord follows)
//   4  → P2PK uncompressed pubkey, even y (32-byte x-coord follows)
//   5  → P2PK uncompressed pubkey, odd  y (32-byte x-coord follows)
//   ≥6 → raw scriptPubKey, length = nsize - 6

static std::string decompress_script_to_addr(SliceCursor& c,
                                               uint64_t nsize,
                                               bool mainnet) {
    const std::string hrp = mainnet ? "bc" : "tb";

    if (nsize == 0) {
        // P2PKH
        auto hash = c.read_bytes(20);
        std::vector<uint8_t> payload = {mainnet ? uint8_t(0x00) : uint8_t(0x6f)};
        payload.insert(payload.end(), hash.begin(), hash.end());
        return btcwallet::base58check_encode(payload);
    }
    if (nsize == 1) {
        // P2SH
        auto hash = c.read_bytes(20);
        std::vector<uint8_t> payload = {mainnet ? uint8_t(0x05) : uint8_t(0xc4)};
        payload.insert(payload.end(), hash.begin(), hash.end());
        return btcwallet::base58check_encode(payload);
    }
    if (nsize == 2 || nsize == 3) {
        // P2PK compressed: reconstruct 33-byte pubkey from stored x-coord + type prefix
        auto x = c.read_bytes(32);
        uint8_t prefix = (nsize == 2) ? 0x02 : 0x03;
        std::vector<uint8_t> pubkey = {prefix};
        pubkey.insert(pubkey.end(), x.begin(), x.end());
        auto h = btcwallet::hash160(std::span<const uint8_t>(pubkey));
        std::vector<uint8_t> payload = {mainnet ? uint8_t(0x00) : uint8_t(0x6f)};
        payload.insert(payload.end(), h.begin(), h.end());
        return btcwallet::base58check_encode(payload);
    }
    if (nsize == 4 || nsize == 5) {
        // P2PK uncompressed: stored as 32-byte x-coord; we hash as P2PKH of the
        // P2PK script form.  Bitcoin Core stores these as P2PKH of the uncompressed
        // key, but without secp256k1 we can't reconstruct y.  Use the same
        // convention as btcaddr: hash160 of the reconstructed 33-byte even/odd key
        // as a P2PKH. Note: this may differ from the actual uncompressed-key address,
        // but it's consistent with how Core tracks them in coin db.
        auto x = c.read_bytes(32);
        uint8_t prefix = (nsize == 4) ? 0x02 : 0x03;
        std::vector<uint8_t> pubkey = {prefix};
        pubkey.insert(pubkey.end(), x.begin(), x.end());
        auto h = btcwallet::hash160(std::span<const uint8_t>(pubkey));
        std::vector<uint8_t> payload = {mainnet ? uint8_t(0x00) : uint8_t(0x6f)};
        payload.insert(payload.end(), h.begin(), h.end());
        return btcwallet::base58check_encode(payload);
    }

    // Raw script
    size_t script_len = static_cast<size_t>(nsize - 6);
    auto script = c.read_bytes(script_len);
    const uint8_t* s = script.data();

    // P2WPKH: OP_0 <20>
    if (script_len == 22 && s[0] == 0x00 && s[1] == 0x14) {
        std::vector<uint8_t> hash(s + 2, s + 22);
        return btcwallet::bech32_encode(hrp, 0, hash);
    }
    // P2WSH: OP_0 <32>
    if (script_len == 34 && s[0] == 0x00 && s[1] == 0x20) {
        std::vector<uint8_t> hash(s + 2, s + 34);
        return btcwallet::bech32_encode(hrp, 0, hash);
    }
    // P2TR: OP_1 <32>
    if (script_len == 34 && s[0] == 0x51 && s[1] == 0x20) {
        std::vector<uint8_t> key(s + 2, s + 34);
        return btcwallet::bech32_encode(hrp, 1, key);
    }
    return "";
}

// ---- XOR deobfuscation ----
static void xor_bytes(std::string& data, const std::string& key) {
    if (key.empty()) return;
    for (size_t i = 0; i < data.size(); ++i)
        data[i] ^= key[i % key.size()];
}

// ---- Hex helpers ----
static const char HEX_CHARS[] = "0123456789abcdef";

static std::string to_hex(const uint8_t* data, size_t len) {
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        out += HEX_CHARS[(data[i] >> 4) & 0xf];
        out += HEX_CHARS[data[i] & 0xf];
    }
    return out;
}

// ---- Public API ----

void scan_chainstate(const std::string& chainstate_dir,
                     const ScanOptions& opts,
                     const std::function<void(const Utxo&)>& on_utxo) {
    leveldb::Options ldb_opts;
    ldb_opts.create_if_missing = false;

    leveldb::DB* raw_db = nullptr;
    leveldb::Status s = leveldb::DB::Open(ldb_opts, chainstate_dir, &raw_db);
    if (!s.ok())
        throw std::runtime_error("cannot open chainstate: " + s.ToString());

    std::unique_ptr<leveldb::DB> db(raw_db);

    // Read obfuscation key (stored under "\x0e" + "obfuscate_key").
    // Value format: "\x00" + key_bytes
    std::string obf_key_db_key = std::string("\x0e", 1) + "obfuscate_key";
    std::string obf_val;
    std::string obf_key;
    if (db->Get(leveldb::ReadOptions(), obf_key_db_key, &obf_val).ok()
        && obf_val.size() > 1) {
        // First byte is a length prefix (not the key itself)
        obf_key = obf_val.substr(1);
    }

    // Iterate all keys starting with 'C' (coin entries)
    static const char COIN_PREFIX = 'C';

    std::unique_ptr<leveldb::Iterator> it(db->NewIterator(leveldb::ReadOptions()));
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        auto k = it->key();
        if (k.size() < 1 || k.data()[0] != COIN_PREFIX) continue;

        // Key layout: 'C'(1) + txid(32) + CVarInt(vout_index)
        if (k.size() < 33) continue;  // malformed

        // txid is stored in internal (reversed) byte order → reverse for display
        const uint8_t* txid_bytes = reinterpret_cast<const uint8_t*>(k.data() + 1);
        uint8_t txid_display[32];
        for (int i = 0; i < 32; ++i) txid_display[i] = txid_bytes[31 - i];
        std::string txid_hex = to_hex(txid_display, 32);

        // Decode vout CVarInt from remaining key bytes
        const uint8_t* vout_p   = reinterpret_cast<const uint8_t*>(k.data() + 33);
        size_t         vout_rem = k.size() - 33;
        SliceCursor vout_c{vout_p, vout_rem};
        uint64_t vout_index = 0;
        try { vout_index = vout_c.read_cvarint(); }
        catch (...) { continue; }

        // Value: XOR with obfuscation key then decode
        std::string val_str = it->value().ToString();
        xor_bytes(val_str, obf_key);

        SliceCursor vc{reinterpret_cast<const uint8_t*>(val_str.data()), val_str.size()};
        try {
            uint64_t code       = vc.read_cvarint();  // height*2 | coinbase_flag
            uint32_t height     = static_cast<uint32_t>(code >> 1);
            bool     coinbase   = (code & 1) != 0;
            uint64_t compressed = vc.read_cvarint();
            int64_t  satoshis   = decompress_amount(compressed);
            uint64_t nsize      = vc.read_cvarint();  // script type / length
            std::string addr    = decompress_script_to_addr(vc, nsize, opts.mainnet);

            Utxo u;
            u.txid_hex = std::move(txid_hex);
            u.vout     = static_cast<uint32_t>(vout_index);
            u.satoshis = satoshis;
            u.height   = height;
            u.coinbase = coinbase;
            u.address  = std::move(addr);
            on_utxo(u);
        } catch (...) {
            // Skip malformed entries
        }
    }

    if (!it->status().ok())
        throw std::runtime_error("leveldb iterator error: " + it->status().ToString());
}

} // namespace btcutxo
