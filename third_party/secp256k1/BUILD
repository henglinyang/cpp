load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "secp256k1",
    srcs = [
        "src/secp256k1.c",
        "src/precomputed_ecmult.c",
        "src/precomputed_ecmult_gen.c",
    ] + glob(["src/**/*.h"]),
    hdrs = glob(["include/*.h"]),
    includes = ["include", "src"],
    copts = [
        "-DENABLE_MODULE_EXTRAKEYS",
        "-DENABLE_MODULE_SCHNORRSIG",
        "-DENABLE_MODULE_RECOVERY",
        "-DSECP256K1_STATIC",
    ],
    visibility = ["//visibility:public"],
)
