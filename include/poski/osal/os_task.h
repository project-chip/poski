/*
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2018 Google LLC
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef CHIP_OS_TASK_H
#define CHIP_OS_TASK_H

#include "poski/osal/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a task.
 *
 * This routine initializes a task, then schedules it for immediate execution.
 * Typically, tasks are created and then started with @a chip_os_sched_start.
 *
 * @param t Pointer to uninitialized struct chip_os_task.
 * @param name Debugging name of the task.
 * @param func Task entry function.
 * @param arg Argument parameter pointer.
 * @param prio task priority.
 * @param stack_size Stack size in bytes.
 *
 * @retval CHIP_OS_OK Task is initialized.
 * @retval CHIP_OS_ERROR Task is invalid.
 */
chip_os_error_t chip_os_task_init(struct chip_os_task * t, const char * name, chip_os_task_func_t func, void * arg, uint8_t prio,
                                  uint16_t stack_size);

/**
 * @brief Yield the current task.
 *
 * This routine causes the current task to yield execution to another
 * task of the same or higher priority. If there are no other ready tasks
 * of the same or higher priority, the routine returns immediately.
 *
 * @return N/A
 */
void chip_os_task_yield(void);

/**
 * @brief Put the current thread to sleep.
 *
 * This routine puts the current thread to sleep for @a duration
 * [ticks].
 *
 * @param ticks Number of CPU ticks to sleep.
 *
 * @return N/A
 */
void chip_os_task_sleep(chip_os_time_t ticks);

void chip_os_task_sleep_ms(chip_os_time_t ms);

/**
 * @brief Get task ID of the current task.
 *
 * @return ID of current task.
 */
void * chip_os_get_current_task_id(void);

/**
 * @brief Abort a task.
 *
 * This routine permanently stops execution of @a task. The task is taken
 * off all kernel queues it is part of (i.e. the ready queue, the timeout
 * queue, or a kernel object wait queue). However, any kernel resources the
 * task might currently own (such as mutexes or memory blocks) are not
 * released. It is the responsibility of the caller of this routine to ensure
 * all necessary cleanup is performed.
 *
 * @param task ID of task to abort.
 *
 * @retval CHIP_OS_OK Task was removed.
 * @retval CHIP_OS_ERROR Problem removing task.
 */
chip_os_error_t chip_os_task_remove(struct chip_os_task * t);

#ifdef __cplusplus
}
#endif

#endif /* CHIP_OS_TASK_H */
