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
 * POSIX event queue and event timers.
 *
 * All state is owned by the event queue and protected by its mutex.  Event
 * timers bound to a queue are kept on the queue's `timers` list, sorted by
 * expiry.  Every queue operation first moves the events of expired timers onto
 * the FIFO (soonest first, so they stay ordered relative to events posted
 * afterwards), and a blocking get waits no longer than the earliest pending
 * expiry.  Timers therefore need no OS resources or helper threads, and
 * start/stop/restart cannot race with an expiry that is already in flight.
 */

#include <pthread.h>
#include <string.h>
#include <time.h>

#include <poski/osal/osal.h>

#ifdef __APPLE__
/* pthread_cond_timedwait() always uses CLOCK_REALTIME on macOS. */
#define EVENTQ_CLOCK CLOCK_REALTIME
#else
#define EVENTQ_CLOCK CLOCK_MONOTONIC
#endif

#define NSEC_PER_SEC 1000000000ULL

/* Return true if tick time `now` is at or after `when` (wrap-safe). */
static bool time_reached(pos_time_t now, pos_time_t when)
{
    return (pos_time_t) (now - when) <= POS_EVENT_TIMER_MAX_TICKS;
}

/* =========================================================================
 * Internal helpers.  The caller must hold evq->lock.
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
static void timers_expire_locked(struct pos_eventq * evq, pos_time_t now)
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

/* Lock the queue and bring it up to date by posting any expired timers. */
static void eventq_lock(struct pos_eventq * evq)
{
    pthread_mutex_lock(&evq->lock);
    timers_expire_locked(evq, pos_time_get());
}

static void eventq_unlock(struct pos_eventq * evq)
{
    pthread_mutex_unlock(&evq->lock);
}

/* Wait on the queue's condition variable forever, or for at most `ticks`. */
static void eventq_wait_locked(struct pos_eventq * evq, bool forever, pos_time_t ticks)
{
    struct timespec ts;
    uint64_t nsec;

    if (forever)
    {
        pthread_cond_wait(&evq->cond, &evq->lock);
        return;
    }

    clock_gettime(EVENTQ_CLOCK, &ts);
    nsec = (uint64_t) ts.tv_nsec + (uint64_t) (ticks % POS_TICKS_PER_SEC) * (NSEC_PER_SEC / POS_TICKS_PER_SEC);
    ts.tv_sec += (time_t) (ticks / POS_TICKS_PER_SEC) + (time_t) (nsec / NSEC_PER_SEC);
    ts.tv_nsec = (long) (nsec % NSEC_PER_SEC);

    /* Timeouts and spurious wakeups are handled by the caller's loop. */
    (void) pthread_cond_timedwait(&evq->cond, &evq->lock, &ts);
}

/* =========================================================================
 * Event Queue
 * ========================================================================= */

pos_error_t pos_eventq_init(struct pos_eventq * evq)
{
    pthread_condattr_t attr;
    int ret;

    if (evq == NULL)
    {
        return POS_INVALID_PARAM;
    }

    memset(evq, 0, sizeof(*evq));

    ret = pthread_mutex_init(&evq->lock, NULL);
    if (ret != 0)
    {
        return POS_ENOMEM;
    }

    ret = pthread_condattr_init(&attr);
    if (ret != 0)
    {
        pthread_mutex_destroy(&evq->lock);
        return POS_ENOMEM;
    }

#ifndef __APPLE__
    pthread_condattr_setclock(&attr, EVENTQ_CLOCK);
#endif

    ret = pthread_cond_init(&evq->cond, &attr);
    pthread_condattr_destroy(&attr);
    if (ret != 0)
    {
        pthread_mutex_destroy(&evq->lock);
        return POS_ENOMEM;
    }

    evq->inited = true;
    return POS_OK;
}

pos_error_t pos_eventq_deinit(struct pos_eventq * evq)
{
    if (evq == NULL)
    {
        return POS_INVALID_PARAM;
    }

    if (!evq->inited)
    {
        return POS_OK;
    }

    pthread_mutex_lock(&evq->lock);
    while (event_pop_locked(evq) != NULL)
    {
    }
    while (evq->timers != NULL)
    {
        timer_unlink_locked(evq, evq->timers);
    }
    evq->inited = false;
    pthread_cond_broadcast(&evq->cond);
    pthread_mutex_unlock(&evq->lock);

    pthread_cond_destroy(&evq->cond);
    pthread_mutex_destroy(&evq->lock);

    return POS_OK;
}

int pos_eventq_inited(const struct pos_eventq * evq)
{
    return (evq != NULL && evq->inited) ? 1 : 0;
}

pos_error_t pos_eventq_put(struct pos_eventq * evq, struct pos_event * ev)
{
    if (evq == NULL || !evq->inited || ev == NULL)
    {
        return POS_INVALID_PARAM;
    }

    eventq_lock(evq);
    event_append_locked(evq, ev);
    pthread_cond_signal(&evq->cond);
    eventq_unlock(evq);

    return POS_OK;
}

struct pos_event * pos_eventq_get(struct pos_eventq * evq, pos_time_t timeout)
{
    struct pos_event * ev;
    pos_time_t start;

    if (evq == NULL || !evq->inited)
    {
        return NULL;
    }

    pthread_mutex_lock(&evq->lock);
    start = pos_time_get();

    for (;;)
    {
        pos_time_t now  = pos_time_get();
        bool forever    = (timeout == POS_TIME_FOREVER);
        pos_time_t wait = 0;

        timers_expire_locked(evq, now);
        ev = event_pop_locked(evq);
        if (ev != NULL || !evq->inited || timeout == POS_TIME_NO_WAIT)
        {
            break;
        }

        if (!forever)
        {
            pos_time_t elapsed = (pos_time_t) (now - start);
            if (elapsed >= timeout)
            {
                break;
            }
            wait = timeout - elapsed;
        }

        /* Wake up in time to post the next timer expiry.  It is in the future,
         * because expired timers were just posted. */
        if (evq->timers != NULL)
        {
            pos_time_t until = (pos_time_t) (evq->timers->expiry - now);
            if (forever || until < wait)
            {
                forever = false;
                wait    = until;
            }
        }

        eventq_wait_locked(evq, forever, wait);
    }

    pthread_mutex_unlock(&evq->lock);
    return ev;
}

pos_error_t pos_eventq_remove(struct pos_eventq * evq, struct pos_event * ev)
{
    if (evq == NULL || !evq->inited || ev == NULL)
    {
        return POS_INVALID_PARAM;
    }

    eventq_lock(evq);
    event_unlink_locked(evq, ev);
    eventq_unlock(evq);

    return POS_OK;
}

bool pos_eventq_is_empty(struct pos_eventq * evq)
{
    bool empty;

    if (evq == NULL || !evq->inited)
    {
        return true;
    }

    eventq_lock(evq);
    empty = (evq->head == NULL);
    eventq_unlock(evq);

    return empty;
}

/* =========================================================================
 * Event Timer
 * ========================================================================= */

static bool event_timer_usable(const struct pos_event_timer * et)
{
    return (et != NULL) && (et->evq != NULL) && et->evq->inited;
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

    if (!event_timer_usable(et) || ticks > POS_EVENT_TIMER_MAX_TICKS)
    {
        return POS_INVALID_PARAM;
    }
    evq = et->evq;

    eventq_lock(evq);
    timer_unlink_locked(evq, et);
    event_unlink_locked(evq, &et->ev);
    et->expiry = pos_time_get() + ticks;
    timer_insert_locked(evq, et);
    if (evq->timers == et)
    {
        /* New earliest expiry: let blocked consumers shorten their wait. */
        pthread_cond_broadcast(&evq->cond);
    }
    eventq_unlock(evq);

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

    eventq_lock(evq);
    timer_unlink_locked(evq, et);
    event_unlink_locked(evq, &et->ev);
    eventq_unlock(evq);

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

    eventq_lock(et->evq);
    active = et->armed;
    eventq_unlock(et->evq);

    return active;
}

pos_time_t pos_event_timer_get_ticks(struct pos_event_timer * et)
{
    pos_time_t expiry;

    if (!event_timer_usable(et))
    {
        return 0;
    }

    eventq_lock(et->evq);
    expiry = et->expiry;
    eventq_unlock(et->evq);

    return expiry;
}

struct pos_event * pos_event_timer_event_get(struct pos_event_timer * et)
{
    return (et != NULL) ? &et->ev : NULL;
}
