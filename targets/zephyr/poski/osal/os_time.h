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

#ifndef POSKI_OS_ZEPHYR_TIME_H
#define POSKI_OS_ZEPHYR_TIME_H

#include <zephyr/kernel.h>

#define POS_TIME_FOREVER 0xFFFFFFFF
#define POS_TIME_NO_WAIT 0
#define POS_TICKS_PER_SEC CONFIG_SYS_CLOCK_TICKS_PER_SEC

typedef uint32_t pos_time_t;
typedef int32_t pos_stime_t;

struct pos_timer
{
    struct k_timer timer;
    void (*cb)(void *);
    void * arg;
};

#endif // POSKI_OS_ZEPHYR_TIME_H
