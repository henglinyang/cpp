#include "chainstate.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>
#include <algorithm>

static void print_help(const char* prog) {
    fprintf(stdout,
        "Usage: %s [options] <chainstate-dir>\n"
        "\n"
        "Scan Bitcoin Core's chainstate LevelDB and print all current UTXOs.\n"
        "\n"
        "Arguments:\n"
        "  <chainstate-dir>   Path to the chainstate directory\n"
        "                     (e.g. ~/.bitcoin/chainstate)\n"
        "\n"
        "Options:\n"
        "  --addresses-only   Print only unique addresses with current funds,\n"
        "                     sorted ascending. Does not print per-UTXO detail.\n"
        "  --testnet          Use testnet address encoding\n"
        "  -h, --help         Show this help\n"
        "\n"
        "Default output (one UTXO per line, tab-separated):\n"
        "  txid:vout<TAB>satoshis<TAB>height<TAB>coinbase<TAB>address\n"
        "\n"
        "Examples:\n"
        "  %s ~/.bitcoin/chainstate\n"
        "  %s --addresses-only ~/.bitcoin/chainstate\n"
        "  %s --testnet ~/.bitcoin/testnet3/chainstate\n",
        prog, prog, prog, prog);
}

int main(int argc, char* argv[]) {
    btcutxo::ScanOptions opts;
    std::string chainstate_dir;
    bool addresses_only = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--testnet") == 0) {
            opts.mainnet = false;
        } else if (strcmp(argv[i], "--addresses-only") == 0) {
            addresses_only = true;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
            fprintf(stderr, "Run '%s --help' for usage.\n", argv[0]);
            return 1;
        } else if (chainstate_dir.empty()) {
            chainstate_dir = argv[i];
        } else {
            fprintf(stderr, "error: unexpected argument '%s'\n", argv[i]);
            return 1;
        }
    }

    if (chainstate_dir.empty()) {
        print_help(argv[0]);
        return 1;
    }

    if (addresses_only) {
        // Collect unique non-empty addresses, then sort and print.
        std::unordered_set<std::string> addr_set;
        try {
            btcutxo::scan_chainstate(chainstate_dir, opts,
                [&](const btcutxo::Utxo& u) {
                    if (!u.address.empty())
                        addr_set.insert(u.address);
                });
        } catch (const std::exception& e) {
            fprintf(stderr, "error: %s\n", e.what());
            return 1;
        }

        std::vector<std::string> sorted(addr_set.begin(), addr_set.end());
        std::sort(sorted.begin(), sorted.end());
        for (const auto& a : sorted)
            puts(a.c_str());

    } else {
        // Stream each UTXO to stdout.
        uint64_t count = 0;
        try {
            btcutxo::scan_chainstate(chainstate_dir, opts,
                [&](const btcutxo::Utxo& u) {
                    printf("%s:%u\t%lld\t%u\t%d\t%s\n",
                           u.txid_hex.c_str(),
                           u.vout,
                           (long long)u.satoshis,
                           u.height,
                           (int)u.coinbase,
                           u.address.c_str());
                    ++count;
                });
        } catch (const std::exception& e) {
            fprintf(stderr, "error: %s\n", e.what());
            return 1;
        }
        fprintf(stderr, "Done. %llu UTXOs scanned.\n", (unsigned long long)count);
    }

    return 0;
}
