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
#include <poski/OsTask.h>

static int s_task_run_count = 0;

static void * dummy_task_run(void * arg)
{
    int* counter = (int*)arg;
    *counter = 42;
    s_task_run_count++;
    return NULL;
}

TEST(OsTaskCpp, BasicStartRemove) {
    poski::OsTask task;
    int counter = 0;
    s_task_run_count = 0;

    EXPECT_EQ(task.Start("dummy", dummy_task_run, &counter, 1, 1028), POS_OK);
    
    // Wait for the task to run and yield/complete
    poski::OsTask::Sleep(10);
    
    EXPECT_EQ(counter, 42);
    EXPECT_EQ(s_task_run_count, 1);
    
    EXPECT_EQ(task.Remove(), POS_OK);
}

TEST(OsTaskCpp, SchedStartStarted) {
    EXPECT_TRUE(poski::OsTask::SchedStarted());
}
