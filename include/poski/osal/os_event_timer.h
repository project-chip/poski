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

/**
 * @file
 *   OSAL Event Timer (`pos_event_timer`) API, modeled on Apache Mynewt
 *   `os_callout`.
 *
 *   An event timer is a one-shot timer bound to an event queue
 *   (<poski/osal/os_eventq.h>).  When it expires, its embedded event
 *   (<poski/osal/os_event.h>) is posted to that queue, so the callback runs in
 *   the consumer task rather than in an ISR or timer-service context.
 *
 *   Unless noted otherwise, functions must be called from task context.  On
 *   RTOS backends, `pos_event_timer_start()` and `pos_event_timer_stop()` may
 *   also be called from an ISR.
 *
 *   Porting: a target defines `struct pos_event_timer` in its port headers and
 *   implements the non-inline functions declared here in `os_event_timer.c`.
 *   The inline helpers and the C++ wrapper (<poski/OsEventTimer.h>) are shared
 *   by all targets.
 */

#ifndef POSKI_OS_EVENT_TIMER_H
#define POSKI_OS_EVENT_TIMER_H

#include "poski/osal/os_event.h"
#include "poski/osal/os_eventq.h"
#include "poski/osal/os_time.h"
#include "poski/osal/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize an event timer bound to an event queue.
 *
 * When the timer expires, its embedded event (`fn`, `arg`) is posted to @a evq,
 * so @a fn runs in the task that services @a evq.  The timer is initially
 * stopped.
 *
 * @param et  Address of the event timer to initialize.
 * @param evq Event queue to post the expiry event to.
 * @param fn  Event callback function.
 * @param arg User argument of the event.
 *
 * @retval POS_OK            Event timer initialized.
 * @retval POS_INVALID_PARAM `et`, `evq`, or `fn` is NULL.
 */
pos_error_t pos_event_timer_init(struct pos_event_timer * et, struct pos_eventq * evq, pos_event_fn * fn, void * arg);

/**
 * @brief Stop an event timer and release any backend resources.
 *
 * The structure may be reused or freed once this returns.  Deinitializing a
 * timer that is not initialized (e.g. zero-filled) is a no-op.
 *
 * @param et Address of the event timer.
 *
 * @retval POS_OK            Event timer deinitialized.
 * @retval POS_INVALID_PARAM `et` is NULL.
 */
pos_error_t pos_event_timer_deinit(struct pos_event_timer * et);

/**
 * Longest delay, in ticks, accepted by `pos_event_timer_start()`: half the
 * range of `pos_time_t` (e.g. `INT32_MAX` for 32-bit ticks), so that expiry
 * times still compare correctly across tick-counter wraparound.
 */
#define POS_EVENT_TIMER_MAX_TICKS ((pos_time_t) (~(pos_time_t) 0) / 2)

/**
 * @brief Start, or restart, the event timer to expire @a ticks ticks from now.
 *
 * If the timer is already running it is rescheduled.  If the event from a
 * previous expiry is still pending in the queue it is removed first, so each
 * start delivers the event at most once.  A @a ticks of 0 expires as soon as
 * possible.
 *
 * @note May be called from an ISR on RTOS backends.
 *
 * @param et    Address of the event timer.
 * @param ticks Delay in OS ticks before the event is posted, at most
 *              `POS_EVENT_TIMER_MAX_TICKS`.
 *
 * @retval POS_OK            Timer started.
 * @retval POS_INVALID_PARAM `et` is NULL or not initialized, or @a ticks is
 *                           greater than `POS_EVENT_TIMER_MAX_TICKS`.
 */
pos_error_t pos_event_timer_start(struct pos_event_timer * et, pos_time_t ticks);

/**
 * @brief Start, or restart, the event timer to expire @a ms milliseconds from
 *        now.  See `pos_event_timer_start()`.
 */
static inline pos_error_t pos_event_timer_start_ms(struct pos_event_timer * et, pos_time_t ms)
{
    return pos_event_timer_start(et, pos_time_ms_to_ticks(ms));
}

/**
 * @brief Stop the event timer and remove its event from the queue if pending.
 *
 * Once this returns, the event will not be delivered for any earlier start
 * (its callback may still be running if the consumer had already dequeued it).
 * Stopping a timer that is not running is a no-op.
 *
 * @note May be called from an ISR on RTOS backends.
 *
 * @param et Address of the event timer.
 *
 * @retval POS_OK            Timer stopped.
 * @retval POS_INVALID_PARAM `et` is NULL or not initialized.
 */
pos_error_t pos_event_timer_stop(struct pos_event_timer * et);

/**
 * @brief Check whether the event timer is initialized.
 *
 * @param et Address of the event timer.
 *
 * @retval POS_OK     Event timer is initialized.
 * @retval POS_EINVAL `et` is NULL or not initialized.
 */
pos_error_t pos_event_timer_inited(struct pos_event_timer * et);

/**
 * @brief Return whether the event timer is running, i.e. started and its
 *        expiry event not yet posted.
 */
bool pos_event_timer_is_active(struct pos_event_timer * et);

/**
 * @brief Return the absolute expiry time, in `pos_time_get()` ticks, set by the
 *        most recent start.  Only meaningful while the timer is active.
 */
pos_time_t pos_event_timer_get_ticks(struct pos_event_timer * et);

/**
 * @brief Return the ticks remaining from @a now until the event timer expires,
 *        or 0 if the timer is not active or already due.
 */
static inline pos_time_t pos_event_timer_remaining_ticks(struct pos_event_timer * et, pos_time_t now)
{
    pos_time_t remaining;

    if (!pos_event_timer_is_active(et))
    {
        return 0;
    }

    /* Wrap-safe: an expiry at or before `now` yields a difference in the upper
     * half of the (unsigned) tick range. */
    remaining = (pos_time_t) (pos_event_timer_get_ticks(et) - now);
    return (remaining <= POS_EVENT_TIMER_MAX_TICKS) ? remaining : 0;
}

/**
 * @brief Return a pointer to the event timer's embedded event.
 */
struct pos_event * pos_event_timer_event_get(struct pos_event_timer * et);

/**
 * @brief Get the user argument of the event timer's event.
 */
static inline void * pos_event_timer_arg_get(struct pos_event_timer * et)
{
    return pos_event_arg_get(pos_event_timer_event_get(et));
}

/**
 * @brief Set the user argument of the event timer's event.
 */
static inline void pos_event_timer_arg_set(struct pos_event_timer * et, void * arg)
{
    pos_event_arg_set(pos_event_timer_event_get(et), arg);
}

#ifdef __cplusplus
}
#endif

#endif /* POSKI_OS_EVENT_TIMER_H */
