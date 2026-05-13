#include "parser.h"
#include "src/btcwallet/crypto.h"

#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace btcaddr {

// ---- Safe buffer cursor ----

struct Cursor {
    const uint8_t* buf;
    size_t pos  = 0;
    size_t size = 0;

    bool has(size_t n) const noexcept { return pos + n <= size; }
    const uint8_t* peek() const noexcept { return buf + pos; }

    void need(size_t n) const {
        if (pos + n > size) throw std::runtime_error("unexpected end of block");
    }

    uint8_t u8() { need(1); return buf[pos++]; }

    uint32_t u32le() {
        need(4);
        uint32_t v = (uint32_t)buf[pos]
                   | ((uint32_t)buf[pos+1] <<  8)
                   | ((uint32_t)buf[pos+2] << 16)
                   | ((uint32_t)buf[pos+3] << 24);
        pos += 4;
        return v;
    }

    uint64_t u64le() {
        need(8);
        uint64_t v = 0;
        for (int i = 0; i < 8; i++) v |= (uint64_t)buf[pos+i] << (8*i);
        pos += 8;
        return v;
    }

    uint64_t varint() {
        uint8_t b = u8();
        if (b < 0xfd) return b;
        if (b == 0xfd) {
            need(2);
            uint64_t v = (uint64_t)buf[pos] | ((uint64_t)buf[pos+1] << 8);
            pos += 2; return v;
        }
        if (b == 0xfe) {
            need(4);
            uint64_t v = (uint64_t)buf[pos] | ((uint64_t)buf[pos+1] << 8)
                       | ((uint64_t)buf[pos+2] << 16) | ((uint64_t)buf[pos+3] << 24);
            pos += 4; return v;
        }
        return u64le();
    }

    void skip(uint64_t n) { need(static_cast<size_t>(n)); pos += n; }
};

// ---- ScriptPubKey → address ----

static std::string script_to_addr(const uint8_t* s, size_t len,
                                    bool mainnet, bool include_p2pk) {
    const std::string hrp = mainnet ? "bc" : "tb";

    // P2PKH: OP_DUP OP_HASH160 <20 bytes> OP_EQUALVERIFY OP_CHECKSIG
    if (len == 25 && s[0]==0x76 && s[1]==0xa9 && s[2]==0x14
                  && s[23]==0x88 && s[24]==0xac) {
        std::vector<uint8_t> p = {mainnet ? uint8_t(0x00) : uint8_t(0x6f)};
        p.insert(p.end(), s+3, s+23);
        return btcwallet::base58check_encode(p);
    }

    // P2SH: OP_HASH160 <20 bytes> OP_EQUAL
    if (len == 23 && s[0]==0xa9 && s[1]==0x14 && s[22]==0x87) {
        std::vector<uint8_t> p = {mainnet ? uint8_t(0x05) : uint8_t(0xc4)};
        p.insert(p.end(), s+2, s+22);
        return btcwallet::base58check_encode(p);
    }

    // P2WPKH: OP_0 <20 bytes>
    if (len == 22 && s[0]==0x00 && s[1]==0x14) {
        std::vector<uint8_t> hash(s+2, s+22);
        return btcwallet::bech32_encode(hrp, 0, hash);
    }

    // P2WSH: OP_0 <32 bytes>
    if (len == 34 && s[0]==0x00 && s[1]==0x20) {
        std::vector<uint8_t> hash(s+2, s+34);
        return btcwallet::bech32_encode(hrp, 0, hash);
    }

    // P2TR: OP_1 <32 bytes>  (witness version 1 = 0x51)
    if (len == 34 && s[0]==0x51 && s[1]==0x20) {
        std::vector<uint8_t> key(s+2, s+34);
        return btcwallet::bech32_encode(hrp, 1, key);
    }

    // P2PK compressed: <0x21> <33-byte pubkey> OP_CHECKSIG
    if (include_p2pk && len == 35 && s[0]==0x21 && s[34]==0xac) {
        auto h = btcwallet::hash160(std::span<const uint8_t>(s+1, 33));
        std::vector<uint8_t> p = {mainnet ? uint8_t(0x00) : uint8_t(0x6f)};
        p.insert(p.end(), h.begin(), h.end());
        return btcwallet::base58check_encode(p);
    }

    // P2PK uncompressed: <0x41> <65-byte pubkey> OP_CHECKSIG
    if (include_p2pk && len == 67 && s[0]==0x41 && s[66]==0xac) {
        auto h = btcwallet::hash160(std::span<const uint8_t>(s+1, 65));
        std::vector<uint8_t> p = {mainnet ? uint8_t(0x00) : uint8_t(0x6f)};
        p.insert(p.end(), h.begin(), h.end());
        return btcwallet::base58check_encode(p);
    }

    return "";
}

// ---- Parse one transaction, accumulate output addresses ----

static uint64_t parse_tx(Cursor& c, const ScanOptions& opts,
                           std::unordered_set<std::string>& addrs) {
    c.skip(4);  // version

    // SegWit: marker=0x00 followed by non-zero flag
    bool segwit = c.has(2) && c.peek()[0] == 0x00 && c.peek()[1] != 0x00;
    if (segwit) c.skip(2);

    uint64_t vin_n = c.varint();
    for (uint64_t i = 0; i < vin_n; i++) {
        c.skip(36);           // txid(32) + vout_index(4)
        c.skip(c.varint());   // scriptSig
        c.skip(4);            // sequence
    }

    uint64_t vout_n = c.varint();
    for (uint64_t i = 0; i < vout_n; i++) {
        c.skip(8);            // value (satoshis)
        uint64_t slen = c.varint();
        auto addr = script_to_addr(c.peek(), static_cast<size_t>(slen),
                                   opts.mainnet, opts.include_p2pk);
        c.skip(slen);
        if (!addr.empty()) addrs.insert(std::move(addr));
    }

    if (segwit) {
        for (uint64_t i = 0; i < vin_n; i++) {
            uint64_t items = c.varint();
            for (uint64_t j = 0; j < items; j++) c.skip(c.varint());
        }
    }

    c.skip(4);  // locktime
    return vout_n;
}

// ---- Public API ----

ScanStats scan_block_file(const std::string& path,
                           const ScanOptions& opts,
                           std::unordered_set<std::string>& addresses) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) throw std::runtime_error("cannot open: " + path);

    auto fsize = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> data(fsize);
    if (!f.read(reinterpret_cast<char*>(data.data()),
                static_cast<std::streamsize>(fsize)))
        throw std::runtime_error("read error: " + path);

    ScanStats stats;
    size_t pos = 0;

    while (pos + 8 <= fsize) {
        if (memcmp(data.data() + pos, opts.magic, 4) != 0) {
            ++pos;  // skip padding / unknown bytes
            continue;
        }
        pos += 4;

        uint32_t bsize = (uint32_t)data[pos]
                       | ((uint32_t)data[pos+1] <<  8)
                       | ((uint32_t)data[pos+2] << 16)
                       | ((uint32_t)data[pos+3] << 24);
        pos += 4;

        if (pos + bsize > fsize) break;  // truncated final block

        try {
            Cursor c{data.data() + pos, 0, bsize};
            c.skip(80);  // block header

            uint64_t ntx = c.varint();
            for (uint64_t t = 0; t < ntx; t++) {
                stats.outputs += parse_tx(c, opts, addresses);
                ++stats.transactions;
            }
            ++stats.blocks;
            if (opts.flush_threshold > 0 &&
                addresses.size() >= opts.flush_threshold &&
                opts.on_flush)
                opts.on_flush(addresses);
        } catch (const std::exception& e) {
            fprintf(stderr, "  warning: parse error in %s at +%zu: %s\n",
                    path.c_str(), pos, e.what());
        }

        pos += bsize;
    }

    return stats;
}

} // namespace btcaddr
