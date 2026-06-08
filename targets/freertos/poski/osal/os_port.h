/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2018 Google LLC
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

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#ifdef __cplusplus
extern "C" {
#endif

#define POS_TIME_NO_WAIT 0
#define POS_TIME_FOREVER portMAX_DELAY
#define POS_TICKS_PER_SEC configTICK_RATE_HZ

#define POS_PRIORITY_MIN 1
#define POS_PRIORITY_MAX configMAX_PRIORITIES
#define POS_PRIORITY_APP (POS_PRIORITY_MIN + 1)

typedef BaseType_t pos_base_t;
typedef StackType_t pos_stack_t;

typedef TickType_t pos_time_t;
typedef int32_t pos_stime_t;

struct pos_mutex
{
    SemaphoreHandle_t handle;
};

struct pos_sem
{
    SemaphoreHandle_t handle;
};

struct pos_queue
{
    QueueHandle_t  handle;
    pos_signal_fn *signal_cb;       /* NULL by default */
    void          *signal_data;
};

struct pos_timer
{
    TimerHandle_t handle;
    pos_timer_fn * func;
    void * arg;
};

struct pos_task
{
    TaskHandle_t handle;
    pos_task_func_t func;
    void * arg;
};

static inline bool pos_os_started(void)
{
    return xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED;
}

static inline uint16_t pos_sem_get_count(struct pos_sem * sem)
{
    assert(sem);
    assert(sem->handle);
    return uxSemaphoreGetCount(sem->handle);
}

static inline pos_error_t pos_timer_stop(struct pos_timer * tm)
{
    assert(tm);
    assert(tm->handle);
    return xTimerStop(tm->handle, portMAX_DELAY);
}

static inline bool pos_timer_is_active(struct pos_timer * tm)
{
    assert(tm);
    assert(tm->handle);
    return xTimerIsTimerActive(tm->handle) == pdTRUE;
}

static inline int pos_queue_inited(const struct pos_queue * queue)
{
    return (queue->handle != NULL);
}

pos_error_t pos_timer_start(struct pos_timer * timer, pos_time_t ticks);

static inline pos_time_t pos_time_ms_to_ticks(pos_time_t ms)
{
    return (ms * POS_TICKS_PER_SEC) / 1000;
}

static inline pos_time_t pos_time_ticks_to_ms(pos_time_t ticks)
{
    return (ticks * 1000) / POS_TICKS_PER_SEC;
}

/* Dispatch on ISR context.  ISR-context calls keep the FromISR
 * variant (with its required critical section); task-context
 * calls drop the redundant FromISR overhead. */
static inline pos_time_t pos_time_get(void)
{
    return xPortIsInsideInterrupt()
         ? xTaskGetTickCountFromISR()
         : xTaskGetTickCount();
}

static inline pos_time_t pos_time_get_ms(void)
{
    return pos_time_ticks_to_ms(pos_time_get());
}

static inline pos_error_t pos_timer_start_ms(struct pos_timer * timer, pos_time_t duration)
{
    pos_time_t ticks = pos_time_ms_to_ticks(duration);
    return pos_timer_start(timer, ticks);
}

static inline pos_time_t pos_timer_get_ticks(struct pos_timer * tm)
{
    assert(tm);
    assert(tm->handle);
    return xTimerGetExpiryTime(tm->handle);
}

static inline void * pos_timer_arg_get(struct pos_timer * timer)
{
    assert(timer);
    return timer->arg;
}

static inline void pos_timer_arg_set(struct pos_timer * timer, void * arg)
{
    assert(timer);
    timer->arg = arg;
}

static inline void pos_task_yield(void)
{
    taskYIELD();
}

static inline void pos_task_sleep(pos_time_t ticks)
{
    vTaskDelay(ticks);
}

static inline void pos_task_sleep_ms(pos_time_t ms)
{
    pos_task_sleep(pos_time_ms_to_ticks(ms));
}

static inline void * pos_get_current_task_id(void)
{
    return xTaskGetCurrentTaskHandle();
}

static inline void pos_sched_start(void)
{
    vTaskStartScheduler();
}

#ifdef __cplusplus
}
#endif

#endif /* POSKI_OS_PORT_H */
