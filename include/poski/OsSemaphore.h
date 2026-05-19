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

#ifndef POSKI_CPP_OS_SEMAPHORE_H
#define POSKI_CPP_OS_SEMAPHORE_H

#include "poski/osal/os_sem.h"
#include <assert.h>
#include <stdint.h>
#include <atomic>

namespace poski {

class OsSemaphore {
public:
    // Constructor initializes and optionally takes the semaphore immediately (RAII)
    explicit OsSemaphore(uint16_t initialTokens = 0, bool take_on_construction = false, pos_time_t timeout = POS_TIME_FOREVER)
        : take_depth_(0) {
        pos_error_t err = pos_sem_init(&sem_, initialTokens);
        assert(err == POS_OK);
        if (take_on_construction) {
            err = pos_sem_take(&sem_, timeout);
            assert(err == POS_OK);
            take_depth_ = 1;
        }
    }

    // Destructor recursively gives/increments the semaphore automatically if held
    ~OsSemaphore() {
        while (take_depth_ > 0) {
            pos_error_t err = pos_sem_give(&sem_);
            assert(err == POS_OK);
            take_depth_--;
        }
    }

    // Prevent copy and assignment
    OsSemaphore(const OsSemaphore&) = delete;
    OsSemaphore& operator=(const OsSemaphore&) = delete;

    pos_error_t Take(pos_time_t timeout = POS_TIME_FOREVER) {
        pos_error_t err = pos_sem_take(&sem_, timeout);
        if (err == POS_OK) {
            take_depth_++;
        }
        return err;
    }

    pos_error_t Give() {
        pos_error_t err = pos_sem_give(&sem_);
        if (err == POS_OK) {
            take_depth_--;
        }
        return err;
    }

    struct pos_sem* GetNative() { return &sem_; }

private:
    struct pos_sem sem_;
    std::atomic<int32_t> take_depth_;
};

} // namespace poski
#endif // POSKI_CPP_OS_SEMAPHORE_H
