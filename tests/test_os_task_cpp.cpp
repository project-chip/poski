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

#include <poski/OsTask.h>
#include "test_util.h"
#include <stdlib.h>

#define TASK0_ARG 55
#define TASK1_ARG 66

#define TEST_TASK_PRIORITY POS_PRIORITY_APP
#define TEST_TASK_STACK_SIZE 1028

static poski::OsTask s_task[2];
static int s_task_arg[2] = { TASK0_ARG, TASK1_ARG };

void * task0_run(void * args)
{
    VerifyOrQuit(args == &s_task_arg[0], "Wrong args passed to task0");

    while (1)
    {
        poski::OsTask::Yield();
    }

    return NULL;
}

void * task1_run(void * args)
{
    int i = 10000;
    VerifyOrQuit(args == &s_task_arg[1], "Wrong args passed to task1");

    while (i--)
    {
        poski::OsTask::Yield();
    }

    printf("All tests passed\n");
    exit(PASS);

    return NULL;
}

int test_init(void)
{
    int err;
    
    err = s_task[0].Start("s_task[0]", task0_run, &s_task_arg[0], TEST_TASK_PRIORITY, TEST_TASK_STACK_SIZE);
    SuccessOrQuit(err, "OsTask::Start failed.");

    err = s_task[1].Start("s_task[1]", task1_run, &s_task_arg[1], TEST_TASK_PRIORITY, TEST_TASK_STACK_SIZE);
    SuccessOrQuit(err, "OsTask::Start failed.");

    return err;
}

int main(void)
{
    SuccessOrQuit(test_init(), "Failed: OsTask::Start");

    poski::OsTask::SchedStart();

    return FAIL;
}
