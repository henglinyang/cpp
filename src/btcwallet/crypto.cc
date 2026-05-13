#include "crypto.h"
#include <cstring>
#include <stdexcept>

namespace btcwallet {

// ============================================================
// SHA-256  (FIPS 180-4)
// ============================================================

static const uint32_t K256[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2,
};

static inline uint32_t rotr32(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

static void sha256_block(uint32_t s[8], const uint8_t block[64]) {
    uint32_t w[64];
    for (int i = 0; i < 16; ++i)
        w[i] = ((uint32_t)block[i*4]<<24)|((uint32_t)block[i*4+1]<<16)|
               ((uint32_t)block[i*4+2]<<8)|(uint32_t)block[i*4+3];
    for (int i = 16; i < 64; ++i) {
        uint32_t s0 = rotr32(w[i-15],7)^rotr32(w[i-15],18)^(w[i-15]>>3);
        uint32_t s1 = rotr32(w[i-2],17)^rotr32(w[i-2],19)^(w[i-2]>>10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    uint32_t a=s[0],b=s[1],c=s[2],d=s[3],e=s[4],f=s[5],g=s[6],h=s[7];
    for (int i = 0; i < 64; ++i) {
        uint32_t S1 = rotr32(e,6)^rotr32(e,11)^rotr32(e,25);
        uint32_t ch = (e&f)^(~e&g);
        uint32_t t1 = h + S1 + ch + K256[i] + w[i];
        uint32_t S0 = rotr32(a,2)^rotr32(a,13)^rotr32(a,22);
        uint32_t maj = (a&b)^(a&c)^(b&c);
        uint32_t t2 = S0 + maj;
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    s[0]+=a; s[1]+=b; s[2]+=c; s[3]+=d; s[4]+=e; s[5]+=f; s[6]+=g; s[7]+=h;
}

static std::array<uint8_t, 32> sha256_raw(const uint8_t* data, size_t len) {
    uint32_t s[8] = {
        0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
        0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19,
    };
    uint8_t buf[64];
    size_t pos = 0;
    uint64_t bitlen = (uint64_t)len * 8;

    while (pos + 64 <= len) { sha256_block(s, data + pos); pos += 64; }
    size_t rem = len - pos;
    memcpy(buf, data + pos, rem);
    buf[rem++] = 0x80;
    if (rem > 56) {
        while (rem < 64) buf[rem++] = 0;
        sha256_block(s, buf);
        rem = 0;
    }
    while (rem < 56) buf[rem++] = 0;
    for (int i = 7; i >= 0; --i) { buf[rem++] = (uint8_t)(bitlen >> (i*8)); }
    sha256_block(s, buf);

    std::array<uint8_t, 32> out;
    for (int i = 0; i < 8; ++i) {
        out[i*4+0]=(s[i]>>24)&0xff; out[i*4+1]=(s[i]>>16)&0xff;
        out[i*4+2]=(s[i]>>8)&0xff;  out[i*4+3]=s[i]&0xff;
    }
    return out;
}

std::array<uint8_t, 32> sha256(std::span<const uint8_t> data) {
    return sha256_raw(data.data(), data.size());
}
std::array<uint8_t, 32> sha256d(std::span<const uint8_t> data) {
    auto h = sha256_raw(data.data(), data.size());
    return sha256_raw(h.data(), 32);
}

// ============================================================
// SHA-512 (for HMAC-SHA512)
// ============================================================

static const uint64_t K512[80] = {
    0x428a2f98d728ae22ULL,0x7137449123ef65cdULL,0xb5c0fbcfec4d3b2fULL,0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL,0x59f111f1b605d019ULL,0x923f82a4af194f9bULL,0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL,0x12835b0145706fbeULL,0x243185be4ee4b28cULL,0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL,0x80deb1fe3b1696b1ULL,0x9bdc06a725c71235ULL,0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL,0xefbe4786384f25e3ULL,0x0fc19dc68b8cd5b5ULL,0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL,0x4a7484aa6ea6e483ULL,0x5cb0a9dcbd41fbd4ULL,0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL,0xa831c66d2db43210ULL,0xb00327c898fb213fULL,0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL,0xd5a79147930aa725ULL,0x06ca6351e003826fULL,0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL,0x2e1b21385c26c926ULL,0x4d2c6dfc5ac42aedULL,0x53380d139d95b3dfULL,
    0x650a73548baf63deULL,0x766a0abb3c77b2a8ULL,0x81c2c92e47edaee6ULL,0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL,0xa81a664bbc423001ULL,0xc24b8b70d0f89791ULL,0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL,0xd69906245565a910ULL,0xf40e35855771202aULL,0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL,0x1e376c085141ab53ULL,0x2748774cdf8eeb99ULL,0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL,0x4ed8aa4ae3418acbULL,0x5b9cca4f7763e373ULL,0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL,0x78a5636f43172f60ULL,0x84c87814a1f0ab72ULL,0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL,0xa4506cebde82bde9ULL,0xbef9a3f7b2c67915ULL,0xc67178f2e372532bULL,
    0xca273eceea26619cULL,0xd186b8c721c0c207ULL,0xeada7dd6cde0eb1eULL,0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL,0x0a637dc5a2c898a6ULL,0x113f9804bef90daeULL,0x1b710b35131c471bULL,
    0x28db77f523047d84ULL,0x32caab7b40c72493ULL,0x3c9ebe0a15c9bebcULL,0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL,0x597f299cfc657e2aULL,0x5fcb6fab3ad6faecULL,0x6c44198c4a475817ULL,
};

static inline uint64_t rotr64(uint64_t x, int n) { return (x >> n) | (x << (64 - n)); }

static void sha512_block(uint64_t s[8], const uint8_t block[128]) {
    uint64_t w[80];
    for (int i = 0; i < 16; ++i) {
        w[i] = 0;
        for (int j = 0; j < 8; ++j) w[i] = (w[i]<<8)|block[i*8+j];
    }
    for (int i = 16; i < 80; ++i) {
        uint64_t s0 = rotr64(w[i-15],1)^rotr64(w[i-15],8)^(w[i-15]>>7);
        uint64_t s1 = rotr64(w[i-2],19)^rotr64(w[i-2],61)^(w[i-2]>>6);
        w[i] = w[i-16]+s0+w[i-7]+s1;
    }
    uint64_t a=s[0],b=s[1],c=s[2],d=s[3],e=s[4],f=s[5],g=s[6],h=s[7];
    for (int i = 0; i < 80; ++i) {
        uint64_t S1=(rotr64(e,14))^(rotr64(e,18))^(rotr64(e,41));
        uint64_t ch=(e&f)^(~e&g);
        uint64_t t1=h+S1+ch+K512[i]+w[i];
        uint64_t S0=(rotr64(a,28))^(rotr64(a,34))^(rotr64(a,39));
        uint64_t maj=(a&b)^(a&c)^(b&c);
        uint64_t t2=S0+maj;
        h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    s[0]+=a;s[1]+=b;s[2]+=c;s[3]+=d;s[4]+=e;s[5]+=f;s[6]+=g;s[7]+=h;
}

static std::array<uint8_t, 64> sha512_raw(const uint8_t* data, size_t len) {
    uint64_t s[8] = {
        0x6a09e667f3bcc908ULL,0xbb67ae8584caa73bULL,0x3c6ef372fe94f82bULL,0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL,0x9b05688c2b3e6c1fULL,0x1f83d9abfb41bd6bULL,0x5be0cd19137e2179ULL,
    };
    uint8_t buf[128];
    size_t pos = 0;
    uint64_t bitlen_lo = (uint64_t)len * 8;
    uint64_t bitlen_hi = (uint64_t)(len >> 61);

    while (pos + 128 <= len) { sha512_block(s, data + pos); pos += 128; }
    size_t rem = len - pos;
    memcpy(buf, data + pos, rem);
    buf[rem++] = 0x80;
    if (rem > 112) {
        while (rem < 128) buf[rem++] = 0;
        sha512_block(s, buf);
        rem = 0;
    }
    while (rem < 112) buf[rem++] = 0;
    for (int i = 7; i >= 0; --i) buf[rem++] = (uint8_t)(bitlen_hi >> (i*8));
    for (int i = 7; i >= 0; --i) buf[rem++] = (uint8_t)(bitlen_lo >> (i*8));
    sha512_block(s, buf);

    std::array<uint8_t, 64> out;
    for (int i = 0; i < 8; ++i)
        for (int j = 7; j >= 0; --j) out[i*8+(7-j)] = (uint8_t)(s[i]>>(j*8));
    return out;
}

std::array<uint8_t, 64> sha512(std::span<const uint8_t> data) {
    return sha512_raw(data.data(), data.size());
}

std::array<uint8_t, 32> sha512_half(std::span<const uint8_t> data) {
    auto h = sha512_raw(data.data(), data.size());
    std::array<uint8_t, 32> out;
    std::copy(h.begin(), h.begin()+32, out.begin());
    return out;
}

std::array<uint8_t, 64> hmac_sha512(std::span<const uint8_t> key,
                                     std::span<const uint8_t> data) {
    uint8_t k[128] = {};
    if (key.size() > 128) {
        auto hk = sha512_raw(key.data(), key.size());
        memcpy(k, hk.data(), 64);
    } else {
        memcpy(k, key.data(), key.size());
    }
    uint8_t ipad[128], opad[128];
    for (int i = 0; i < 128; ++i) { ipad[i] = k[i]^0x36; opad[i] = k[i]^0x5c; }

    std::vector<uint8_t> inner(128 + data.size());
    memcpy(inner.data(), ipad, 128);
    memcpy(inner.data()+128, data.data(), data.size());
    auto inner_h = sha512_raw(inner.data(), inner.size());

    std::vector<uint8_t> outer(128 + 64);
    memcpy(outer.data(), opad, 128);
    memcpy(outer.data()+128, inner_h.data(), 64);
    return sha512_raw(outer.data(), outer.size());
}

// ============================================================
// RIPEMD-160
// ============================================================

#define F(x,y,z) ((x)^(y)^(z))
#define G(x,y,z) (((x)&(y))|(~(x)&(z)))
#define H(x,y,z) (((x)|(~(y)))^(z))
#define I(x,y,z) (((x)&(z))|((y)&~(z)))
#define J(x,y,z) ((x)^((y)|(~(z))))
#define ROL(x,n) (((x)<<(n))|((x)>>(32-(n))))

static void rmd160_block(uint32_t s[5], const uint8_t block[64]) {
    static const uint8_t r[80] = {
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
        7,4,13,1,10,6,15,3,12,0,9,5,2,14,11,8,
        3,10,14,4,9,15,8,1,2,7,0,6,13,11,5,12,
        1,9,11,10,0,8,12,4,13,3,7,15,14,5,6,2,
        4,0,5,9,7,12,2,10,14,1,3,8,11,6,15,13,
    };
    static const uint8_t rp[80] = {
        5,14,7,0,9,2,11,4,13,6,15,8,1,10,3,12,
        6,11,3,7,0,13,5,10,14,15,8,12,4,9,1,2,
        15,5,1,3,7,14,6,9,11,8,12,2,10,0,4,13,
        8,6,4,1,3,11,15,0,5,12,2,13,9,7,10,14,
        12,15,10,4,1,5,8,7,6,2,13,14,0,3,9,11,
    };
    static const uint8_t sl[80] = {
        11,14,15,12,5,8,7,9,11,13,14,15,6,7,9,8,
        7,6,8,13,11,9,7,15,7,12,15,9,11,7,13,12,
        11,13,6,7,14,9,13,15,14,8,13,6,5,12,7,5,
        11,12,14,15,14,15,9,8,9,14,5,6,8,6,5,12,
        9,15,5,11,6,8,13,12,5,12,13,14,11,8,5,6,
    };
    static const uint8_t slp[80] = {
        8,9,9,11,13,15,15,5,7,7,8,11,14,14,12,6,
        9,13,15,7,12,8,9,11,7,7,12,7,6,15,13,11,
        9,7,15,11,8,6,6,14,12,13,5,14,13,13,7,5,
        15,5,8,11,14,14,6,14,6,9,12,9,12,5,15,8,
        8,5,12,9,12,5,14,6,8,13,6,5,15,13,11,11,
    };
    uint32_t w[16];
    for (int i = 0; i < 16; ++i)
        w[i] = ((uint32_t)block[i*4])|((uint32_t)block[i*4+1]<<8)|
               ((uint32_t)block[i*4+2]<<16)|((uint32_t)block[i*4+3]<<24);

    uint32_t a=s[0],b=s[1],c=s[2],d=s[3],e=s[4];
    uint32_t ap=s[0],bp=s[1],cp=s[2],dp=s[3],ep=s[4];

    static const uint32_t K[5]  = {0,0x5a827999,0x6ed9eba1,0x8f1bbcdc,0xa953fd4e};
    static const uint32_t Kp[5] = {0x50a28be6,0x5c4dd124,0x6d703ef3,0x7a6d76e9,0};

    for (int i = 0; i < 80; ++i) {
        int round = i/16;
        uint32_t f,fp;
        switch (round) {
            case 0: f=F(b,c,d); fp=J(bp,cp,dp); break;
            case 1: f=G(b,c,d); fp=I(bp,cp,dp); break;
            case 2: f=H(b,c,d); fp=H(bp,cp,dp); break;
            case 3: f=I(b,c,d); fp=G(bp,cp,dp); break;
            default: f=J(b,c,d); fp=F(bp,cp,dp); break;
        }
        uint32_t t=ROL(a+f+w[r[i]]+K[round],sl[i])+e; a=e;e=d;d=ROL(c,10);c=b;b=t;
        t=ROL(ap+fp+w[rp[i]]+Kp[round],slp[i])+ep; ap=ep;ep=dp;dp=ROL(cp,10);cp=bp;bp=t;
    }
    uint32_t t=s[1]+c+dp; s[1]=s[2]+d+ep; s[2]=s[3]+e+ap; s[3]=s[4]+a+bp;
    s[4]=s[0]+b+cp; s[0]=t;
}

std::array<uint8_t, 20> ripemd160(std::span<const uint8_t> data) {
    uint32_t s[5] = {0x67452301,0xefcdab89,0x98badcfe,0x10325476,0xc3d2e1f0};
    uint8_t buf[64];
    const uint8_t* d = data.data();
    size_t len = data.size(), pos = 0;
    uint64_t bitlen = (uint64_t)len*8;

    while (pos+64 <= len) { rmd160_block(s, d+pos); pos+=64; }
    size_t rem = len-pos;
    memcpy(buf, d+pos, rem);
    buf[rem++] = 0x80;
    if (rem > 56) {
        while (rem < 64) buf[rem++]=0;
        rmd160_block(s, buf); rem=0;
    }
    while (rem < 56) buf[rem++]=0;
    for (int i=0;i<8;i++) buf[56+i]=(uint8_t)(bitlen>>(i*8));
    rmd160_block(s, buf);

    std::array<uint8_t, 20> out;
    for (int i=0;i<5;i++) {
        out[i*4+0]=s[i]&0xff; out[i*4+1]=(s[i]>>8)&0xff;
        out[i*4+2]=(s[i]>>16)&0xff; out[i*4+3]=(s[i]>>24)&0xff;
    }
    return out;
}

#undef F
#undef G
#undef H
#undef I
#undef J
#undef ROL

std::array<uint8_t, 20> hash160(std::span<const uint8_t> data) {
    auto h = sha256(data);
    return ripemd160(h);
}

// ============================================================
// Base58Check
// ============================================================

static const char B58ALPHA[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

std::string base58check_encode(std::span<const uint8_t> payload) {
    auto chk = sha256d(payload);
    std::vector<uint8_t> buf(payload.begin(), payload.end());
    buf.insert(buf.end(), chk.begin(), chk.begin()+4);

    // Count leading zeros
    int leading = 0;
    for (uint8_t b : buf) { if (b) break; ++leading; }

    // Convert to base58
    std::vector<uint8_t> digits;
    for (uint8_t byte : buf) {
        int carry = byte;
        for (auto& d : digits) { carry += 256 * d; d = carry % 58; carry /= 58; }
        while (carry) { digits.push_back(carry % 58); carry /= 58; }
    }

    std::string result(leading, '1');
    for (auto it = digits.rbegin(); it != digits.rend(); ++it)
        result += B58ALPHA[*it];
    return result;
}

// ============================================================
// Bech32 / Bech32m  (BIP173 / BIP350)
// ============================================================

static const char BECH32_CHARSET[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

static uint32_t bech32_polymod(const std::vector<uint8_t>& v) {
    uint32_t c = 1;
    static const uint32_t GEN[5] = {0x3b6a57b2,0x26508e6d,0x1ea119fa,0x3d4233dd,0x2a1462b3};
    for (uint8_t d : v) {
        uint8_t c0 = c >> 25;
        c = ((c & 0x1ffffff) << 5) ^ d;
        for (int i = 0; i < 5; ++i) if ((c0>>i)&1) c ^= GEN[i];
    }
    return c;
}

// Convert 8-bit groups to 5-bit groups
static std::vector<uint8_t> convertbits(std::span<const uint8_t> data, int from, int to) {
    int acc = 0, bits = 0;
    std::vector<uint8_t> out;
    int maxv = (1 << to) - 1;
    for (uint8_t v : data) {
        acc = (acc << from) | v;
        bits += from;
        while (bits >= to) { bits -= to; out.push_back((acc >> bits) & maxv); }
    }
    if (bits) out.push_back((acc << (to - bits)) & maxv);
    return out;
}

std::string bech32_encode(const std::string& hrp, int version,
                           std::span<const uint8_t> data) {
    // BIP350: version 0 uses bech32 constant 1, version 1+ uses bech32m constant
    uint32_t const_val = (version == 0) ? 1 : 0x2bc830a3;

    auto prog5 = convertbits(data, 8, 5);

    // Build values for polymod: hrp expanded + [version] + prog5 + [0,0,0,0,0,0]
    std::vector<uint8_t> enc;
    for (char c : hrp) enc.push_back((uint8_t)c >> 5);
    enc.push_back(0);
    for (char c : hrp) enc.push_back((uint8_t)c & 0x1f);
    enc.push_back((uint8_t)version);
    for (uint8_t v : prog5) enc.push_back(v);
    for (int i = 0; i < 6; ++i) enc.push_back(0);

    uint32_t mod = bech32_polymod(enc) ^ const_val;

    std::string result = hrp + "1";
    result += BECH32_CHARSET[version];
    for (uint8_t v : prog5) result += BECH32_CHARSET[v];
    for (int i = 5; i >= 0; --i) result += BECH32_CHARSET[(mod >> (5*i)) & 31];
    return result;
}

// ============================================================
// Hex utilities
// ============================================================

std::string to_hex(std::span<const uint8_t> data) {
    static const char HEX[] = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (uint8_t b : data) { out += HEX[b>>4]; out += HEX[b&0xf]; }
    return out;
}

std::vector<uint8_t> from_hex(const std::string& hex) {
    if (hex.size() % 2) throw std::invalid_argument("odd-length hex string");
    std::vector<uint8_t> out;
    out.reserve(hex.size()/2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        auto nibble = [](char c) -> uint8_t {
            if (c>='0'&&c<='9') return c-'0';
            if (c>='a'&&c<='f') return c-'a'+10;
            if (c>='A'&&c<='F') return c-'A'+10;
            throw std::invalid_argument("invalid hex character");
        };
        out.push_back((nibble(hex[i])<<4)|nibble(hex[i+1]));
    }
    return out;
}

} // namespace btcwallet
