/*
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

#include <poski/OsEvent.h>
#include <poski/OsEventQueue.h>
#include <poski/OsEventTimer.h>
#include <poski/OsTime.h>
#include "test_util.h"

static int s_count = 0;

static void cpp_event_cb(struct pos_event * ev)
{
    int * count = static_cast<int *>(pos_event_arg_get(ev));
    VerifyOrQuit(count != nullptr, "OsEvent: null arg");
    (*count)++;
}

/* Wrappers are usable as statically allocated objects; OsEvent is constant-initialized. */
static poski::OsEventQueue s_evq;
static poski::OsEvent s_event(cpp_event_cb, &s_count);

class Widget {
public:
    explicit Widget(poski::OsEventQueue & evq) : mEvent(*this), mTimer(evq, *this) {}

    int mEvents = 0;
    int mTimers = 0;

private:
    void HandleEvent() { mEvents++; }
    void HandleTimer() { mTimers++; }

public:
    poski::OsEventIn<Widget, &Widget::HandleEvent> mEvent;
    poski::OsEventTimerIn<Widget, &Widget::HandleTimer> mTimer;
};

int main(void)
{
    poski::OsEventTimer et(s_evq, cpp_event_cb, &s_count);
    Widget widget(s_evq);

    VerifyOrQuit(s_evq.Inited(), "OsEventQueue: not inited");
    VerifyOrQuit(s_evq.IsEmpty(), "OsEventQueue: not empty");
    VerifyOrQuit(et.Inited(), "OsEventTimer: not inited");

    VerifyOrQuit(s_evq.Put(s_event) == POS_OK, "OsEventQueue: Put failed");
    VerifyOrQuit(s_event.IsQueued(), "OsEvent: should be queued");
    VerifyOrQuit(s_evq.Poll(POS_TIME_NO_WAIT) == POS_OK, "OsEventQueue: Poll failed");
    VerifyOrQuit(s_count == 1, "OsEvent: count should be 1");

    VerifyOrQuit(et.StartMs(40) == POS_OK, "OsEventTimer: StartMs failed");
    VerifyOrQuit(s_evq.Poll(poski::OsTime::MsToTicks(400)) == POS_OK, "OsEventTimer: Poll timed out");
    VerifyOrQuit(s_count == 2, "OsEventTimer: count should be 2");

    /* Member-function dispatch */
    VerifyOrQuit(s_evq.Put(widget.mEvent) == POS_OK, "OsEventIn: Put failed");
    VerifyOrQuit(s_evq.Poll(POS_TIME_NO_WAIT) == POS_OK, "OsEventIn: Poll failed");
    VerifyOrQuit(widget.mEvents == 1, "OsEventIn: handler not called");

    VerifyOrQuit(widget.mTimer.StartMs(20) == POS_OK, "OsEventTimerIn: StartMs failed");
    VerifyOrQuit(s_evq.Poll(poski::OsTime::MsToTicks(400)) == POS_OK, "OsEventTimerIn: Poll timed out");
    VerifyOrQuit(widget.mTimers == 1, "OsEventTimerIn: handler not called");
    VerifyOrQuit(s_count == 2, "OsEventTimerIn: unexpected C callback");

    printf("All C++ event tests passed\n");
    return PASS;
}
