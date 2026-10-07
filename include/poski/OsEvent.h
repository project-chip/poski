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
 *   C++ wrapper for the OSAL event (`struct pos_event`, <poski/osal/os_event.h>).
 *
 *   `OsEvent`, `OsEventQueue` (<poski/OsEventQueue.h>), and `OsEventTimer`
 *   (<poski/OsEventTimer.h>) are pure, zero-overhead wrappers of the C API.
 *   Each one holds its C object by value, adds no other data members, and only
 *   calls the public `pos_*` API, so targets implement just the C API.  There is
 *   no heap allocation or pointer-to-implementation, and sizeof(wrapper) ==
 *   sizeof(C struct), so wrappers can be static/global objects, class members,
 *   or stack objects, exactly like the C structs.
 *
 *   The `OsEvent` constructor is constexpr (via `POS_EVENT_INITIALIZER()`), so a
 *   namespace-scope `OsEvent` is constant-initialized: it is placed in .data,
 *   with no static-initialization code or ordering issues.
 *
 *   `OsEventIn<Owner, &Owner::Handler>` dispatches to a member function of its
 *   owner through a static trampoline that is instantiated at compile time.  The
 *   owner pointer travels in the event's existing `arg` slot, so this costs no
 *   storage and needs no std::function or virtual dispatch:
 *
 *   @code
 *   class Radio {
 *   public:
 *       explicit Radio(poski::OsEventQueue & evq) : mEvq(evq), mRxEvent(*this) {}
 *
 *       void OnRxDone() { (void) mEvq.Put(mRxEvent); } // may be called from an ISR
 *
 *   private:
 *       void HandleRx(); // runs in the task that services evq
 *
 *       poski::OsEventQueue & mEvq;
 *       poski::OsEventIn<Radio, &Radio::HandleRx> mRxEvent;
 *   };
 *   @endcode
 *
 *   Events are linked into queues intrusively, so wrappers are neither copyable
 *   nor movable, and an event must not be destroyed while it is queued.
 */

#ifndef POSKI_CPP_OS_EVENT_H
#define POSKI_CPP_OS_EVENT_H

// IWYU pragma: begin_exports
#include "poski/osal/os_event.h"
// IWYU pragma: end_exports
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
    explicit constexpr OsEvent(pos_event_fn * fn = nullptr, void * arg = nullptr) : event_(POS_EVENT_INITIALIZER(fn, arg)) {}

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

static_assert(sizeof(OsEvent) == sizeof(struct pos_event), "OsEvent must add no storage");
static_assert(std::is_standard_layout<OsEvent>::value, "OsEvent must be layout-compatible with pos_event");

} // namespace poski

#endif // POSKI_CPP_OS_EVENT_H
