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

#include <poski/OsQueue.h>
#include <poski/OsTask.h>
#include "test_util.h"
#include <stdlib.h>

#define TEST_ITERATIONS (100)
#define TEST_TASK_PRIO (1)
#define TEST_STACK_SIZE (1024)
#define TEST_QUEUE_SIZE (32)

typedef struct
{
    int _type;
    int _value;
    int _param;
} my_msg_t;

static poski::OsTask s_task1;
static poski::OsTask s_task2;

// Use pointers to control dynamic instantiation if needed,
// but queue template works statically in POSIX.
static poski::OsQueue<my_msg_t, TEST_QUEUE_SIZE>* s_task1_queue = nullptr;
static poski::OsQueue<my_msg_t, TEST_QUEUE_SIZE>* s_task2_queue = nullptr;
static poski::OsQueue<my_msg_t, TEST_QUEUE_SIZE>* s_queue = nullptr;

static my_msg_t s_task1_msg_rx;
static my_msg_t s_task2_msg_rx;

my_msg_t s_task1_msgs_src[] = {
    { ._type = 0, ._value = 0, ._param = 10000 },
    { ._type = 1, ._value = 1, ._param = 10001 },
    { ._type = 2, ._value = 2, ._param = 10002 },
    { ._type = 3, ._value = 3, ._param = 10003 },
};

my_msg_t s_task2_msgs_src[] = {
    { ._type = 0, ._value = 20, ._param = 20000 },
    { ._type = 1, ._value = 21, ._param = 20001 },
    { ._type = 2, ._value = 22, ._param = 20002 },
    { ._type = 3, ._value = 23, ._param = 20003 },
};

#define TEST_MSGS_COUNT (sizeof(s_task1_msgs_src) / sizeof(my_msg_t))

void test_queue_init()
{
    s_queue = new poski::OsQueue<my_msg_t, TEST_QUEUE_SIZE>();
    VerifyOrQuit(s_queue->Inited(), "queue: not initialized");
}

void test_put(void)
{
    s_queue->Push(s_task1_msgs_src[0]);
}

void test_get_verify(my_msg_t * msg_rx, my_msg_t * msg_src)
{
    VerifyOrQuit(msg_rx->_type == msg_src->_type, "queue: wrong msg type passed");
    VerifyOrQuit(msg_rx->_value == msg_src->_value, "queue: wrong msg value passed");
    VerifyOrQuit(msg_rx->_param == msg_src->_param, "queue: wrong msg param passed");
}

void test_get(void)
{
    s_queue->Pop(s_task1_msg_rx, POS_TIME_FOREVER);
    test_get_verify(&s_task1_msg_rx, &s_task1_msgs_src[0]);
}

void test_queue_units()
{
    test_queue_init();
    test_put();
    test_get();
    test_put();
}

void * task1_run(void * arg)
{
    int i, idx;

    test_queue_units();

    for (i = 0; i < TEST_ITERATIONS; i++)
    {
        idx = i % TEST_MSGS_COUNT;

        SuccessOrQuit(s_task2_queue->Push(s_task2_msgs_src[idx]), "OsQueue::Push: error posting.");
        SuccessOrQuit(s_task1_queue->Pop(s_task1_msg_rx, POS_TIME_FOREVER), "OsQueue::Pop: error waiting.");

        test_get_verify(&s_task1_msg_rx, &s_task1_msgs_src[idx]);
        TEST_LOG("task1: recieved msg.value = %d\n", s_task1_msg_rx._value);
    }

    printf("All tests passed\n");
    exit(PASS);
}

void * task2_run(void * arg)
{
    int i = 0;
    int idx;

    while (1)
    {
        idx = i++ % TEST_MSGS_COUNT;

        SuccessOrQuit(s_task2_queue->Pop(s_task2_msg_rx, POS_TIME_FOREVER), "OsQueue::Pop: error waiting.");
        test_get_verify(&s_task2_msg_rx, &s_task2_msgs_src[idx]);
        TEST_LOG("task2: recieved msg.value = %d\n", s_task2_msg_rx._value);

        SuccessOrQuit(s_task1_queue->Push(s_task1_msgs_src[idx]), "OsQueue::Push: error posting.");
    }
}

int main(void)
{
    s_task1_queue = new poski::OsQueue<my_msg_t, TEST_QUEUE_SIZE>();
    s_task2_queue = new poski::OsQueue<my_msg_t, TEST_QUEUE_SIZE>();

    SuccessOrQuit(s_task1.Start("task1", task1_run, NULL, TEST_TASK_PRIO, TEST_STACK_SIZE),
                  "OsTask::Start: failed to start task1.");
    SuccessOrQuit(s_task2.Start("task2", task2_run, NULL, TEST_TASK_PRIO, TEST_STACK_SIZE),
                  "OsTask::Start: failed to start task2.");

    poski::OsTask::SchedStart();

    return FAIL;
}
