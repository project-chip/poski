# POSKI — Portable Operating System Kernel Interface

[![CI](https://github.com/project-chip/poski/actions/workflows/ci.yml/badge.svg)](https://github.com/project-chip/poski/actions/workflows/ci.yml)

POSKI (Portable Operating System Kernel Interface) is a lightweight Operating
System Abstraction Layer (OSAL) that began as the CHIP OSAL in Project CHIP
(now Matter). POSKI acts as a "POSIX for embedded," providing a thin C and C++
abstraction layer that ensures seamless application portability across RTOS
and host targets.

By bridging kernels like FreeRTOS, Zephyr, RT-Thread, and POSIX (Linux/macOS),
POSKI empowers a single codebase to traverse the entire development lifecycle:
host-based simulation, bring-up testing, and the final production environment.
This ensures long-term portability, future-proofing applications against RTOS
shifts while ending platform fragmentation.

The name POSKI (and the `pos_` / `poski::` namespaces) disambiguates this OSAL
from external OSAL layers used by other projects or vendor SDKs.

## Introduction

POSKI is designed to provide a thin adaptation layer for portability of
embedded applications and device layers across a range of Real-Time Operating
Systems (RTOS) and host platforms. The intent is to leverage native OS
primitives as much as possible while providing a unified C and C++ interface
surface for those primitives, and to deliver common OS functionality suitable
for deeply embedded environments.

POSKI provides abstractions for:

| Module                     | C API                     | C++ API                | Summary                                     |
| :------------------------- | :------------------------ | :--------------------- | :------------------------------------------ |
| [Task](#task)              | `<poski/osal/os_task.h>`  | `poski::OsTask`        | Independent threads of execution            |
| [Mutex](#mutex)            | `<poski/osal/os_mutex.h>` | `poski::OsMutex`       | Recursive mutual exclusion                  |
| [Semaphore](#semaphore)    | `<poski/osal/os_sem.h>`   | `poski::OsSemaphore`   | Counting semaphores                         |
| [Critical section](#critical-section) | `<poski/osal/os_crit.h>` | `poski::OsCriticalSection` | Interrupt-masking mutual exclusion |
| [Queue](#queue)            | `<poski/osal/os_queue.h>` | `poski::OsQueue<T, N>` | Fixed-size message queues (copy semantics)  |
| [Event queue](#event-queue) | `<poski/osal/os_event.h>`, `<poski/osal/os_eventq.h>`, `<poski/osal/os_event_timer.h>` | `poski::OsEvent`, `OsEventQueue`, `OsEventTimer` | Allocation-free intrusive events, queues, and timers |
| [Timer](#timer)            | `<poski/osal/os_timer.h>` | `poski::OsTimer`       | One-shot software timers                    |
| [Time](#time)              | `<poski/osal/os_time.h>`  | `poski::OsTime`        | System time and tick/millisecond conversion |
| [Scheduler](#scheduler)    | `<poski/osal/os_sched.h>` | `poski::OsTask`        | Start and query the scheduler               |
| [Panic](#panic)            | `<poski/osal/os_panic.h>` | —                      | Fatal error handler                         |
| [Ring buffer](#ring-buffer) | —                        | `poski::OsRing`        | Ring buffer of fixed-size items             |

`<poski/osal/osal.h>` includes every C module. Each C++ class `poski::OsX` is
defined in its own header-only `<poski/OsX.h>`.

### Supported targets

| Target    | Port                | Native primitives                                                         | Build                                                            | CI                         | Status                                                    |
| :-------- | :------------------ | :------------------------------------------------------------------------ | :--------------------------------------------------------------- | :------------------------- | :-------------------------------------------------------- |
| Linux     | `targets/posix`     | pthreads, `sem_t`, `timer_create`, `clock_gettime`                        | Make; Bazel `:osal_posix`                                        | Build and all tests        | Supported; reference port                                 |
| macOS     | `targets/posix`     | pthreads, `dispatch_semaphore`, `dispatch_source`, Mach `clock_get_time` | Make; Bazel `:osal_posix`                                        | —                          | Supported; not CI-tested                                  |
| FreeRTOS  | `targets/freertos`  | Tasks, recursive mutexes, counting semaphores, queues, software timers    | Bazel `:osal_freertos` (compile check); Make `PLATFORM=nrf52840` | Bazel compile check        | Supported; no `pos_queue_set_signal_cb`                   |
| Zephyr    | `targets/zephyr`    | `k_thread`, `k_mutex`, `k_sem`, `k_msgq`, `k_timer`, `k_uptime_get`       | Bazel `:osal_zephyr` (`manual`; needs Zephyr headers)            | —                          | Experimental; no `pos_sched_*`                            |
| RT-Thread | `targets/rt-thread` | `rt_thread`, `rt_mutex`, `rt_sem`, `rt_mq`, `rt_timer`, `rt_tick_get`     | None in-tree                                                     | —                          | Partial; some timer, queue, and scheduler functions missing |

### Design principles

-   **Thin, native mapping.** Each POSKI object wraps the closest native
    primitive (for example, `pos_sem` is a FreeRTOS counting semaphore, a
    Zephyr `k_sem`, or a POSIX `sem_t`). POSKI adds no scheduler, timer
    service, or IPC mechanism of its own.
-   **Caller-owned objects.** Every object is a `struct pos_*` that the caller
    allocates (statically, on a stack, or embedded in another struct) and
    passes by pointer; the API never hands out heap-allocated handles. Where a
    native API allocates internally (FreeRTOS `x*Create()`, RT-Thread
    `rt_*_create()`, and the POSIX and Zephyr queue buffers), the port
    inherits that behavior.
-   **One predictable namespace.** C functions are named
    `pos_<module>_<verb>()`, constants are `POS_*`, most calls return a
    `pos_error_t`, and C++ classes are `poski::Os*`.
-   **Lightweight C++.** The C++ wrappers are header-only RAII classes that
    hold the C object by value (plus small bookkeeping such as an init flag or
    lock depth) and expose it through `GetNative()`. They need no virtual
    functions, exceptions, or RTTI, and only `OsRing` allocates (its buffer,
    with `new[]`).
-   **Host-first testing.** The same portable tests in `tests/` run on Linux
    and macOS through Make or Bazel, as plain executables or as GoogleTest
    cases, before the code reaches a target.

### Motivation

POSKI grew out of the goals of Project CHIP: a unifying, interoperable,
versatile, low-overhead, and robust connected-home solution with an explicit
focus on time-to-market. These goals require the platform layer design to be
highly scalable, allowing disparate and diverse platforms to be integrated with
high velocity. Supporting rapid integration of new platforms in a scalable and
maintainable way requires:

-   Maximum reuse of code, verification, and host-side unit testing
-   Minimum code fragmentation, forking, and conditional compilation
-   Adaptable and thin pathway to optimized native RTOS APIs

Device platforms tend to be highly unique on the first order, but also pivot on
three major axes of common functionality. These axes define a
three-dimensional matrix of possible device configurations, where a shared
point on any one axis allows code reuse across otherwise disparate platforms:

-   **Device (board)**
    -   Platforms that share a specific choice of chip combinations and wiring
        at the PCB level can share a common device layer port (for example, a
        Matter `DeviceLayer` port).
    -   The device layer provides the minimum interface required to connect
        the application stack to all the hardware-specific details of a
        device, such as BLE, Wi-Fi, and storage.
    -   Target examples are silicon vendor development boards or final product
        PCBs.
    -   A device layer is able to own all decisions about a device and impose
        hard assumptions on the particular combination of board + OS + HW.
    -   A device layer is also free to use an abstraction of the underlying OS
        or HW layers, such as POSKI, to provide better portability between RTOS
        environments or across a family of SoCs.

-   **Operating system (OS)**
    -   Platforms that share a common OS or RTOS can share a common POSKI port
        (`targets/freertos`, `targets/zephyr`, `targets/rt-thread`,
        `targets/posix`).
    -   Target examples are FreeRTOS, Zephyr, RT-Thread, Linux, and macOS.
    -   A given app or device layer codebase may need to be retargeted from one
        OS to another. This could happen during an upgrade cycle, for
        instance, or when a new product wants to use an existing device layer
        port but on the RTOS it typically uses.
    -   An application or driver stack written against POSKI runs unchanged in
        Linux/macOS host unit tests and on target RTOS firmware.

-   **Hardware (HW / SoC)**
    -   Platforms that share a common chipset, SoC, or silicon can share a
        common Hardware Abstraction Layer (HAL) from the vendor SDK.
    -   By leveraging a HAL, the same application can be retargeted to
        different chipsets across a family of similar SoCs.

The primary motivation of POSKI is to enable code sharing and reuse in
applications and device layers. Rather than having a separate example app for
each combination of HW + OS + board, POSKI allows an app to be written in a
common way and be retargeted to a different OS/RTOS such as FreeRTOS, Zephyr,
or Linux. POSKI is intended to help keep the system scalable as the matrix of
HW + OS + app combinations grows over time.

### Context

There is a long history of OSAL layers. Why does POSKI exist?

While it is true that POSKI is "Yet Another OSAL," it was designed to meet the
specific requirements of Project CHIP. Other OSAL projects were considered,
some with contributors in common with POSKI, but each had gaps relative to
those requirements:

-   [nler](https://github.com/nestlabs/nler) — Nest Labs Embedded Runtime
    -   Uses event queues with event-pointer semantics, whereas POSKI's core
        IPC primitive is the message queue with copy semantics, which maps
        directly onto native RTOS queues.
    -   Has an inconsistent API namespace, whereas all POSKI C functions
        predictably begin with `pos_` (originally `chip_os_`).
    -   Was designed to enforce a particular embedded programming philosophy;
        semaphores, for example, are notably missing.
    -   Imposes its own centralized timer system rather than providing a thin
        pass-through to native OS timers. That timer system relies on nler
        event queues, which are antithetical to the message-queue paradigm.

-   [npl](https://github.com/apache/mynewt-nimble/tree/master/porting/npl) —
    Apache Mynewt NimBLE Porting Layer
    -   Uses event queues with event-pointer semantics, whereas POSKI's core
        IPC primitive is the message queue with copy semantics.
    -   Is embedded within a larger BLE stack project and as such isn't easily
        composable as a submodule.
    -   Uses a consistent but domain-specific API namespace: `ble_npl_`.

---

## Quick Start

### Prerequisites

-   **Make builds:** GNU Make, GCC (Linux) or Clang (macOS), and `ccache`,
    which the Makefiles use to invoke the compiler. `make gtest` also needs
    `curl` to download GoogleTest.
-   **Bazel builds:** [Bazel](https://bazel.build/install) with Bzlmod
    ([Bazelisk](https://github.com/bazelbuild/bazelisk) is recommended) and
    Clang, which `.bazelrc` selects. Bazel fetches GoogleTest and the FreeRTOS
    kernel automatically.

#### Linux (Debian/Ubuntu)

```bash
sudo apt install build-essential ccache clang curl
# Then install Bazel, for example with Bazelisk.
```

#### macOS

```bash
xcode-select --install   # Clang and make
brew install ccache bazelisk
```

### Build and test with Make

```bash
make          # POSIX library: tests/libosal.a
make test     # build and run the C and C++ tests
make gtest    # download GoogleTest v1.14.0 into tests/, then build and run the gtest suites
make clean    # remove build outputs, including the GoogleTest download
```

`make -C tests all` builds the library and the test executables without running
them.

To build the **FreeRTOS** port for the Nordic nRF52840, using the FreeRTOS
kernel and configuration bundled with the nRF5 SDK (override them with
`FREERTOS_DIR` and `FREERTOS_CONFIG_DIR`) and the `arm-none-eabi-` toolchain:

```bash
# Export the path to the Nordic nRF5 SDK
export NRF5_SDK_ROOT=/path/to/nRF5_SDK_17.1.0_ddde560

PLATFORM=nrf52840 make               # tests/libosal_freertos.a
PLATFORM=nrf52840 make -C tests all  # plus test firmware images (tests/*.hex)
```

### Build and test with Bazel

```bash
bazel build //...          # POSIX and FreeRTOS libraries and all test binaries
bazel test //...           # run every test
bazel test //:test         # C and C++ tests only
bazel test //:gtest        # GoogleTest suites only
bazel build //:test //:gtest  # build the test binaries without running them
```

To build a single library:

```bash
bazel build //:osal        # POSIX (alias of //:osal_posix)
bazel build //:freertos    # FreeRTOS (alias of //:osal_freertos)
```

-   `//:osal_freertos` compiles the port against the FreeRTOS-Kernel V10.4.3
    headers using the POSIX simulator configuration in
    `targets/freertos/config/posix/`. It checks that the port builds; it does
    not link a kernel.
-   `//:osal_zephyr` (alias `//:zephyr`) is tagged `manual`, so wildcard
    patterns such as `//...` skip it; building it requires the Zephyr headers
    and toolchain.
-   `.bazelrc` disables the `bazel-*` convenience symlinks. Use
    `bazel info bazel-bin` or `bazel info bazel-testlogs` to find outputs.

### Continuous integration

[GitHub Actions](.github/workflows/ci.yml) runs `make`, `make test`,
`make gtest`, `bazel build //:osal`, `bazel test //:test //:gtest`, and
`bazel build //:freertos` on Ubuntu for pushes and pull requests to `main`.
Run the same checks locally before sending a pull request (see
[CONTRIBUTING.md](CONTRIBUTING.md)).

### Using POSKI in your project

With **Bazel** (Bzlmod), depend on the `osal` module:

```starlark
# MODULE.bazel
bazel_dep(name = "osal")
git_override(
    module_name = "osal",
    remote = "https://github.com/project-chip/poski.git",
    commit = "<commit>",
)
```

```starlark
# BUILD
cc_binary(
    name = "app",
    srcs = ["app.c"],
    deps = ["@osal//:osal_posix"],
)
```

Refer to the libraries by their full names (`osal_posix`, `osal_freertos`,
`osal_zephyr`) from other repositories; the short aliases (`//:osal`,
`//:freertos`, `//:zephyr`) are only visible within this repository.

With **other build systems**, add `include/` and `targets/<port>/` to the
include path, compile `targets/<port>/*.c` (plus `*.cc` for POSIX), and link
against your RTOS kernel, or against `-lpthread -lrt -lstdc++` on Linux.

### Example

A minimal producer and consumer, in C:

```c
#include <poski/osal/osal.h>

struct sensor_msg
{
    uint16_t id;
    int32_t value;
};

static struct pos_queue s_msgq; /* caller-owned: static, on a stack, or embedded */
static struct pos_task s_worker;

static void * worker(void * arg)
{
    struct sensor_msg msg;
    (void) arg;

    while (pos_queue_get(&s_msgq, &msg, POS_TIME_FOREVER) == POS_OK)
    {
        /* Handle msg.id / msg.value in task context. */
    }
    return NULL;
}

int main(void)
{
    struct sensor_msg msg = { .id = 1, .value = 42 };

    pos_queue_init(&s_msgq, sizeof(struct sensor_msg), 8);
    pos_task_init(&s_worker, "worker", worker, NULL, POS_PRIORITY_APP, 2048);

    pos_queue_put(&s_msgq, &msg); /* copied into the queue */

    pos_sched_start(); /* does not return */
    return 0;
}
```

And with the C++ wrappers:

```cpp
#include <poski/OsQueue.h>
#include <poski/OsTask.h>

struct SensorMsg {
    uint16_t id;
    int32_t value;
};

static poski::OsQueue<SensorMsg, 8> sMsgQueue; // RAII: init in ctor, deinit in dtor
static poski::OsTask sWorker;

static void * Worker(void *)
{
    SensorMsg msg;
    while (sMsgQueue.Pop(msg) == POS_OK) {
        // Handle msg in task context.
    }
    return nullptr;
}

int main()
{
    sWorker.Start("worker", Worker, nullptr, POS_PRIORITY_APP, 2048);
    sMsgQueue.Push({ 1, 42 }); // copied into the queue
    poski::OsTask::SchedStart(); // does not return
}
```

---

## Reference

Conventions shared by all modules:

-   Most functions return a `pos_error_t`: `POS_OK` on success, or an error
    such as `POS_TIMEOUT`, `POS_EBUSY`, or `POS_EINVAL` (see
    [`os_types.h`](include/poski/osal/os_types.h)).
-   Blocking calls take a `pos_time_t timeout`, which may also be
    `POS_TIME_NO_WAIT` or `POS_TIME_FOREVER`. The headers specify milliseconds,
    but the FreeRTOS and RT-Thread ports currently pass the value through as OS
    ticks, which is equivalent only at a 1 kHz tick rate.
-   Calls documented as ISR-safe may be made from interrupt handlers; blocking
    calls made from an ISR must use `POS_TIME_NO_WAIT`.

### Task

`<poski/osal/os_task.h>` · `struct pos_task` · `poski::OsTask`

A task (also known as a thread) is an independent context of code execution
that runs without any dependency on other concurrent tasks within the system.
Only one task runs on a core at any given time. The scheduler starts and stops
tasks as necessary to manage resources according to the priorities and
policies of the system. A task has no knowledge of the underlying scheduler
activity and can be swapped in and out, but always runs with a consistent
execution context and stack.

`pos_task_init()` creates a task from an entry function, an argument, a
priority (`POS_PRIORITY_MIN` to `POS_PRIORITY_MAX`; `POS_PRIORITY_APP` is a
reasonable default), and a stack size in bytes, and makes it ready to run. The
running task can call `pos_task_yield()`, `pos_task_sleep()` (ticks), or
`pos_task_sleep_ms()`, and `pos_get_current_task_id()` identifies it.
`pos_task_remove()` is experimental.

### Mutex

`<poski/osal/os_mutex.h>` · `struct pos_mutex` · `poski::OsMutex`

A mutex provides a locking mechanism for enforcing mutual exclusion and
protection of shared resources between independent tasks. POSKI mutexes are
recursive: the owning task may lock a mutex again and must unlock it as many
times as it locked it (`pos_mutex_init()`, `pos_mutex_lock()`,
`pos_mutex_unlock()`). To signal a task from an interrupt, use a semaphore
instead. `OsMutex` can lock on construction and releases any locks it holds in
its destructor.

### Semaphore

`<poski/osal/os_sem.h>` · `struct pos_sem` · `poski::OsSemaphore`

A counting semaphore is a synchronization primitive which provides a means to
block one task until it is released by a signal from another task or
interrupt. `pos_sem_init()` sets the initial token count, `pos_sem_take()`
waits for a token, and `pos_sem_give()` releases one. Both are ISR-safe.

### Critical Section

`<poski/osal/os_crit.h>` · `poski::OsCriticalSection` / `poski::OsAtomicGuard`

A critical section provides nestable, short-duration protection for atomic
code sequences by masking interrupts (`pos_crit_enter()` / `pos_crit_exit()`,
with `pos_atomic_enter()` / `pos_atomic_exit()` aliases). The native interrupt
state is saved in `pos_crit_state_t` (`uintptr_t`) and must be restored in
LIFO order.

`pos_crit_is_active()` reports whether the calling execution context is
inside a critical section, and `pos_crit_in_isr()` reports whether execution
is inside an Interrupt Service Routine.

### Queue

`<poski/osal/os_queue.h>` · `struct pos_queue` · `poski::OsQueue<T, N>`

A message queue is a basic primitive for intertask communication. It carries
fixed-size messages from a task or interrupt producer to a consumer task using
copy semantics: `pos_queue_put()` copies the message in and `pos_queue_get()`
copies it out, so producer and consumer never share a buffer. Queues are
created with `pos_queue_init()` (message size and capacity) and released with
`pos_queue_deinit()`. `pos_queue_is_empty()` and `pos_queue_inited()` query
the queue, and `pos_queue_set_signal_cb()` registers a callback that runs after
each put (for example, to wake an external event loop). `pos_queue_put()` and
`pos_queue_get()` are ISR-safe. `OsQueue<T, N>` provides type-safe `Push()`
and `Pop()`.

### Event Queue

`<poski/osal/os_event.h>` · `<poski/osal/os_eventq.h>` · `<poski/osal/os_event_timer.h>` · `poski::OsEvent` / `OsEventQueue` / `OsEventTimer`

Events, event queues, and event timers (modeled on Mynewt `os_event` /
`os_eventq` / `os_callout`) provide allocation-free deferred work. An event
(`struct pos_event`) is a caller-owned `{callback, argument}` record that
queues link intrusively, so posting never copies or allocates, and posting an
already-pending event is a no-op. Events can be statically initialized with
`POS_EVENT_INITIALIZER()`. A consumer task drains the queue
(`struct pos_eventq`) with `pos_eventq_get()` / `pos_eventq_run()`. An event
timer (`struct pos_event_timer`) posts its event to a queue when it expires, so
the callback runs in the consumer task rather than in an ISR or timer context.

The C++ classes `poski::OsEvent`, `OsEventQueue`, and `OsEventTimer`
(`<poski/OsEvent.h>`, `<poski/OsEventQueue.h>`, `<poski/OsEventTimer.h>`) are
pure wrappers of the C API that hold the C structs by value with no extra
storage, so a port implements only the C API. `OsEventIn` / `OsEventTimerIn`
dispatch directly to an owner's member function.

### Timer

`<poski/osal/os_timer.h>` · `struct pos_timer` · `poski::OsTimer`

A software timer triggers a callback function after a given amount of time has
passed. Timers are one-shot: `pos_timer_init()` binds the callback and its
argument, and `pos_timer_start()` (ticks) or `pos_timer_start_ms()` arms the
timer. `pos_timer_stop()` (ISR-safe), `pos_timer_is_active()`,
`pos_timer_get_ticks()`, `pos_timer_remaining_ticks()`, and
`pos_timer_arg_set()` / `pos_timer_arg_get()` manage and query it.

The callback runs in a port-specific context, such as the FreeRTOS timer
service task, a POSIX timer thread, or interrupt context on Zephyr and
RT-Thread, so keep it short and never block in it.

### Time

`<poski/osal/os_time.h>` · `poski::OsTime`

A collection of utility functions for getting the current system time and
converting between milliseconds and OS ticks: `pos_time_get()` (ticks),
`pos_time_get_ms()`, `pos_time_ms_to_ticks()`, and `pos_time_ticks_to_ms()`.
`POS_TICKS_PER_SEC` is the port's tick rate and `pos_time_t` its tick type;
for example, 32-bit 1 kHz ticks on Linux, 64-bit 1 MHz ticks on macOS, and
`TickType_t` on FreeRTOS.

### Scheduler

`<poski/osal/os_sched.h>` · `poski::OsTask::SchedStart()` / `SchedStarted()`

`pos_sched_start()` starts the underlying scheduler and does not return; on
POSIX it keeps the calling thread alive (on macOS, by running the main dispatch
queue). `pos_sched_started()` reports whether the scheduler is running.

### Panic

`<poski/osal/os_panic.h>`

`pos_panic(const char *msg)` is a `noreturn` fatal error handler. It prints the
message with a `POSKI PANIC:` prefix, then aborts (POSIX) or asserts and halts
(RTOS targets).

### Ring Buffer

`<poski/OsRing.h>` · `poski::OsRing`

`OsRing` is a portable ring buffer of fixed-size items (`push_back()`,
`front()`, `pop_front()`, `size()`, `empty()`, `full()`). The item count must
be a power of two, and the buffer is allocated with `new[]`. `OsRing` is not
thread-safe by itself; the POSIX port wraps it in `RingPthread` to implement
`pos_queue`.

---

## Porting Guide

POSKI separates its public interface headers cleanly from target-specific
implementations:

| File / Folder                         | Contents                                                           |
| :------------------------------------ | :----------------------------------------------------------------- |
| `include/poski/osal/osal.h`           | Umbrella C header that includes every POSKI module                 |
| `include/poski/osal/os_*.h`           | Public C API, one header per module, shared by all ports           |
| `include/poski/Os*.h`                 | Header-only C++ RAII wrappers (`namespace poski`)                  |
| `targets/<port>/poski/osal/os_port.h` | Maps POSKI types and constants to the target's native definitions  |
| `targets/<port>/os_*.c`               | Target implementation of the C API                                 |
| `tests/`                              | Portable C, C++, and GoogleTest suites for the POSKI APIs          |

### Directory Structure

```text
.
├── BUILD                     - Bazel libraries (osal_posix, osal_freertos, osal_zephyr) and tests
├── MODULE.bazel              - Bzlmod dependencies (rules_cc, googletest, FreeRTOS kernel)
├── Makefile                  - Convenience wrapper around tests/Makefile
├── include/poski
│   ├── OsCriticalSection.h   - C++ RAII guard for pos_crit / pos_atomic
│   ├── OsEvent.h             - C++ wrapper for pos_event and member dispatch
│   ├── OsEventQueue.h        - C++ wrapper for pos_eventq
│   ├── OsEventTimer.h        - C++ wrapper for pos_event_timer
│   ├── OsMutex.h             - C++ wrapper for pos_mutex
│   ├── OsQueue.h             - C++ template wrapper for pos_queue
│   ├── OsRing.h              - Portable ring buffer class
│   ├── OsSemaphore.h         - C++ wrapper for pos_sem
│   ├── OsTask.h              - C++ wrapper for pos_task and pos_sched
│   ├── OsTime.h              - C++ wrapper for pos_time
│   ├── OsTimer.h             - C++ wrapper for pos_timer
│   └── osal
│       ├── os_crit.h         - Critical section / interrupt masking C API
│       ├── os_event.h        - Event (pos_event) C API
│       ├── os_event_timer.h  - Event timer (pos_event_timer) C API
│       ├── os_eventq.h       - Event queue (pos_eventq) C API
│       ├── os_mutex.h        - Mutex C API
│       ├── os_panic.h        - Fatal error (panic) C API
│       ├── os_queue.h        - Message queue C API
│       ├── os_sched.h        - Scheduler control C API
│       ├── os_sem.h          - Semaphore C API
│       ├── os_task.h         - Task C API
│       ├── os_time.h         - System time and tick conversion C API
│       ├── os_timer.h        - Software timer C API
│       ├── os_types.h        - Common types and error codes (pos_error_t); includes os_port.h
│       └── osal.h            - Umbrella C header
├── patches/                  - GoogleTest patches applied by Bazel
├── targets
│   ├── freertos/             - FreeRTOS port (config/posix/ holds the Bazel compile-check config)
│   ├── posix/                - POSIX port (Linux and macOS)
│   ├── rt-thread/            - RT-Thread port
│   └── zephyr/               - Zephyr port
└── tests
    ├── Makefile              - Make build of the library and tests (PLATFORM=posix or nrf52840)
    ├── Makefile.freertos-nrf52840 - Settings for FreeRTOS on the Nordic nRF52840
    ├── test_os_*.c           - Portable tests of the C APIs
    ├── test_os_*_cpp.cpp     - Tests of the C++ wrappers
    ├── test_os_*_cpp_gtest.cpp - Native GoogleTest versions of the C++ tests
    ├── test_os_ring.cpp      - Tests of OsRing
    ├── test_gtest_wrapper.cpp - Runs a C test's main() as a GoogleTest case
    └── test_util.h           - Common test utilities (SuccessOrQuit, VerifyOrQuit, TEST_LOG)
```

### Adding a New Target

1.  Create `targets/<port>/poski/osal/os_port.h`, which `os_types.h` includes.
    Directly or through headers it includes, it must define:
    -   the tick type `pos_time_t` and the constants `POS_TIME_NO_WAIT`,
        `POS_TIME_FOREVER`, and `POS_TICKS_PER_SEC`;
    -   the priorities `POS_PRIORITY_MIN`, `POS_PRIORITY_MAX`, and
        `POS_PRIORITY_APP`;
    -   the object layouts `struct pos_task`, `struct pos_mutex`,
        `struct pos_sem`, `struct pos_queue`, and `struct pos_timer`, which
        typically embed or point to the native kernel object.
2.  Implement the C API declared in `include/poski/osal/os_*.h` in
    `targets/<port>/os_*.c`. Trivial functions may instead be `static inline`
    in `os_port.h`.
3.  Build with `include/` and `targets/<port>/` on the include path (see the
    `osal_*` libraries in `BUILD`), and run the portable tests from `tests/`
    on the target.
4.  Add CI coverage where practical; at minimum a compile check such as
    `//:osal_freertos`.

### POSIX Port

The POSIX port (`targets/posix`) includes both Linux and macOS implementations.
Linux uses the standard POSIX APIs for all functionality. macOS uses POSIX
pthreads, Grand Central Dispatch for the semaphore and timer implementations,
and a Mach clock for time.

| OS        | Task      | Mutex                     | Semaphore            | Timer                           | Time                                    | Queue         |
| :-------- | :-------- | :------------------------ | :------------------- | :------------------------------ | :-------------------------------------- | :------------ |
| **Linux** | `pthread` | recursive `pthread_mutex` | `sem_t`              | `timer_create` (`SIGEV_THREAD`) | `clock_gettime(CLOCK_MONOTONIC)`, 1 kHz | `RingPthread` |
| **macOS** | `pthread` | recursive `pthread_mutex` | `dispatch_semaphore` | `dispatch_source`               | Mach `clock_get_time`, 1 MHz            | `RingPthread` |

```text
targets/posix
├── poski/osal
│   ├── os_port.h             - Port header included by <poski/osal/os_types.h>
│   ├── os_time.h             - Time and timer types (pos_time_t, POS_TIME_*, struct pos_timer)
│   └── os_types.h            - Task, mutex, semaphore, and queue types; task priorities
├── os_crit.c                 - Implementation of pos_crit
├── os_event_timer.c          - Implementation of pos_event_timer
├── os_eventq.c               - Implementation of pos_eventq
├── os_mutex.c                - Implementation of pos_mutex
├── os_panic.c                - Implementation of pos_panic
├── os_queue.cc               - Implementation of pos_queue (C++, using RingPthread)
├── os_sem.c                  - Implementation of pos_sem
├── os_task.c                 - Implementation of pos_task and pos_sched
├── os_time.c                 - Implementation of pos_time
├── os_timer.c                - Implementation of pos_timer
├── os_utils.c                - Shared code for the POSIX port, most notably error mapping
├── os_utils.h                - Shared header for the POSIX port, most notably SuccessOrExit-style macros
└── RingPthread.h             - Thread-safe OsRing using a pthread mutex and condition variable
```

### Other Ports

[`targets/freertos/README.md`](targets/freertos/README.md) and
[`targets/zephyr/README.md`](targets/zephyr/README.md) describe those ports
and how to set up their toolchains. They were written for the original CHIP
OSAL, so they still use its `chip_os_` names and source paths.

---

## History

POSKI began as the CHIP OSAL inside Project CHIP. When it moved to its own
repository, the API was renamed:

| CHIP OSAL                                            | POSKI                                                     |
| :--------------------------------------------------- | :-------------------------------------------------------- |
| `#include <chip/osal.h>`                             | `#include <poski/osal/osal.h>` (or `<poski/osal/os_*.h>`) |
| `chip_os_*()`, `struct chip_os_*`, `chip_os_error_t` | `pos_*()`, `struct pos_*`, `pos_error_t`                  |
| `CHIP_OS_*` constants                                | `POS_*` constants                                         |
| `chip_os_mutex_take()` / `chip_os_mutex_give()`      | `pos_mutex_lock()` / `pos_mutex_unlock()`                 |
| `Ring` (`Ring.h`)                                    | `poski::OsRing` (`<poski/OsRing.h>`)                      |

## Contributing

Contributions are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for the
workflow, CLA, and required checks, and follow the
[Code of Conduct](CODE_OF_CONDUCT.md). Please report bugs and propose features
through [GitHub Issues](https://github.com/project-chip/poski/issues).

## License

POSKI is licensed under the [Apache License 2.0](LICENSE).
