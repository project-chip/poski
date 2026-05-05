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

static void zephyr_timer_wrapper(struct k_timer * timer_id)
{
    struct chip_os_timer * timer = CONTAINER_OF(timer_id, struct chip_os_timer, timer);
    if (timer->cb) {
        timer->cb(timer->arg);
    }
}

chip_os_error_t chip_os_timer_init(struct chip_os_timer * timer, chip_os_timer_fn cb, void * arg)
{
    k_timer_init(&timer->timer, zephyr_timer_wrapper, NULL);
    timer->cb = cb;
    timer->arg = arg;
    return CHIP_OS_OK;
}

chip_os_error_t chip_os_timer_start_ms(struct chip_os_timer * timer, chip_os_time_t duration)
{
    k_timer_start(&timer->timer, K_MSEC(duration), K_NO_WAIT);
    return CHIP_OS_OK;
}

chip_os_error_t chip_os_timer_start(struct chip_os_timer * timer, chip_os_time_t ticks)
{
    k_timer_start(&timer->timer, K_TICKS(ticks), K_NO_WAIT);
    return CHIP_OS_OK;
}

chip_os_error_t chip_os_timer_stop(struct chip_os_timer * timer)
{
    k_timer_stop(&timer->timer);
    return CHIP_OS_OK;
}

chip_os_error_t chip_os_timer_inited(struct chip_os_timer * timer)
{
    return CHIP_OS_OK;
}

bool chip_os_timer_is_active(struct chip_os_timer * timer)
{
    return k_timer_remaining_ticks(&timer->timer) > 0;
}

chip_os_time_t chip_os_timer_get_ticks(struct chip_os_timer * timer)
{
    // Not easily supported by Zephyr without storing it.
    return 0;
}

chip_os_time_t chip_os_timer_remaining_ticks(struct chip_os_timer * timer, chip_os_time_t time)
{
    return (chip_os_time_t)k_timer_remaining_ticks(&timer->timer);
}

void chip_os_timer_arg_set(struct chip_os_timer * timer, void * arg)
{
    timer->arg = arg;
}

void * chip_os_timer_arg_get(struct chip_os_timer * timer)
{
    return timer->arg;
}
