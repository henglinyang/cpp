# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build system

All targets use [Bazel](https://bazel.build/) via the `bazelisk` wrapper.

```bash
# Build a single target
bazelisk build //src/hanson:hanson

# Build everything
bazelisk build //...

# Run a single test binary
bazelisk test //src/hanson:plan_test --test_output=all

# Run all tests
bazelisk test //... --test_output=all

# Run a compiled binary
./bazel-bin/src/hanson/hanson --goal 3:30:00 --json /tmp/json --age 40
```

No `cmake`, `make`, or compiler flags need to be set manually. `MODULE.bazel` defines the dependency graph; third-party libraries live under `third_party/`.

## Projects

### `src/fit2tcx` — FIT → TCX converter
Converts Garmin `.fit` activity files to TCX v2 XML or Garmin Connect JSON workout format. The core is `fit2tcx_lib.cc` which parses the Garmin FIT SDK messages and writes `WorkoutData` / `WorkoutStepData` structs to XML (via pugixml) or JSON.

Key functions in `fit2tcx_lib.cc`:
- `writeWorkoutTcx(WorkoutData, ostream)` — emits TCX v2 XML
- `writeWorkoutJson(WorkoutData, ostream)` — emits Garmin Connect JSON (used by hanson/first for JSON export)
- `writeExecStep()` — maps FIT target types to JSON/TCX target fields; `workoutTargetTypeId=4` = heart rate zone, `=6` = pace zone, `=0` = no target

### `src/hanson` — Hansons Marathon Method plan generator
Generates an 18-week marathon training plan (beginner or advanced). SOS (Something of Substance) workouts have structured `PlanStep` arrays. Export to TCX or Garmin Connect JSON via `tcx_export.cc`.

CLI: `--program <beginner|advanced> --goal <H:MM:SS> --tcx <dir> --json <dir> --age <N>`

Plan structure: `TrainingPlan → Week[] → Day[] → PlanStep[]`. Days 0–6 = Mon–Sun. Steps have `kind` (RUN/RECOVER/WARMUP/COOLDOWN), `duration_val` (meters or seconds), `dist_based` flag, `speed_low_mms`/`speed_high_mms` (mm/s), `hr_high` (BPM for MAF steps).

### `src/first` — FIRST (Furman Institute) plan generator
Generates a 16-week training plan for 5k/10k/half/marathon with three Key Runs (KR1/KR2/KR3) per week. Same `PlanStep` pattern as hanson. Weeks count down (week 16 first, week 1 = race week).

CLI: `--distance <5k|10k|half|marathon> --goal <time> --tcx <dir> --json <dir> --age <N>`

### `src/maf` — MAF warmup/cooldown library
Shared library used by both `hanson_tcx_lib` and `first_tcx_lib`. Provides `push_maf_warmup()` and `push_maf_cooldown()`, each appending 5 equal-duration `WorkoutStepData` steps with progressively increasing (warmup) or decreasing (cooldown) HR ranges. The range walks between a fixed floor of 90 BPM and `maf_hr = 180 - age`. Depends on `//src/fit2tcx:workout_data` only — not on `fit2tcx_lib`, so it does not pull in the FIT SDK or pugixml.

### `src/btcwallet` — Bitcoin key pair generator
Generates ECDSA/Schnorr key pairs with optional BIP32 HD derivation. Uses vendored `third_party/secp256k1`. Crypto helpers (SHA256, RIPEMD160, HMAC-SHA512, Base58Check, Bech32/Bech32m) are implemented inline in `crypto.cc`.

CLI: `--type <ecdsa-compressed|ecdsa-uncompressed|schnorr> --entropy <hex> --hd-seed <hex> --path <m/...> --format <hex|wif|address|all> --testnet`

### `src/btcaddr` — Bitcoin address scanner
Scans a set of Bitcoin addresses (e.g. from a file dump) against a query set. Depends on `btcwallet_lib` for address decoding.

### `src/btcutxo` — Bitcoin UTXO scanner
Reads Bitcoin Core's LevelDB chainstate directory and enumerates all UTXOs. Depends on `btcwallet_lib` for address decoding and `third_party/leveldb`.

## Test harness pattern

All test files use a self-contained harness (no external test framework). Pattern:

```cpp
#define TEST(suite, name) \
    static void test_##suite##_##name(); \
    static RegisterTest reg_##suite##_##name(#suite "." #name, test_##suite##_##name); \
    static void test_##suite##_##name()
```

`EXPECT_*` macros throw `std::runtime_error` on failure. `main()` iterates `tests()` and prints `[ OK ]` / `[FAIL]`. Same pattern is used in `fit2tcx_test.cc`, `plan_test.cc` (both hanson and first), `keygen_test.cc`, `parser_test.cc`, and `chainstate_test.cc`.

## Third-party dependencies

| Path | What |
|------|------|
| `third_party/secp256k1/` | Vendored from `repos/bitcoin/src/secp256k1/`; unity build via `secp256k1.c` |
| `third_party/pugixml-1.15/` | XML parsing/writing for TCX output |
| `third_party/garmin/fit-sdk/` | Garmin FIT SDK (C++) for decoding `.fit` files |
| `third_party/leveldb/` | LevelDB for reading Bitcoin Core chainstate |
| `bazel/libxml2/` | libxml2 local override module |

## MAF heart rate
`maf_hr = 180 - age`. Warmup (15 min) and cooldown (10 min) are each split into 5 equal steps via `src/maf`. HR ranges walk from `[90, 90+delta]` up to `[maf_hr-delta, maf_hr]` for warmup, and the reverse for cooldown, where `delta = (maf_hr - 90) / 5`. Both `targetValueOne` (high) and `targetValueTwo` (low) are emitted in JSON (`workoutTargetTypeId=4`, `heart.rate.zone`); both `<Low>` and `<High>` are written in TCX.
