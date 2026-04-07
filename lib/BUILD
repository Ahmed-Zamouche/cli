load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_test")

cc_library(
    name = "utils",
    srcs = ["ringbuffer.c"],
    hdrs = ["ringbuffer.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "cli",
    srcs = ["cli.c"],
    hdrs = ["cli.h"],
    deps = ["utils"],
    visibility = ["//visibility:public"],
)

cc_test(
  name = "test_cmd_list",
  size = "small",
  srcs = ["test_cmd_list.cc"],
  deps = ["@googletest//:gtest_main", ":cli"]
)

cc_test(
  name = "test_ringbuffer",
  size = "small",
  srcs = ["test_ringbuffer.cc"],
  deps = ["@googletest//:gtest_main", ":utils"]
)
