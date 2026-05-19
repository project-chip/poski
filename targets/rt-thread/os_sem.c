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

pos_error_t pos_sem_init(struct pos_sem * sem, uint16_t tokens)
{
    if (!sem)
    {
        return POS_INVALID_PARAM;
    }

    sem->handle = rt_sem_create("chip_sem", tokens, RT_IPC_FLAG_PRIO);
    assert(sem->handle);

    return POS_OK;
}

pos_error_t pos_sem_take(struct pos_sem * sem, pos_time_t timeout)
{
    rt_err_t ret;

    if (!sem)
    {
        return POS_INVALID_PARAM;
    }

    assert(sem->handle);

    ret = rt_sem_take(sem->handle, timeout);

    return ret == RT_EOK ? POS_OK : POS_TIMEOUT;
}

pos_error_t pos_sem_give(struct pos_sem * sem)
{
    rt_err_t ret;

    if (!sem)
    {
        return POS_INVALID_PARAM;
    }

    assert(sem->handle);

    ret = rt_sem_release(sem->handle);

    return ret == RT_EOK ? POS_OK : POS_ERROR;
}
