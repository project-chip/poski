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

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#include <poski/osal/osal.h>

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
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
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
    struct pos_event * cur;

    if (evq == NULL)
    {
        return POS_INVALID_PARAM;
    }

    if (!evq->inited)
    {
        return POS_OK;
    }

    pthread_mutex_lock(&evq->lock);
    cur = evq->head;
    while (cur != NULL)
    {
        struct pos_event * next = cur->next;
        cur->next               = NULL;
        cur->queued             = false;
        cur                     = next;
    }
    evq->head   = NULL;
    evq->tail   = NULL;
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

    pthread_mutex_lock(&evq->lock);

    /* Idempotent enqueue: if already queued, do not corrupt the linked list. */
    if (ev->queued)
    {
        pthread_mutex_unlock(&evq->lock);
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

    pthread_cond_signal(&evq->cond);
    pthread_mutex_unlock(&evq->lock);

    return POS_OK;
}

struct pos_event * pos_eventq_get(struct pos_eventq * evq, pos_time_t timeout)
{
    struct pos_event * ev = NULL;

    if (evq == NULL || !evq->inited)
    {
        return NULL;
    }

    pthread_mutex_lock(&evq->lock);

    if (evq->head == NULL)
    {
        if (timeout == POS_TIME_NO_WAIT)
        {
            pthread_mutex_unlock(&evq->lock);
            return NULL;
        }
        else if (timeout == POS_TIME_FOREVER)
        {
            while (evq->head == NULL && evq->inited)
            {
                pthread_cond_wait(&evq->cond, &evq->lock);
            }
        }
        else
        {
            struct timespec ts;
            uint64_t wait_ms = pos_time_ticks_to_ms(timeout);
            uint64_t nsec;

#ifdef __APPLE__
            clock_gettime(CLOCK_REALTIME, &ts);
#else
            clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
            ts.tv_sec += (time_t) (wait_ms / 1000);
            nsec = (uint64_t) ts.tv_nsec + (wait_ms % 1000) * 1000000ULL;
            ts.tv_sec += (time_t) (nsec / 1000000000ULL);
            ts.tv_nsec = (long) (nsec % 1000000000ULL);

            while (evq->head == NULL && evq->inited)
            {
                int rc = pthread_cond_timedwait(&evq->cond, &evq->lock, &ts);
                if (rc == ETIMEDOUT)
                {
                    break;
                }
            }
        }
    }

    ev = evq->head;
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

    pthread_mutex_unlock(&evq->lock);
    return ev;
}

pos_error_t pos_eventq_remove(struct pos_eventq * evq, struct pos_event * ev)
{
    struct pos_event * prev = NULL;
    struct pos_event * cur;

    if (evq == NULL || !evq->inited || ev == NULL)
    {
        return POS_INVALID_PARAM;
    }

    pthread_mutex_lock(&evq->lock);

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
                break;
            }
            prev = cur;
            cur  = cur->next;
        }
    }

    pthread_mutex_unlock(&evq->lock);
    return POS_OK;
}

bool pos_eventq_is_empty(struct pos_eventq * evq)
{
    bool empty;

    if (evq == NULL || !evq->inited)
    {
        return true;
    }

    pthread_mutex_lock(&evq->lock);
    empty = (evq->head == NULL);
    pthread_mutex_unlock(&evq->lock);

    return empty;
}

/* =========================================================================
 * Event Timer Implementation (POSIX)
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

#ifndef __APPLE__
    if (et->timer.tm_timer != NULL)
    {
        timer_delete(et->timer.tm_timer);
        et->timer.tm_timer = NULL;
    }
#endif

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
    if (et == NULL)
    {
        return POS_INVALID_PARAM;
    }

    return pos_timer_inited(&et->timer);
}

bool pos_event_timer_is_active(struct pos_event_timer * et)
{
    if (et == NULL)
    {
        return false;
    }

    return pos_timer_is_active(&et->timer);
}

pos_time_t pos_event_timer_get_ticks(struct pos_event_timer * et)
{
    if (et == NULL)
    {
        return 0;
    }

    return pos_timer_get_ticks(&et->timer);
}

pos_time_t pos_event_timer_remaining_ticks(struct pos_event_timer * et, pos_time_t now)
{
    pos_time_t exp;

    if (et == NULL || !pos_timer_is_active(&et->timer))
    {
        return 0;
    }

    exp = pos_timer_get_ticks(&et->timer);
    return (exp > now) ? (exp - now) : 0;
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
