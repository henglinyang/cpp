load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

cc_library(
    name = "first_lib",
    srcs = ["tables.cc", "plan.cc"],
    hdrs = ["tables.h", "plan.h"],
    copts = ["-std=c++20"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "first_tcx_lib",
    srcs = ["tcx_export.cc"],
    hdrs = ["tcx_export.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":first_lib",
        "//src/fit2tcx:fit2tcx_lib",
        "//src/runplan/maf:maf_lib",
    ],
)

cc_binary(
    name = "first",
    srcs = ["main.cc"],
    copts = ["-std=c++20"],
    deps = [
        ":first_lib",
        ":first_tcx_lib",
    ],
)

cc_test(
    name = "plan_test",
    srcs = ["plan_test.cc"],
    copts = ["-std=c++20"],
    deps = [
        ":first_lib",
        ":first_tcx_lib",
    ],
)
