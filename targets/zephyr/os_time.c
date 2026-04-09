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

#include <chip/osal.h>

chip_os_time_t chip_os_time_get(void)
{
    return (chip_os_time_t)k_uptime_ticks();
}

chip_os_time_t chip_os_time_get_ms(void)
{
    return k_uptime_get_32();
}

chip_os_time_t chip_os_time_ms_to_ticks(chip_os_time_t ms)
{
    return k_ms_to_ticks_ceil32(ms);
}

chip_os_time_t chip_os_time_ticks_to_ms(chip_os_time_t ticks)
{
    return k_ticks_to_ms_floor32(ticks);
}
