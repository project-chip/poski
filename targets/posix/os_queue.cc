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

#include <poski/osal/osal.h>
#include "RingPthread.h"

extern "C" {

typedef RingPthread ring_t;

pos_error_t pos_queue_init(struct pos_queue * msgq, size_t msg_size, size_t max_msgs)
{
    msgq->q       = new ring_t(msg_size, max_msgs);
    msgq->sig_cb  = NULL;
    msgq->sig_arg = NULL;

    return (msgq->q) ? POS_OK : POS_ENOMEM;
}

void pos_queue_set_signal_cb(struct pos_queue * msgq, pos_signal_fn signal_cb, void * data)
{
    msgq->sig_cb  = signal_cb;
    msgq->sig_arg = data;
}

bool pos_queue_is_empty(struct pos_queue * msgq)
{
    ring_t * q = static_cast<ring_t *>(msgq->q);
    return (q->size() == 0);
}

int pos_queue_inited(const struct pos_queue * msgq)
{
    return (msgq->q != NULL);
}

pos_error_t pos_queue_put(struct pos_queue * msgq, void * data)
{
    ring_t * q = static_cast<ring_t *>(msgq->q);
    q->put(data);

    if (msgq->sig_cb)
    {
        msgq->sig_cb(msgq->sig_arg);
    }

    return POS_OK;
}

pos_error_t pos_queue_get(struct pos_queue * msgq, void * data, pos_time_t tmo)
{
    ring_t * q   = static_cast<ring_t *>(msgq->q);
    bool success = q->get(data, tmo);

    return (success) ? POS_OK : POS_TIMEOUT;
}

} // extern "C"
