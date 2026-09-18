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

#ifndef POSKI_CPP_OS_EVENT_H
#define POSKI_CPP_OS_EVENT_H

#include "poski/osal/os_event.h"
#include <assert.h>

namespace poski {

class OsEvent {
public:
    OsEvent(pos_event_fn * fn = NULL, void * arg = NULL) : event_{} {
        pos_event_init(&event_, fn, arg);
    }

    ~OsEvent() = default;
    OsEvent(const OsEvent&) = delete;
    OsEvent& operator=(const OsEvent&) = delete;

    void Init(pos_event_fn * fn, void * arg) {
        pos_event_init(&event_, fn, arg);
    }

    bool IsQueued() const {
        return pos_event_is_queued(&event_);
    }

    void * ArgGet() const {
        return pos_event_arg_get(&event_);
    }

    void ArgSet(void * arg) {
        pos_event_arg_set(&event_, arg);
    }

    void Run() {
        pos_event_run(&event_);
    }

    struct pos_event * GetNative() { return &event_; }
    const struct pos_event * GetNative() const { return &event_; }

private:
    struct pos_event event_;
};

class OsEventQueue {
public:
    OsEventQueue() : evq_{}, initialized_(false) {
        pos_error_t err = pos_eventq_init(&evq_);
        initialized_ = (err == POS_OK);
        assert(err == POS_OK);
    }

    ~OsEventQueue() {
        if (initialized_) {
            pos_eventq_deinit(&evq_);
            initialized_ = false;
        }
    }

    OsEventQueue(const OsEventQueue&) = delete;
    OsEventQueue& operator=(const OsEventQueue&) = delete;

    pos_error_t Put(struct pos_event * ev) {
        return pos_eventq_put(&evq_, ev);
    }

    pos_error_t Put(OsEvent & ev) {
        return pos_eventq_put(&evq_, ev.GetNative());
    }

    struct pos_event * Get(pos_time_t timeout = POS_TIME_FOREVER) {
        return pos_eventq_get(&evq_, timeout);
    }

    struct pos_event * GetNoWait() {
        return pos_eventq_get_no_wait(&evq_);
    }

    pos_error_t Remove(struct pos_event * ev) {
        return pos_eventq_remove(&evq_, ev);
    }

    pos_error_t Remove(OsEvent & ev) {
        return pos_eventq_remove(&evq_, ev.GetNative());
    }

    bool IsEmpty() {
        return pos_eventq_is_empty(&evq_);
    }

    bool Inited() const {
        return initialized_ && pos_eventq_inited(&evq_);
    }

    pos_error_t Poll(pos_time_t timeout = POS_TIME_NO_WAIT) {
        return pos_eventq_poll(&evq_, timeout);
    }

    void Run() {
        pos_eventq_run(&evq_);
    }

    struct pos_eventq * GetNative() { return &evq_; }

private:
    struct pos_eventq evq_;
    bool initialized_;
};

class OsEventTimer {
public:
    OsEventTimer(OsEventQueue & evq, pos_event_fn * fn, void * arg = NULL)
        : et_{}, initialized_(false) {
        pos_error_t err = pos_event_timer_init(&et_, evq.GetNative(), fn, arg);
        initialized_ = (err == POS_OK);
        assert(err == POS_OK);
    }

    OsEventTimer(struct pos_eventq * evq, pos_event_fn * fn, void * arg = NULL)
        : et_{}, initialized_(false) {
        pos_error_t err = pos_event_timer_init(&et_, evq, fn, arg);
        initialized_ = (err == POS_OK);
        assert(err == POS_OK);
    }

    ~OsEventTimer() {
        if (initialized_) {
            pos_event_timer_deinit(&et_);
            initialized_ = false;
        }
    }

    OsEventTimer(const OsEventTimer&) = delete;
    OsEventTimer& operator=(const OsEventTimer&) = delete;

    pos_error_t Start(pos_time_t ticks) {
        return pos_event_timer_start(&et_, ticks);
    }

    pos_error_t StartMs(pos_time_t ms) {
        return pos_event_timer_start_ms(&et_, ms);
    }

    pos_error_t Arm(pos_time_t ticks) {
        return pos_event_timer_arm(&et_, ticks);
    }

    pos_error_t ArmMs(pos_time_t ms) {
        return pos_event_timer_arm_ms(&et_, ms);
    }

    pos_error_t Stop() {
        return pos_event_timer_stop(&et_);
    }

    bool IsActive() {
        return pos_event_timer_is_active(&et_);
    }

    pos_time_t GetTicks() {
        return pos_event_timer_get_ticks(&et_);
    }

    pos_time_t RemainingTicks(pos_time_t now) {
        return pos_event_timer_remaining_ticks(&et_, now);
    }

    void ArgSet(void * arg) {
        pos_event_timer_arg_set(&et_, arg);
    }

    void * ArgGet() {
        return pos_event_timer_arg_get(&et_);
    }

    struct pos_event * EventGet() {
        return pos_event_timer_event_get(&et_);
    }

    struct pos_event_timer * GetNative() { return &et_; }

private:
    struct pos_event_timer et_;
    bool initialized_;
};

} // namespace poski

#endif // POSKI_CPP_OS_EVENT_H
