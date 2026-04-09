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

#include <chip/osal.h>

chip_os_error_t chip_os_sem_init(struct chip_os_sem * sem, uint16_t tokens)
{
    k_sem_init(&sem->sem, tokens, K_SEM_MAX_LIMIT);
    return CHIP_OS_OK;
}

chip_os_error_t chip_os_sem_take(struct chip_os_sem * sem, chip_os_time_t timeout)
{
    k_timeout_t tmo;
    if (timeout == CHIP_OS_TIME_FOREVER) {
        tmo = K_FOREVER;
    } else if (timeout == CHIP_OS_TIME_NO_WAIT) {
        tmo = K_NO_WAIT;
    } else {
        tmo = K_MSEC(timeout);
    }

    int err = k_sem_take(&sem->sem, tmo);
    if (err == 0) {
        return CHIP_OS_OK;
    } else if (err == -EAGAIN) {
        return CHIP_OS_TIMEOUT;
    } else {
        return CHIP_OS_ERROR;
    }
}

chip_os_error_t chip_os_sem_give(struct chip_os_sem * sem)
{
    k_sem_give(&sem->sem);
    return CHIP_OS_OK;
}
