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
 *   C++ wrapper for the OSAL event queue (`struct pos_eventq`,
 *   <poski/osal/os_event.h>).  Like `OsEvent`, it is a pure, zero-overhead
 *   wrapper of the C API (see <poski/OsEvent.h>).
 */

#ifndef POSKI_CPP_OS_EVENT_QUEUE_H
#define POSKI_CPP_OS_EVENT_QUEUE_H

// IWYU pragma: begin_exports
#include "poski/OsEvent.h"
#include "poski/osal/os_event.h"
// IWYU pragma: end_exports
#include <assert.h>

namespace poski {

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

static_assert(sizeof(OsEventQueue) == sizeof(struct pos_eventq), "OsEventQueue must add no storage");

} // namespace poski

#endif // POSKI_CPP_OS_EVENT_QUEUE_H
