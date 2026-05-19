/*
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2018 Google LLC
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

#ifndef POSKI_OS_MUTEX_H
#define POSKI_OS_MUTEX_H

#include "poski/osal/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize a mutex.
 *
 * This routine initializes a mutex object, prior to its first use.
 *
 * Upon completion, the mutex is available and does not have an owner.
 *
 * @param mutex Address of the mutex.
 *
 * @return N/A
 */
pos_error_t pos_mutex_init(struct pos_mutex * mutex);

/**
 * @brief Lock a mutex.
 *
 * This routine locks @a mutex. If the mutex is locked by another task,
 * the calling task waits until the mutex becomes available or until
 * a timeout occurs.
 *
 * A task is permitted to lock a mutex it has already locked. The operation
 * completes immediately and the lock count is increased by 1.
 *
 * @param mutex Address of the mutex.
 * @param timeout Waiting period to lock the mutex (in milliseconds),
 *                or one of the special values POS_TIME_NO_WAIT and POS_TIME_FOREVER.
 *
 * @retval POS_OK Mutex locked.
 * @retval POS_EBUSY Returned without waiting.
 * @retval POS_TIMEOUT Waiting period timed out.
 */
pos_error_t pos_mutex_take(struct pos_mutex * mutex, pos_time_t timeout);

/**
 * @brief Unlock a mutex.
 *
 * This routine unlocks @a mutex. The mutex must already be locked by the
 * calling task.
 *
 * The mutex cannot be claimed by another task until it has been unlocked by
 * the calling task as many times as it was previously locked by that
 * task.
 *
 * @param mutex Address of the mutex.
 *
 * @return N/A
 */
pos_error_t pos_mutex_give(struct pos_mutex * mutex);

#ifdef __cplusplus
}
#endif

#endif /* POSKI_OS_MUTEX_H */
