cc_library(
    name = "osal",
    srcs = glob([
        "targets/posix/*.c",
        "targets/posix/*.cc",
    ]),
    hdrs = glob([
        "include/**/*.h",
        "targets/posix/**/*.h",
    ]),
    includes = [
        "include",
        "targets/posix",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "test_util",
    hdrs = ["tests/test_util.h"],
    deps = [":osal"],
)

cc_test(
    name = "test_os_task",
    srcs = ["tests/test_os_task.c"],
    deps = [
        ":osal",
        ":test_util",
    ],
)

cc_test(
    name = "test_os_queue",
    srcs = ["tests/test_os_queue.c"],
    deps = [
        ":osal",
        ":test_util",
    ],
)

cc_test(
    name = "test_os_mutex",
    srcs = ["tests/test_os_mutex.c"],
    deps = [
        ":osal",
        ":test_util",
    ],
)

cc_test(
    name = "test_os_sem",
    srcs = ["tests/test_os_sem.c"],
    deps = [
        ":osal",
        ":test_util",
    ],
)

cc_test(
    name = "test_os_timer",
    srcs = ["tests/test_os_timer.c"],
    deps = [
        ":osal",
        ":test_util",
    ],
)

cc_test(
    name = "test_ring",
    srcs = ["tests/test_ring.cpp"],
    deps = [
        ":osal",
        ":test_util",
    ],
)
