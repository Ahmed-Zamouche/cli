load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")
#CLANGD_FLAGS=--compile-commands-dir=$(pwd)
refresh_compile_commands(
    name = "refresh_compile_commands",
    targets = {
      "//lib:cli": "",
      "//lib:utils": "",
      "//lib:test_cmd_list": "",

      "//example:cli_example": "",
      "//example:cmd_list": "",
      "//example:uart": "",
    },
)
