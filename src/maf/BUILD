load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "maf_lib",
    srcs = ["maf.cc"],
    hdrs = ["maf.h"],
    deps = ["//src/fit2tcx:workout_data"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "maf_test",
    srcs = ["maf_test.cc"],
    deps = [":maf_lib"],
)
