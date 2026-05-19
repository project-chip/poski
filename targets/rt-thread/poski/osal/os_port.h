/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2022 tangweikang, <BalanceTWK@yeah.net>
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

#ifndef CHIP_OS_PORT_H
#define CHIP_OS_PORT_H

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHIP_OS_TIME_NO_WAIT RT_WAITING_NO
#define CHIP_OS_TIME_FOREVER RT_WAITING_FOREVER
#define CHIP_OS_TICKS_PER_SEC RT_TICK_PER_SECOND

#define CHIP_OS_PRIORITY_MIN 1
#define CHIP_OS_PRIORITY_MAX RT_THREAD_PRIORITY_MAX
#define CHIP_OS_PRIORITY_APP (RT_THREAD_PRIORITY_MAX - 10)

typedef rt_base_t chip_os_base_t;
typedef rt_tick_t chip_os_time_t;

struct chip_os_mutex
{
    rt_mutex_t handle;
};

struct chip_os_sem
{
    rt_sem_t handle;
};

struct chip_os_queue
{
    rt_mq_t handle;
};

struct chip_os_timer
{
    rt_timer_t handle;
    chip_os_timer_fn * func;
    void * arg;
};

struct chip_os_task
{
    rt_thread_t handle;
    chip_os_task_func_t func;
    void * arg;
};

static inline int chip_os_queue_inited(const struct chip_os_queue * queue)
{
    return (queue->handle != NULL);
}
static inline chip_os_time_t chip_os_time_get(void)
{
    return rt_tick_get();
}
static inline chip_os_time_t chip_os_time_get_ms(void)
{
    return rt_tick_get_millisecond();
}
static inline chip_os_time_t chip_os_time_ticks_to_ms(chip_os_time_t ticks)
{
    return (ticks * 1000) / CHIP_OS_TICKS_PER_SEC;
}
static inline chip_os_time_t chip_os_time_ms_to_ticks(chip_os_time_t ms)
{
    return (ms * CHIP_OS_TICKS_PER_SEC) / 1000;
}
static inline void * chip_os_timer_arg_get(struct chip_os_timer * timer)
{
    assert(timer);
    return timer->arg;
}
static inline bool chip_os_timer_is_active(struct chip_os_timer * tm)
{
    rt_uint32_t state;
    assert(tm);
    assert(tm->handle);
    rt_timer_control(tm->handle,RT_TIMER_CTRL_GET_STATE,&state);
    if(RT_TIMER_FLAG_ACTIVATED == state)
    {
        return RT_TRUE;
    }
    else
    {
        return RT_FALSE;
    }
}
chip_os_error_t chip_os_timer_start(struct chip_os_timer * timer, chip_os_time_t ticks);
static inline chip_os_error_t chip_os_timer_start_ms(struct chip_os_timer * timer, chip_os_time_t duration)
{
    chip_os_time_t ticks = chip_os_time_ms_to_ticks(duration);
    return chip_os_timer_start(timer, ticks);
}
static inline void chip_os_sched_start(void)
{
}

#ifdef __cplusplus
}
#endif

#endif /* CHIP_OS_PORT_H */
