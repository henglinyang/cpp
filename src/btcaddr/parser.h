#pragma once
#include <cstdint>
#include <string>
#include <unordered_set>

namespace btcaddr {

struct ScanOptions {
    bool mainnet      = true;
    bool include_p2pk = true;
    // Raw magic bytes as they appear in the file. Mainnet: f9 be b4 d9.
    uint8_t magic[4]  = {0xf9, 0xbe, 0xb4, 0xd9};
};

struct ScanStats {
    uint64_t blocks       = 0;
    uint64_t transactions = 0;
    uint64_t outputs      = 0;
};

// Scan one blk*.dat file. Recognized addresses are inserted into `addresses`.
ScanStats scan_block_file(const std::string& path,
                           const ScanOptions& opts,
                           std::unordered_set<std::string>& addresses);

} // namespace btcaddr
