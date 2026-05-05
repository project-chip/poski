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

#ifndef CHIP_OS_TYPES_H
#define CHIP_OS_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void chip_os_timer_fn(void * arg);
typedef void chip_os_signal_fn(void * arg);
typedef void * (*chip_os_task_func_t)(void *);

enum chip_os_error
{
    CHIP_OS_OK              = 0,
    CHIP_OS_ENOMEM          = 1,
    CHIP_OS_EINVAL          = 2,
    CHIP_OS_INVALID_PARAM   = 3,
    CHIP_OS_MEM_NOT_ALIGNED = 4,
    CHIP_OS_BAD_MUTEX       = 5,
    CHIP_OS_TIMEOUT         = 6,
    CHIP_OS_ERR_IN_ISR      = 7,
    CHIP_OS_ERR_PRIV        = 8,
    CHIP_OS_OS_NOT_STARTED  = 9,
    CHIP_OS_ENOENT          = 10,
    CHIP_OS_EBUSY           = 11,
    CHIP_OS_ERROR           = 12,
};

typedef enum chip_os_error chip_os_error_t;

/* Include OS-specific definitions */
#include "chip/os_port.h"

#ifdef __cplusplus
}
#endif

#endif /* CHIP_OS_TYPES_H */
