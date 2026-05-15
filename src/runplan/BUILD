load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "runplan",
    srcs = ["main.cc"],
    deps = [
        "//src/fit2tcx:fit2tcx_lib",
        "//src/first:first_tcx_lib",
        "//src/hanson:hanson_tcx_lib",
        "//src/maf:maf_lib",
    ],
)
