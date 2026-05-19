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

/**
 *    @file
 *          Provides an definitions of CHIP OSAL data structures for portability
 *          to POSIX platforms.
 */

#ifndef POSKI_OS_POSIX_TIME_H
#define POSKI_OS_POSIX_TIME_H

#include <time.h>

#ifdef __APPLE__

#include <dispatch/dispatch.h>

// Apple MacOS
#define POS_TIME_FOREVER DISPATCH_TIME_FOREVER
#define POS_TIME_NO_WAIT 0
#define POS_TICKS_PER_SEC 1000000

typedef uint64_t pos_time_t;
typedef int64_t pos_stime_t;

#else

// Linux and stock POSIX
#define POS_TIME_FOREVER INT32_MAX
#define POS_TIME_NO_WAIT 0
#define POS_TICKS_PER_SEC 1000

typedef uint32_t pos_time_t;
typedef int32_t pos_stime_t;

#endif

struct pos_timer
{
    pos_timer_fn * tm_cb;
    pos_time_t tm_ticks;
#ifdef __APPLE__
    dispatch_source_t tm_timer;
#else
    timer_t tm_timer;
#endif
    bool tm_active;
    void * tm_arg;
};

#endif // POSKI_OS_POSIX_TIME_H
