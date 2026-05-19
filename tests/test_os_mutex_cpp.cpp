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

#include <poski/OsMutex.h>
#include <poski/OsTask.h>
#include "test_util.h"
#include <stdlib.h>
#include <string.h>

#define TEST_ITERATIONS 10
#define TASK1_VALUE 0x11
#define TASK2_VALUE 0x22

#define TASK1_PRIO 1
#define TASK2_PRIO 1

#define TASK1_STACK_SIZE 1028
#define TASK2_STACK_SIZE 1028

static poski::OsTask task1;
static poski::OsTask task2;

// Globally constructed, but NOT locked on construction
static poski::OsMutex task1_mtx(false);

uint8_t s_buffer[TEST_ITERATIONS];

void test_mutex_nested(uint8_t test_value, int task, bool recursive, int round)
{
    TEST_LOG("task %d: LOCK START ROUND %d\n", task, round);
    SuccessOrQuit(task1_mtx.Lock(POS_TIME_FOREVER), "OsMutex::Lock: error waiting for task1_mutex.");
    for (int i = 0; i < TEST_ITERATIONS; i++)
    {
        if (recursive)
        {
            TEST_LOG("task %d: take #%d RECURSIVE\n", task, i);
            SuccessOrQuit(task1_mtx.Lock(POS_TIME_FOREVER), "OsMutex::Lock: error waiting for task1_mutex.");
        }
        VerifyOrQuit(s_buffer[i] != test_value, "unexpected value: mutex did not protect resource");
        s_buffer[i] = test_value;
    }

    for (int i = 0; i < TEST_ITERATIONS; i++)
    {
        VerifyOrQuit(s_buffer[i] == test_value, "unexpected value: mutex did not protect resource");
        if (recursive)
        {
            TEST_LOG("task %d: give #%d RECURSIVE\n", task, i);
            SuccessOrQuit(task1_mtx.Unlock(), "OsMutex::Unlock: error releasing task1_mutex.");
        }
    }
    memset(&s_buffer, 0, sizeof(s_buffer));
    TEST_LOG("task %d: LOCK END ROUND %d\n", task, round);
    SuccessOrQuit(task1_mtx.Unlock(), "OsMutex::Unlock: error releasing task1_mutex.");

    poski::OsTask::Sleep(10);
}

void * task1_run(void * arg)
{
    for (int round = 0; round < TEST_ITERATIONS; round++)
    {
        bool recursive = (round % 5 == 0);
        test_mutex_nested(TASK1_VALUE, 1, recursive, round);
    }

    printf("All tests passed\n");
    exit(PASS);

    return NULL;
}

void * task2_run(void * arg)
{
    int i = 0;
    while (1)
    {
        bool recursive = (i % 5 == 0);
        test_mutex_nested(TASK2_VALUE, 2, recursive, i);
        i++;
    }

    return NULL;
}

static int init_app_tasks(void)
{
    memset(&s_buffer, 0, sizeof(s_buffer));

    task1.Start("task1", task1_run, NULL, TASK1_PRIO, TASK1_STACK_SIZE);
    task2.Start("task2", task2_run, NULL, TASK2_PRIO, TASK2_STACK_SIZE);

    return 0;
}

int main(int argc, char ** arg)
{
    init_app_tasks();
    poski::OsTask::SchedStart();
    return FAIL;
}
