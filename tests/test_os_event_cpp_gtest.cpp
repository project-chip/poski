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

#include <gtest/gtest.h>
#include <poski/OsEvent.h>
#include <poski/OsTime.h>

static void gtest_event_cb(struct pos_event * ev)
{
    int * count = static_cast<int *>(pos_event_arg_get(ev));
    ASSERT_NE(count, nullptr);
    (*count)++;
}

TEST(OsEventCpp, QueueAndCoalescing) {
    int count = 0;
    poski::OsEventQueue evq;
    poski::OsEvent ev1(gtest_event_cb, &count);
    poski::OsEvent ev2(gtest_event_cb, &count);

    EXPECT_TRUE(evq.Inited());
    EXPECT_TRUE(evq.IsEmpty());

    // Idempotent duplicate put
    EXPECT_EQ(evq.Put(ev1), POS_OK);
    EXPECT_TRUE(ev1.IsQueued());
    EXPECT_EQ(evq.Put(ev1), POS_OK);
    EXPECT_EQ(evq.Put(ev2), POS_OK);

    EXPECT_EQ(evq.Remove(ev2), POS_OK);
    EXPECT_FALSE(ev2.IsQueued());

    EXPECT_EQ(evq.Poll(POS_TIME_NO_WAIT), POS_OK);
    EXPECT_EQ(count, 1);
    EXPECT_TRUE(evq.IsEmpty());
}

TEST(OsEventCpp, EventTimerDispatchAndCancel) {
    int count = 0;
    poski::OsEventQueue evq;
    poski::OsEventTimer et(evq, gtest_event_cb, &count);

    EXPECT_EQ(et.StartMs(40), POS_OK);
    EXPECT_TRUE(et.IsActive());
    EXPECT_EQ(evq.Poll(poski::OsTime::MsToTicks(400)), POS_OK);
    EXPECT_EQ(count, 1);

    EXPECT_EQ(et.StartMs(200), POS_OK);
    EXPECT_EQ(et.Stop(), POS_OK);
    EXPECT_FALSE(et.IsActive());
    EXPECT_EQ(evq.Poll(poski::OsTime::MsToTicks(50)), POS_TIMEOUT);
    EXPECT_EQ(count, 1);
}
