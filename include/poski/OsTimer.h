/*
 *
 *    Copyright (c) 2026 Google LLC
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

#ifndef POSKI_CPP_OS_TIMER_H
#define POSKI_CPP_OS_TIMER_H

#include "poski/osal/os_timer.h"
#include <assert.h>

namespace poski {

class OsTimer {
public:
    OsTimer(pos_timer_fn cb, void* arg) {
        pos_error_t err = pos_timer_init(&timer_, cb, arg);
        assert(err == POS_OK);
    }

    ~OsTimer() = default;
    OsTimer(const OsTimer&) = delete;
    OsTimer& operator=(const OsTimer&) = delete;

    pos_error_t Start(pos_time_t ticks) {
        return pos_timer_start(&timer_, ticks);
    }

    pos_error_t StartMs(pos_time_t ms) {
        return pos_timer_start_ms(&timer_, ms);
    }

    pos_error_t Stop() {
        return pos_timer_stop(&timer_);
    }

    bool IsActive() {
        return pos_timer_is_active(&timer_);
    }

    pos_time_t GetTicks() {
        return pos_timer_get_ticks(&timer_);
    }

    pos_time_t RemainingTicks(pos_time_t time) {
        return pos_timer_remaining_ticks(&timer_, time);
    }

    void ArgSet(void* arg) {
        pos_timer_arg_set(&timer_, arg);
    }

    void* ArgGet() {
        return pos_timer_arg_get(&timer_);
    }

    struct pos_timer* GetNative() { return &timer_; }

private:
    struct pos_timer timer_;
};

} // namespace poski
#endif // POSKI_CPP_OS_TIMER_H
