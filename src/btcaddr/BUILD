load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

cc_library(
    name = "btcaddr_lib",
    srcs = ["parser.cc"],
    hdrs = ["parser.h"],
    copts = ["-std=c++20"],
    deps = ["//src/btcwallet:btcwallet_lib"],
)

cc_binary(
    name = "btcaddr",
    srcs = ["main.cc"],
    copts = ["-std=c++20"],
    deps = [":btcaddr_lib"],
)

cc_test(
    name = "btcaddr_test",
    srcs = ["parser_test.cc"],
    copts = ["-std=c++20"],
    deps = [":btcaddr_lib"],
)
