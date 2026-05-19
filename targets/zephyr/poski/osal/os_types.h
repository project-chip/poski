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

#ifndef POSKI_OS_ZEPHYR_TYPES_H
#define POSKI_OS_ZEPHYR_TYPES_H

#include <zephyr/kernel.h>

/* The highest and lowest task priorities */
/* Zephyr priorities are 0 (highest) to N (lowest) for preemptive threads. */
/* We assume CONFIG_NUM_PREEMPT_PRIORITIES is defined. */
#define POS_PRIORITY_MIN (CONFIG_NUM_PREEMPT_PRIORITIES - 1)
#define POS_PRIORITY_MAX 0
#define POS_PRIORITY_APP (POS_PRIORITY_MIN - 1)

typedef int pos_base_t;
typedef int pos_stack_t;

struct pos_task
{
    struct k_thread thread;
    k_tid_t tid;
    void * stack;
};

struct pos_queue
{
    struct k_msgq msgq;
    void * buffer;
    pos_signal_fn * sig_cb;
    void * sig_arg;
};

struct pos_mutex
{
    struct k_mutex mutex;
};

struct pos_sem
{
    struct k_sem sem;
};

#endif // POSKI_OS_ZEPHYR_TYPES_H
