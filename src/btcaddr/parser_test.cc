#include "parser.h"

#include <cassert>
#include <cstring>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

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
#define EXPECT_TRUE(x)      check((x),       #x " is false",      __FILE__, __LINE__)
#define EXPECT_FALSE(x)     check(!(x),      #x " is true",       __FILE__, __LINE__)
#define EXPECT_EQ(a, b)     check((a)==(b),  #a " != " #b,        __FILE__, __LINE__)
#define EXPECT_IN(s, v)     check((s).count(v)>0, "not in set: " + std::string(v), __FILE__, __LINE__)
#define EXPECT_NOT_IN(s, v) check((s).count(v)==0, "unexpectedly in set: " + std::string(v), __FILE__, __LINE__)

// ---- block / transaction builders ----

static void pu32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(x&0xff); v.push_back((x>>8)&0xff);
    v.push_back((x>>16)&0xff); v.push_back((x>>24)&0xff);
}
static void pu64(std::vector<uint8_t>& v, uint64_t x) {
    for (int i = 0; i < 8; i++) v.push_back((x>>(8*i))&0xff);
}
static void pvarint(std::vector<uint8_t>& v, uint64_t n) {
    if (n < 0xfd) { v.push_back(static_cast<uint8_t>(n)); return; }
    v.push_back(0xfd); v.push_back(n&0xff); v.push_back((n>>8)&0xff);
}
static void pappend(std::vector<uint8_t>& v, const std::vector<uint8_t>& b) {
    v.insert(v.end(), b.begin(), b.end());
}

struct TxOutput { uint64_t value; std::vector<uint8_t> script; };
struct WitnessInput { std::vector<std::vector<uint8_t>> stack; };

// Non-SegWit transaction with one coinbase-style input.
static std::vector<uint8_t> make_tx(const std::vector<TxOutput>& outs) {
    std::vector<uint8_t> tx;
    pu32(tx, 1);                            // version
    pvarint(tx, 1);                         // 1 input
    tx.insert(tx.end(), 32, 0x00);          //   prev txid
    pu32(tx, 0xffffffff);                   //   prev vout
    pvarint(tx, 0);                         //   scriptSig (empty)
    pu32(tx, 0xffffffff);                   //   sequence
    pvarint(tx, outs.size());
    for (auto& o : outs) {
        pu64(tx, o.value);
        pvarint(tx, o.script.size());
        pappend(tx, o.script);
    }
    pu32(tx, 0);                            // locktime
    return tx;
}

// SegWit transaction (marker 0x00, flag 0x01) with explicit witness per input.
static std::vector<uint8_t> make_segwit_tx(const std::vector<TxOutput>& outs,
                                              const std::vector<WitnessInput>& witnesses) {
    std::vector<uint8_t> tx;
    pu32(tx, 1);                            // version
    tx.push_back(0x00);                     // marker
    tx.push_back(0x01);                     // flag
    pvarint(tx, witnesses.size());          // input count
    for (size_t i = 0; i < witnesses.size(); i++) {
        tx.insert(tx.end(), 32, 0x00);      //   prev txid
        pu32(tx, 0xffffffff);               //   prev vout
        pvarint(tx, 0);                     //   scriptSig (empty)
        pu32(tx, 0xffffffff);               //   sequence
    }
    pvarint(tx, outs.size());
    for (auto& o : outs) {
        pu64(tx, o.value);
        pvarint(tx, o.script.size());
        pappend(tx, o.script);
    }
    for (auto& w : witnesses) {
        pvarint(tx, w.stack.size());
        for (auto& item : w.stack) {
            pvarint(tx, item.size());
            pappend(tx, item);
        }
    }
    pu32(tx, 0);                            // locktime
    return tx;
}

// Wrap transactions into a block (80-byte header + tx count + txs).
static std::vector<uint8_t> make_block(const std::vector<std::vector<uint8_t>>& txs) {
    std::vector<uint8_t> blk(80, 0x00);    // header
    pvarint(blk, txs.size());
    for (auto& tx : txs) pappend(blk, tx);
    return blk;
}

// Wrap blocks into a blk*.dat file with the given magic.
static std::vector<uint8_t> make_blkfile(
        const std::vector<std::vector<uint8_t>>& blocks,
        const uint8_t magic[4] = nullptr) {
    static const uint8_t MAINNET[4] = {0xf9, 0xbe, 0xb4, 0xd9};
    const uint8_t* m = magic ? magic : MAINNET;
    std::vector<uint8_t> file;
    for (auto& blk : blocks) {
        file.insert(file.end(), m, m+4);
        pu32(file, static_cast<uint32_t>(blk.size()));
        pappend(file, blk);
    }
    return file;
}

// Write bytes to a temp file, return the path.
static std::string write_tmp(const std::string& tag, const std::vector<uint8_t>& data) {
    std::string path = "/tmp/btcaddr_test_" + tag + ".dat";
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot create " + path);
    f.write(reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size()));
    return path;
}

static int tmp_counter = 0;

static btcaddr::ScanStats scan(const std::vector<uint8_t>& filedata,
                                 std::unordered_set<std::string>& addrs,
                                 const btcaddr::ScanOptions& opts = {}) {
    auto path = write_tmp(std::to_string(tmp_counter++), filedata);
    return btcaddr::scan_block_file(path, opts, addrs);
}

// ---- known scripts and their verified addresses (Python-verified) ----

// P2PKH: hash160=62e907b15cbf27d5425399ebf6f0fb50ebb88f18
static const std::vector<uint8_t> P2PKH_SCRIPT = {
    0x76,0xa9,0x14,
    0x62,0xe9,0x07,0xb1,0x5c,0xbf,0x27,0xd5,0x42,0x53,
    0x99,0xeb,0xf6,0xf0,0xfb,0x50,0xeb,0xb8,0x8f,0x18,
    0x88,0xac
};
static const std::string P2PKH_ADDR = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa";

// P2SH: hash160=3f4aa1fedf1f54eeb03b759deadb36676b184911
static const std::vector<uint8_t> P2SH_SCRIPT = {
    0xa9,0x14,
    0x3f,0x4a,0xa1,0xfe,0xdf,0x1f,0x54,0xee,0xb0,0x3b,
    0x75,0x9d,0xea,0xdb,0x36,0x67,0x6b,0x18,0x49,0x11,
    0x87
};
static const std::string P2SH_ADDR = "37TfuCwExPRH2urc5VJfS4FHeWQ6qkXDfU";

// P2WPKH: hash160=751e76e8199196f454f092f4ab4955bc6c8e6e1b
static const std::vector<uint8_t> P2WPKH_SCRIPT = {
    0x00,0x14,
    0x75,0x1e,0x76,0xe8,0x19,0x91,0x96,0xf4,0x54,0xf0,
    0x92,0xf4,0xab,0x49,0x55,0xbc,0x6c,0x8e,0x6e,0x1b
};
static const std::string P2WPKH_ADDR = "bc1qw508d6qejxt0g48sjt62kj24h3kgumsm03j2ff";

// P2WSH: all-zeros 32-byte hash
static std::vector<uint8_t> p2wsh_script() {
    std::vector<uint8_t> s = {0x00, 0x20};
    s.insert(s.end(), 32, 0x00);
    return s;
}
static const std::string P2WSH_ADDR = "bc1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqthqst8";

// P2TR: all-zeros 32-byte x-only key
static std::vector<uint8_t> p2tr_script() {
    std::vector<uint8_t> s = {0x51, 0x20};
    s.insert(s.end(), 32, 0x00);
    return s;
}
static const std::string P2TR_ADDR = "bc1pqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqpqqenm";

// P2PK compressed: secp256k1 generator G
// hash160(G_compressed) → 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
static const std::vector<uint8_t> P2PK_COMPRESSED_SCRIPT = {
    0x21,  // push 33 bytes
    0x02,0x79,0xbe,0x66,0x7e,0xf9,0xdc,0xbb,0xac,0x55,
    0xa0,0x62,0x95,0xce,0x87,0x0b,0x07,0x02,0x9b,0xfc,
    0xdb,0x2d,0xce,0x28,0xd9,0x59,0xf2,0x81,0x5b,0x16,
    0xf8,0x17,0x98,
    0xac   // OP_CHECKSIG
};
static const std::string P2PK_COMPRESSED_ADDR = "1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH";

// P2PK uncompressed: secp256k1 generator G (uncompressed, 65 bytes)
// hash160(G_uncompressed) → 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
static const std::vector<uint8_t> P2PK_UNCOMPRESSED_SCRIPT = {
    0x41,  // push 65 bytes
    0x04,0x79,0xbe,0x66,0x7e,0xf9,0xdc,0xbb,0xac,0x55,
    0xa0,0x62,0x95,0xce,0x87,0x0b,0x07,0x02,0x9b,0xfc,
    0xdb,0x2d,0xce,0x28,0xd9,0x59,0xf2,0x81,0x5b,0x16,
    0xf8,0x17,0x98,0x48,0x3a,0xda,0x77,0x26,0xa3,0xc4,
    0x65,0x5d,0xa4,0xfb,0xfc,0x0e,0x11,0x08,0xa8,0xfd,
    0x17,0xb4,0x48,0xa6,0x85,0x54,0x19,0x9c,0x47,0xd0,
    0x8f,0xfb,0x10,0xd4,0xb8,
    0xac   // OP_CHECKSIG
};
static const std::string P2PK_UNCOMPRESSED_ADDR = "1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm";

// ---- Script recognition tests ----

TEST(Script, P2PKH) {
    auto file = make_blkfile({make_block({make_tx({{0, P2PKH_SCRIPT}})})});
    std::unordered_set<std::string> addrs;
    scan(file, addrs);
    EXPECT_IN(addrs, P2PKH_ADDR);
    EXPECT_EQ(addrs.size(), size_t(1));
}

TEST(Script, P2SH) {
    auto file = make_blkfile({make_block({make_tx({{0, P2SH_SCRIPT}})})});
    std::unordered_set<std::string> addrs;
    scan(file, addrs);
    EXPECT_IN(addrs, P2SH_ADDR);
    EXPECT_EQ(addrs.size(), size_t(1));
}

TEST(Script, P2WPKH) {
    auto file = make_blkfile({make_block({make_tx({{0, P2WPKH_SCRIPT}})})});
    std::unordered_set<std::string> addrs;
    scan(file, addrs);
    EXPECT_IN(addrs, P2WPKH_ADDR);
    EXPECT_TRUE(P2WPKH_ADDR.substr(0, 4) == "bc1q");
    EXPECT_EQ(P2WPKH_ADDR.size(), size_t(42));  // bc1q + 38 chars
}

TEST(Script, P2WSH) {
    auto file = make_blkfile({make_block({make_tx({{0, p2wsh_script()}})})});
    std::unordered_set<std::string> addrs;
    scan(file, addrs);
    EXPECT_IN(addrs, P2WSH_ADDR);
    EXPECT_TRUE(P2WSH_ADDR.substr(0, 4) == "bc1q");
    EXPECT_EQ(P2WSH_ADDR.size(), size_t(62));   // bc1q + 58 chars
}

TEST(Script, P2TR) {
    auto file = make_blkfile({make_block({make_tx({{0, p2tr_script()}})})});
    std::unordered_set<std::string> addrs;
    scan(file, addrs);
    EXPECT_IN(addrs, P2TR_ADDR);
    EXPECT_TRUE(P2TR_ADDR.substr(0, 4) == "bc1p");
    EXPECT_EQ(P2TR_ADDR.size(), size_t(62));    // bc1p + 58 chars
}

TEST(Script, P2PKCompressed) {
    auto file = make_blkfile({make_block({make_tx({{0, P2PK_COMPRESSED_SCRIPT}})})});
    std::unordered_set<std::string> addrs;
    scan(file, addrs);
    EXPECT_IN(addrs, P2PK_COMPRESSED_ADDR);
    EXPECT_EQ(P2PK_COMPRESSED_ADDR[0], '1');
}

TEST(Script, P2PKUncompressed) {
    auto file = make_blkfile({make_block({make_tx({{0, P2PK_UNCOMPRESSED_SCRIPT}})})});
    std::unordered_set<std::string> addrs;
    scan(file, addrs);
    EXPECT_IN(addrs, P2PK_UNCOMPRESSED_ADDR);
}

TEST(Script, P2PKSkippedWhenDisabled) {
    btcaddr::ScanOptions opts;
    opts.include_p2pk = false;
    auto file = make_blkfile({make_block({
        make_tx({{0, P2PK_COMPRESSED_SCRIPT}, {0, P2PK_UNCOMPRESSED_SCRIPT}})
    })});
    std::unordered_set<std::string> addrs;
    scan(file, addrs, opts);
    EXPECT_NOT_IN(addrs, P2PK_COMPRESSED_ADDR);
    EXPECT_NOT_IN(addrs, P2PK_UNCOMPRESSED_ADDR);
    EXPECT_EQ(addrs.size(), size_t(0));
}

TEST(Script, NonStandardSkipped) {
    // OP_RETURN data — unspendable, should not be decoded as any address
    std::vector<uint8_t> op_return = {0x6a, 0x04, 0xde, 0xad, 0xbe, 0xef};
    // Garbage script that partially resembles P2PKH but wrong length
    std::vector<uint8_t> garbage = {0x76, 0xa9, 0x13, 0x00, 0x01, 0x88, 0xac};
    auto file = make_blkfile({make_block({make_tx({{0, op_return}, {0, garbage}})})});
    std::unordered_set<std::string> addrs;
    scan(file, addrs);
    EXPECT_EQ(addrs.size(), size_t(0));
}

// ---- Stats tests ----

TEST(Stats, BlockCount) {
    auto block = make_block({make_tx({{0, P2PKH_SCRIPT}})});
    auto file = make_blkfile({block, block, block});
    std::unordered_set<std::string> addrs;
    auto s = scan(file, addrs);
    EXPECT_EQ(s.blocks, uint64_t(3));
}

TEST(Stats, TransactionCount) {
    auto tx = make_tx({{0, P2PKH_SCRIPT}});
    auto file = make_blkfile({make_block({tx, tx, tx}), make_block({tx, tx})});
    std::unordered_set<std::string> addrs;
    auto s = scan(file, addrs);
    EXPECT_EQ(s.transactions, uint64_t(5));
}

TEST(Stats, OutputCount) {
    // tx1: 3 outputs, tx2: 2 outputs
    auto tx1 = make_tx({{0, P2PKH_SCRIPT}, {0, P2SH_SCRIPT}, {0, P2WPKH_SCRIPT}});
    auto tx2 = make_tx({{0, p2wsh_script()}, {0, p2tr_script()}});
    auto file = make_blkfile({make_block({tx1, tx2})});
    std::unordered_set<std::string> addrs;
    auto s = scan(file, addrs);
    EXPECT_EQ(s.outputs, uint64_t(5));
}

TEST(Stats, ZeroTransactionBlock) {
    auto file = make_blkfile({make_block({})});
    std::unordered_set<std::string> addrs;
    auto s = scan(file, addrs);
    EXPECT_EQ(s.blocks, uint64_t(1));
    EXPECT_EQ(s.transactions, uint64_t(0));
    EXPECT_EQ(s.outputs, uint64_t(0));
    EXPECT_EQ(addrs.size(), size_t(0));
}

// ---- Deduplication tests ----

TEST(Dedup, SameAddressTwoTx) {
    auto file = make_blkfile({make_block({
        make_tx({{0, P2PKH_SCRIPT}}),
        make_tx({{0, P2PKH_SCRIPT}})  // same address again
    })});
    std::unordered_set<std::string> addrs;
    scan(file, addrs);
    EXPECT_EQ(addrs.size(), size_t(1));
    EXPECT_IN(addrs, P2PKH_ADDR);
}

TEST(Dedup, AllFiveTypesDistinct) {
    auto file = make_blkfile({make_block({make_tx({
        {0, P2PKH_SCRIPT},
        {0, P2SH_SCRIPT},
        {0, P2WPKH_SCRIPT},
        {0, p2wsh_script()},
        {0, p2tr_script()},
    })})});
    std::unordered_set<std::string> addrs;
    scan(file, addrs);
    EXPECT_EQ(addrs.size(), size_t(5));
    EXPECT_IN(addrs, P2PKH_ADDR);
    EXPECT_IN(addrs, P2SH_ADDR);
    EXPECT_IN(addrs, P2WPKH_ADDR);
    EXPECT_IN(addrs, P2WSH_ADDR);
    EXPECT_IN(addrs, P2TR_ADDR);
}

TEST(Dedup, AccumulatesAcrossFiles) {
    // Second scan call adds to the same set
    auto file1 = make_blkfile({make_block({make_tx({{0, P2PKH_SCRIPT}})})});
    auto file2 = make_blkfile({make_block({make_tx({{0, P2SH_SCRIPT}})})});
    std::unordered_set<std::string> addrs;
    scan(file1, addrs);
    scan(file2, addrs);
    EXPECT_EQ(addrs.size(), size_t(2));
}

// ---- Parsing tests ----

TEST(Parse, MultipleBlocksInFile) {
    auto tx1 = make_tx({{0, P2PKH_SCRIPT}});
    auto tx2 = make_tx({{0, P2SH_SCRIPT}});
    auto tx3 = make_tx({{0, P2WPKH_SCRIPT}});
    auto file = make_blkfile({
        make_block({tx1}),
        make_block({tx2}),
        make_block({tx3}),
    });
    std::unordered_set<std::string> addrs;
    auto s = scan(file, addrs);
    EXPECT_EQ(s.blocks, uint64_t(3));
    EXPECT_EQ(addrs.size(), size_t(3));
}

TEST(Parse, SegWitEmptyWitness) {
    // SegWit transaction with 0-item witness stack — parser must skip the witness correctly.
    auto tx = make_segwit_tx({{0, P2WPKH_SCRIPT}}, {{{}}});  // 1 input, empty witness
    auto file = make_blkfile({make_block({tx})});
    std::unordered_set<std::string> addrs;
    scan(file, addrs);
    EXPECT_IN(addrs, P2WPKH_ADDR);
}

TEST(Parse, SegWitWithWitnessData) {
    // SegWit TX with real witness stack items — verify outputs extracted after skipping witness.
    std::vector<uint8_t> sig(72, 0xab);   // fake 72-byte signature
    std::vector<uint8_t> pubkey(33, 0x02);  // fake 33-byte pubkey
    auto tx = make_segwit_tx({{0, P2WPKH_SCRIPT}}, {{{{sig, pubkey}}}});
    auto file = make_blkfile({make_block({tx})});
    std::unordered_set<std::string> addrs;
    scan(file, addrs);
    EXPECT_IN(addrs, P2WPKH_ADDR);
}

TEST(Parse, SegWitMultipleInputs) {
    // Two inputs each with a 2-item witness stack.
    std::vector<WitnessInput> witnesses = {
        {{{std::vector<uint8_t>(64, 0xaa)}}},
        {{{std::vector<uint8_t>(33, 0x02), std::vector<uint8_t>(71, 0xbb)}}}
    };
    // Build manually: 2-input segwit tx
    std::vector<uint8_t> tx;
    pu32(tx, 1);
    tx.push_back(0x00); tx.push_back(0x01);  // marker + flag
    pvarint(tx, 2);                           // 2 inputs
    for (int i = 0; i < 2; i++) {
        tx.insert(tx.end(), 32, 0x00); pu32(tx, 0xffffffff);
        pvarint(tx, 0); pu32(tx, 0xffffffff);
    }
    auto out_script = p2tr_script();
    pvarint(tx, 1);
    pu64(tx, 0);
    pvarint(tx, out_script.size());
    pappend(tx, out_script);
    for (auto& w : witnesses) {
        pvarint(tx, w.stack.size());
        for (auto& item : w.stack) { pvarint(tx, item.size()); pappend(tx, item); }
    }
    pu32(tx, 0);

    auto file = make_blkfile({make_block({tx})});
    std::unordered_set<std::string> addrs;
    scan(file, addrs);
    EXPECT_IN(addrs, P2TR_ADDR);
}

TEST(Parse, TruncatedFileHandledGracefully) {
    auto blk = make_block({make_tx({{0, P2PKH_SCRIPT}})});
    auto file = make_blkfile({blk});
    // Append a partial magic + size header with no block data → parser stops cleanly
    file.insert(file.end(), {0xf9, 0xbe, 0xb4, 0xd9});
    uint32_t big = 0x00100000;  // 1 MB block that doesn't exist
    file.push_back(big & 0xff); file.push_back((big>>8)&0xff);
    file.push_back((big>>16)&0xff); file.push_back((big>>24)&0xff);
    std::unordered_set<std::string> addrs;
    auto s = scan(file, addrs);
    // The first complete block must have been parsed
    EXPECT_EQ(s.blocks, uint64_t(1));
    EXPECT_IN(addrs, P2PKH_ADDR);
}

TEST(Parse, WrongMagicParsesNothing) {
    auto file = make_blkfile({make_block({make_tx({{0, P2PKH_SCRIPT}})})});
    btcaddr::ScanOptions opts;
    opts.magic[0] = 0x0b; opts.magic[1] = 0x11;  // testnet3 magic
    opts.magic[2] = 0x09; opts.magic[3] = 0x07;
    std::unordered_set<std::string> addrs;
    auto s = scan(file, addrs, opts);
    EXPECT_EQ(s.blocks, uint64_t(0));
    EXPECT_EQ(addrs.size(), size_t(0));
}

TEST(Parse, PaddingBetweenBlocksSkipped) {
    // Insert zero bytes between two valid blocks (can happen in blk*.dat files)
    auto blk1 = make_block({make_tx({{0, P2PKH_SCRIPT}})});
    auto blk2 = make_block({make_tx({{0, P2SH_SCRIPT}})});
    std::vector<uint8_t> file;
    // First block
    file.insert(file.end(), {0xf9, 0xbe, 0xb4, 0xd9});
    pu32(file, static_cast<uint32_t>(blk1.size()));
    pappend(file, blk1);
    // Padding
    file.insert(file.end(), 128, 0x00);
    // Second block
    file.insert(file.end(), {0xf9, 0xbe, 0xb4, 0xd9});
    pu32(file, static_cast<uint32_t>(blk2.size()));
    pappend(file, blk2);

    std::unordered_set<std::string> addrs;
    auto s = scan(file, addrs);
    EXPECT_EQ(s.blocks, uint64_t(2));
    EXPECT_IN(addrs, P2PKH_ADDR);
    EXPECT_IN(addrs, P2SH_ADDR);
}

// ---- Testnet tests ----

TEST(Network, TestnetP2PKH) {
    static const uint8_t TESTNET_MAGIC[4] = {0x0b, 0x11, 0x09, 0x07};
    btcaddr::ScanOptions opts;
    opts.mainnet = false;
    memcpy(opts.magic, TESTNET_MAGIC, 4);

    auto file = make_blkfile({make_block({make_tx({{0, P2PKH_SCRIPT}})})},
                              TESTNET_MAGIC);
    std::unordered_set<std::string> addrs;
    scan(file, addrs, opts);
    // Testnet P2PKH version=0x6f → base58 prefix m or n
    EXPECT_EQ(addrs.size(), size_t(1));
    char first = addrs.begin()->front();
    EXPECT_TRUE(first == 'm' || first == 'n');
    // Must NOT contain the mainnet form
    EXPECT_NOT_IN(addrs, P2PKH_ADDR);
}

TEST(Network, TestnetP2WPKH) {
    static const uint8_t TESTNET_MAGIC[4] = {0x0b, 0x11, 0x09, 0x07};
    btcaddr::ScanOptions opts;
    opts.mainnet = false;
    memcpy(opts.magic, TESTNET_MAGIC, 4);

    auto file = make_blkfile({make_block({make_tx({{0, P2WPKH_SCRIPT}})})},
                              TESTNET_MAGIC);
    std::unordered_set<std::string> addrs;
    scan(file, addrs, opts);
    EXPECT_EQ(addrs.size(), size_t(1));
    // Testnet bech32: tb1q...
    EXPECT_TRUE(addrs.begin()->substr(0, 4) == "tb1q");
    EXPECT_NOT_IN(addrs, P2WPKH_ADDR);  // mainnet form absent
}

// ---- Flush / memory-limit tests ----

TEST(Flush, CallbackFiredAtThreshold) {
    // threshold=1: on_flush must be called after the very first block.
    int flush_calls = 0;
    btcaddr::ScanOptions opts;
    opts.flush_threshold = 1;
    opts.on_flush = [&](std::unordered_set<std::string>& set) {
        ++flush_calls;
        set.clear();
    };

    // 3 blocks, each with one distinct address
    auto tx1 = make_tx({{0, P2PKH_SCRIPT}});
    auto tx2 = make_tx({{0, P2SH_SCRIPT}});
    auto tx3 = make_tx({{0, P2WPKH_SCRIPT}});
    auto file = make_blkfile({make_block({tx1}), make_block({tx2}), make_block({tx3})});

    std::unordered_set<std::string> addrs;
    auto s = scan(file, addrs, opts);

    EXPECT_EQ(s.blocks, uint64_t(3));
    EXPECT_TRUE(flush_calls >= 1);
}

TEST(Flush, AllAddressesDeliveredToCallback) {
    // Collect every address the callback receives; verify all five types are seen.
    std::unordered_set<std::string> flushed;
    btcaddr::ScanOptions opts;
    opts.flush_threshold = 1;   // flush after every block
    opts.on_flush = [&](std::unordered_set<std::string>& set) {
        flushed.insert(set.begin(), set.end());
        set.clear();
    };

    auto file = make_blkfile({make_block({make_tx({
        {0, P2PKH_SCRIPT},
        {0, P2SH_SCRIPT},
        {0, P2WPKH_SCRIPT},
        {0, p2wsh_script()},
        {0, p2tr_script()},
    })})});

    std::unordered_set<std::string> addrs;  // will be empty after flush
    scan(file, addrs, opts);

    EXPECT_IN(flushed, P2PKH_ADDR);
    EXPECT_IN(flushed, P2SH_ADDR);
    EXPECT_IN(flushed, P2WPKH_ADDR);
    EXPECT_IN(flushed, P2WSH_ADDR);
    EXPECT_IN(flushed, P2TR_ADDR);
}

TEST(Flush, ThresholdDisabledByDefault) {
    // Default ScanOptions must not call on_flush.
    bool called = false;
    btcaddr::ScanOptions opts;
    opts.on_flush = [&](std::unordered_set<std::string>&) { called = true; };
    // flush_threshold defaults to 0, so on_flush must never fire.

    auto file = make_blkfile({make_block({make_tx({{0, P2PKH_SCRIPT}})})});
    std::unordered_set<std::string> addrs;
    scan(file, addrs, opts);

    EXPECT_FALSE(called);
    EXPECT_IN(addrs, P2PKH_ADDR);
}

TEST(Flush, HighThresholdNotTriggered) {
    // threshold larger than number of addresses → no flush.
    bool called = false;
    btcaddr::ScanOptions opts;
    opts.flush_threshold = 1'000'000;
    opts.on_flush = [&](std::unordered_set<std::string>&) { called = true; };

    auto file = make_blkfile({make_block({make_tx({{0, P2PKH_SCRIPT}})})});
    std::unordered_set<std::string> addrs;
    scan(file, addrs, opts);

    EXPECT_FALSE(called);
    EXPECT_EQ(addrs.size(), size_t(1));
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
