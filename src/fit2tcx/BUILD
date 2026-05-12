load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "fit2tcx",
    srcs = ["fit2tcx.cc"],
    deps = [
        "//third_party/garmin/fit-sdk:fit-sdk",
        "//third_party/pugixml-1.15:pugixml",
    ],
)
