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

#include <poski/osal/osal.h>
#include "os_utils.h"

#define MAX_TASKS 10
#define STACK_SIZE 2048

K_THREAD_STACK_ARRAY_DEFINE(s_task_stacks, MAX_TASKS, STACK_SIZE);
static bool s_stack_used[MAX_TASKS];

static void pos_task_wrapper(void *p1, void *p2, void *p3)
{
    pos_task_func_t func = (pos_task_func_t)p1;
    void * arg = p2;
    func(arg);
}

pos_error_t pos_task_init(struct pos_task * t, const char * name, pos_task_func_t func, void * arg, uint8_t prio,
                                  uint16_t stack_size)
{
    if (stack_size > STACK_SIZE) {
        return POS_EINVAL;
    }

    // Find unused stack
    int stack_idx = -1;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!s_stack_used[i]) {
            stack_idx = i;
            break;
        }
    }

    if (stack_idx == -1) {
        return POS_ENOMEM;
    }

    s_stack_used[stack_idx] = true;
    t->stack = s_task_stacks[stack_idx];

    t->tid = k_thread_create(&t->thread, t->stack, STACK_SIZE,
                             pos_task_wrapper, (void *)func, arg, NULL,
                             prio, 0, K_NO_WAIT);

    if (t->tid) {
        k_thread_name_set(t->tid, name);
        return POS_OK;
    } else {
        s_stack_used[stack_idx] = false;
        return POS_ERROR;
    }
}

void pos_task_yield(void)
{
    k_yield();
}

void pos_task_sleep(pos_time_t ticks)
{
    k_sleep(K_TICKS(ticks));
}

void pos_task_sleep_ms(pos_time_t ms)
{
    k_sleep(K_MSEC(ms));
}

void * pos_get_current_task_id(void)
{
    return k_current_get();
}

pos_error_t pos_task_remove(struct pos_task * t)
{
    k_thread_abort(t->tid);
    
    // Free stack
    for (int i = 0; i < MAX_TASKS; i++) {
        if (t->stack == s_task_stacks[i]) {
            s_stack_used[i] = false;
            break;
        }
    }
    
    return POS_OK;
}
