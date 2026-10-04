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
 * Zephyr event timers, mapped 1:1 onto struct k_timer.
 *
 * An event timer is a k_timer whose expiry function, run from the system clock
 * ISR, posts the embedded event to its bound pos_eventq.  A spinlock serializes
 * each timer's `armed` and `expiry` state so that stop() and restart() remove
 * a pending expiry event and an expiry that is already in flight (possible on
 * SMP, where k_timer_stop() does not wait for a running expiry function)
 * cannot post afterwards.
 */

#include <string.h>

#include <zephyr/kernel.h>

#include <poski/osal/os_event_timer.h>

static struct k_spinlock s_timer_lock;

/* Return true if tick time `now` is at or after `when` (wrap-safe). */
static bool time_reached(pos_time_t now, pos_time_t when)
{
    return (pos_time_t) (now - when) <= POS_EVENT_TIMER_MAX_TICKS;
}

static bool event_timer_usable(const struct pos_event_timer * et)
{
    return (et != NULL) && (pos_eventq_inited(et->evq) != 0);
}

/* k_timer expiry function; runs in the system clock ISR. */
static void event_timer_expiry(struct k_timer * timer)
{
    struct pos_event_timer * et = CONTAINER_OF(timer, struct pos_event_timer, timer);
    pos_time_t now              = pos_time_get();
    k_spinlock_key_t key;

    key = k_spin_lock(&s_timer_lock);
    /* Skip an expiry that raced with stop() (not armed), with a restart (not
     * yet due), or with deinit.  k_timer never expires early, so a current
     * expiry is always due.  pos_eventq_put() is safe under the spinlock here
     * because an ISR never switches threads inside k_queue_append(). */
    if (et->armed && et->evq != NULL && time_reached(now, et->expiry))
    {
        et->armed = false;
        (void) pos_eventq_put(et->evq, &et->ev);
    }
    k_spin_unlock(&s_timer_lock, key);
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

    k_timer_init(&et->timer, event_timer_expiry, NULL);
    pos_event_init(&et->ev, fn, arg);
    et->evq = evq;

    return POS_OK;
}

pos_error_t pos_event_timer_deinit(struct pos_event_timer * et)
{
    k_spinlock_key_t key;

    if (et == NULL)
    {
        return POS_INVALID_PARAM;
    }

    if (et->evq == NULL)
    {
        return POS_OK;
    }

    /* Stop the k_timer even if the queue is gone, so it cannot fire into
     * reused memory. */
    key       = k_spin_lock(&s_timer_lock);
    et->armed = false;
    (void) pos_eventq_remove(et->evq, &et->ev);
    et->evq = NULL;
    k_spin_unlock(&s_timer_lock, key);

    k_timer_stop(&et->timer);

    return POS_OK;
}

pos_error_t pos_event_timer_start(struct pos_event_timer * et, pos_time_t ticks)
{
    k_spinlock_key_t key;
    pos_time_t expiry;

    if (!event_timer_usable(et) || ticks > POS_EVENT_TIMER_MAX_TICKS)
    {
        return POS_INVALID_PARAM;
    }

    /* Drop a stale expiry event and re-arm before (re)starting the k_timer. */
    expiry = pos_time_get() + ticks;
    key    = k_spin_lock(&s_timer_lock);
    (void) pos_eventq_remove(et->evq, &et->ev);
    et->expiry = expiry;
    et->armed  = true;
    k_spin_unlock(&s_timer_lock, key);

    /* Restarts the k_timer if it is already running. */
    k_timer_start(&et->timer, K_TICKS(ticks), K_NO_WAIT);

    return POS_OK;
}

pos_error_t pos_event_timer_stop(struct pos_event_timer * et)
{
    k_spinlock_key_t key;

    if (!event_timer_usable(et))
    {
        return POS_INVALID_PARAM;
    }

    key       = k_spin_lock(&s_timer_lock);
    et->armed = false;
    (void) pos_eventq_remove(et->evq, &et->ev);
    k_spin_unlock(&s_timer_lock, key);

    k_timer_stop(&et->timer);

    return POS_OK;
}

pos_error_t pos_event_timer_inited(struct pos_event_timer * et)
{
    return (et != NULL && et->evq != NULL) ? POS_OK : POS_EINVAL;
}

bool pos_event_timer_is_active(struct pos_event_timer * et)
{
    k_spinlock_key_t key;
    bool active;

    if (!event_timer_usable(et))
    {
        return false;
    }

    key    = k_spin_lock(&s_timer_lock);
    active = et->armed;
    k_spin_unlock(&s_timer_lock, key);

    return active;
}

pos_time_t pos_event_timer_get_ticks(struct pos_event_timer * et)
{
    k_spinlock_key_t key;
    pos_time_t expiry;

    if (!event_timer_usable(et))
    {
        return 0;
    }

    key    = k_spin_lock(&s_timer_lock);
    expiry = et->expiry;
    k_spin_unlock(&s_timer_lock, key);

    return expiry;
}

struct pos_event * pos_event_timer_event_get(struct pos_event_timer * et)
{
    return (et != NULL) ? &et->ev : NULL;
}
