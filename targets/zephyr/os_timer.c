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

#include <poski/osal/osal.h>

static void zephyr_timer_wrapper(struct k_timer * timer_id)
{
    struct pos_timer * timer = CONTAINER_OF(timer_id, struct pos_timer, timer);
    if (timer->cb)
    {
        timer->cb(timer->arg);
    }
}

pos_error_t pos_timer_init(struct pos_timer * timer, pos_timer_fn cb, void * arg)
{
    if (timer == NULL || cb == NULL)
    {
        return POS_INVALID_PARAM;
    }
    k_timer_init(&timer->timer, zephyr_timer_wrapper, NULL);
    timer->cb  = cb;
    timer->arg = arg;
    return POS_OK;
}

pos_error_t pos_timer_start_ms(struct pos_timer * timer, pos_time_t duration)
{
    if (timer == NULL)
    {
        return POS_INVALID_PARAM;
    }
    k_timer_start(&timer->timer, K_MSEC(duration), K_NO_WAIT);
    return POS_OK;
}

pos_error_t pos_timer_start(struct pos_timer * timer, pos_time_t ticks)
{
    if (timer == NULL)
    {
        return POS_INVALID_PARAM;
    }
    k_timer_start(&timer->timer, K_TICKS(ticks), K_NO_WAIT);
    return POS_OK;
}

pos_error_t pos_timer_stop(struct pos_timer * timer)
{
    if (timer == NULL)
    {
        return POS_INVALID_PARAM;
    }
    k_timer_stop(&timer->timer);
    return POS_OK;
}

pos_error_t pos_timer_inited(struct pos_timer * timer)
{
    return (timer != NULL) ? POS_OK : POS_ENOENT;
}

bool pos_timer_is_active(struct pos_timer * timer)
{
    return (timer != NULL) && (k_timer_remaining_ticks(&timer->timer) > 0);
}

pos_time_t pos_timer_get_ticks(struct pos_timer * timer)
{
    if (timer == NULL)
    {
        return 0;
    }
    return pos_time_get() + (pos_time_t) k_timer_remaining_ticks(&timer->timer);
}

pos_time_t pos_timer_remaining_ticks(struct pos_timer * timer, pos_time_t time)
{
    (void) time;
    return (timer != NULL) ? (pos_time_t) k_timer_remaining_ticks(&timer->timer) : 0;
}

void pos_timer_arg_set(struct pos_timer * timer, void * arg)
{
    if (timer != NULL)
    {
        timer->arg = arg;
    }
}

void * pos_timer_arg_get(struct pos_timer * timer)
{
    return (timer != NULL) ? timer->arg : NULL;
}
