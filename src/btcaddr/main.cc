#include "parser.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

#ifdef __linux__
#include <unistd.h>
#endif

namespace fs = std::filesystem;

// Estimated bytes consumed per address in the unordered_set (node + heap string).
static constexpr size_t BYTES_PER_ADDR = 150;

// Minimum addresses per chunk to avoid creating thousands of tiny files.
static constexpr size_t MIN_CHUNK_ADDRS = 100'000;

// ---- Available memory detection ----

static size_t detect_available_bytes() {
#ifdef __linux__
    std::ifstream f("/proc/meminfo");
    std::string line;
    while (std::getline(f, line)) {
        if (line.compare(0, 13, "MemAvailable:") == 0) {
            size_t kb = std::stoull(line.c_str() + 13);
            return kb * 1024;
        }
    }
#endif
    return 0;
}

// ---- K-way merge of sorted chunk files → unique sorted output ----

static bool merge_chunks(const std::vector<std::string>& chunk_paths,
                           const std::string& output_path) {
    struct Entry {
        std::string line;
        size_t idx;
        bool operator>(const Entry& o) const { return line > o.line; }
    };

    std::vector<std::ifstream> files(chunk_paths.size());
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pq;

    for (size_t i = 0; i < chunk_paths.size(); i++) {
        files[i].open(chunk_paths[i]);
        if (!files[i]) {
            fprintf(stderr, "error: cannot open chunk '%s'\n", chunk_paths[i].c_str());
            return false;
        }
        std::string line;
        if (std::getline(files[i], line))
            pq.push({std::move(line), i});
    }

    std::ofstream out(output_path);
    if (!out) {
        fprintf(stderr, "error: cannot create '%s'\n", output_path.c_str());
        return false;
    }

    std::string prev;
    size_t written = 0;
    while (!pq.empty()) {
        auto top = pq.top(); pq.pop();
        if (top.line != prev) {
            out << top.line << '\n';
            prev = top.line;
            ++written;
        }
        std::string next;
        if (std::getline(files[top.idx], next))
            pq.push({std::move(next), top.idx});
    }

    fprintf(stderr, "Done. %zu unique addresses written to %s\n",
            written, output_path.c_str());
    return out.good();
}

// ---- Help ----

static void print_help(const char* prog) {
    size_t avail_mb = detect_available_bytes() / (1024 * 1024);
    size_t default_mb = avail_mb > 0 ? avail_mb * 9 / 10 : 0;

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
        "  --max-memory N  Limit address set to N MB of RAM.\n"
        "                  When exceeded, sorted chunks are flushed to disk\n"
        "                  and merged at the end (external sort).\n"
        "                  Default: 90%% of MemAvailable",
        prog);

    if (avail_mb > 0)
        fprintf(stdout, " (~%zu MB on this system)\n", default_mb);
    else
        fprintf(stdout, " (could not detect; unlimited)\n");

    fprintf(stdout,
        "  --chunk-dir D   Directory for temporary chunk files (default: /tmp).\n"
        "                  Use a path on a real disk if /tmp is a RAM disk\n"
        "                  (e.g. tmpfs) and too small for the chunks.\n"
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
        "Examples:\n"
        "  %s ~/.bitcoin/blocks addrs.txt\n"
        "  %s --max-memory 2048 ~/.bitcoin/blocks addrs.txt\n"
        "  %s --max-memory 2048 --chunk-dir /data/scratch ~/.bitcoin/blocks addrs.txt\n"
        "  %s --testnet ~/.bitcoin/testnet3/blocks testnet_addrs.txt\n",
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
    size_t max_memory_mb = 0;
    bool max_memory_set = false;
    std::string chunk_dir = "/tmp";

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
        } else if (strcmp(argv[i], "--chunk-dir") == 0) {
            if (i+1 >= argc) {
                fprintf(stderr, "error: --chunk-dir requires a path\n");
                return 1;
            }
            chunk_dir = argv[++i];
        } else if (strcmp(argv[i], "--max-memory") == 0) {
            if (i+1 >= argc) {
                fprintf(stderr, "error: --max-memory requires a value in MB\n");
                return 1;
            }
            char* end;
            long v = strtol(argv[++i], &end, 10);
            if (*end != '\0' || v <= 0) {
                fprintf(stderr, "error: --max-memory must be a positive integer (MB)\n");
                return 1;
            }
            max_memory_mb = static_cast<size_t>(v);
            max_memory_set = true;
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

    // Resolve memory limit
    if (!max_memory_set) {
        size_t avail = detect_available_bytes();
        if (avail > 0) {
            max_memory_mb = (avail / (1024 * 1024)) * 9 / 10;
        }
    }

    size_t flush_threshold = 0;
    if (max_memory_mb > 0) {
        size_t limit_bytes = max_memory_mb * 1024 * 1024;
        flush_threshold = std::max(limit_bytes / BYTES_PER_ADDR, MIN_CHUNK_ADDRS);
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
    if (flush_threshold > 0)
        fprintf(stderr, "Memory:  %zu MB limit (~%zu addrs/chunk), chunks in %s\n",
                max_memory_mb, flush_threshold, chunk_dir.c_str());
    else
        fprintf(stderr, "Memory:  unlimited\n");
    fprintf(stderr, "P2PK:    %s\n\n", opts.include_p2pk ? "included" : "skipped");

    // Set up flush callback when memory limit is active
    std::unordered_set<std::string> addresses;
    std::vector<std::string> chunk_paths;
    int chunk_counter = 0;

#ifdef __linux__
    pid_t pid = getpid();
#else
    int pid = 0;
#endif

    if (flush_threshold > 0) {
        opts.flush_threshold = flush_threshold;
        opts.on_flush = [&](std::unordered_set<std::string>& set) {
            auto path = chunk_dir + "/btcaddr_" + std::to_string(pid)
                        + "_" + std::to_string(chunk_counter++) + ".chunk";
            chunk_paths.push_back(path);

            // Extract strings from set (moves content, no copy) then sort.
            std::vector<std::string> chunk;
            chunk.reserve(set.size());
            while (!set.empty()) {
                auto node = set.extract(set.begin());
                chunk.push_back(std::move(node.value()));
            }
            // set is now empty; sort and write the chunk.
            std::sort(chunk.begin(), chunk.end());
            std::ofstream f(path);
            for (auto& a : chunk) f << a << '\n';

            fprintf(stderr, "  [chunk %d] %zu addrs → %s\n",
                    chunk_counter - 1, chunk.size(), path.c_str());
        };
    }

    // Scan all files
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
                    "%6llu blocks  %8llu txs  buffered: %zu\n",
                    (unsigned long long)s.blocks,
                    (unsigned long long)s.transactions,
                    addresses.size());
        } catch (const std::exception& e) {
            fprintf(stderr, "ERROR: %s\n", e.what());
        }
    }

    fprintf(stderr,
            "\nTotal  blocks: %llu  transactions: %llu  outputs: %llu\n",
            (unsigned long long)total.blocks,
            (unsigned long long)total.transactions,
            (unsigned long long)total.outputs);

    // ---- Output phase ----

    if (chunk_paths.empty()) {
        // All addresses fit in memory — sort and write directly.
        fprintf(stderr, "Sorting %zu addresses...\n", addresses.size());
        std::vector<std::string> sorted(addresses.begin(), addresses.end());
        std::sort(sorted.begin(), sorted.end());

        fprintf(stderr, "Writing to %s...\n", output_file.c_str());
        std::ofstream out(output_file);
        if (!out) {
            fprintf(stderr, "error: cannot create '%s'\n", output_file.c_str());
            return 1;
        }
        for (const auto& a : sorted) out << a << '\n';
        if (!out) {
            fprintf(stderr, "error: write failed for '%s'\n", output_file.c_str());
            return 1;
        }
        fprintf(stderr, "Done. %zu addresses written to %s\n",
                sorted.size(), output_file.c_str());

    } else {
        // Flush the remaining in-memory addresses as a final chunk.
        if (!addresses.empty())
            opts.on_flush(addresses);

        fprintf(stderr, "Merging %zu chunk files...\n", chunk_paths.size());
        bool ok = merge_chunks(chunk_paths, output_file);

        // Clean up temp files regardless of merge outcome.
        for (auto& p : chunk_paths) std::remove(p.c_str());

        if (!ok) return 1;
    }

    return 0;
}
