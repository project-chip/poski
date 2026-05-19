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

pos_error_t pos_mutex_init(struct pos_mutex * mu)
{
    if (!mu)
    {
        return POS_INVALID_PARAM;
    }

    mu->handle = rt_mutex_create("chip_mutex",RT_IPC_FLAG_PRIO);
    assert(mu->handle);

    return (mu->handle == NULL) ? POS_BAD_MUTEX : POS_OK;
}

pos_error_t pos_mutex_lock(struct pos_mutex * mu, pos_time_t timeout)
{
    rt_err_t ret;

    if (!mu)
    {
        return POS_INVALID_PARAM;
    }

    assert(mu->handle);

    ret = rt_mutex_take(mu->handle, timeout);

    return ret == RT_EOK ? POS_OK : POS_BAD_MUTEX;
}

pos_error_t pos_mutex_unlock(struct pos_mutex * mu)
{
    assert(!pos_hw_in_isr());

    if (!mu)
    {
        return POS_INVALID_PARAM;
    }

    assert(mu->handle);

    if (rt_mutex_release(mu->handle) != RT_EOK)
    {
        return POS_BAD_MUTEX;
    }

    return POS_OK;
}
