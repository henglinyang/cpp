load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

cc_library(
    name = "workout_data",
    hdrs = ["workout_data.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "fit2tcx_lib",
    srcs = ["fit2tcx_lib.cc"],
    hdrs = ["fit2tcx.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":workout_data",
        "//third_party/garmin/fit-sdk:fit-sdk",
        "//third_party/pugixml-1.15:pugixml",
    ],
)

cc_binary(
    name = "fit2tcx",
    srcs = ["fit2tcx.cc"],
    deps = [":fit2tcx_lib"],
)

cc_test(
    name = "fit2tcx_test",
    srcs = ["fit2tcx_test.cc"],
    deps = [
        ":fit2tcx_lib",
        "//third_party/pugixml-1.15:pugixml",
    ],
)
