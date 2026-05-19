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
#include <poski/OsQueue.h>
#include <poski/OsTask.h>

#define TEST_ITERATIONS 5
#define TEST_QUEUE_SIZE 8

typedef struct
{
    int val;
} test_msg_t;

static poski::OsQueue<test_msg_t, TEST_QUEUE_SIZE>* s_task1_queue = nullptr;
static poski::OsQueue<test_msg_t, TEST_QUEUE_SIZE>* s_task2_queue = nullptr;

static void * task1_run(void * arg)
{
    test_msg_t msg;
    for (int i = 0; i < TEST_ITERATIONS; i++)
    {
        msg.val = i;
        EXPECT_EQ(s_task2_queue->Push(msg), POS_OK);
        EXPECT_EQ(s_task1_queue->Pop(msg, POS_TIME_FOREVER), POS_OK);
        EXPECT_EQ(msg.val, 100 + i);
    }
    return NULL;
}

static void * task2_run(void * arg)
{
    test_msg_t msg;
    for (int i = 0; i < TEST_ITERATIONS; i++)
    {
        EXPECT_EQ(s_task2_queue->Pop(msg, POS_TIME_FOREVER), POS_OK);
        EXPECT_EQ(msg.val, i);
        msg.val = 100 + i;
        EXPECT_EQ(s_task1_queue->Push(msg), POS_OK);
    }
    return NULL;
}

TEST(OsQueueCpp, BasicPushPop) {
    poski::OsQueue<int, 4> queue;
    EXPECT_TRUE(queue.IsEmpty());
    
    EXPECT_EQ(queue.Push(42), POS_OK);
    EXPECT_FALSE(queue.IsEmpty());
    
    int val = 0;
    EXPECT_EQ(queue.Pop(val, POS_TIME_NO_WAIT), POS_OK);
    EXPECT_EQ(val, 42);
    EXPECT_TRUE(queue.IsEmpty());
}

TEST(OsQueueCpp, ConcurrencyFIFO) {
    s_task1_queue = new poski::OsQueue<test_msg_t, TEST_QUEUE_SIZE>();
    s_task2_queue = new poski::OsQueue<test_msg_t, TEST_QUEUE_SIZE>();
    
    poski::OsTask task1;
    poski::OsTask task2;

    EXPECT_EQ(task1.Start("task1", task1_run, NULL, 1, 1028), POS_OK);
    EXPECT_EQ(task2.Start("task2", task2_run, NULL, 1, 1028), POS_OK);

    poski::OsTask::Sleep(50);

    task1.Remove();
    task2.Remove();

    delete s_task1_queue;
    delete s_task2_queue;
}
