#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace btcutxo {

struct Utxo {
    // txid in display order (reversed from on-disk byte order), hex string
    std::string txid_hex;
    uint32_t    vout       = 0;
    int64_t     satoshis   = 0;
    uint32_t    height     = 0;
    bool        coinbase   = false;
    std::string address;   // empty if script type not recognized
};

struct ScanOptions {
    bool mainnet = true;
};

// Scan Bitcoin Core's chainstate LevelDB directory.
// For each UTXO found, calls `on_utxo`.
// Throws std::runtime_error on fatal errors (cannot open DB, etc.).
void scan_chainstate(const std::string& chainstate_dir,
                     const ScanOptions& opts,
                     const std::function<void(const Utxo&)>& on_utxo);

} // namespace btcutxo
