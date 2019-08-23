package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

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
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "robots_test",
    srcs = ["robots_test.cc"],
    deps = [
        ":robots",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_binary(
    name = "robots_main",
    srcs = ["robots_main.cc"],
    deps = [
        ":robots",
    ],
)
