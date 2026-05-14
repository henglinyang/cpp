load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "hanson_lib",
    srcs = ["tables.cc", "plan.cc"],
    hdrs = ["tables.h", "plan.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "hanson_tcx_lib",
    srcs = ["tcx_export.cc"],
    hdrs = ["tcx_export.h"],
    deps = [
        ":hanson_lib",
        "//src/fit2tcx:fit2tcx_lib",
    ],
)

cc_binary(
    name = "hanson",
    srcs = ["main.cc"],
    deps = [
        ":hanson_lib",
        ":hanson_tcx_lib",
    ],
)
