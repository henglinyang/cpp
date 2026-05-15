load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "maf_lib",
    srcs = ["maf.cc"],
    hdrs = ["maf.h"],
    deps = ["//src/fit2tcx:workout_data"],
    visibility = ["//visibility:public"],
)
