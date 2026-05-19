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
#include <poski/osal/osal.h>
#include "os_hw.h"

pos_error_t pos_sem_init(struct pos_sem * sem, uint16_t tokens)
{
    if (!sem)
    {
        return POS_INVALID_PARAM;
    }

    sem->handle = xSemaphoreCreateCounting(128, tokens);
    assert(sem->handle);

    return POS_OK;
}

pos_error_t pos_sem_take(struct pos_sem * sem, pos_time_t timeout)
{
    BaseType_t woken;
    BaseType_t ret;

    if (!sem)
    {
        return POS_INVALID_PARAM;
    }

    assert(sem->handle);

    if (pos_hw_in_isr())
    {
        assert(timeout == 0);
        ret = xSemaphoreTakeFromISR(sem->handle, &woken);
        portYIELD_FROM_ISR(woken);
    }
    else
    {
        ret = xSemaphoreTake(sem->handle, timeout);
    }

    return ret == pdPASS ? POS_OK : POS_TIMEOUT;
}

pos_error_t pos_sem_give(struct pos_sem * sem)
{
    BaseType_t ret;
    BaseType_t woken;

    if (!sem)
    {
        return POS_INVALID_PARAM;
    }

    assert(sem->handle);

    if (pos_hw_in_isr())
    {
        ret = xSemaphoreGiveFromISR(sem->handle, &woken);
        portYIELD_FROM_ISR(woken);
    }
    else
    {
        ret = xSemaphoreGive(sem->handle);
    }

    assert(ret == pdPASS);
    return POS_OK;
}
