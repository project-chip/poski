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
#include <poski/OsMutex.h>
#include <poski/OsTask.h>
#include <string.h>

#define TEST_ITERATIONS 10
#define TASK1_VALUE 0x11
#define TASK2_VALUE 0x22

// Globally constructed, but NOT locked on construction
static poski::OsMutex task1_mtx(false);
static uint8_t s_buffer[TEST_ITERATIONS];

static void test_mutex_nested(uint8_t test_value, int task, bool recursive, int round)
{
    EXPECT_EQ(task1_mtx.Lock(POS_TIME_FOREVER), POS_OK);
    for (int i = 0; i < TEST_ITERATIONS; i++)
    {
        if (recursive)
        {
            EXPECT_EQ(task1_mtx.Lock(POS_TIME_FOREVER), POS_OK);
        }
        EXPECT_NE(s_buffer[i], test_value);
        s_buffer[i] = test_value;
    }

    for (int i = 0; i < TEST_ITERATIONS; i++)
    {
        EXPECT_EQ(s_buffer[i], test_value);
        if (recursive)
        {
            EXPECT_EQ(task1_mtx.Unlock(), POS_OK);
        }
    }
    memset(&s_buffer, 0, sizeof(s_buffer));
    EXPECT_EQ(task1_mtx.Unlock(), POS_OK);

    poski::OsTask::Sleep(2);
}

static void * task1_run(void * arg)
{
    for (int round = 0; round < TEST_ITERATIONS; round++)
    {
        bool recursive = (round % 5 == 0);
        test_mutex_nested(TASK1_VALUE, 1, recursive, round);
    }
    return NULL;
}

static void * task2_run(void * arg)
{
    for (int round = 0; round < TEST_ITERATIONS; round++)
    {
        bool recursive = (round % 5 == 0);
        test_mutex_nested(TASK2_VALUE, 2, recursive, round);
    }
    return NULL;
}

TEST(OsMutexCpp, BasicLockUnlock) {
    poski::OsMutex mutex(true); // Locks immediately in constructor (depth = 1)
    EXPECT_EQ(mutex.Unlock(), POS_OK); // Manual unlock (depth = 0)
}

TEST(OsMutexCpp, BasicRAIIScopedLock) {
    {
        poski::OsMutex lock(true); // Constructor locks it
        EXPECT_EQ(lock.Unlock(), POS_OK);
        EXPECT_EQ(lock.Lock(POS_TIME_NO_WAIT), POS_OK);
    } // Destructor unlocks it

    poski::OsMutex relock(false);
    EXPECT_EQ(relock.Lock(POS_TIME_NO_WAIT), POS_OK);
    EXPECT_EQ(relock.Unlock(), POS_OK);
}

TEST(OsMutexCpp, ThreadConcurrency) {
    memset(&s_buffer, 0, sizeof(s_buffer));
    
    poski::OsTask task1;
    poski::OsTask task2;

    EXPECT_EQ(task1.Start("task1", task1_run, NULL, 1, 1028), POS_OK);
    EXPECT_EQ(task2.Start("task2", task2_run, NULL, 1, 1028), POS_OK);

    poski::OsTask::Sleep(100); // Wait for execution

    task1.Remove();
    task2.Remove();
}
