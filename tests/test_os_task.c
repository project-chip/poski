/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2018 Google LLC
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

#include <poski/osal/osal.h>

#include "test_util.h"

#define TASK0_ARG 55
#define TASK1_ARG 66

#define TEST_TASK_PRIORITY POS_PRIORITY_APP
#define TEST_TASK_STACK_SIZE 1028

static struct pos_task s_task[2];
static int s_task_arg[2] = { TASK0_ARG, TASK1_ARG };

void * task0_run(void * args)
{
    VerifyOrQuit(args == &s_task_arg[0], "Wrong args passed to task0");

    while (1)
    {
        pos_task_yield();
    }

    return NULL;
}

void * task1_run(void * args)
{
    int i = 10000;
    VerifyOrQuit(args == &s_task_arg[1], "Wrong args passed to task1");

    while (i--)
    {
        pos_task_yield();
    }

    printf("All tests passed\n");
    exit(PASS);

    return NULL;
}

/**
 * Unit test for initializing a task.
 *
 * int pos_task_init(struct pos_task *t, const char *name, pos_task_func_t func,
 *                  void *arg, uint8_t prio, pos_time_t sanity_itvl,
 *                  pos_stack_t *stack_bottom, uint16_t stack_size)
 *
 */
int test_init(void)
{
    int err;
    err = pos_task_init(NULL, "Null task", NULL, NULL, TEST_TASK_PRIORITY, TEST_TASK_STACK_SIZE);
    VerifyOrQuit(err, "pos_task_init accepted NULL parameters.");

    err = pos_task_init(&s_task[0], "s_task[0]", task0_run, &s_task_arg[0], TEST_TASK_PRIORITY, TEST_TASK_STACK_SIZE);
    SuccessOrQuit(err, "pos_task_init failed.");

    err = pos_task_init(&s_task[1], "s_task[1]", task1_run, &s_task_arg[1], TEST_TASK_PRIORITY, TEST_TASK_STACK_SIZE);

    return err;
}

int main(void)
{
    SuccessOrQuit(test_init(), "Failed: pos_task_init");

    pos_sched_start();

    /* main never returns */

    exit(FAIL);
}
