load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "btcutxo_lib",
    srcs = ["chainstate.cc"],
    hdrs = ["chainstate.h"],
    copts = ["-std=c++20"],
    deps = [
        "//src/btcwallet:btcwallet_lib",
        "//third_party/leveldb:leveldb",
    ],
)

cc_binary(
    name = "btcutxo",
    srcs = ["main.cc"],
    copts = ["-std=c++20"],
    deps = [":btcutxo_lib"],
)

load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "btcutxo_test",
    srcs = ["chainstate_test.cc"],
    copts = ["-std=c++20"],
    deps = [
        ":btcutxo_lib",
        "//third_party/leveldb:leveldb",
    ],
)
