load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "osal_posix",
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
    name = "osal_zephyr",
    srcs = glob([
        "targets/zephyr/*.c",
    ]),
    hdrs = glob([
        "include/**/*.h",
        "targets/zephyr/**/*.h",
    ]),
    includes = [
        "include",
        "targets/zephyr",
    ],
    visibility = ["//visibility:public"],
)

alias(
    name = "osal",
    actual = ":osal_posix",
)

alias(
    name = "zephyr",
    actual = ":osal_zephyr",
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

cc_test(
    name = "test_os_task_gtest",
    srcs = [
        "tests/test_gtest_wrapper.cpp",
        "tests/test_os_task.c",
    ],
    copts = [
        "-Dmain=test_os_task_main",
        "-DTEST_NAME=Task",
        "-DTEST_MAIN=test_os_task_main",
        "-DWRAP_C_MAIN",
    ],
    deps = [
        ":osal",
        ":test_util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_os_queue_gtest",
    srcs = [
        "tests/test_gtest_wrapper.cpp",
        "tests/test_os_queue.c",
    ],
    copts = [
        "-Dmain=test_os_queue_main",
        "-DTEST_NAME=Queue",
        "-DTEST_MAIN=test_os_queue_main",
        "-DWRAP_C_MAIN",
    ],
    deps = [
        ":osal",
        ":test_util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_os_mutex_gtest",
    srcs = [
        "tests/test_gtest_wrapper.cpp",
        "tests/test_os_mutex.c",
    ],
    copts = [
        "-Dmain=test_os_mutex_main",
        "-DTEST_NAME=Mutex",
        "-DTEST_MAIN=test_os_mutex_main",
        "-DWRAP_C_MAIN",
    ],
    deps = [
        ":osal",
        ":test_util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_os_sem_gtest",
    srcs = [
        "tests/test_gtest_wrapper.cpp",
        "tests/test_os_sem.c",
    ],
    copts = [
        "-Dmain=test_os_sem_main",
        "-DTEST_NAME=Sem",
        "-DTEST_MAIN=test_os_sem_main",
        "-DWRAP_C_MAIN",
    ],
    deps = [
        ":osal",
        ":test_util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_os_timer_gtest",
    srcs = [
        "tests/test_gtest_wrapper.cpp",
        "tests/test_os_timer.c",
    ],
    copts = [
        "-Dmain=test_os_timer_main",
        "-DTEST_NAME=Timer",
        "-DTEST_MAIN=test_os_timer_main",
        "-DWRAP_C_MAIN",
    ],
    deps = [
        ":osal",
        ":test_util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_ring_gtest",
    srcs = [
        "tests/test_gtest_wrapper.cpp",
        "tests/test_ring.cpp",
    ],
    copts = [
        "-Dmain=test_ring_main",
        "-DTEST_NAME=Ring",
        "-DTEST_MAIN=test_ring_main",
    ],
    deps = [
        ":osal",
        ":test_util",
        "@com_google_googletest//:gtest_main",
    ],
)

test_suite(
    name = "test",
    tests = [
        ":test_os_task",
        ":test_os_queue",
        ":test_os_mutex",
        ":test_os_sem",
        ":test_os_timer",
        ":test_ring",
    ],
)

test_suite(
    name = "gtest",
    tests = [
        ":test_os_task_gtest",
        ":test_os_queue_gtest",
        ":test_os_mutex_gtest",
        ":test_os_sem_gtest",
        ":test_os_timer_gtest",
        ":test_ring_gtest",
    ],
)


