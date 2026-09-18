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
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

#include <poski/osal/osal.h>

pos_error_t pos_eventq_init(struct pos_eventq * evq)
{
    if (evq == NULL)
    {
        return POS_INVALID_PARAM;
    }

    memset(evq, 0, sizeof(*evq));
    k_fifo_init(&evq->fifo);
    evq->inited = true;

    return POS_OK;
}

pos_error_t pos_eventq_deinit(struct pos_eventq * evq)
{
    struct pos_event * ev;

    if (evq == NULL)
    {
        return POS_INVALID_PARAM;
    }

    while ((ev = (struct pos_event *) k_fifo_get(&evq->fifo, K_NO_WAIT)) != NULL)
    {
        ev->next   = NULL;
        ev->queued = false;
    }
    evq->inited = false;

    return POS_OK;
}

int pos_eventq_inited(const struct pos_eventq * evq)
{
    return (evq != NULL && evq->inited) ? 1 : 0;
}

pos_error_t pos_eventq_put(struct pos_eventq * evq, struct pos_event * ev)
{
    unsigned int key;

    if (evq == NULL || !evq->inited || ev == NULL)
    {
        return POS_INVALID_PARAM;
    }

    key = irq_lock();
    if (ev->queued)
    {
        irq_unlock(key);
        return POS_OK;
    }
    ev->queued = true;
    k_fifo_put(&evq->fifo, ev);
    irq_unlock(key);

    return POS_OK;
}

struct pos_event * pos_eventq_get(struct pos_eventq * evq, pos_time_t timeout)
{
    struct pos_event * ev;
    k_timeout_t tmo;
    unsigned int key;

    if (evq == NULL || !evq->inited)
    {
        return NULL;
    }

    if (timeout == POS_TIME_FOREVER)
    {
        tmo = K_FOREVER;
    }
    else if (timeout == POS_TIME_NO_WAIT)
    {
        tmo = K_NO_WAIT;
    }
    else
    {
        tmo = K_TICKS(timeout);
    }

    ev = (struct pos_event *) k_fifo_get(&evq->fifo, tmo);
    if (ev != NULL)
    {
        key        = irq_lock();
        ev->next   = NULL;
        ev->queued = false;
        irq_unlock(key);
    }

    return ev;
}

pos_error_t pos_eventq_remove(struct pos_eventq * evq, struct pos_event * ev)
{
    unsigned int key;

    if (evq == NULL || !evq->inited || ev == NULL)
    {
        return POS_INVALID_PARAM;
    }

    key = irq_lock();
    if (ev->queued)
    {
        sys_slist_find_and_remove(&evq->fifo._queue.data_q, (sys_snode_t *) ev);
        ev->next   = NULL;
        ev->queued = false;
    }
    irq_unlock(key);

    return POS_OK;
}

bool pos_eventq_is_empty(struct pos_eventq * evq)
{
    if (evq == NULL || !evq->inited)
    {
        return true;
    }

    return k_fifo_is_empty(&evq->fifo) != 0;
}

/* =========================================================================
 * Event Timer Implementation (Zephyr)
 * ========================================================================= */

static void zephyr_event_timer_expiry(struct k_timer * timer_id)
{
    struct pos_event_timer * et = CONTAINER_OF(timer_id, struct pos_event_timer, timer);
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
    k_timer_init(&et->timer, zephyr_event_timer_expiry, NULL);

    return POS_OK;
}

pos_error_t pos_event_timer_deinit(struct pos_event_timer * et)
{
    if (et == NULL)
    {
        return POS_INVALID_PARAM;
    }

    return pos_event_timer_stop(et);
}

pos_error_t pos_event_timer_start(struct pos_event_timer * et, pos_time_t ticks)
{
    if (et == NULL || et->evq == NULL)
    {
        return POS_INVALID_PARAM;
    }

    pos_eventq_remove(et->evq, &et->ev);
    et->ticks = pos_time_get() + ticks;
    k_timer_start(&et->timer, K_TICKS(ticks), K_NO_WAIT);

    return POS_OK;
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
    if (et == NULL)
    {
        return POS_INVALID_PARAM;
    }

    k_timer_stop(&et->timer);
    if (et->evq != NULL)
    {
        pos_eventq_remove(et->evq, &et->ev);
    }

    return POS_OK;
}

pos_error_t pos_event_timer_inited(struct pos_event_timer * et)
{
    return (et != NULL && et->evq != NULL) ? POS_OK : POS_ENOENT;
}

bool pos_event_timer_is_active(struct pos_event_timer * et)
{
    return (et != NULL) && (k_timer_remaining_ticks(&et->timer) > 0);
}

pos_time_t pos_event_timer_get_ticks(struct pos_event_timer * et)
{
    return (et != NULL) ? et->ticks : 0;
}

pos_time_t pos_event_timer_remaining_ticks(struct pos_event_timer * et, pos_time_t now)
{
    (void) now;
    return (et != NULL) ? (pos_time_t) k_timer_remaining_ticks(&et->timer) : 0;
}

void * pos_event_timer_arg_get(struct pos_event_timer * et)
{
    return (et != NULL) ? pos_event_arg_get(&et->ev) : NULL;
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
