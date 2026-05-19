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

#ifndef POSKI_OS_PORT_H
#define POSKI_OS_PORT_H

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POS_TIME_NO_WAIT RT_WAITING_NO
#define POS_TIME_FOREVER RT_WAITING_FOREVER
#define POS_TICKS_PER_SEC RT_TICK_PER_SECOND

#define POS_PRIORITY_MIN 1
#define POS_PRIORITY_MAX RT_THREAD_PRIORITY_MAX
#define POS_PRIORITY_APP (RT_THREAD_PRIORITY_MAX - 10)

typedef rt_base_t pos_base_t;
typedef rt_tick_t pos_time_t;

struct pos_mutex
{
    rt_mutex_t handle;
};

struct pos_sem
{
    rt_sem_t handle;
};

struct pos_queue
{
    rt_mq_t handle;
};

struct pos_timer
{
    rt_timer_t handle;
    pos_timer_fn * func;
    void * arg;
};

struct pos_task
{
    rt_thread_t handle;
    pos_task_func_t func;
    void * arg;
};

static inline int pos_queue_inited(const struct pos_queue * queue)
{
    return (queue->handle != NULL);
}
static inline pos_time_t pos_time_get(void)
{
    return rt_tick_get();
}
static inline pos_time_t pos_time_get_ms(void)
{
    return rt_tick_get_millisecond();
}
static inline pos_time_t pos_time_ticks_to_ms(pos_time_t ticks)
{
    return (ticks * 1000) / POS_TICKS_PER_SEC;
}
static inline pos_time_t pos_time_ms_to_ticks(pos_time_t ms)
{
    return (ms * POS_TICKS_PER_SEC) / 1000;
}
static inline void * pos_timer_arg_get(struct pos_timer * timer)
{
    assert(timer);
    return timer->arg;
}
static inline bool pos_timer_is_active(struct pos_timer * tm)
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
pos_error_t pos_timer_start(struct pos_timer * timer, pos_time_t ticks);
static inline pos_error_t pos_timer_start_ms(struct pos_timer * timer, pos_time_t duration)
{
    pos_time_t ticks = pos_time_ms_to_ticks(duration);
    return pos_timer_start(timer, ticks);
}
static inline void pos_sched_start(void)
{
}

#ifdef __cplusplus
}
#endif

#endif /* POSKI_OS_PORT_H */
