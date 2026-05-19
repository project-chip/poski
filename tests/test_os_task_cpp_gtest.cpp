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

#include <gtest/gtest.h>
#include <poski/OsTask.h>
#include <atomic>

static std::atomic<int> s_task_run_count{0};

struct DummyTaskArgs
{
    std::atomic<int> counter;
};

static void * dummy_task_run(void * arg)
{
    DummyTaskArgs * task_args = static_cast<DummyTaskArgs *>(arg);
    task_args->counter.store(42);
    s_task_run_count.fetch_add(1);

    // Keep the task alive until Remove() cancels it so the test does not race
    // with thread exit on platforms where removal maps to pthread_cancel().
    while (true)
    {
        poski::OsTask::Sleep(1);
    }

    return NULL;
}

TEST(OsTaskCpp, BasicStartRemove) {
    poski::OsTask task;
    DummyTaskArgs args = {};
    args.counter.store(0);
    s_task_run_count.store(0);

    EXPECT_EQ(task.Start("dummy", dummy_task_run, &args, 1, 1028), POS_OK);

    bool task_ran = false;
    for (int i = 0; i < 100; ++i)
    {
        if (args.counter.load() == 42 && s_task_run_count.load() == 1)
        {
            task_ran = true;
            break;
        }
        poski::OsTask::Sleep(1);
    }

    EXPECT_TRUE(task_ran);
    EXPECT_EQ(args.counter.load(), 42);
    EXPECT_EQ(s_task_run_count.load(), 1);

    EXPECT_EQ(task.Remove(), POS_OK);
}
