# POSKI — Portable Operating System Kernel Interface (OSAL)

POSKI (Portable Operating System Kernel Interface) is a lightweight Operating System Abstraction Layer (OSAL) originally developed within Project CHIP.
POSKI acts as a "POSIX for embedded," providing a thin C and C++ abstraction layer that ensures seamless application portability across different RTOS and host targets.

By bridging kernels like FreeRTOS, Zephyr, RT-Thread, and POSIX (Linux/macOS), POSKI empowers a single codebase to traverse the entire development lifecycle: host-based simulation, bring-up testing, and the final production environment.
This ensures long-term portability, future-proofing applications against RTOS shifts while ending platform fragmentation.

The name POSKI (and the `pos_` / `poski::` namespace) disambiguates this OSAL from external OSAL layers used by other projects or vendor SDKs.

## Introduction

POSKI is designed to provide a thin adaptation layer for portability of embedded applications and device layers across a wide range of Real-Time Operating Systems (RTOS) and host platforms. The intent is to leverage native OS primitives directly while providing a unified C and C++ interface surface suitable for deeply embedded environments.

POSKI provides C (`<poski/osal/*.h>`) and C++ (`<poski/Os*.h>`) abstractions for:

-   [Tasks](#Task) (`pos_task` / `poski::OsTask`)
-   [Mutexes](#Mutex) (`pos_mutex` / `poski::OsMutex`)
-   [Semaphores](#Semaphore) (`pos_sem` / `poski::OsSemaphore`)
-   [Message Queues](#Queue) (`pos_queue` / `poski::OsQueue`)
-   [Software Timers](#Timer) (`pos_timer` / `poski::OsTimer`)
-   [System Time](#Time) (`pos_time` / `poski::OsTime`)
-   [Scheduler Control](#Scheduler) (`pos_sched`)
-   [Fatal Panic Handling](#Panic) (`pos_panic`)
-   [Ring Buffers](#Ring-Buffer) (`poski::OsRing`)

POSKI currently supports the following OS targets:

-   **POSIX (Linux)** (`targets/posix`) — Standard POSIX pthreads, semaphores, and timers
-   **POSIX (macOS / Apple)** (`targets/posix`) — POSIX pthreads with Grand Central Dispatch (`dispatch`) timers and semaphores
-   **FreeRTOS** (`targets/freertos`) — Native FreeRTOS tasks, queues, semaphores, mutexes, and timers
-   **Zephyr RTOS** (`targets/zephyr`) — Native Zephyr kernel threads, `k_msgq`, `k_mutex`, `k_sem`, and `k_timer`
-   **RT-Thread RTOS** (`targets/rt-thread`) — Native RT-Thread kernel primitives

### Motivation

Embedded software platforms require the system abstraction layer to be scalable, allowing disparate and diverse hardware and RTOS targets to be integrated with high velocity. Supporting rapid integration of new platforms in a scalable and maintainable way requires:

-   Maximum reuse of code, verification, and host-side unit testing
-   Minimum code fragmentation, forking, and conditional compilation
-   Adaptable and thin pathway to optimized native RTOS APIs

Device platforms tend to pivot on three major axes of common functionality, defining a three-dimensional matrix of configurations where a shared point on any one axis enables code reuse across otherwise disparate platforms:

-   **Device (Board)**
    -   Platforms that share a specific choice of chip combinations and wiring at the PCB level can share a common board/device port.
    -   Target examples are silicon vendor development boards or final product PCBs.

-   **Operating System (OS)**
    -   Platforms that share a common OS or RTOS share a common POSKI target port (`targets/freertos`, `targets/zephyr`, `targets/posix`, `targets/rt-thread`).
    -   An application or driver stack written against POSKI can run unchanged in Linux/macOS host unit tests and on target RTOS firmware.

-   **Hardware (HW / SoC)**
    -   Platforms that share a common chipset, SoC, or silicon share a common Hardware Abstraction Layer (HAL) from the vendor SDK.

### Context

Why POSKI? Other embedded OSAL projects were evaluated, but each had gaps relative to POSKI's goals:

-   [nler](https://github.com/nestlabs/nler) - Nest Labs Embedded Runtime
    -   Uses an inconsistent API namespace, whereas all POSKI C functions predictably begin with `pos_` (and C++ classes live in `namespace poski`).
    -   Omits standard primitives such as counting semaphores.
    -   Imposes its own centralized timer system rather than providing a thin pass-through to native OS timers.

-   [npl](https://github.com/apache/mynewt-nimble/tree/master/porting/npl) - Mynewt NimBLE Porting Layer
    -   Embedded within a larger BLE stack project and relies heavily on port-level `static inline` duck typing rather than a formal standalone OSAL specification.
    -   Uses a domain-specific BLE namespace (`ble_npl_`).

---

## Reference

### Task (`<poski/osal/os_task.h>` / `<poski/OsTask.h>`)

A task (`struct pos_task` / `poski::OsTask`) is an independent context of code execution managed by the underlying scheduler according to priority and scheduling policy (`pos_task_init`, `pos_task_remove`, `pos_task_yield`, `pos_task_sleep`, `pos_task_sleep_ms`).

### Mutex (`<poski/osal/os_mutex.h>` / `<poski/OsMutex.h>`)

A mutex (`struct pos_mutex` / `poski::OsMutex`) provides recursive mutual exclusion for protecting shared resources across tasks (`pos_mutex_init`, `pos_mutex_lock`, `pos_mutex_unlock`, `pos_mutex_deinit`).

### Semaphore (`<poski/osal/os_sem.h>` / `<poski/OsSemaphore.h>`)

A counting semaphore (`struct pos_sem` / `poski::OsSemaphore`) provides task synchronization and signaling from tasks or Interrupt Service Routines (`pos_sem_init`, `pos_sem_take`, `pos_sem_give`).

### Queue (`<poski/osal/os_queue.h>` / `<poski/OsQueue.h>`)

A message queue (`struct pos_queue` / `poski::OsQueue<T, N>`) provides thread- and ISR-safe intertask communication of fixed-size messages using copy semantics (`pos_queue_init`, `pos_queue_put`, `pos_queue_get`, `pos_queue_is_empty`, `pos_queue_deinit`).

### Timer (`<poski/osal/os_timer.h>` / `<poski/OsTimer.h>`)

A software timer (`struct pos_timer` / `poski::OsTimer`) triggers a callback function after a specified duration in ticks or milliseconds (`pos_timer_init`, `pos_timer_start`, `pos_timer_start_ms`, `pos_timer_stop`, `pos_timer_is_active`, `pos_timer_remaining_ticks`).

### Time (`<poski/osal/os_time.h>` / `<poski/OsTime.h>`)

Utility functions (`pos_time_get`, `pos_time_get_ms`, `pos_time_ms_to_ticks`, `pos_time_ticks_to_ms`) for querying system uptime and converting between milliseconds and OS ticks.

### Scheduler (`<poski/osal/os_sched.h>`)

Controls and queries the underlying RTOS scheduler state (`pos_sched_start`, `pos_sched_started`).

### Panic (`<poski/osal/os_panic.h>`)

Provides an unrecoverable fatal error handler primitive (`pos_panic(const char *msg)`) that logs diagnostic context and halts or aborts execution.

### Ring Buffer (`<poski/OsRing.h>`)

Provides a portable C++ ring buffer (`poski::OsRing`) for byte and element buffering.

---

## Porting Guide & Repository Layout

POSKI separates its public interface headers cleanly from target-specific implementations:

| File / Folder | Contents |
| :--- | :--- |
| `include/poski/osal/osal.h` | Top-level C umbrella header including all POSKI OSAL modules |
| `include/poski/osal/os_*.h` | Modular C OSAL interface headers (`os_task.h`, `os_mutex.h`, `os_sem.h`, `os_queue.h`, `os_timer.h`, `os_time.h`, `os_sched.h`, `os_panic.h`, `os_types.h`) |
| `include/poski/Os*.h` | Header-only C++ RAII wrapper classes (`OsTask.h`, `OsMutex.h`, `OsSemaphore.h`, `OsQueue.h`, `OsTimer.h`, `OsTime.h`, `OsRing.h`) |
| `targets/<port>/poski/osal/os_port.h` | Maps POSKI `struct pos_*` types to target-specific RTOS primitives |
| `targets/<port>/os_*.c` | Target implementation of POSKI C APIs for `<port>` (`posix`, `freertos`, `zephyr`, `rt-thread`) |
| `tests/` | Portable C, C++, and GoogleTest (`gtest`) test suites |

### Directory Structure

```text
.
├── BUILD                     - Bazel build rules for libraries and test suites
├── MODULE.bazel              - Bzlmod dependency definitions (rules_cc, googletest, freertos)
├── Makefile                  - GNU Make wrapper
├── include/poski
│   ├── OsMutex.h             - C++ wrapper for pos_mutex
│   ├── OsQueue.h             - C++ template wrapper for pos_queue
│   ├── OsRing.h              - Portable ring buffer class
│   ├── OsSemaphore.h         - C++ wrapper for pos_sem
│   ├── OsTask.h              - C++ wrapper for pos_task
│   ├── OsTime.h              - C++ wrapper for pos_time
│   ├── OsTimer.h             - C++ wrapper for pos_timer
│   └── osal
│       ├── os_mutex.h        - Mutex C API
│       ├── os_panic.h        - Fatal error panic C API
│       ├── os_queue.h        - Message queue C API
│       ├── os_sched.h        - Scheduler control C API
│       ├── os_sem.h          - Semaphore C API
│       ├── os_task.h         - Task management C API
│       ├── os_time.h         - System time & tick conversion C API
│       ├── os_timer.h        - Software timer C API
│       ├── os_types.h        - Common POSKI types and error codes (pos_error_t)
│       └── osal.h            - Umbrella C header
├── targets
│   ├── freertos/             - FreeRTOS target port
│   ├── posix/                - POSIX (Linux & macOS) target port
│   ├── rt-thread/            - RT-Thread RTOS target port
│   └── zephyr/               - Zephyr RTOS target port
└── tests/                    - C, C++, and GTest unit test suites
```

#### POSIX Port Details

The POSIX port (`targets/posix`) supports both Linux and Apple (macOS) hosts:

| OS | Task | Mutex | Semaphore | Timer | Time | Queue |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Linux** | `pthread` | `pthread_mutex` | `sem_t` | `timer_create` | `clock_gettime` | `pthread` + `RingPthread` |
| **macOS** | `pthread` | `pthread_mutex` | `dispatch_semaphore` | `dispatch_source` | `mach_absolute_time` | `pthread` + `RingPthread` |

---

## Quick Start

### Prerequisites

- **Bazel** (recommended, builds hermetic dependencies including GoogleTest and FreeRTOS kernel)
- **Make** (optional convenience wrapper)

#### Linux
```bash
sudo apt install make ccache bazel
```

#### macOS
```bash
brew install make ccache bazel
```

### Build

#### Bazel

To build all default targets (including `:osal_posix`, `:osal_freertos`, and all unit test binaries):
```bash
bazel build //...
```
*(Note: `:osal_zephyr` is tagged `manual` so wildcard builds `//...` succeed without requiring a full Zephyr workspace.)*

To build only the **POSIX** library target (`:osal` / `:osal_posix`):
```bash
bazel build //:osal
```

To build the **FreeRTOS** static library target (`:freertos` / `:osal_freertos`):
```bash
bazel build //:freertos
```

### Test

#### Bazel

To run the entire test suite (C tests, C++ tests, and GoogleTest suites):
```bash
bazel test //...
```

To run only the standard C/C++ test suite or only the GoogleTest suite:
```bash
bazel test //:test
bazel test //:gtest
```

#### Make
```bash
make test
make gtest
```
