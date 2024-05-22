load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

exports_files(["LICENSE"])

cc_library(
    name = "robots",
    srcs = [
        "robots.cc",
    ],
    hdrs = [
        "robots.h",
    ],
    deps = [
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/container:fixed_array",
        "@abseil-cpp//absl/strings",
    ],
)

cc_test(
    name = "robots_test",
    srcs = ["robots_test.cc"],
    deps = [
        ":robots",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "robots_main",
    srcs = ["robots_main.cc"],
    deps = [
        ":robots",
    ],
)
