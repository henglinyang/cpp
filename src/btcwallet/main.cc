#include "keygen.h"
#include "crypto.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

static void printHelp(const char* prog) {
    fprintf(stdout,
        "Usage: %s [options]\n"
        "\n"
        "Generate Bitcoin key pairs with full control over all parameters.\n"
        "\n"
        "Options:\n"
        "  --type <ecdsa-compressed|ecdsa-uncompressed|schnorr>\n"
        "                      Key type (default: ecdsa-compressed)\n"
        "  --entropy <hex>     32-byte private key seed as hex (64 hex chars)\n"
        "  --hd-seed <hex>     64-byte BIP32 master seed as hex (128 hex chars)\n"
        "  --path <path>       BIP32 derivation path, e.g. m/44'/0'/0'/0/0\n"
        "  --format <fmt>      Output format: hex, wif, address, or all (default: all)\n"
        "  --testnet           Use testnet address encoding\n"
        "  -h, --help          Show this help message\n"
        "\n"
        "Examples:\n"
        "  %s\n"
        "  %s --type schnorr\n"
        "  %s --entropy 0000000000000000000000000000000000000000000000000000000000000001\n"
        "  %s --path m/44'/0'/0'/0/0\n"
        "  %s --hd-seed <128-hex-chars> --path m/44'/0'/0'/0/0 --format address\n",
        prog, prog, prog, prog, prog, prog);
}

int main(int argc, char* argv[]) {
    btcwallet::KeyGenParams params;
    std::string format = "all";
    bool mainnet = true;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printHelp(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--testnet") == 0) {
            mainnet = false;
        } else if (strcmp(argv[i], "--type") == 0) {
            if (i+1 >= argc) { fprintf(stderr, "error: --type requires an argument\n"); return 1; }
            std::string t = argv[++i];
            if (t == "ecdsa-compressed")   params.key_type = btcwallet::KeyType::ECDSA_COMPRESSED;
            else if (t == "ecdsa-uncompressed") params.key_type = btcwallet::KeyType::ECDSA_UNCOMPRESSED;
            else if (t == "schnorr")       params.key_type = btcwallet::KeyType::SCHNORR;
            else { fprintf(stderr, "error: unknown type '%s'\n", t.c_str()); return 1; }
        } else if (strcmp(argv[i], "--entropy") == 0) {
            if (i+1 >= argc) { fprintf(stderr, "error: --entropy requires an argument\n"); return 1; }
            try { params.entropy = btcwallet::from_hex(argv[++i]); }
            catch (const std::exception& e) { fprintf(stderr, "error: --entropy: %s\n", e.what()); return 1; }
        } else if (strcmp(argv[i], "--hd-seed") == 0) {
            if (i+1 >= argc) { fprintf(stderr, "error: --hd-seed requires an argument\n"); return 1; }
            try { params.hd_seed = btcwallet::from_hex(argv[++i]); }
            catch (const std::exception& e) { fprintf(stderr, "error: --hd-seed: %s\n", e.what()); return 1; }
        } else if (strcmp(argv[i], "--path") == 0) {
            if (i+1 >= argc) { fprintf(stderr, "error: --path requires an argument\n"); return 1; }
            try { params.hd_path = btcwallet::DerivationPath::parse(argv[++i]); }
            catch (const std::exception& e) { fprintf(stderr, "error: --path: %s\n", e.what()); return 1; }
        } else if (strcmp(argv[i], "--format") == 0) {
            if (i+1 >= argc) { fprintf(stderr, "error: --format requires an argument\n"); return 1; }
            format = argv[++i];
            if (format != "hex" && format != "wif" && format != "address" && format != "all") {
                fprintf(stderr, "error: unknown format '%s'\n", format.c_str());
                return 1;
            }
        } else {
            fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
            fprintf(stderr, "Run '%s --help' for usage.\n", argv[0]);
            return 1;
        }
    }

    try {
        auto kp = btcwallet::generateKeyPair(params, mainnet);

        if (format == "hex" || format == "all") {
            printf("Private key (hex): %s\n", kp.private_key_hex().c_str());
            printf("Public key  (hex): %s\n", kp.public_key_hex().c_str());
        }
        if (format == "wif" || format == "all") {
            printf("Private key (WIF): %s\n", kp.private_key_wif().c_str());
        }
        if (format == "address" || format == "all") {
            printf("Address:           %s\n", kp.address().c_str());
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }

    return 0;
}
