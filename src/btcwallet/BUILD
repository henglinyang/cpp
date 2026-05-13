load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

cc_library(
    name = "btcwallet_lib",
    srcs = ["keygen.cc", "crypto.cc"],
    hdrs = ["keygen.h", "crypto.h"],
    copts = ["-std=c++20"],
    deps = ["//third_party/secp256k1:secp256k1"],
)

cc_binary(
    name = "btcwallet",
    srcs = ["main.cc"],
    copts = ["-std=c++20"],
    deps = [":btcwallet_lib"],
)

cc_test(
    name = "btcwallet_test",
    srcs = ["keygen_test.cc"],
    copts = ["-std=c++20"],
    deps = [":btcwallet_lib"],
)
