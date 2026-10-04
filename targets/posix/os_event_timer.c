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
 * POSIX event timers.
 *
 * Event timers bound to a queue are kept on the queue's `timers` list, sorted
 * by expiry, under the queue's mutex.  The queue (os_eventq.c) expires due
 * timers and bounds blocking gets to the earliest pending expiry, so timers
 * need no OS resources or helper threads, and start/stop/restart cannot race
 * with an expiry that is already in flight.
 */

#include <pthread.h>
#include <string.h>

#include <poski/osal/os_event_timer.h>

/* Return true if tick time `now` is at or after `when` (wrap-safe). */
static bool time_reached(pos_time_t now, pos_time_t when)
{
    return (pos_time_t) (now - when) <= POS_EVENT_TIMER_MAX_TICKS;
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

static bool event_timer_usable(const struct pos_event_timer * et)
{
    return (et != NULL) && (pos_eventq_inited(et->evq) != 0);
}

/* =========================================================================
 * Event Timer
 * ========================================================================= */

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

    if (!event_timer_usable(et) || ticks > POS_EVENT_TIMER_MAX_TICKS)
    {
        return POS_INVALID_PARAM;
    }
    evq = et->evq;

    pthread_mutex_lock(&evq->lock);
    timer_unlink_locked(evq, et);
    pthread_mutex_unlock(&evq->lock);

    (void) pos_eventq_remove(evq, &et->ev);

    pthread_mutex_lock(&evq->lock);
    et->expiry = pos_time_get() + ticks;
    timer_insert_locked(evq, et);
    if (evq->timers == et)
    {
        /* New earliest expiry: let blocked consumers shorten their wait. */
        pthread_cond_broadcast(&evq->cond);
    }
    pthread_mutex_unlock(&evq->lock);

    return POS_OK;
}

pos_error_t pos_event_timer_stop(struct pos_event_timer * et)
{
    struct pos_eventq * evq;

    if (!event_timer_usable(et))
    {
        return POS_INVALID_PARAM;
    }
    evq = et->evq;

    pthread_mutex_lock(&evq->lock);
    timer_unlink_locked(evq, et);
    pthread_mutex_unlock(&evq->lock);

    (void) pos_eventq_remove(evq, &et->ev);

    return POS_OK;
}

pos_error_t pos_event_timer_inited(struct pos_event_timer * et)
{
    return (et != NULL && et->evq != NULL) ? POS_OK : POS_EINVAL;
}

bool pos_event_timer_is_active(struct pos_event_timer * et)
{
    bool active;

    if (!event_timer_usable(et))
    {
        return false;
    }

    /* Bring the queue up to date so an already-due timer is expired first. */
    (void) pos_eventq_is_empty(et->evq);

    pthread_mutex_lock(&et->evq->lock);
    active = et->armed;
    pthread_mutex_unlock(&et->evq->lock);

    return active;
}

pos_time_t pos_event_timer_get_ticks(struct pos_event_timer * et)
{
    pos_time_t expiry;

    if (!event_timer_usable(et))
    {
        return 0;
    }

    pthread_mutex_lock(&et->evq->lock);
    expiry = et->expiry;
    pthread_mutex_unlock(&et->evq->lock);

    return expiry;
}

struct pos_event * pos_event_timer_event_get(struct pos_event_timer * et)
{
    return (et != NULL) ? &et->ev : NULL;
}
