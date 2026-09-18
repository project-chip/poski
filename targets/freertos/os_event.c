/*
 *    Copyright (c) 2026 Project CHIP Authors
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <string.h>

#include <poski/osal/osal.h>
#include "os_hw.h"

#define POS_EVENTQ_MAX_TOKENS 0xFFFFU

static inline UBaseType_t pos_freertos_crit_enter(void)
{
    if (pos_hw_in_isr())
    {
        return taskENTER_CRITICAL_FROM_ISR();
    }
    taskENTER_CRITICAL();
    return 0;
}

static inline void pos_freertos_crit_exit(UBaseType_t state)
{
    if (pos_hw_in_isr())
    {
        taskEXIT_CRITICAL_FROM_ISR(state);
    }
    else
    {
        taskEXIT_CRITICAL();
    }
}

pos_error_t pos_eventq_init(struct pos_eventq * evq)
{
    if (evq == NULL)
    {
        return POS_INVALID_PARAM;
    }

    memset(evq, 0, sizeof(*evq));
    evq->sem = xSemaphoreCreateCounting(POS_EVENTQ_MAX_TOKENS, 0);
    if (evq->sem == NULL)
    {
        return POS_ENOMEM;
    }

    return POS_OK;
}

pos_error_t pos_eventq_deinit(struct pos_eventq * evq)
{
    struct pos_event * cur;
    UBaseType_t crit;

    if (evq == NULL)
    {
        return POS_INVALID_PARAM;
    }

    crit = pos_freertos_crit_enter();
    cur  = evq->head;
    while (cur != NULL)
    {
        struct pos_event * next = cur->next;
        cur->next               = NULL;
        cur->queued             = false;
        cur                     = next;
    }
    evq->head = NULL;
    evq->tail = NULL;
    pos_freertos_crit_exit(crit);

    if (evq->sem != NULL)
    {
        vSemaphoreDelete(evq->sem);
        evq->sem = NULL;
    }

    return POS_OK;
}

int pos_eventq_inited(const struct pos_eventq * evq)
{
    return (evq != NULL && evq->sem != NULL) ? 1 : 0;
}

pos_error_t pos_eventq_put(struct pos_eventq * evq, struct pos_event * ev)
{
    UBaseType_t crit;

    if (evq == NULL || evq->sem == NULL || ev == NULL)
    {
        return POS_INVALID_PARAM;
    }

    crit = pos_freertos_crit_enter();

    if (ev->queued)
    {
        pos_freertos_crit_exit(crit);
        return POS_OK;
    }

    ev->next   = NULL;
    ev->queued = true;

    if (evq->tail != NULL)
    {
        evq->tail->next = ev;
        evq->tail       = ev;
    }
    else
    {
        evq->head = ev;
        evq->tail = ev;
    }

    pos_freertos_crit_exit(crit);

    if (pos_hw_in_isr())
    {
        BaseType_t woken = pdFALSE;
        xSemaphoreGiveFromISR(evq->sem, &woken);
        portYIELD_FROM_ISR(woken);
    }
    else
    {
        xSemaphoreGive(evq->sem);
    }

    return POS_OK;
}

struct pos_event * pos_eventq_get(struct pos_eventq * evq, pos_time_t timeout)
{
    struct pos_event * ev = NULL;
    BaseType_t sem_ret;
    UBaseType_t crit;

    if (evq == NULL || evq->sem == NULL)
    {
        return NULL;
    }

    if (pos_hw_in_isr())
    {
        BaseType_t woken = pdFALSE;
        sem_ret          = xSemaphoreTakeFromISR(evq->sem, &woken);
        portYIELD_FROM_ISR(woken);
    }
    else
    {
        sem_ret = xSemaphoreTake(evq->sem, timeout);
    }

    if (sem_ret != pdTRUE)
    {
        return NULL;
    }

    crit = pos_freertos_crit_enter();
    ev   = evq->head;
    if (ev != NULL)
    {
        evq->head = ev->next;
        if (evq->head == NULL)
        {
            evq->tail = NULL;
        }
        ev->next   = NULL;
        ev->queued = false;
    }
    pos_freertos_crit_exit(crit);

    return ev;
}

pos_error_t pos_eventq_remove(struct pos_eventq * evq, struct pos_event * ev)
{
    struct pos_event * prev = NULL;
    struct pos_event * cur;
    bool removed = false;
    UBaseType_t crit;

    if (evq == NULL || evq->sem == NULL || ev == NULL)
    {
        return POS_INVALID_PARAM;
    }

    crit = pos_freertos_crit_enter();

    if (ev->queued)
    {
        cur = evq->head;
        while (cur != NULL)
        {
            if (cur == ev)
            {
                if (prev != NULL)
                {
                    prev->next = cur->next;
                }
                else
                {
                    evq->head = cur->next;
                }

                if (evq->tail == cur)
                {
                    evq->tail = prev;
                }

                cur->next   = NULL;
                cur->queued = false;
                removed     = true;
                break;
            }
            prev = cur;
            cur  = cur->next;
        }
    }

    pos_freertos_crit_exit(crit);

    if (removed)
    {
        if (pos_hw_in_isr())
        {
            BaseType_t woken = pdFALSE;
            xSemaphoreTakeFromISR(evq->sem, &woken);
        }
        else
        {
            xSemaphoreTake(evq->sem, 0);
        }
    }

    return POS_OK;
}

bool pos_eventq_is_empty(struct pos_eventq * evq)
{
    bool empty;
    UBaseType_t crit;

    if (evq == NULL || evq->sem == NULL)
    {
        return true;
    }

    crit  = pos_freertos_crit_enter();
    empty = (evq->head == NULL);
    pos_freertos_crit_exit(crit);

    return empty;
}

/* =========================================================================
 * Event Timer Implementation (FreeRTOS)
 * ========================================================================= */

static void pos_event_timer_cb(void * arg)
{
    struct pos_event_timer * et = (struct pos_event_timer *) arg;
    if (et != NULL && et->evq != NULL)
    {
        pos_eventq_put(et->evq, &et->ev);
    }
}

pos_error_t pos_event_timer_init(struct pos_event_timer * et,
                                 struct pos_eventq * evq,
                                 pos_event_fn * fn,
                                 void * arg)
{
    if (et == NULL || evq == NULL || fn == NULL)
    {
        return POS_INVALID_PARAM;
    }

    memset(et, 0, sizeof(*et));
    et->evq = evq;
    pos_event_init(&et->ev, fn, arg);

    return pos_timer_init(&et->timer, pos_event_timer_cb, et);
}

pos_error_t pos_event_timer_deinit(struct pos_event_timer * et)
{
    if (et == NULL)
    {
        return POS_INVALID_PARAM;
    }

    pos_event_timer_stop(et);

    if (et->timer.handle != NULL)
    {
        xTimerDelete(et->timer.handle, portMAX_DELAY);
        et->timer.handle = NULL;
    }

    return POS_OK;
}

pos_error_t pos_event_timer_start(struct pos_event_timer * et, pos_time_t ticks)
{
    if (et == NULL || et->evq == NULL)
    {
        return POS_INVALID_PARAM;
    }

    pos_eventq_remove(et->evq, &et->ev);
    return pos_timer_start(&et->timer, ticks);
}

pos_error_t pos_event_timer_start_ms(struct pos_event_timer * et, pos_time_t ms)
{
    if (et == NULL || et->evq == NULL)
    {
        return POS_INVALID_PARAM;
    }

    return pos_event_timer_start(et, pos_time_ms_to_ticks(ms));
}

pos_error_t pos_event_timer_stop(struct pos_event_timer * et)
{
    pos_error_t err;

    if (et == NULL)
    {
        return POS_INVALID_PARAM;
    }

    err = pos_timer_stop(&et->timer);
    if (et->evq != NULL)
    {
        pos_eventq_remove(et->evq, &et->ev);
    }

    return err;
}

pos_error_t pos_event_timer_inited(struct pos_event_timer * et)
{
    if (et == NULL || et->timer.handle == NULL)
    {
        return POS_ENOENT;
    }

    return POS_OK;
}

bool pos_event_timer_is_active(struct pos_event_timer * et)
{
    if (et == NULL || et->timer.handle == NULL)
    {
        return false;
    }

    return pos_timer_is_active(&et->timer);
}

pos_time_t pos_event_timer_get_ticks(struct pos_event_timer * et)
{
    if (et == NULL || et->timer.handle == NULL)
    {
        return 0;
    }

    return pos_timer_get_ticks(&et->timer);
}

pos_time_t pos_event_timer_remaining_ticks(struct pos_event_timer * et, pos_time_t now)
{
    if (et == NULL || et->timer.handle == NULL)
    {
        return 0;
    }

    return pos_timer_remaining_ticks(&et->timer, now);
}

void * pos_event_timer_arg_get(struct pos_event_timer * et)
{
    if (et == NULL)
    {
        return NULL;
    }

    return pos_event_arg_get(&et->ev);
}

void pos_event_timer_arg_set(struct pos_event_timer * et, void * arg)
{
    if (et != NULL)
    {
        pos_event_arg_set(&et->ev, arg);
    }
}

struct pos_event * pos_event_timer_event_get(struct pos_event_timer * et)
{
    return (et != NULL) ? &et->ev : NULL;
}
