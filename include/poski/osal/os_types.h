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

#ifndef POSKI_OS_TYPES_H
#define POSKI_OS_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void pos_timer_fn(void * arg);
typedef void pos_signal_fn(void * arg);
typedef void * (*pos_task_func_t)(void *);

enum pos_error
{
    POS_OK              = 0,
    POS_ENOMEM          = 1,
    POS_EINVAL          = 2,
    POS_INVALID_PARAM   = 3,
    POS_MEM_NOT_ALIGNED = 4,
    POS_BAD_MUTEX       = 5,
    POS_TIMEOUT         = 6,
    POS_ERR_IN_ISR      = 7,
    POS_ERR_PRIV        = 8,
    POS_OS_NOT_STARTED  = 9,
    POS_ENOENT          = 10,
    POS_EBUSY           = 11,
    POS_ERROR           = 12,
};

typedef enum pos_error pos_error_t;

/* Include OS-specific definitions */
#include "poski/osal/os_port.h"

#ifdef __cplusplus
}
#endif

#endif /* POSKI_OS_TYPES_H */
