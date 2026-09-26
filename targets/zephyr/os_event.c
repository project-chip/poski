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
 * Zephyr event queue and event timers, mapped 1:1 onto kernel objects.
 *
 * An event queue is a k_queue of intrusive events, so posting never allocates
 * and consumers block in k_queue_get().  An event timer is a k_timer whose
 * expiry function, run from the system clock ISR, posts the embedded event.
 *
 * A file-scope spinlock serializes each event's `queued` flag and each timer's
 * `armed` state:
 *  - posting an event that is already pending is a no-op;
 *  - stop() and restart() remove a pending expiry event, and an expiry that is
 *    already in flight (possible on SMP, where k_timer_stop() does not wait
 *    for a running expiry function) cannot post afterwards, because the expiry
 *    function only posts while its timer is armed and due.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/sflist.h>

#include <poski/osal/osal.h>

static struct k_spinlock s_event_lock;

static k_timeout_t to_timeout(pos_time_t ticks)
{
    if (ticks == POS_TIME_FOREVER)
    {
        return K_FOREVER;
    }
    if (ticks == POS_TIME_NO_WAIT)
    {
        return K_NO_WAIT;
    }
    return K_TICKS(ticks);
}

/* Return true if tick time `now` is at or after `when` (wrap-safe). */
static bool time_reached(pos_time_t now, pos_time_t when)
{
    return (pos_time_t) (now - when) <= POS_EVENT_TIMER_MAX_TICKS;
}

/*
 * Unlink a pending event from its queue.  The caller holds s_event_lock.
 *
 * k_queue_remove() does not take the queue's lock in Zephyr releases (through
 * at least v4.4), which races with k_queue_get() on another CPU, so unlink the
 * node under the queue's own lock, exactly as newer kernels do internally.
 */
static void event_remove_locked(struct pos_eventq * evq, struct pos_event * ev)
{
    k_spinlock_key_t key;
    bool removed;

    if (!ev->queued)
    {
        return;
    }

    key     = k_spin_lock(&evq->queue.lock);
    removed = sys_sflist_find_and_remove(&evq->queue.data_q, (sys_sfnode_t *) ev);
    k_spin_unlock(&evq->queue.lock, key);

    /* If not found, a consumer has already dequeued the event and is about to
     * clear `queued` itself. */
    if (removed)
    {
        ev->next   = NULL;
        ev->queued = false;
    }
}

/* =========================================================================
 * Event Queue
 * ========================================================================= */

pos_error_t pos_eventq_init(struct pos_eventq * evq)
{
    if (evq == NULL)
    {
        return POS_INVALID_PARAM;
    }

    k_queue_init(&evq->queue);
    evq->inited = true;

    return POS_OK;
}

pos_error_t pos_eventq_deinit(struct pos_eventq * evq)
{
    struct pos_event * ev;
    k_spinlock_key_t key;

    if (evq == NULL)
    {
        return POS_INVALID_PARAM;
    }

    if (!evq->inited)
    {
        return POS_OK;
    }

    /* Timers still bound to the queue stop posting to it. */
    key         = k_spin_lock(&s_event_lock);
    evq->inited = false;
    k_spin_unlock(&s_event_lock, key);

    while ((ev = k_queue_get(&evq->queue, K_NO_WAIT)) != NULL)
    {
        key        = k_spin_lock(&s_event_lock);
        ev->next   = NULL;
        ev->queued = false;
        k_spin_unlock(&s_event_lock, key);
    }

    return POS_OK;
}

int pos_eventq_inited(const struct pos_eventq * evq)
{
    return (evq != NULL && evq->inited) ? 1 : 0;
}

pos_error_t pos_eventq_put(struct pos_eventq * evq, struct pos_event * ev)
{
    k_spinlock_key_t key;
    bool post;

    if (evq == NULL || !evq->inited || ev == NULL)
    {
        return POS_INVALID_PARAM;
    }

    key        = k_spin_lock(&s_event_lock);
    post       = !ev->queued;
    ev->queued = true;
    k_spin_unlock(&s_event_lock, key);

    /* Append outside the spinlock: from a thread, k_queue_append() may switch
     * straight to the consumer it wakes. */
    if (post)
    {
        k_queue_append(&evq->queue, ev);
    }

    return POS_OK;
}

struct pos_event * pos_eventq_get(struct pos_eventq * evq, pos_time_t timeout)
{
    struct pos_event * ev;
    k_spinlock_key_t key;

    if (evq == NULL || !evq->inited)
    {
        return NULL;
    }

    /* ISRs cannot block: poll once. */
    ev = k_queue_get(&evq->queue, k_is_in_isr() ? K_NO_WAIT : to_timeout(timeout));
    if (ev != NULL)
    {
        key        = k_spin_lock(&s_event_lock);
        ev->next   = NULL;
        ev->queued = false;
        k_spin_unlock(&s_event_lock, key);
    }

    return ev;
}

pos_error_t pos_eventq_remove(struct pos_eventq * evq, struct pos_event * ev)
{
    k_spinlock_key_t key;

    if (evq == NULL || !evq->inited || ev == NULL)
    {
        return POS_INVALID_PARAM;
    }

    key = k_spin_lock(&s_event_lock);
    event_remove_locked(evq, ev);
    k_spin_unlock(&s_event_lock, key);

    return POS_OK;
}

bool pos_eventq_is_empty(struct pos_eventq * evq)
{
    if (evq == NULL || !evq->inited)
    {
        return true;
    }

    return k_queue_is_empty(&evq->queue) != 0;
}

/* =========================================================================
 * Event Timer
 * ========================================================================= */

static bool event_timer_usable(const struct pos_event_timer * et)
{
    return (et != NULL) && (et->evq != NULL) && et->evq->inited;
}

/* k_timer expiry function; runs in the system clock ISR. */
static void event_timer_expiry(struct k_timer * timer)
{
    struct pos_event_timer * et = CONTAINER_OF(timer, struct pos_event_timer, timer);
    pos_time_t now              = pos_time_get();
    struct pos_eventq * evq;
    k_spinlock_key_t key;

    key = k_spin_lock(&s_event_lock);
    evq = et->evq;
    /* Skip an expiry that raced with stop() (not armed), with a restart (not
     * yet due), or with deinit.  k_timer never expires early, so a current
     * expiry is always due. */
    if (et->armed && evq != NULL && evq->inited && time_reached(now, et->expiry))
    {
        et->armed = false;
        if (!et->ev.queued)
        {
            et->ev.queued = true;
            /* Appending under the spinlock is fine here: an ISR never switches
             * threads inside k_queue_append(). */
            k_queue_append(&evq->queue, &et->ev);
        }
    }
    k_spin_unlock(&s_event_lock, key);
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
    key       = k_spin_lock(&s_event_lock);
    et->armed = false;
    if (et->evq->inited)
    {
        event_remove_locked(et->evq, &et->ev);
    }
    et->evq = NULL;
    k_spin_unlock(&s_event_lock, key);

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
    key    = k_spin_lock(&s_event_lock);
    event_remove_locked(et->evq, &et->ev);
    et->expiry = expiry;
    et->armed  = true;
    k_spin_unlock(&s_event_lock, key);

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

    key       = k_spin_lock(&s_event_lock);
    et->armed = false;
    event_remove_locked(et->evq, &et->ev);
    k_spin_unlock(&s_event_lock, key);

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

    key    = k_spin_lock(&s_event_lock);
    active = et->armed;
    k_spin_unlock(&s_event_lock, key);

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

    key    = k_spin_lock(&s_event_lock);
    expiry = et->expiry;
    k_spin_unlock(&s_event_lock, key);

    return expiry;
}

struct pos_event * pos_event_timer_event_get(struct pos_event_timer * et)
{
    return (et != NULL) ? &et->ev : NULL;
}
