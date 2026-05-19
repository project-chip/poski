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

#ifndef POSKI_OS_SEM_H
#define POSKI_OS_SEM_H

#include "poski/osal/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize a semaphore.
 *
 * This routine initializes a semaphore object, prior to its first use.
 *
 * @param sem Address of the semaphore.
 * @param tokens Initial semaphore count.
 *
 * @return N/A
 */
pos_error_t pos_sem_init(struct pos_sem * sem, uint16_t tokens);

/**
 * @brief Take a semaphore.
 *
 * This routine takes @a sem.
 *
 * @note Can be called by ISRs, but @a timeout must be set to POS_TIME_NO_WAIT.
 *
 * @param sem Address of the semaphore.
 * @param timeout Waiting period to take the semaphore (in milliseconds),
 *                or one of the special values POS_TIME_NO_WAIT and POS_TIME_FOREVER.
 *
 * @retval POS_OK Semaphore taken.
 * @retval POS_EBUSY Returned without waiting.
 * @retval POS_TIMEOUT Waiting period timed out.
 */
pos_error_t pos_sem_take(struct pos_sem * sem, pos_time_t timeout);

/**
 * @brief Give a semaphore.
 *
 * This routine gives @a sem, unless the semaphore is already at its maximum
 * permitted count.
 *
 * @note Can be called by ISRs.
 *
 * @param sem Address of the semaphore.
 *
 * @return N/A
 */
pos_error_t pos_sem_give(struct pos_sem * sem);

#ifdef __cplusplus
}
#endif

#endif /* POSKI_OS_SEM_H */
