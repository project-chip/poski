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

#include <time.h>
#include <errno.h>
#include <semaphore.h>

#include <poski/osal/osal.h>
#include "os_utils.h"

#ifdef __APPLE__

pos_error_t pos_sem_init(struct pos_sem * sem, uint16_t tokens)
{
    sem->lock = dispatch_semaphore_create(tokens);
    return (sem->lock == NULL) ? POS_ENOMEM : POS_OK;
}

pos_error_t pos_sem_give(struct pos_sem * sem)
{
    int woke = dispatch_semaphore_signal(sem->lock);
    (void) woke;
    return POS_OK;
}

pos_error_t pos_sem_take(struct pos_sem * sem, pos_time_t timeout)
{
    int expired = dispatch_semaphore_wait(sem->lock, timeout);
    return (expired) ? POS_TIMEOUT : POS_OK;
}

#else

pos_error_t pos_sem_init(struct pos_sem * sem, uint16_t tokens)
{
    int ret = sem_init(&sem->lock, 0, tokens);
    return map_posix_to_osal_error(ret);
}

pos_error_t pos_sem_give(struct pos_sem * sem)
{
    int ret = sem_post(&sem->lock);
    return map_posix_to_osal_error(ret);
}

pos_error_t pos_sem_take(struct pos_sem * sem, pos_time_t timeout)
{
    int ret;
    struct timespec wait;

    if (timeout == POS_TIME_FOREVER)
    {
        ret = sem_wait(&sem->lock);
        SuccessOrExit(ret);
    }
    else
    {
        ret = clock_gettime(CLOCK_REALTIME, &wait);
        SuccessOrExit(ret);

        wait.tv_sec += timeout / 1000;
        wait.tv_nsec += (timeout % 1000) * 1000000;
        ret = sem_timedwait(&sem->lock, &wait);
        ret = (ret) ? errno : POS_OK;
        SuccessOrExit(ret);
    }

exit:
    return map_posix_to_osal_error(ret);
}

#endif // __APPLE__
