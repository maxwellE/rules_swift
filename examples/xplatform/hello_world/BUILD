load("//swift:swift.bzl", "swift_binary")

licenses(["notice"])

swift_binary(
    name = "hello_world",
    srcs = ["main.swift"],
    env = {
        "HELLO_WORLD_ENV_VAR": "TRUE"
    }
)
