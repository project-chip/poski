/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2022 tangweikang, <BalanceTWK@yeah.net>
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

#include <poski/osal/osal.h>
#include "os_hw.h"

pos_error_t pos_queue_init(struct pos_queue * msgq, size_t msg_size, size_t max_msgs)
{
    if (!msgq)
    {
        return POS_INVALID_PARAM;
    }

    msgq->handle = rt_mq_create("chip_msgq", (rt_size_t)msg_size, (rt_size_t)max_msgs, RT_IPC_FLAG_PRIO);
    assert(msgq->handle);

    return (msgq->handle == NULL) ? POS_ERROR : POS_OK;
}

pos_error_t pos_queue_get(struct pos_queue * queue, void * data, pos_time_t tmo)
{
    rt_err_t ret;

    assert(queue->handle);

    ret = rt_mq_recv(queue->handle,data ,queue->handle->msg_size, tmo);

    return ret == RT_EOK ? POS_OK : POS_ERROR;
}

pos_error_t pos_queue_put(struct pos_queue * queue, void * data)
{
    if (!queue)
    {
        return POS_INVALID_PARAM;
    }

    assert(queue->handle);

    if (rt_mq_send(queue->handle, data, queue->handle->msg_size) != RT_EOK)
    {
        return POS_ERROR;
    }

    return POS_OK;
}
