/*
 *
 *    Copyright (c) 2026 Project CHIP Authors
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
#include <stdlib.h>

pos_error_t pos_queue_init(struct pos_queue * msgq, size_t msg_size, size_t max_msgs)
{
    msgq->buffer = malloc(msg_size * max_msgs);
    if (!msgq->buffer) {
        return POS_ENOMEM;
    }

    k_msgq_init(&msgq->msgq, msgq->buffer, msg_size, max_msgs);
    msgq->sig_cb = NULL;
    msgq->sig_arg = NULL;

    return POS_OK;
}

pos_error_t pos_queue_get(struct pos_queue * msgq, void * data, pos_time_t timeout)
{
    k_timeout_t tmo;
    if (timeout == POS_TIME_FOREVER) {
        tmo = K_FOREVER;
    } else if (timeout == POS_TIME_NO_WAIT) {
        tmo = K_NO_WAIT;
    } else {
        tmo = K_MSEC(timeout);
    }

    int err = k_msgq_get(&msgq->msgq, data, tmo);
    if (err == 0) {
        return POS_OK;
    } else if (err == -EAGAIN) {
        return POS_TIMEOUT;
    } else {
        return POS_ERROR;
    }
}

pos_error_t pos_queue_put(struct pos_queue * msgq, void * data)
{
    int err = k_msgq_put(&msgq->msgq, data, K_NO_WAIT);
    
    if (err == 0) {
        if (msgq->sig_cb) {
            msgq->sig_cb(msgq->sig_arg);
        }
        return POS_OK;
    } else if (err == -EAGAIN) {
        return POS_EBUSY;
    } else {
        return POS_ERROR;
    }
}

int pos_queue_inited(const struct pos_queue * msgq)
{
    return (msgq->buffer != NULL);
}

bool pos_queue_is_empty(struct pos_queue * msgq)
{
    return k_msgq_num_used_get(&msgq->msgq) == 0;
}

void pos_queue_set_signal_cb(struct pos_queue * msgq, pos_signal_fn signal_cb, void * data)
{
    msgq->sig_cb = signal_cb;
    msgq->sig_arg = data;
}
