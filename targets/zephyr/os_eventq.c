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
 * Zephyr event queue, mapped 1:1 onto struct k_queue.
 *
 * An event queue is a k_queue of intrusive events, so posting never allocates
 * and consumers block in k_queue_get().  A file-scope spinlock serializes each
 * event's `queued` flag so that posting an event that is already pending is a
 * no-op.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sflist.h>

#include <poski/osal/os_eventq.h>

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

    if (evq == NULL || ev == NULL)
    {
        return POS_INVALID_PARAM;
    }

    key = k_spin_lock(&s_event_lock);
    if (!evq->inited)
    {
        k_spin_unlock(&s_event_lock, key);
        return POS_INVALID_PARAM;
    }
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

    if (evq == NULL || ev == NULL)
    {
        return POS_INVALID_PARAM;
    }

    key = k_spin_lock(&s_event_lock);
    if (!evq->inited)
    {
        k_spin_unlock(&s_event_lock, key);
        return POS_INVALID_PARAM;
    }
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
