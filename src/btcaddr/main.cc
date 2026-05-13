#include "parser.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

static void print_help(const char* prog) {
    fprintf(stdout,
        "Usage: %s [options] <blocks-dir> <output-file>\n"
        "\n"
        "Scan Bitcoin Core block files and extract all unique addresses,\n"
        "sorted in ascending order.\n"
        "\n"
        "Arguments:\n"
        "  <blocks-dir>    Directory containing blk*.dat files\n"
        "                  (e.g. ~/.bitcoin/blocks)\n"
        "  <output-file>   File to write sorted addresses to (one per line)\n"
        "\n"
        "Options:\n"
        "  --testnet       Use testnet address encoding and default magic\n"
        "                  (sets magic to 0b110907 automatically)\n"
        "  --no-p2pk       Skip P2PK outputs (omit derived P2PKH equivalents)\n"
        "  --magic <hex>   Override 4-byte block magic as 8 hex chars\n"
        "                    mainnet:  f9beb4d9  (default)\n"
        "                    testnet3: 0b110907\n"
        "                    signet:   0a03cf40\n"
        "                    regtest:  fabfb5da\n"
        "  -h, --help      Show this help\n"
        "\n"
        "Address types extracted:\n"
        "  P2PKH  (1...)    legacy pay-to-public-key-hash\n"
        "  P2SH   (3...)    pay-to-script-hash\n"
        "  P2WPKH (bc1q...) native segwit v0, 20-byte program\n"
        "  P2WSH  (bc1q...) native segwit v0, 32-byte program\n"
        "  P2TR   (bc1p...) taproot / segwit v1\n"
        "  P2PK   derived   early pay-to-pubkey (shown as equivalent P2PKH)\n"
        "\n"
        "Memory: each unique address uses ~80-100 bytes in the working set.\n"
        "        For full blockchain data (1B+ addresses) consider 100+ GB RAM\n"
        "        or scan a pruned node / recent blocks only.\n"
        "\n"
        "Examples:\n"
        "  %s ~/.bitcoin/blocks addrs.txt\n"
        "  %s --testnet ~/.bitcoin/testnet3/blocks testnet_addrs.txt\n"
        "  %s --no-p2pk ~/.bitcoin/blocks addrs.txt\n",
        prog, prog, prog, prog);
}

static bool parse_magic(const char* hex, uint8_t out[4]) {
    if (strlen(hex) != 8) return false;
    for (int i = 0; i < 4; i++) {
        char byte_s[3] = {hex[i*2], hex[i*2+1], '\0'};
        char* end;
        long v = strtol(byte_s, &end, 16);
        if (*end != '\0' || v < 0 || v > 255) return false;
        out[i] = static_cast<uint8_t>(v);
    }
    return true;
}

int main(int argc, char* argv[]) {
    btcaddr::ScanOptions opts;
    std::string blocks_dir, output_file;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--testnet") == 0) {
            opts.mainnet = false;
            opts.magic[0] = 0x0b; opts.magic[1] = 0x11;
            opts.magic[2] = 0x09; opts.magic[3] = 0x07;
        } else if (strcmp(argv[i], "--no-p2pk") == 0) {
            opts.include_p2pk = false;
        } else if (strcmp(argv[i], "--magic") == 0) {
            if (i+1 >= argc) {
                fprintf(stderr, "error: --magic requires a value\n");
                return 1;
            }
            if (!parse_magic(argv[++i], opts.magic)) {
                fprintf(stderr, "error: --magic must be 8 hex chars, e.g. f9beb4d9\n");
                return 1;
            }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
            fprintf(stderr, "Run '%s --help' for usage.\n", argv[0]);
            return 1;
        } else if (blocks_dir.empty()) {
            blocks_dir = argv[i];
        } else if (output_file.empty()) {
            output_file = argv[i];
        } else {
            fprintf(stderr, "error: unexpected argument '%s'\n", argv[i]);
            return 1;
        }
    }

    if (blocks_dir.empty() || output_file.empty()) {
        print_help(argv[0]);
        return 1;
    }

    // Collect and sort blk*.dat files
    std::vector<fs::path> blk_files;
    try {
        for (auto& entry : fs::directory_iterator(blocks_dir)) {
            auto name = entry.path().filename().string();
            if (name.size() > 7 &&
                name.substr(0, 3) == "blk" &&
                name.substr(name.size() - 4) == ".dat") {
                blk_files.push_back(entry.path());
            }
        }
    } catch (const fs::filesystem_error& e) {
        fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }

    if (blk_files.empty()) {
        fprintf(stderr, "error: no blk*.dat files found in '%s'\n",
                blocks_dir.c_str());
        return 1;
    }
    std::sort(blk_files.begin(), blk_files.end());

    fprintf(stderr, "Found %zu block file(s) in %s\n",
            blk_files.size(), blocks_dir.c_str());
    fprintf(stderr, "Network: %s\n", opts.mainnet ? "mainnet" : "testnet");
    fprintf(stderr, "Magic:   %02x%02x%02x%02x\n",
            opts.magic[0], opts.magic[1], opts.magic[2], opts.magic[3]);
    fprintf(stderr, "P2PK:    %s\n\n", opts.include_p2pk ? "included" : "skipped");

    // Scan all files
    std::unordered_set<std::string> addresses;
    btcaddr::ScanStats total;

    for (size_t fi = 0; fi < blk_files.size(); fi++) {
        const auto& path = blk_files[fi];
        fprintf(stderr, "[%zu/%zu] %-16s ",
                fi + 1, blk_files.size(),
                path.filename().string().c_str());
        fflush(stderr);

        try {
            auto s = btcaddr::scan_block_file(path.string(), opts, addresses);
            total.blocks       += s.blocks;
            total.transactions += s.transactions;
            total.outputs      += s.outputs;
            fprintf(stderr,
                    "%6llu blocks  %8llu txs  unique addrs: %zu\n",
                    (unsigned long long)s.blocks,
                    (unsigned long long)s.transactions,
                    addresses.size());
        } catch (const std::exception& e) {
            fprintf(stderr, "ERROR: %s\n", e.what());
        }
    }

    fprintf(stderr,
            "\nTotal  blocks: %llu  transactions: %llu  outputs: %llu\n"
            "Unique addresses: %zu\n",
            (unsigned long long)total.blocks,
            (unsigned long long)total.transactions,
            (unsigned long long)total.outputs,
            addresses.size());

    // Sort
    fprintf(stderr, "Sorting %zu addresses...\n", addresses.size());
    std::vector<std::string> sorted(addresses.begin(), addresses.end());
    std::sort(sorted.begin(), sorted.end());

    // Write output
    fprintf(stderr, "Writing to %s...\n", output_file.c_str());
    std::ofstream out(output_file);
    if (!out) {
        fprintf(stderr, "error: cannot create '%s'\n", output_file.c_str());
        return 1;
    }
    for (const auto& addr : sorted) out << addr << '\n';
    if (!out) {
        fprintf(stderr, "error: write failed for '%s'\n", output_file.c_str());
        return 1;
    }

    fprintf(stderr, "Done. %zu addresses written to %s\n",
            sorted.size(), output_file.c_str());
    return 0;
}
