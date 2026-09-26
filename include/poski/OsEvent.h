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
 *   Zero-overhead C++ wrappers for the OSAL event API (<poski/osal/os_event.h>).
 *
 *   Each wrapper holds its C object by value and adds no other data members:
 *   no heap allocation, no pointer-to-implementation, and sizeof(wrapper) ==
 *   sizeof(C struct).  Instances can therefore be static/global objects, class
 *   members, or stack objects, exactly like the C structs.  `OsEvent` has a
 *   constexpr constructor, so a namespace-scope `OsEvent` is constant-initialized
 *   (placed in .data, with no static-initialization-order issues).
 *
 *   `OsEventIn` and `OsEventTimerIn` dispatch to a member function of their
 *   owner through a static trampoline that is instantiated at compile time.  The
 *   owner pointer travels in the event's existing `arg` slot, so this costs no
 *   storage and needs no std::function or virtual dispatch:
 *
 *   @code
 *   class Radio {
 *   public:
 *       explicit Radio(poski::OsEventQueue & evq) : mRxEvent(*this), mTimeout(evq, *this) {}
 *
 *   private:
 *       void HandleRx();      // runs in the task that services evq
 *       void HandleTimeout(); // runs in the task that services evq
 *
 *       poski::OsEventIn<Radio, &Radio::HandleRx> mRxEvent;
 *       poski::OsEventTimerIn<Radio, &Radio::HandleTimeout> mTimeout;
 *   };
 *   @endcode
 *
 *   Events are linked into queues intrusively, so wrappers are neither copyable
 *   nor movable, and an event must not be destroyed while it is queued.
 */

#ifndef POSKI_CPP_OS_EVENT_H
#define POSKI_CPP_OS_EVENT_H

#include "poski/osal/os_event.h"
#include <assert.h>
#include <type_traits>

namespace poski {

namespace detail {

/** `pos_event_fn` trampoline that calls `Handler` on the `Owner` stored in the event's argument. */
template <typename Owner, void (Owner::*Handler)()>
void DispatchToMember(struct pos_event * ev)
{
    (static_cast<Owner *>(pos_event_arg_get(ev))->*Handler)();
}

} // namespace detail

/** An intrusive event: a callback function plus a user argument. */
class OsEvent {
public:
    explicit constexpr OsEvent(pos_event_fn * fn = nullptr, void * arg = nullptr) : event_{ nullptr, fn, arg, false } {}

    OsEvent(const OsEvent &)             = delete;
    OsEvent & operator=(const OsEvent &) = delete;

    void Init(pos_event_fn * fn, void * arg) { pos_event_init(&event_, fn, arg); }

    bool IsQueued() const { return pos_event_is_queued(&event_); }

    void * ArgGet() const { return pos_event_arg_get(&event_); }

    void ArgSet(void * arg) { pos_event_arg_set(&event_, arg); }

    void Run() { pos_event_run(&event_); }

    struct pos_event * GetNative() { return &event_; }
    const struct pos_event * GetNative() const { return &event_; }

private:
    struct pos_event event_;
};

/**
 * An event that calls `Owner::Handler()` on the owner object when run.
 *
 * The owner pointer is stored in the event argument, so `Init()` and `ArgSet()`
 * are not available.
 */
template <typename Owner, void (Owner::*Handler)()>
class OsEventIn : public OsEvent {
public:
    explicit constexpr OsEventIn(Owner & owner) : OsEvent(&detail::DispatchToMember<Owner, Handler>, &owner) {}

    void Init(pos_event_fn * fn, void * arg) = delete;
    void ArgSet(void * arg)                  = delete;
};

/** An event queue drained by one or more consumer tasks. */
class OsEventQueue {
public:
    OsEventQueue()
    {
        pos_error_t err = pos_eventq_init(&evq_);
        assert(err == POS_OK);
        (void) err;
    }

    ~OsEventQueue() { (void) pos_eventq_deinit(&evq_); }

    OsEventQueue(const OsEventQueue &)             = delete;
    OsEventQueue & operator=(const OsEventQueue &) = delete;

    pos_error_t Put(struct pos_event * ev) { return pos_eventq_put(&evq_, ev); }
    pos_error_t Put(OsEvent & ev) { return pos_eventq_put(&evq_, ev.GetNative()); }

    struct pos_event * Get(pos_time_t timeout = POS_TIME_FOREVER) { return pos_eventq_get(&evq_, timeout); }
    struct pos_event * GetNoWait() { return pos_eventq_get_no_wait(&evq_); }

    pos_error_t Remove(struct pos_event * ev) { return pos_eventq_remove(&evq_, ev); }
    pos_error_t Remove(OsEvent & ev) { return pos_eventq_remove(&evq_, ev.GetNative()); }

    bool IsEmpty() { return pos_eventq_is_empty(&evq_); }

    bool Inited() const { return pos_eventq_inited(&evq_) != 0; }

    /** Dequeue one event (waiting up to `timeout` ticks) and run it. */
    pos_error_t Poll(pos_time_t timeout = POS_TIME_NO_WAIT) { return pos_eventq_poll(&evq_, timeout); }

    /** Wait for the next event and run it. */
    void Run() { pos_eventq_run(&evq_); }

    struct pos_eventq * GetNative() { return &evq_; }

private:
    struct pos_eventq evq_;
};

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

static_assert(sizeof(OsEvent) == sizeof(struct pos_event), "OsEvent must add no storage");
static_assert(sizeof(OsEventQueue) == sizeof(struct pos_eventq), "OsEventQueue must add no storage");
static_assert(sizeof(OsEventTimer) == sizeof(struct pos_event_timer), "OsEventTimer must add no storage");
static_assert(std::is_standard_layout<OsEvent>::value, "OsEvent must be layout-compatible with pos_event");

} // namespace poski

#endif // POSKI_CPP_OS_EVENT_H
