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

#include <poski/OsSemaphore.h>
#include <poski/OsTask.h>
#include "test_util.h"
#include <stdlib.h>

#define TEST_ITERATIONS 10

#define TASK1_PRIO POS_PRIORITY_APP
#define TASK2_PRIO POS_PRIORITY_APP

#define TASK1_STACK_SIZE 1028
#define TASK2_STACK_SIZE 1028

static poski::OsTask task1;
static poski::OsTask task2;

// Globally constructed, but NOT taken on construction
static poski::OsSemaphore task1_sem(0, false);
static poski::OsSemaphore task2_sem(0, false);

void * task1_run(void * arg)
{
    for (int i = 0; i < TEST_ITERATIONS; i++)
    {
        /* Release semaphore to task 2 */
        SuccessOrQuit(task1_sem.Give(), "OsSemaphore::Give: error releasing task2_sem.");

        /* Wait for semaphore from task 2 */
        SuccessOrQuit(task2_sem.Take(POS_TIME_FOREVER), "OsSemaphore::Take: error waiting for task2_sem.");
    }

    printf("All tests passed\n");
    exit(PASS);

    return NULL;
}

void * task2_run(void * arg)
{
    while (1)
    {
        /* Wait for semaphore from task1 */
        SuccessOrQuit(task1_sem.Take(POS_TIME_FOREVER), "OsSemaphore::Take: error waiting for task1_sem.");

        /* Release task2 semaphore */
        SuccessOrQuit(task2_sem.Give(), "OsSemaphore::Give: error releasing task1_sem.");
    }

    return NULL;
}

static int init_app_tasks(void)
{
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
