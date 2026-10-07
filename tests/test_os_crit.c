/*
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

/**
  Unit tests for the Critical Section api (pos_crit):

  pos_crit_state_t pos_crit_enter(void);
  void pos_crit_exit(pos_crit_state_t state);
  bool pos_crit_is_active(void);
  bool pos_crit_in_isr(void);
*/

#include <stdint.h>

#include <poski/osal/osal.h>

#include "test_util.h"

#define TEST_ITERATIONS 100000

#define TASK_PRIO 1
#define TASK_STACK_SIZE 1028

static struct pos_task s_task1;
static struct pos_task s_task2;
static struct pos_sem s_task2_done;

/* Deliberately updated with a non-atomic read-modify-write: the final count is
 * only correct if the critical section provides mutual exclusion. */
static volatile uint32_t s_counter;

static void test_crit_nesting(void)
{
    VerifyOrQuit(!pos_crit_is_active(), "crit: should not be active initially");
    VerifyOrQuit(!pos_crit_in_isr(), "crit: should not be in ISR in task context");

    pos_crit_state_t s1 = pos_crit_enter();
    VerifyOrQuit(pos_crit_is_active(), "crit: should be active after first enter");

    /* Verify nested critical sections (using the atomic aliases) */
    pos_atomic_state_t s2 = pos_atomic_enter();
    VerifyOrQuit(pos_atomic_is_active(), "crit: should remain active in nested enter");

    pos_atomic_exit(s2);
    VerifyOrQuit(pos_crit_is_active(), "crit: should still be active after inner exit");

    pos_crit_exit(s1);
    VerifyOrQuit(!pos_crit_is_active(), "crit: should be inactive after outer exit");
}

static void test_crit_increment(void)
{
    for (uint32_t i = 0; i < TEST_ITERATIONS; i++)
    {
        /* Another task's critical section must never be reported to this one. */
        VerifyOrQuit(!pos_crit_is_active(), "crit: should not be active outside of this task's critical section");

        pos_crit_state_t state = pos_crit_enter();
        VerifyOrQuit(pos_crit_is_active(), "crit: should be active inside critical section");
        uint32_t value = s_counter;
        s_counter      = value + 1;
        pos_crit_exit(state);
    }
}

/* Task 1 handler function */
static void * task1_run(void * arg)
{
    (void) arg;

    test_crit_nesting();
    test_crit_increment();

    SuccessOrQuit(pos_sem_take(&s_task2_done, POS_TIME_FOREVER), "pos_sem_take: error waiting for task2");
    VerifyOrQuit(s_counter == 2 * TEST_ITERATIONS, "crit: critical section did not protect shared counter");

    printf("All critical section tests passed\n");
    exit(PASS);

    return NULL;
}

/* Task 2 handler function */
static void * task2_run(void * arg)
{
    (void) arg;

    test_crit_increment();
    SuccessOrQuit(pos_sem_give(&s_task2_done), "pos_sem_give: error signaling task1");

    /* RTOS task functions must not return. */
    while (1)
    {
        pos_task_sleep_ms(1000);
    }

    return NULL;
}

int main(int argc, char ** argv)
{
    (void) argc;
    (void) argv;

    SuccessOrQuit(pos_sem_init(&s_task2_done, 0), "pos_sem_init: error initializing semaphore");

    SuccessOrQuit(pos_task_init(&s_task1, "crit1", task1_run, NULL, TASK_PRIO, TASK_STACK_SIZE),
                  "pos_task_init: error initializing task1");
    SuccessOrQuit(pos_task_init(&s_task2, "crit2", task2_run, NULL, TASK_PRIO, TASK_STACK_SIZE),
                  "pos_task_init: error initializing task2");

    pos_sched_start();

    /* main never returns */

    return FAIL;
}
