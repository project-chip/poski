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

#ifndef CHIP_OS_TIME_H
#define CHIP_OS_TIME_H

#include "poski/osal/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get system uptime.
 *
 * This routine returns the elapsed time since the system booted,
 * in milliseconds.
 *
 * @return Current uptime.
 */
chip_os_time_t chip_os_time_get(void);

chip_os_time_t chip_os_time_get_ms(void);

chip_os_time_t chip_os_time_ms_to_ticks(chip_os_time_t ms);

chip_os_time_t chip_os_time_ticks_to_ms(chip_os_time_t ticks);

#ifdef __cplusplus
}
#endif

#endif /* CHIP_OS_TIME_H */
