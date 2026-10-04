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
 *   OSAL Event (`pos_event`) API, modeled on Apache Mynewt `os_event`.
 *
 *   An event is an intrusive `{callback, argument}` record owned by the caller,
 *   typically embedded in (or statically allocated next to) the object that
 *   handles it.  The OSAL never allocates, copies, or frees events; event
 *   queues (<poski/osal/os_eventq.h>) only link them, so posting cannot fail
 *   for lack of memory.
 *
 *   `struct pos_event`, the inline helpers declared here, and the C++ wrapper
 *   (<poski/OsEvent.h>) are shared by all targets and need no target-specific
 *   code.  See <poski/osal/os_eventq.h> for event queues (`pos_eventq`) and
 *   <poski/osal/os_event_timer.h> for event timers (`pos_event_timer`).
 */

#ifndef POSKI_OS_EVENT_H
#define POSKI_OS_EVENT_H

#include "poski/osal/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Static initializer for a `struct pos_event`, equivalent to
 *        `pos_event_init()`.
 *
 * A statically allocated event initialized this way is constant-initialized
 * and needs no run-time setup:
 *
 * @code
 * static struct pos_event s_event = POS_EVENT_INITIALIZER(on_event, &s_ctx);
 * @endcode
 *
 * @param fn  Callback function invoked when the event is run.
 * @param arg User-defined context argument associated with the event.
 */
#define POS_EVENT_INITIALIZER(fn, arg) { NULL, (fn), (arg), false }

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

#ifdef __cplusplus
}
#endif

#endif /* POSKI_OS_EVENT_H */
