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

#include <assert.h>
#include <pthread.h>

#include <poski/osal/osal.h>

/*
 * Hosted targets have no interrupts to mask, so a critical section is emulated
 * with a single process-wide lock.  The nesting depth is tracked per thread:
 * the lock is only taken by the outermost pos_crit_enter() and released by the
 * matching outermost pos_crit_exit(), and pos_crit_is_active() reports whether
 * the *calling* thread is inside a critical section.
 */
static pthread_mutex_t s_crit_mutex = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local unsigned s_crit_nesting;

pos_crit_state_t pos_crit_enter(void)
{
    if (s_crit_nesting++ == 0)
    {
        pthread_mutex_lock(&s_crit_mutex);
    }
    return 0;
}

void pos_crit_exit(pos_crit_state_t state)
{
    (void) state;
    assert(s_crit_nesting > 0);
    if (s_crit_nesting > 0 && --s_crit_nesting == 0)
    {
        pthread_mutex_unlock(&s_crit_mutex);
    }
}

bool pos_crit_is_active(void)
{
    return s_crit_nesting > 0;
}

bool pos_crit_in_isr(void)
{
    return false;
}
