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

#include <rthw.h>
#include <rtthread.h>

#include <poski/osal/osal.h>

static volatile uint32_t s_crit_nesting = 0;

pos_crit_state_t pos_crit_enter(void)
{
    rt_base_t level = rt_hw_interrupt_disable();
    s_crit_nesting++;
    return (pos_crit_state_t) level;
}

void pos_crit_exit(pos_crit_state_t state)
{
    if (s_crit_nesting > 0)
    {
        s_crit_nesting--;
    }
    rt_hw_interrupt_enable((rt_base_t) state);
}

bool pos_crit_is_active(void)
{
    return s_crit_nesting > 0;
}

bool pos_crit_in_isr(void)
{
    return rt_interrupt_get_nest() > 0;
}
