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

#ifndef POSKI_CPP_OS_TASK_H
#define POSKI_CPP_OS_TASK_H

#include "poski/osal/os_task.h"
#include "poski/osal/os_sched.h"
#include <assert.h>

namespace poski {

class OsTask {
public:
    OsTask() : inited_(false) {}
    ~OsTask() = default;
    OsTask(const OsTask&) = delete;
    OsTask& operator=(const OsTask&) = delete;

    pos_error_t Start(const char* name, pos_task_func_t func, void* arg, uint8_t prio, uint16_t stackSize) {
        pos_error_t err = pos_task_init(&task_, name, func, arg, prio, stackSize);
        if (err == POS_OK) {
            inited_ = true;
        }
        return err;
    }

    pos_error_t Remove() {
        if (inited_) {
            pos_error_t err = pos_task_remove(&task_);
            if (err == POS_OK) inited_ = false;
            return err;
        }
        return POS_ERROR;
    }

    static void Yield() {
        pos_task_yield();
    }

    static void Sleep(pos_time_t ms) {
        pos_task_sleep_ms(ms);
    }

    static void SleepTicks(pos_time_t ticks) {
        pos_task_sleep(ticks);
    }

    static void SleepMs(pos_time_t ms) {
        pos_task_sleep_ms(ms);
    }

    static void SchedStart() {
        pos_sched_start();
    }

    static bool SchedStarted() {
        return pos_sched_started();
    }

    static void* GetCurrentTaskId() {
        return pos_get_current_task_id();
    }

    struct pos_task* GetNative() { return &task_; }

private:
    struct pos_task task_;
    bool inited_;
};

} // namespace poski
#endif // POSKI_CPP_OS_TASK_H
