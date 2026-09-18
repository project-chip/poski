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
#include <poski/OsTime.h>
#include "test_util.h"

static void cpp_event_cb(struct pos_event * ev)
{
    int * count = static_cast<int *>(pos_event_arg_get(ev));
    VerifyOrQuit(count != nullptr, "OsEvent: null arg");
    (*count)++;
}

int main(void)
{
    int count = 0;
    poski::OsEventQueue evq;
    poski::OsEvent ev1(cpp_event_cb, &count);
    poski::OsEventTimer et(evq, cpp_event_cb, &count);

    VerifyOrQuit(evq.Inited(), "OsEventQueue: not inited");
    VerifyOrQuit(evq.IsEmpty(), "OsEventQueue: not empty");

    VerifyOrQuit(evq.Put(ev1) == POS_OK, "OsEventQueue: Put failed");
    VerifyOrQuit(ev1.IsQueued(), "OsEvent: should be queued");
    VerifyOrQuit(evq.Poll(POS_TIME_NO_WAIT) == POS_OK, "OsEventQueue: Poll failed");
    VerifyOrQuit(count == 1, "OsEvent: count should be 1");

    VerifyOrQuit(et.StartMs(40) == POS_OK, "OsEventTimer: StartMs failed");
    VerifyOrQuit(evq.Poll(poski::OsTime::MsToTicks(400)) == POS_OK, "OsEventTimer: Poll timed out");
    VerifyOrQuit(count == 2, "OsEventTimer: count should be 2");

    printf("All C++ event tests passed\n");
    return PASS;
}
