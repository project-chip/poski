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
 *   OSAL Event (`pos_event`), Event Queue (`pos_eventq`), and Event Timer
 *   (`pos_event_timer`) API, modeled on Apache Mynewt `os_event`, `os_eventq`,
 *   and `os_callout`.
 *
 *   - An event is an intrusive `{callback, argument}` record owned by the
 *     caller, typically embedded in (or statically allocated next to) the
 *     object that handles it.  The OSAL never allocates, copies, or frees
 *     events; queues only link them, so posting cannot fail for lack of memory.
 *   - An event queue is a FIFO of pending events that one or more consumer
 *     tasks drain with `pos_eventq_get()` / `pos_eventq_run()`.  Posting an
 *     event that is already pending is a no-op, so an event is never queued
 *     twice.
 *   - An event timer is a one-shot timer bound to an event queue.  When it
 *     expires, its embedded event is posted to that queue, so the callback runs
 *     in the consumer task rather than in an ISR or timer-service context.
 *
 *   Unless noted otherwise, functions must be called from task context.  On
 *   RTOS backends, `pos_eventq_put()`, `pos_eventq_remove()`,
 *   `pos_event_timer_start()`, and `pos_event_timer_stop()` may also be called
 *   from an ISR, and `pos_eventq_get()` may be called from an ISR with
 *   `POS_TIME_NO_WAIT`.
 */

#ifndef POSKI_OS_EVENT_H
#define POSKI_OS_EVENT_H

#include "poski/osal/os_time.h"
#include "poski/osal/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Event API (pos_event)
 * ========================================================================= */

/**
 * @brief Initialize an event with a callback function and user argument.
 *
 * @param ev  Pointer to the event structure to initialize.
 * @param fn  Callback function invoked when the event is run.
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
 * @brief Check whether an event is currently pending in an event queue.
 *
 * The result is a snapshot: another task, an ISR, or an expiring event timer
 * may change it at any time.
 *
 * @param ev Pointer to the event to query.
 * @return true if the event is pending in an event queue, false otherwise.
 */
static inline bool pos_event_is_queued(const struct pos_event * ev)
{
    return (ev != NULL) ? ev->queued : false;
}

/**
 * @brief Get the user-defined argument of an event.
 *
 * @param ev Pointer to the event.
 * @return User argument pointer.
 */
static inline void * pos_event_arg_get(const struct pos_event * ev)
{
    return (ev != NULL) ? ev->arg : NULL;
}

/**
 * @brief Set the user-defined argument of an event.
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
 * @brief Run the event's callback function directly in the calling context.
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
 * Pending events are dropped (they are no longer marked as queued).  Event
 * timers bound to the queue must be stopped or deinitialized first, and no
 * other task may be using the queue.  Deinitializing a queue that is not
 * initialized is a no-op.
 *
 * @param evq Address of the event queue structure.
 *
 * @retval POS_OK            Event queue deinitialized.
 * @retval POS_INVALID_PARAM `evq` is NULL.
 */
pos_error_t pos_eventq_deinit(struct pos_eventq * evq);

/**
 * @brief Check whether the given event queue is initialized.
 *
 * @param evq Address of the event queue structure.
 * @return Non-zero (true) if initialized, 0 (false) otherwise.
 */
int pos_eventq_inited(const struct pos_eventq * evq);

/**
 * @brief Post an event to the tail of the event queue.
 *
 * Posting is idempotent: if @a ev is already queued, this call is a no-op and
 * returns `POS_OK` without moving or duplicating the event.
 *
 * @note May be called from an ISR on RTOS backends.
 *
 * @param evq Address of the target event queue.
 * @param ev  Address of the event to post.
 *
 * @retval POS_OK            Event posted (or already pending).
 * @retval POS_INVALID_PARAM `evq` or `ev` is NULL, or `evq` is not initialized.
 */
pos_error_t pos_eventq_put(struct pos_eventq * evq, struct pos_event * ev);

/**
 * @brief Remove and return the event at the head of the event queue, waiting
 *        up to @a timeout ticks for one to arrive.
 *
 * The event is marked as no longer queued before it is returned, so its
 * callback may safely re-post it.
 *
 * @note May be called from an ISR on RTOS backends, but @a timeout must be
 *       `POS_TIME_NO_WAIT`.
 *
 * @param evq     Address of the event queue.
 * @param timeout Maximum ticks to wait (`POS_TIME_NO_WAIT`, `POS_TIME_FOREVER`,
 *                or a tick count).
 * @return Pointer to the dequeued event, or `NULL` on timeout (or if `evq` is
 *         NULL or not initialized).
 */
struct pos_event * pos_eventq_get(struct pos_eventq * evq, pos_time_t timeout);

/**
 * @brief Remove and return the event at the head of the event queue without
 *        waiting.
 *
 * @param evq Address of the event queue.
 * @return Pointer to the dequeued event, or `NULL` if the queue is empty.
 */
static inline struct pos_event * pos_eventq_get_no_wait(struct pos_eventq * evq)
{
    return pos_eventq_get(evq, POS_TIME_NO_WAIT);
}

/**
 * @brief Remove a specific event from the event queue if it is pending there.
 *
 * If @a ev is not pending in @a evq, this call is a no-op.
 *
 * @note May be called from an ISR on RTOS backends.
 *
 * @param evq Address of the event queue.
 * @param ev  Address of the event to remove.
 *
 * @retval POS_OK            Event removed (or was not queued).
 * @retval POS_INVALID_PARAM `evq` or `ev` is NULL, or `evq` is not initialized.
 */
pos_error_t pos_eventq_remove(struct pos_eventq * evq, struct pos_event * ev);

/**
 * @brief Return whether the event queue currently has no pending events.
 *
 * @param evq Address of the event queue.
 * @return true if empty (or if `evq` is NULL or not initialized), false if one
 *         or more events are pending.
 */
bool pos_eventq_is_empty(struct pos_eventq * evq);

/**
 * @brief Dequeue one event (waiting up to @a timeout ticks) and run its callback.
 *
 * @param evq     Address of the event queue.
 * @param timeout Maximum ticks to wait for an event.
 *
 * @retval POS_OK      An event was dequeued and run.
 * @retval POS_TIMEOUT No event arrived within @a timeout.
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
 * @brief Wait indefinitely for the next event on the queue and run its callback.
 *
 * Equivalent to Mynewt `os_eventq_run(evq)`; typically called in a loop by the
 * task that owns the queue.
 *
 * @param evq Address of the event queue.
 */
static inline void pos_eventq_run(struct pos_eventq * evq)
{
    (void) pos_eventq_poll(evq, POS_TIME_FOREVER);
}

/* =========================================================================
 * Event Timer API (pos_event_timer, replaces the Mynewt/NPL callout)
 * ========================================================================= */

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

#endif /* POSKI_OS_EVENT_H */
