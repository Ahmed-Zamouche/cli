load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_binary")


cc_library(
    name = "uart",
    srcs = ["uart.c"],
    hdrs = ["uart.h"],
    linkopts=["-lpthread"],
    visibility = ["//visibility:private"],
)

cc_library(
    name = "cmd_list",
    srcs = ["cmd_list.c"],
    hdrs = ["cmd_list.h"],
    deps = ["@cli//lib:cli", ":uart"],
    visibility = ["//visibility:private"],
)

cc_binary(
  name = "cli_example",
    srcs = ["main.c"],
    deps = [":cmd_list", ":uart"],
    visibility = ["//visibility:public"],
)

