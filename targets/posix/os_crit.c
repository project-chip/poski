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

#include <pthread.h>
#include <stdatomic.h>

#include <poski/osal/osal.h>

static pthread_once_t s_crit_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t s_crit_mutex;
static atomic_int s_crit_nesting = 0;

static void init_crit_mutex(void)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&s_crit_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

pos_crit_state_t pos_crit_enter(void)
{
    pthread_once(&s_crit_once, init_crit_mutex);
    pthread_mutex_lock(&s_crit_mutex);
    return (pos_crit_state_t) atomic_fetch_add(&s_crit_nesting, 1);
}

void pos_crit_exit(pos_crit_state_t state)
{
    (void) state;
    atomic_fetch_sub(&s_crit_nesting, 1);
    pthread_mutex_unlock(&s_crit_mutex);
}

bool pos_crit_is_active(void)
{
    return atomic_load(&s_crit_nesting) > 0;
}

bool pos_crit_in_isr(void)
{
    return false;
}
