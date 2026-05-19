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

#ifndef POSKI_OS_SCHED_H
#define POSKI_OS_SCHED_H

#include "poski/osal/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the task scheduler.
 *
 * This routine will begin execution of all initialized @a tasks.
 *
 * @note This routine will never return.
 */
void pos_sched_start(void);

/**
 * @brief Returns whether the task scheduler has been started.
 *
 * @return true is started, false otherwise.
 */
bool pos_sched_started(void);

#ifdef __cplusplus
}
#endif

#endif /* POSKI_OS_SCHED_H */
