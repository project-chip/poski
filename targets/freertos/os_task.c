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
#include <poski/osal/os_sched.h>

#include <assert.h>

void pos_task_dispatch(void * arg)
{
    struct pos_task * task = (struct pos_task *) arg;

    assert(task);
    assert(task->func);
    task->func(task->arg);
}

pos_error_t pos_task_init(struct pos_task * task, const char * name, pos_task_func_t func, void * arg, uint8_t prio,
                                  uint16_t stack_size)
{
    pos_base_t err;

    if ((task == NULL) || (func == NULL))
    {
        return POS_INVALID_PARAM;
    }

    task->func = func;
    task->arg  = arg;

    err = xTaskCreate(pos_task_dispatch, name, stack_size / sizeof(pos_base_t), task, prio, &task->handle);

    return (err == pdPASS) ? POS_OK : POS_ENOMEM;
}

/* Abort a task and reclaim its TCB/stack. */
pos_error_t pos_task_remove(struct pos_task * t)
{
    TaskHandle_t h = NULL;

    if (t == NULL)
        return POS_INVALID_PARAM;

    taskENTER_CRITICAL();
    h = t->handle;
    if (h != NULL)
    {
        t->handle = NULL;
    }
    taskEXIT_CRITICAL();

    if (h == NULL)
        return POS_INVALID_PARAM;

    vTaskDelete(h);
    return POS_OK;
}

/* Public-API shim for the existing inline pos_os_started()
 * helper in os_port.h.  Kept inline + bridge to avoid renaming the
 * internal helper that other backend code already uses. */
bool pos_sched_started(void)
{
    return pos_os_started();
}
