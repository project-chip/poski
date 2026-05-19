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

#include <poski/osal/osal.h>

#define ONE_THOUSAND 1000.0
#define ONE_BILLION 1000000000.0
#define POS_TICKS_PER_SEC_F ((double) POS_TICKS_PER_SEC)
#define POS_TICKS_PER_MILLISEC_F (POS_TICKS_PER_SEC_F * ONE_THOUSAND)
#define POS_TICKS_PER_NANOSEC_F (POS_TICKS_PER_SEC_F * ONE_BILLION)

#ifdef __APPLE__
// OS X does not have clock_gettime, use clock_get_time

#include <mach/clock.h>
#include <mach/mach.h>

pos_time_t pos_time_get(void)
{
    pos_time_t ticks;
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ticks = mts.tv_sec * POS_TICKS_PER_SEC_F + mts.tv_nsec / POS_TICKS_PER_NANOSEC_F;
    return ticks;
}

#else
// True POSIX Implementation

pos_time_t pos_time_get(void)
{
    pos_time_t ticks;
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now))
    {
        return 0;
    }
    ticks = now.tv_sec * POS_TICKS_PER_SEC_F + now.tv_nsec / POS_TICKS_PER_NANOSEC_F;
    return ticks;
}

#endif // __APPLE__

pos_time_t pos_time_ms_to_ticks(pos_time_t ms)
{
    return (ms * POS_TICKS_PER_SEC_F) / ONE_THOUSAND;
}

pos_time_t pos_time_ticks_to_ms(pos_time_t ticks)
{
    return (ticks * ONE_THOUSAND) / POS_TICKS_PER_SEC_F;
}

pos_time_t pos_time_get_ms(void)
{
    return pos_time_ticks_to_ms(pos_time_get());
}
