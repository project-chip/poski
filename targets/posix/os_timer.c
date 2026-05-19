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

#include <signal.h>

#include <poski/osal/osal.h>
#include "os_utils.h"

#ifdef __APPLE__

#define DEFAULT_TIMER_LEEWAY 100000 //< 100 [us]

static void pos_timer_cb(void * arg)
{
    struct pos_timer * timer = (struct pos_timer *) arg;
    assert(timer);

    pos_timer_stop(timer);

    timer->tm_cb(timer->tm_arg);
}

pos_error_t pos_timer_init(struct pos_timer * timer, pos_timer_fn * tm_cb, void * tm_arg)
{
    dispatch_source_t dispatch = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler_f(dispatch, pos_timer_cb);
    dispatch_set_context(dispatch, timer);

    /* Initialize the timer. */
    memset(timer, 0, sizeof(*timer));
    timer->tm_cb     = tm_cb;
    timer->tm_arg    = tm_arg;
    timer->tm_active = false;
    timer->tm_timer  = dispatch;

    return POS_OK;
}

pos_error_t pos_timer_start(struct pos_timer * timer, pos_time_t ticks)
{
    pos_error_t err;
    dispatch_source_t dispatch;
    pos_time_t delay_ns;

    err = pos_timer_inited(timer);
    SuccessOrExit(err);

    dispatch = timer->tm_timer;
    err      = (dispatch == NULL) ? POS_EINVAL : POS_OK;
    SuccessOrExit(err);

    timer->tm_ticks = pos_time_get() + ticks;
    delay_ns        = ticks * (1000000000.0 / POS_TICKS_PER_SEC);
    dispatch_source_set_timer(dispatch, dispatch_time(DISPATCH_TIME_NOW, delay_ns), DISPATCH_TIME_FOREVER, DEFAULT_TIMER_LEEWAY);
    dispatch_resume(dispatch);
    timer->tm_active = true;

exit:
    return err;
}

pos_error_t pos_timer_stop(struct pos_timer * timer)
{
    pos_error_t err;

    err = pos_timer_inited(timer);
    SuccessOrExit(err);

    err = (timer->tm_timer == NULL) ? POS_EINVAL : POS_OK;
    SuccessOrExit(err);

    dispatch_source_cancel(timer->tm_timer);
    timer->tm_active = false;

exit:
    return err;
}

pos_time_t pos_timer_remaining_ticks(struct pos_timer * timer, pos_time_t now)
{
    // TODO: properly determine ticks remaining if possible or remove API if unused.
    return 0;
}

#else

static void pos_timer_cb(union sigval sv)
{
    struct pos_timer * timer = (struct pos_timer *) sv.sival_ptr;
    assert(timer);

    timer->tm_active = false;
    timer->tm_cb(timer->tm_arg);
}

pos_error_t pos_timer_init(struct pos_timer * timer, pos_timer_fn * tm_cb, void * tm_arg)
{
    int ret;
    struct sigevent event;

    /* Initialize the timer. */
    memset(timer, 0, sizeof(*timer));
    timer->tm_cb     = tm_cb;
    timer->tm_arg    = tm_arg;
    timer->tm_active = false;

    event.sigev_notify            = SIGEV_THREAD;
    event.sigev_value.sival_ptr   = timer; // put timer obj in signal args
    event.sigev_notify_function   = pos_timer_cb;
    event.sigev_notify_attributes = NULL;

    ret = timer_create(CLOCK_REALTIME, &event, &timer->tm_timer);
    return (ret) ? POS_ERROR : POS_OK;
}

pos_error_t pos_timer_start(struct pos_timer * timer, pos_time_t ticks)
{
    struct itimerspec its;
    pos_error_t err;
    int ret;

    err = pos_timer_inited(timer);
    SuccessOrExit(err);

    if (ticks == 0)
    {
        ticks = 1;
    }

    timer->tm_ticks = pos_time_get() + ticks;

    its.it_interval.tv_sec  = 0;
    its.it_interval.tv_nsec = 0; // one shot
    its.it_value.tv_sec     = (ticks / 1000);
    its.it_value.tv_nsec    = (ticks % 1000) * 1000000; // expiration
    its.it_value.tv_nsec %= 1000000000;
    timer->tm_active = true;

    ret = timer_settime(timer->tm_timer, 0, &its, NULL);
    err = (ret) ? POS_EINVAL : POS_OK;
    SuccessOrExit(err);

exit:
    return err;
}

pos_error_t pos_timer_stop(struct pos_timer * timer)
{
    pos_error_t err;
    int ret;

    err = pos_timer_inited(timer);
    SuccessOrExit(err);

    struct itimerspec its;
    its.it_interval.tv_sec  = 0;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec     = 0;
    its.it_value.tv_nsec    = 0;

    ret = timer_settime(timer->tm_timer, 0, &its, NULL);
    err = (ret) ? POS_EINVAL : POS_OK;
    SuccessOrExit(err);

    timer->tm_active = false;

exit:
    return err;
}

pos_time_t pos_timer_remaining_ticks(struct pos_timer * timer, pos_time_t now)
{
    pos_time_t rt;
    uint32_t exp;

    struct itimerspec its;
    timer_gettime(timer->tm_timer, &its);

    exp = its.it_value.tv_sec * 1000;

    if (exp > now)
    {
        rt = exp - now;
    }
    else
    {
        rt = 0;
    }

    return rt;
}

#endif // __APPLE__

pos_error_t pos_timer_start_ms(struct pos_timer * timer, pos_time_t duration)
{
    pos_time_t delta = pos_time_ms_to_ticks(duration);
    return pos_timer_start(timer, delta);
}

bool pos_timer_is_active(struct pos_timer * timer)
{
    // TODO: seek native posix method to determine whether timer_t is active.
    // TODO: fix bug where one-shot timer is still active after fired.
    return timer->tm_active;
}

pos_error_t pos_timer_inited(struct pos_timer * timer)
{
    return (timer->tm_timer == NULL) ? POS_EINVAL : POS_OK;
}

pos_time_t pos_timer_get_ticks(struct pos_timer * timer)
{
    return timer->tm_ticks;
}

void * pos_timer_arg_get(struct pos_timer * timer)
{
    return timer->tm_arg;
}

void pos_timer_arg_set(struct pos_timer * timer, void * arg)
{
    timer->tm_arg = arg;
}
