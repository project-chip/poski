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

#ifndef CHIP_OS_SEM_H
#define CHIP_OS_SEM_H

#include "chip/os_types.h"

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
chip_os_error_t chip_os_sem_init(struct chip_os_sem * sem, uint16_t tokens);

/**
 * @brief Take a semaphore.
 *
 * This routine takes @a sem.
 *
 * @note Can be called by ISRs, but @a timeout must be set to CHIP_OS_TIME_NO_WAIT.
 *
 * @param sem Address of the semaphore.
 * @param timeout Waiting period to take the semaphore (in milliseconds),
 *                or one of the special values CHIP_OS_TIME_NO_WAIT and CHIP_OS_TIME_FOREVER.
 *
 * @retval CHIP_OS_OK Semaphore taken.
 * @retval CHIP_OS_EBUSY Returned without waiting.
 * @retval CHIP_OS_TIMEOUT Waiting period timed out.
 */
chip_os_error_t chip_os_sem_take(struct chip_os_sem * sem, chip_os_time_t timeout);

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
chip_os_error_t chip_os_sem_give(struct chip_os_sem * sem);

#ifdef __cplusplus
}
#endif

#endif /* CHIP_OS_SEM_H */
