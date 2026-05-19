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
#include <poski/OsSemaphore.h>
#include <poski/OsTask.h>

#define TEST_ITERATIONS 5

// Globally constructed, but NOT taken on construction
static poski::OsSemaphore task1_sem(0, false);
static poski::OsSemaphore task2_sem(0, false);

static void * task1_run(void * arg)
{
    for (int i = 0; i < TEST_ITERATIONS; i++)
    {
        EXPECT_EQ(task1_sem.Give(), POS_OK);
        EXPECT_EQ(task2_sem.Take(POS_TIME_FOREVER), POS_OK);
    }
    return NULL;
}

static void * task2_run(void * arg)
{
    for (int i = 0; i < TEST_ITERATIONS; i++)
    {
        EXPECT_EQ(task1_sem.Take(POS_TIME_FOREVER), POS_OK);
        EXPECT_EQ(task2_sem.Give(), POS_OK);
    }
    return NULL;
}

TEST(OsSemaphoreCpp, BasicGiveTake) {
    poski::OsSemaphore sem(1, false); // 1 token initially, no take in constructor
    EXPECT_EQ(sem.Take(POS_TIME_NO_WAIT), POS_OK);
    EXPECT_EQ(sem.Take(POS_TIME_NO_WAIT), POS_TIMEOUT);
    EXPECT_EQ(sem.Give(), POS_OK);
    EXPECT_EQ(sem.Take(POS_TIME_NO_WAIT), POS_OK);
}

TEST(OsSemaphoreCpp, BasicRAIIScopedSemaphore) {
    // Initialize sem with 1 token.
    // Constructor will immediately TAKE/decrement it to 0!
    poski::OsSemaphore sem(1, true); // Locks instantly
    
    // Trying to take again should fail/timeout
    EXPECT_EQ(sem.Take(POS_TIME_NO_WAIT), POS_TIMEOUT);
    
    {
        // Destructor of the local block will GIVE/increment it back to 1!
    }
}

TEST(OsSemaphoreCpp, ConcurrencySignaling) {
    poski::OsTask task1;
    poski::OsTask task2;

    EXPECT_EQ(task1.Start("task1", task1_run, NULL, 1, 1028), POS_OK);
    EXPECT_EQ(task2.Start("task2", task2_run, NULL, 1, 1028), POS_OK);

    poski::OsTask::Sleep(50);

    task1.Remove();
    task2.Remove();
}
