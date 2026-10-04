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
 *   OSAL Event Queue (`pos_eventq`) API, modeled on Apache Mynewt `os_eventq`.
 *
 *   An event queue is a FIFO of pending intrusive events (<poski/osal/os_event.h>)
 *   that one or more consumer tasks drain with `pos_eventq_get()`,
 *   `pos_eventq_poll()`, or `pos_eventq_run()`.  Posting an event that is
 *   already pending is a no-op, so an event is never queued twice.
 *
 *   Unless noted otherwise, functions must be called from task context.  On
 *   RTOS backends, `pos_eventq_put()` and `pos_eventq_remove()` may also be
 *   called from an ISR, and `pos_eventq_get()` may be called from an ISR with
 *   `POS_TIME_NO_WAIT`.
 *
 *   Porting: a target defines `struct pos_eventq` in its port headers and
 *   implements the non-inline functions declared here in `os_eventq.c`.  The
 *   inline helpers and the C++ wrapper (<poski/OsEventQueue.h>) are shared by
 *   all targets.
 */

#ifndef POSKI_OS_EVENTQ_H
#define POSKI_OS_EVENTQ_H

#include "poski/osal/os_event.h"
#include "poski/osal/os_time.h"
#include "poski/osal/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* POSKI_OS_EVENTQ_H */
