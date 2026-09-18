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
 *   Formal OSAL specification for Event (`pos_event`), Event Queue (`pos_eventq`),
 *   and Event Timer (`pos_event_timer`, the modern replacement for BSD/Mynewt
 *   callout).
 */

#ifndef POSKI_OS_EVENT_H
#define POSKI_OS_EVENT_H

#include "poski/osal/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Event API (pos_event)
 * ========================================================================= */

/**
 * @brief Initialize an event structure.
 *
 * Prepares an intrusive event item with a callback function and user argument.
 *
 * @param ev  Pointer to the event structure to initialize.
 * @param fn  Callback function invoked when the event is executed.
 * @param arg User-defined context argument associated with the event.
 */
static inline void pos_event_init(struct pos_event * ev, pos_event_fn * fn, void * arg)
{
    if (ev != NULL)
    {
        ev->next   = NULL;
        ev->fn     = fn;
        ev->arg    = arg;
        ev->queued = false;
    }
}

/**
 * @brief Check whether an event is currently queued in an event queue.
 *
 * @param ev Pointer to the event to query.
 * @return true if the event is currently waiting in an event queue, false otherwise.
 */
static inline bool pos_event_is_queued(const struct pos_event * ev)
{
    return (ev != NULL) ? ev->queued : false;
}

/**
 * @brief Get the user-defined argument from an event.
 *
 * @param ev Pointer to the event.
 * @return User argument pointer.
 */
static inline void * pos_event_arg_get(const struct pos_event * ev)
{
    return (ev != NULL) ? ev->arg : NULL;
}

/**
 * @brief Alias of `pos_event_arg_get` for Mynewt/NPL compatibility.
 */
static inline void * pos_event_get_arg(const struct pos_event * ev)
{
    return pos_event_arg_get(ev);
}

/**
 * @brief Set the user-defined argument for an event.
 *
 * @param ev  Pointer to the event.
 * @param arg User argument pointer.
 */
static inline void pos_event_arg_set(struct pos_event * ev, void * arg)
{
    if (ev != NULL)
    {
        ev->arg = arg;
    }
}

/**
 * @brief Alias of `pos_event_arg_set` for Mynewt/NPL compatibility.
 */
static inline void pos_event_set_arg(struct pos_event * ev, void * arg)
{
    pos_event_arg_set(ev, arg);
}

/**
 * @brief Execute the event's callback function directly.
 *
 * @param ev Pointer to the event to run.
 */
static inline void pos_event_run(struct pos_event * ev)
{
    if (ev != NULL && ev->fn != NULL)
    {
        ev->fn(ev);
    }
}

/* =========================================================================
 * Event Queue API (pos_eventq)
 * ========================================================================= */

/**
 * @brief Initialize an event queue.
 *
 * @param evq Address of the event queue structure.
 *
 * @retval POS_OK            Event queue initialized successfully.
 * @retval POS_INVALID_PARAM `evq` is NULL.
 * @retval POS_ENOMEM        Out of resources.
 */
pos_error_t pos_eventq_init(struct pos_eventq * evq);

/**
 * @brief Deinitialize an event queue and release any associated OS resources.
 *
 * @param evq Address of the event queue structure.
 *
 * @retval POS_OK            Event queue deinitialized.
 * @retval POS_INVALID_PARAM `evq` is NULL.
 */
pos_error_t pos_eventq_deinit(struct pos_eventq * evq);

/**
 * @brief Check whether the given event queue is initialized and valid.
 *
 * @param evq Address of the event queue structure.
 * @return Non-zero (true) if initialized, 0 (false) otherwise.
 */
int pos_eventq_inited(const struct pos_eventq * evq);

/**
 * @brief Post an event to the tail of the event queue.
 *
 * Enqueuing is idempotent: if @a ev is already queued (`pos_event_is_queued(ev)`
 * is true), this call is a safe no-op and returns `POS_OK` without duplicating
 * the event on the queue.
 *
 * @note Safe to call from both task and ISR contexts on RTOS backends.
 *
 * @param evq Address of the target event queue.
 * @param ev  Address of the event to post.
 *
 * @retval POS_OK            Event posted (or already pending).
 * @retval POS_INVALID_PARAM `evq` or `ev` is NULL.
 */
pos_error_t pos_eventq_put(struct pos_eventq * evq, struct pos_event * ev);

/**
 * @brief Retrieve the next event from the head of the event queue.
 *
 * Clears the event's `queued` state before returning so the event callback can
 * safely re-post itself if desired.
 *
 * @param evq     Address of the event queue.
 * @param timeout Maximum ticks to wait (`POS_TIME_NO_WAIT`, `POS_TIME_FOREVER`,
 *                or a tick count).
 * @return Pointer to the dequeued `pos_event`, or `NULL` on timeout/empty.
 */
struct pos_event * pos_eventq_get(struct pos_eventq * evq, pos_time_t timeout);

/**
 * @brief Non-blocking poll for the next event from the event queue.
 *
 * @param evq Address of the event queue.
 * @return Pointer to the dequeued `pos_event`, or `NULL` if empty.
 */
static inline struct pos_event * pos_eventq_get_no_wait(struct pos_eventq * evq)
{
    return pos_eventq_get(evq, POS_TIME_NO_WAIT);
}

/**
 * @brief Remove a specific event from the event queue if it is currently queued.
 *
 * If @a ev is not currently queued in @a evq, this function is a safe no-op.
 *
 * @param evq Address of the event queue.
 * @param ev  Address of the event to remove.
 *
 * @retval POS_OK            Event removed (or was not queued).
 * @retval POS_INVALID_PARAM `evq` or `ev` is NULL.
 */
pos_error_t pos_eventq_remove(struct pos_eventq * evq, struct pos_event * ev);

/**
 * @brief Return whether the event queue currently contains no pending events.
 *
 * @param evq Address of the event queue.
 * @return true if empty, false if one or more events are pending.
 */
bool pos_eventq_is_empty(struct pos_eventq * evq);

/**
 * @brief Dequeue a single event (blocking up to `timeout`) and execute its callback.
 *
 * @param evq     Address of the event queue.
 * @param timeout Maximum ticks to wait for an event.
 *
 * @retval POS_OK      An event was dequeued and executed.
 * @retval POS_TIMEOUT No event arrived within `timeout`.
 */
static inline pos_error_t pos_eventq_poll(struct pos_eventq * evq, pos_time_t timeout)
{
    struct pos_event * ev = pos_eventq_get(evq, timeout);
    if (ev == NULL)
    {
        return POS_TIMEOUT;
    }
    pos_event_run(ev);
    return POS_OK;
}

/**
 * @brief Wait indefinitely for the next event on the queue and execute its callback.
 *
 * Equivalent to Mynewt `os_eventq_run(evq)`.
 *
 * @param evq Address of the event queue.
 */
static inline void pos_eventq_run(struct pos_eventq * evq)
{
    (void) pos_eventq_poll(evq, POS_TIME_FOREVER);
}

/* =========================================================================
 * Event Timer API (pos_event_timer - replaces Mynewt/NPL callout)
 * ========================================================================= */

/**
 * @brief Initialize an event timer.
 *
 * Associates an OS software timer with a target event queue (`evq`) and an
 * embedded event (`fn`, `arg`). When the timer expires, its internal event is
 * automatically posted to `evq` so the callback executes in the consumer task's
 * thread context rather than inside an ISR or timer daemon context.
 *
 * @param et  Address of the event timer to initialize.
 * @param evq Target event queue where expiration events are posted.
 * @param fn  Event callback function invoked when the event is processed.
 * @param arg User argument passed to the event.
 *
 * @retval POS_OK            Event timer initialized.
 * @retval POS_INVALID_PARAM Any required pointer is NULL.
 * @retval POS_ENOMEM        Timer creation failed.
 */
pos_error_t pos_event_timer_init(struct pos_event_timer * et,
                                 struct pos_eventq * evq,
                                 pos_event_fn * fn,
                                 void * arg);

/**
 * @brief Deinitialize an event timer and release underlying OS timer resources.
 *
 * @param et Address of the event timer.
 *
 * @retval POS_OK Event timer deinitialized.
 */
pos_error_t pos_event_timer_deinit(struct pos_event_timer * et);

/**
 * @brief Start or reset the event timer to expire after `ticks` OS ticks.
 *
 * If the event timer is already active or its event is currently pending in the
 * queue, it is cancelled and rescheduled for `ticks` in the future.
 *
 * @param et    Address of the event timer.
 * @param ticks Delay in OS ticks before posting the event to the event queue.
 *
 * @retval POS_OK            Timer armed.
 * @retval POS_INVALID_PARAM Invalid parameter.
 */
pos_error_t pos_event_timer_start(struct pos_event_timer * et, pos_time_t ticks);

/**
 * @brief Start or reset the event timer to expire after `ms` milliseconds.
 *
 * @param et Address of the event timer.
 * @param ms Delay in milliseconds before posting the event.
 *
 * @retval POS_OK Timer armed.
 */
pos_error_t pos_event_timer_start_ms(struct pos_event_timer * et, pos_time_t ms);

/**
 * @brief Aliases of `pos_event_timer_start` / `start_ms` (`arm`, `arm_ms`, `reset`).
 */
static inline pos_error_t pos_event_timer_arm(struct pos_event_timer * et, pos_time_t ticks)
{
    return pos_event_timer_start(et, ticks);
}

static inline pos_error_t pos_event_timer_arm_ms(struct pos_event_timer * et, pos_time_t ms)
{
    return pos_event_timer_start_ms(et, ms);
}

static inline pos_error_t pos_event_timer_reset(struct pos_event_timer * et, pos_time_t ticks)
{
    return pos_event_timer_start(et, ticks);
}

/**
 * @brief Stop a running event timer and remove its event from the queue if pending.
 *
 * Guarantees that if `pos_event_timer_stop()` is called before the consumer
 * thread dequeues the event, the event is removed from the target `pos_eventq`.
 *
 * @param et Address of the event timer.
 *
 * @retval POS_OK Timer stopped and any queued expiration event removed.
 */
pos_error_t pos_event_timer_stop(struct pos_event_timer * et);

/**
 * @brief Check whether the event timer is initialized.
 */
pos_error_t pos_event_timer_inited(struct pos_event_timer * et);

/**
 * @brief Return whether the event timer is currently active (counting down).
 */
bool pos_event_timer_is_active(struct pos_event_timer * et);

/**
 * @brief Return the absolute tick timestamp at which the event timer is scheduled to expire.
 */
pos_time_t pos_event_timer_get_ticks(struct pos_event_timer * et);

/**
 * @brief Return the remaining ticks until the event timer expires relative to `now`.
 */
pos_time_t pos_event_timer_remaining_ticks(struct pos_event_timer * et, pos_time_t now);

/**
 * @brief Get the user argument associated with the event timer's event.
 */
void * pos_event_timer_arg_get(struct pos_event_timer * et);

/**
 * @brief Set the user argument associated with the event timer's event.
 */
void pos_event_timer_arg_set(struct pos_event_timer * et, void * arg);

/**
 * @brief Return a pointer to the event timer's embedded `pos_event`.
 */
struct pos_event * pos_event_timer_event_get(struct pos_event_timer * et);

#ifdef __cplusplus
}
#endif

#endif /* POSKI_OS_EVENT_H */
