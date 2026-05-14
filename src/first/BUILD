load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "first_lib",
    srcs = ["tables.cc", "plan.cc"],
    hdrs = ["tables.h", "plan.h"],
    copts = ["-std=c++20"],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "first",
    srcs = ["main.cc"],
    copts = ["-std=c++20"],
    deps = [":first_lib"],
)
