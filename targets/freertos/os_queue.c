/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2018 Google LLC
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "os_hw.h"

#include <poski/osal/osal.h>

pos_error_t pos_queue_get(struct pos_queue * queue, void * data, pos_time_t tmo)
{
    BaseType_t ret;

    if (queue == NULL || queue->handle == NULL || data == NULL)
        return POS_INVALID_PARAM;

    if (pos_hw_in_isr())
    {
        BaseType_t woken = pdFALSE;
        ret = xQueueReceiveFromISR(queue->handle, data, &woken);
        portYIELD_FROM_ISR(woken);
    }
    else
    {
        ret = xQueueReceive(queue->handle, data, tmo);
    }
    /* Distinguish timeout from successful receive. */
    return ret == pdPASS ? POS_OK : POS_TIMEOUT;
}

pos_error_t pos_queue_put(struct pos_queue * queue, void * data)
{
    BaseType_t ret;

    if (queue == NULL || queue->handle == NULL || data == NULL)
        return POS_INVALID_PARAM;

    if (pos_hw_in_isr())
    {
        BaseType_t woken = pdFALSE;
        ret = xQueueSendToBackFromISR(queue->handle, data, &woken);
        portYIELD_FROM_ISR(woken);
    }
    else
    {
        /* Timeout 0 + POS_EBUSY on full -- no blocking forever on
         * portMAX_DELAY, no configASSERT crash. */
        ret = xQueueSendToBack(queue->handle, data, 0);
    }
    if (ret != pdPASS)
        return POS_EBUSY;

    /* Post-send notification, registered via
     * pos_queue_set_signal_cb. */
    if (queue->signal_cb != NULL)
        queue->signal_cb(queue->signal_data);

    return POS_OK;
}

pos_error_t pos_queue_deinit(struct pos_queue * queue)
{
    if (queue && queue->handle)
    {
        vQueueDelete(queue->handle);
        queue->handle = NULL;
    }
    return POS_OK;
}

/* Moved here from os_port.h to live next to the other queue ops.
 * Adds the NULL-handle check that xQueueCreate failure silently
 * dropped on the inline version, and initialises the new
 * signal_cb / signal_data fields. */
pos_error_t pos_queue_init(struct pos_queue * q, size_t msg_size, size_t max_msgs)
{
    if (q == NULL)
        return POS_INVALID_PARAM;
    q->handle = xQueueCreate(max_msgs, msg_size);
    q->signal_cb = NULL;
    q->signal_data = NULL;
    if (q->handle == NULL)
        return POS_ENOMEM;
    return POS_OK;
}

/* Return true when no messages are pending.  Treats an invalid
 * queue as empty so callers can poll a fresh pos_queue without
 * first checking pos_queue_inited. */
bool pos_queue_is_empty(struct pos_queue * q)
{
    if (q == NULL || q->handle == NULL)
        return true;
    if (pos_hw_in_isr())
        return uxQueueMessagesWaitingFromISR(q->handle) == 0;
    return uxQueueMessagesWaiting(q->handle) == 0;
}

/* Register a post-send notification.  The actual invocation
 * happens inside pos_queue_put after a successful enqueue. */
void pos_queue_set_signal_cb(struct pos_queue * q,
                             pos_signal_fn signal_cb, void * data)
{
    if (q == NULL)
        return;
    q->signal_cb = signal_cb;
    q->signal_data = data;
}
