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

/**
 * @file
 *   C++ wrapper for the OSAL event timer (`struct pos_event_timer`,
 *   <poski/osal/os_event.h>).  Like `OsEvent`, it is a pure, zero-overhead
 *   wrapper of the C API (see <poski/OsEvent.h>).
 *
 *   `OsEventTimerIn<Owner, &Owner::Handler>` calls a member function of its
 *   owner when it expires, in the task that services the event queue:
 *
 *   @code
 *   class Radio {
 *   public:
 *       explicit Radio(poski::OsEventQueue & evq) : mTimeout(evq, *this) {}
 *
 *       void Transmit() { (void) mTimeout.StartMs(100); }
 *
 *   private:
 *       void HandleTimeout(); // runs in the task that services evq
 *
 *       poski::OsEventTimerIn<Radio, &Radio::HandleTimeout> mTimeout;
 *   };
 *   @endcode
 */

#ifndef POSKI_CPP_OS_EVENT_TIMER_H
#define POSKI_CPP_OS_EVENT_TIMER_H

#include "poski/OsEvent.h"
#include "poski/OsEventQueue.h"
#include "poski/osal/os_event.h"
#include <assert.h>

namespace poski {

/** A one-shot timer that posts its event to an event queue on expiry. */
class OsEventTimer {
public:
    OsEventTimer(OsEventQueue & evq, pos_event_fn * fn, void * arg = nullptr) : OsEventTimer(evq.GetNative(), fn, arg) {}

    OsEventTimer(struct pos_eventq * evq, pos_event_fn * fn, void * arg = nullptr)
    {
        pos_error_t err = pos_event_timer_init(&et_, evq, fn, arg);
        assert(err == POS_OK);
        (void) err;
    }

    ~OsEventTimer() { (void) pos_event_timer_deinit(&et_); }

    OsEventTimer(const OsEventTimer &)             = delete;
    OsEventTimer & operator=(const OsEventTimer &) = delete;

    pos_error_t Start(pos_time_t ticks) { return pos_event_timer_start(&et_, ticks); }
    pos_error_t StartMs(pos_time_t ms) { return pos_event_timer_start_ms(&et_, ms); }
    pos_error_t Stop() { return pos_event_timer_stop(&et_); }

    bool IsActive() { return pos_event_timer_is_active(&et_); }
    bool Inited() { return pos_event_timer_inited(&et_) == POS_OK; }

    pos_time_t GetTicks() { return pos_event_timer_get_ticks(&et_); }
    pos_time_t RemainingTicks(pos_time_t now) { return pos_event_timer_remaining_ticks(&et_, now); }

    void ArgSet(void * arg) { pos_event_timer_arg_set(&et_, arg); }
    void * ArgGet() { return pos_event_timer_arg_get(&et_); }

    struct pos_event * EventGet() { return pos_event_timer_event_get(&et_); }

    struct pos_event_timer * GetNative() { return &et_; }

private:
    struct pos_event_timer et_;
};

/**
 * An event timer that calls `Owner::Handler()` on the owner object, in the task
 * that services the event queue, when it expires.
 *
 * The owner pointer is stored in the event argument, so `ArgSet()` is not
 * available.
 */
template <typename Owner, void (Owner::*Handler)()>
class OsEventTimerIn : public OsEventTimer {
public:
    OsEventTimerIn(OsEventQueue & evq, Owner & owner) : OsEventTimer(evq, &detail::DispatchToMember<Owner, Handler>, &owner) {}

    void ArgSet(void * arg) = delete;
};

static_assert(sizeof(OsEventTimer) == sizeof(struct pos_event_timer), "OsEventTimer must add no storage");

} // namespace poski

#endif // POSKI_CPP_OS_EVENT_TIMER_H
