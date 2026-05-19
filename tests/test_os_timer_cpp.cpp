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

#include <poski/OsTimer.h>
#include <poski/OsTime.h>
#include <poski/OsTask.h>
#include "test_util.h"
#include <stdlib.h>

#define TEST_TASK_PRIO (1)
#define TEST_STACK_SIZE (1024)

#define TEST_TIME_CONVERSION_MARGIN (1)

#define TEST_TIMER_MARGIN (10)
#define TEST_TIMER1_DURATION (1000)

static poski::OsTask s_task1;
static poski::OsTimer* s_timer1 = nullptr;
static pos_time_t s_timer1_start;

static void test_timer1_fired(void * arg)
{
    poski::OsTimer * t = (poski::OsTimer *) arg;
    pos_time_t now       = poski::OsTime::Get();
    pos_time_t delta     = now - s_timer1_start;
    delta                    = poski::OsTime::TicksToMs(delta);

    TEST_LOG("test_timer fired now=%d\n", now);
    TEST_LOG("test_timer fired start=%d\n", s_timer1_start);
    TEST_LOG("test_timer fired delta=%d\n", delta);

    VerifyOrQuit((delta >= TEST_TIMER1_DURATION), "timer: duration too short");
    VerifyOrQuit((delta < (TEST_TIMER1_DURATION + TEST_TIMER_MARGIN)), "timer: duration too long");
    VerifyOrQuit(!t->IsActive(), "timer: fired, but still active");
    VerifyOrQuit(t->ArgGet() == t, "timer: arg incorrect");

    printf("All tests passed\n");
    exit(PASS);
}

void test_timer1(poski::OsTimer * t)
{
    s_timer1_start = poski::OsTime::Get();
    t->StartMs(TEST_TIMER1_DURATION);
}

void test_sleep()
{
    pos_time_t start = poski::OsTime::GetMs();
    pos_time_t end, delta;

    poski::OsTask::SleepMs(1000);

    end   = poski::OsTime::GetMs();
    delta = end - start;

    TEST_LOG("test_sleep now=%d\n", end);
    TEST_LOG("test_sleep start=%d\n", start);
    TEST_LOG("test_sleep delta=%d\n", delta);

    VerifyOrQuit((delta >= TEST_TIMER1_DURATION), "timer: duration too short");
    VerifyOrQuit((delta < (TEST_TIMER1_DURATION + TEST_TIMER_MARGIN)), "timer: duration too long");
}

void * task1_run(void * arg)
{
    test_sleep();

    test_timer1(s_timer1);

    while (1)
    {
        poski::OsTask::Yield();
    }
}

void test_time_convert(pos_time_t truth_ticks, pos_time_t truth_ms)
{
    pos_time_t ms;
    pos_time_t ticks;

    ms = poski::OsTime::TicksToMs(truth_ticks);
    TEST_LOG("Convert ticks=%d to ms=%d expect=%d\n", truth_ticks, ms, truth_ms);
    VerifyOrQuit((ms >= truth_ms - TEST_TIME_CONVERSION_MARGIN), "time: ticks to ms conversion too short");
    VerifyOrQuit((ms <= truth_ms + TEST_TIME_CONVERSION_MARGIN), "time: ticks to ms conversion too long");

    ticks = poski::OsTime::MsToTicks(truth_ms);
    TEST_LOG("Convert ms=%d to ticks=%d expect=%d\n", truth_ms, ticks, truth_ticks);
    VerifyOrQuit((ticks >= truth_ticks - TEST_TIME_CONVERSION_MARGIN), "time: ms to ticks conversion too short");
    VerifyOrQuit((ticks <= truth_ticks + TEST_TIME_CONVERSION_MARGIN), "time: ms to ticks conversion too long");
}

void test_time_conversions()
{
    pos_time_t tps = POS_TICKS_PER_SEC;
    test_time_convert(tps, 1000);
    test_time_convert(tps * 100, 100000);
    test_time_convert(tps / 100, 10);
}

int main(void)
{
    test_time_conversions();

    // We instantiate s_timer1 dynamically so it passes itself `s_timer1` as the `arg`!
    s_timer1 = new poski::OsTimer(test_timer1_fired, nullptr);
    s_timer1->ArgSet(s_timer1);

    s_task1.Start("task1", task1_run, NULL, TEST_TASK_PRIO, TEST_STACK_SIZE);

    poski::OsTask::SchedStart();

    return FAIL;
}
