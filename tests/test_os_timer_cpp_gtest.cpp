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

#include <gtest/gtest.h>
#include <poski/OsTimer.h>
#include <poski/OsTime.h>
#include <poski/OsTask.h>
#include <stdio.h>

static volatile int s_timer_fired_count = 0;
static pos_time_t s_timer_start_time;

static void timer_cb(void * arg)
{
    s_timer_fired_count++;
    
    pos_time_t now = poski::OsTime::Get();
    pos_time_t delta = now - s_timer_start_time;
    delta = poski::OsTime::TicksToMs(delta);
    
    EXPECT_GE(delta, 150); // Expect at least 150ms (allowing small scheduling slack)
}

TEST(OsTimerCpp, OneShotTimer) {
    s_timer_fired_count = 0;
    poski::OsTimer timer(timer_cb, nullptr);
    timer.ArgSet(&timer);
    
    s_timer_start_time = poski::OsTime::Get();
    EXPECT_EQ(timer.StartMs(200), POS_OK);
    EXPECT_TRUE(timer.IsActive());
    
    // Robust loop wait up to 1 second (1000ms timeout)
    int timeout_ms = 1000;
    while (s_timer_fired_count == 0 && timeout_ms > 0) {
        poski::OsTask::Sleep(20); // sleep 20ms
        timeout_ms -= 20;
    }
    
    EXPECT_EQ(s_timer_fired_count, 1);
    EXPECT_FALSE(timer.IsActive());
}

TEST(OsTimeCpp, Conversions) {
    pos_time_t tps = POS_TICKS_PER_SEC;
    EXPECT_EQ(poski::OsTime::TicksToMs(tps), 1000);
    EXPECT_EQ(poski::OsTime::MsToTicks(1000), tps);
}
