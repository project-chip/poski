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

/*
 * FreeRTOS event queue and event timers.
 *
 * The event queue is an intrusive FIFO guarded by a critical section, so
 * events can be posted and removed from tasks and ISRs.  A binary semaphore
 * only wakes blocked consumers, and is signaled only while one is blocked: the
 * list is the source of truth, so a consumer re-checks the list after every
 * wakeup and the semaphore count never has to match the number of queued
 * events.
 *
 * Event timers are serviced by their queue, like Mynewt callouts: pending
 * timers are kept on the queue's `timers` list, sorted by expiry.  Every queue
 * operation first moves the events of expired timers onto the FIFO (soonest
 * first, so they stay ordered relative to events posted afterwards), and a
 * blocking get sleeps no longer than the earliest pending expiry.  Event timers
 * therefore use no FreeRTOS software timers (no timer service task, timer
 * command queue, or heap), and start/stop/restart cannot race with an expiry
 * that is already in flight.  The POSIX target uses the same algorithm, where
 * it is exercised by the host unit tests.
 *
 * With configSUPPORT_STATIC_ALLOCATION == 1 the wakeup semaphore is allocated
 * inside struct pos_eventq, so no heap is used at all.
 */

#include <string.h>

#include <poski/osal/osal.h>
#include "os_hw.h"

/* Return true if tick time `now` is at or after `when` (wrap-safe). */
static bool time_reached(TickType_t now, TickType_t when)
{
    return (TickType_t) (now - when) <= POS_EVENT_TIMER_MAX_TICKS;
}

/* Critical section usable from both task and ISR context. */
static UBaseType_t eventq_crit_enter(void)
{
    if (pos_hw_in_isr())
    {
        return taskENTER_CRITICAL_FROM_ISR();
    }
    taskENTER_CRITICAL();
    return 0;
}

static void eventq_crit_exit(UBaseType_t state)
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

/* =========================================================================
 * Internal helpers.  The caller must be inside the critical section.
 * ========================================================================= */

static void event_append_locked(struct pos_eventq * evq, struct pos_event * ev)
{
    /* Idempotent: an event that is already queued is left where it is. */
    if (ev->queued)
    {
        return;
    }

    ev->next   = NULL;
    ev->queued = true;
    if (evq->tail != NULL)
    {
        evq->tail->next = ev;
    }
    else
    {
        evq->head = ev;
    }
    evq->tail = ev;
}

static struct pos_event * event_pop_locked(struct pos_eventq * evq)
{
    struct pos_event * ev = evq->head;

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
    return ev;
}

static void event_unlink_locked(struct pos_eventq * evq, struct pos_event * ev)
{
    struct pos_event * prev = NULL;
    struct pos_event * cur;

    if (!ev->queued)
    {
        return;
    }

    for (cur = evq->head; cur != NULL; prev = cur, cur = cur->next)
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
            return;
        }
    }
}

static void timer_insert_locked(struct pos_eventq * evq, struct pos_event_timer * et)
{
    struct pos_event_timer ** link = &evq->timers;

    /* Keep the list sorted by expiry; timers with equal expiry keep start order. */
    while (*link != NULL && time_reached(et->expiry, (*link)->expiry))
    {
        link = &(*link)->next;
    }
    et->next  = *link;
    *link     = et;
    et->armed = true;
}

static void timer_unlink_locked(struct pos_eventq * evq, struct pos_event_timer * et)
{
    struct pos_event_timer ** link;

    if (!et->armed)
    {
        return;
    }

    for (link = &evq->timers; *link != NULL; link = &(*link)->next)
    {
        if (*link == et)
        {
            *link = et->next;
            break;
        }
    }
    et->next  = NULL;
    et->armed = false;
}

/* Post the events of all timers that have expired by `now`, soonest first. */
static void timers_expire_locked(struct pos_eventq * evq, TickType_t now)
{
    while (evq->timers != NULL && time_reached(now, evq->timers->expiry))
    {
        struct pos_event_timer * et = evq->timers;

        evq->timers = et->next;
        et->next    = NULL;
        et->armed   = false;
        event_append_locked(evq, &et->ev);
    }
}

/* Enter the critical section and bring the queue up to date by posting any
 * expired timers. */
static UBaseType_t eventq_lock(struct pos_eventq * evq)
{
    UBaseType_t state = eventq_crit_enter();
    timers_expire_locked(evq, pos_time_get());
    return state;
}

static void eventq_unlock(UBaseType_t state)
{
    eventq_crit_exit(state);
}

/* Wake one consumer blocked in pos_eventq_get() so that it re-checks the queue.
 * Callers check `waiters` inside the critical section and call this after
 * leaving it, since FreeRTOS APIs must not be called from a critical section. */
static void eventq_wake(struct pos_eventq * evq)
{
    if (pos_hw_in_isr())
    {
        BaseType_t woken = pdFALSE;
        (void) xSemaphoreGiveFromISR(evq->wakeup, &woken);
        portYIELD_FROM_ISR(woken);
    }
    else
    {
        /* Fails harmlessly if a wakeup is already pending. */
        (void) xSemaphoreGive(evq->wakeup);
    }
}

/* =========================================================================
 * Event Queue
 * ========================================================================= */

pos_error_t pos_eventq_init(struct pos_eventq * evq)
{
    if (evq == NULL)
    {
        return POS_INVALID_PARAM;
    }

    memset(evq, 0, sizeof(*evq));
#if POS_FREERTOS_STATIC_ALLOCATION
    evq->wakeup = xSemaphoreCreateBinaryStatic(&evq->wakeup_buf);
#else
    evq->wakeup = xSemaphoreCreateBinary();
#endif

    return (evq->wakeup != NULL) ? POS_OK : POS_ENOMEM;
}

pos_error_t pos_eventq_deinit(struct pos_eventq * evq)
{
    UBaseType_t state;

    if (evq == NULL)
    {
        return POS_INVALID_PARAM;
    }

    if (evq->wakeup == NULL)
    {
        return POS_OK;
    }

    state = eventq_crit_enter();
    while (event_pop_locked(evq) != NULL)
    {
    }
    while (evq->timers != NULL)
    {
        timer_unlink_locked(evq, evq->timers);
    }
    eventq_crit_exit(state);

    vSemaphoreDelete(evq->wakeup);
    evq->wakeup = NULL;

    return POS_OK;
}

int pos_eventq_inited(const struct pos_eventq * evq)
{
    return (evq != NULL && evq->wakeup != NULL) ? 1 : 0;
}

pos_error_t pos_eventq_put(struct pos_eventq * evq, struct pos_event * ev)
{
    UBaseType_t state;
    bool wake;

    if (evq == NULL || evq->wakeup == NULL || ev == NULL)
    {
        return POS_INVALID_PARAM;
    }

    state = eventq_lock(evq);
    event_append_locked(evq, ev);
    wake = (evq->waiters != 0);
    eventq_unlock(state);

    if (wake)
    {
        eventq_wake(evq);
    }

    return POS_OK;
}

struct pos_event * pos_eventq_get(struct pos_eventq * evq, pos_time_t timeout)
{
    struct pos_event * ev;
    TimeOut_t timeout_state;
    TickType_t remaining = timeout;
    bool waiting         = false;
    bool handoff;

    if (evq == NULL || evq->wakeup == NULL)
    {
        return NULL;
    }

    if (pos_hw_in_isr())
    {
        /* ISRs cannot block: poll once. */
        UBaseType_t state = eventq_lock(evq);
        ev                = event_pop_locked(evq);
        eventq_unlock(state);
        return ev;
    }

    vTaskSetTimeOutState(&timeout_state);
    for (;;)
    {
        TickType_t wait = remaining;
        UBaseType_t state;
        TickType_t now;

        state = eventq_crit_enter();
        if (waiting)
        {
            evq->waiters--;
        }
        now = xTaskGetTickCount();
        timers_expire_locked(evq, now);
        ev = event_pop_locked(evq);

        /* The binary semaphore coalesces wakeups, so if events are left behind
         * pass a wakeup on to any other blocked consumer. */
        handoff = (ev != NULL) && (evq->head != NULL) && (evq->waiters != 0);

        waiting = (ev == NULL) && (remaining != 0);
        if (waiting)
        {
            /* Register as a waiter before leaving the critical section, so a
             * put() that follows is guaranteed to signal the semaphore. */
            evq->waiters++;

            /* Wake up in time to post the next timer expiry.  It is in the
             * future, because expired timers were just posted. */
            if (evq->timers != NULL && (TickType_t) (evq->timers->expiry - now) < wait)
            {
                wait = evq->timers->expiry - now;
            }
        }
        eventq_crit_exit(state);

        if (!waiting)
        {
            break;
        }

        (void) xSemaphoreTake(evq->wakeup, wait);

        if (xTaskCheckForTimeOut(&timeout_state, &remaining) != pdFALSE)
        {
            /* Timed out: make one last non-blocking attempt. */
            remaining = 0;
        }
    }

    if (handoff)
    {
        eventq_wake(evq);
    }

    return ev;
}

pos_error_t pos_eventq_remove(struct pos_eventq * evq, struct pos_event * ev)
{
    UBaseType_t state;

    if (evq == NULL || evq->wakeup == NULL || ev == NULL)
    {
        return POS_INVALID_PARAM;
    }

    state = eventq_lock(evq);
    event_unlink_locked(evq, ev);
    eventq_unlock(state);

    return POS_OK;
}

bool pos_eventq_is_empty(struct pos_eventq * evq)
{
    UBaseType_t state;
    bool empty;

    if (evq == NULL || evq->wakeup == NULL)
    {
        return true;
    }

    state = eventq_lock(evq);
    empty = (evq->head == NULL);
    eventq_unlock(state);

    return empty;
}

/* =========================================================================
 * Event Timer
 * ========================================================================= */

static bool event_timer_usable(const struct pos_event_timer * et)
{
    return (et != NULL) && (et->evq != NULL) && (et->evq->wakeup != NULL);
}

pos_error_t pos_event_timer_init(struct pos_event_timer * et, struct pos_eventq * evq, pos_event_fn * fn, void * arg)
{
    if (et == NULL)
    {
        return POS_INVALID_PARAM;
    }

    /* Leave a rejected timer zeroed, i.e. safely "not initialized". */
    memset(et, 0, sizeof(*et));
    if (evq == NULL || fn == NULL)
    {
        return POS_INVALID_PARAM;
    }

    et->evq = evq;
    pos_event_init(&et->ev, fn, arg);

    return POS_OK;
}

pos_error_t pos_event_timer_deinit(struct pos_event_timer * et)
{
    if (et == NULL)
    {
        return POS_INVALID_PARAM;
    }

    if (event_timer_usable(et))
    {
        (void) pos_event_timer_stop(et);
    }
    et->evq = NULL;

    return POS_OK;
}

pos_error_t pos_event_timer_start(struct pos_event_timer * et, pos_time_t ticks)
{
    struct pos_eventq * evq;
    UBaseType_t state;
    bool wake;

    if (!event_timer_usable(et) || ticks > POS_EVENT_TIMER_MAX_TICKS)
    {
        return POS_INVALID_PARAM;
    }
    evq = et->evq;

    state = eventq_lock(evq);
    timer_unlink_locked(evq, et);
    event_unlink_locked(evq, &et->ev);
    et->expiry = pos_time_get() + ticks;
    timer_insert_locked(evq, et);
    /* A new earliest expiry must shorten the wait of a blocked consumer. */
    wake = (evq->timers == et) && (evq->waiters != 0);
    eventq_unlock(state);

    if (wake)
    {
        eventq_wake(evq);
    }

    return POS_OK;
}

pos_error_t pos_event_timer_stop(struct pos_event_timer * et)
{
    struct pos_eventq * evq;
    UBaseType_t state;

    if (!event_timer_usable(et))
    {
        return POS_INVALID_PARAM;
    }
    evq = et->evq;

    state = eventq_lock(evq);
    timer_unlink_locked(evq, et);
    event_unlink_locked(evq, &et->ev);
    eventq_unlock(state);

    return POS_OK;
}

pos_error_t pos_event_timer_inited(struct pos_event_timer * et)
{
    return (et != NULL && et->evq != NULL) ? POS_OK : POS_EINVAL;
}

bool pos_event_timer_is_active(struct pos_event_timer * et)
{
    UBaseType_t state;
    bool active;

    if (!event_timer_usable(et))
    {
        return false;
    }

    state  = eventq_lock(et->evq);
    active = et->armed;
    eventq_unlock(state);

    return active;
}

pos_time_t pos_event_timer_get_ticks(struct pos_event_timer * et)
{
    UBaseType_t state;
    pos_time_t expiry;

    if (!event_timer_usable(et))
    {
        return 0;
    }

    state  = eventq_lock(et->evq);
    expiry = et->expiry;
    eventq_unlock(state);

    return expiry;
}

struct pos_event * pos_event_timer_event_get(struct pos_event_timer * et)
{
    return (et != NULL) ? &et->ev : NULL;
}
