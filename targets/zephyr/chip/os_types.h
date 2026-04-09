/*
 *
 *    Copyright (c) 2026 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef CHIP_OS_ZEPHYR_TYPES_H
#define CHIP_OS_ZEPHYR_TYPES_H

#include <zephyr/kernel.h>

/* The highest and lowest task priorities */
/* Zephyr priorities are 0 (highest) to N (lowest) for preemptive threads. */
/* We assume CONFIG_NUM_PREEMPT_PRIORITIES is defined. */
#define CHIP_OS_PRIORITY_MIN (CONFIG_NUM_PREEMPT_PRIORITIES - 1)
#define CHIP_OS_PRIORITY_MAX 0
#define CHIP_OS_PRIORITY_APP (CHIP_OS_PRIORITY_MIN - 1)

typedef int chip_os_base_t;
typedef int chip_os_stack_t;

struct chip_os_task
{
    struct k_thread thread;
    k_tid_t tid;
    void * stack;
};

struct chip_os_queue
{
    struct k_msgq msgq;
    void * buffer;
    chip_os_signal_fn * sig_cb;
    void * sig_arg;
};

struct chip_os_mutex
{
    struct k_mutex mutex;
};

struct chip_os_sem
{
    struct k_sem sem;
};

#endif // CHIP_OS_ZEPHYR_TYPES_H
