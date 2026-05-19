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

#ifndef POSKI_CPP_OS_SEMAPHORE_H
#define POSKI_CPP_OS_SEMAPHORE_H

#include "poski/osal/os_sem.h"
#include <assert.h>

namespace poski {

class OsSemaphore {
public:
    // Constructor initializes and optionally takes the semaphore immediately (RAII)
    explicit OsSemaphore(uint16_t initialTokens = 0, bool take_on_construction = false, pos_time_t timeout = POS_TIME_FOREVER)
        : taken_by_construction_(false) {
        pos_error_t err = pos_sem_init(&sem_, initialTokens);
        assert(err == POS_OK);
        if (take_on_construction) {
            err = pos_sem_take(&sem_, timeout);
            assert(err == POS_OK);
            taken_by_construction_ = true;
        }
    }

    // Destructor gives/increments the semaphore automatically if it was taken by constructor
    ~OsSemaphore() {
        if (taken_by_construction_) {
            pos_error_t err = pos_sem_give(&sem_);
            assert(err == POS_OK);
        }
    }

    // Prevent copy and assignment
    OsSemaphore(const OsSemaphore&) = delete;
    OsSemaphore& operator=(const OsSemaphore&) = delete;

    pos_error_t Take(pos_time_t timeout = POS_TIME_FOREVER) {
        return pos_sem_take(&sem_, timeout);
    }

    pos_error_t Give() {
        return pos_sem_give(&sem_);
    }

    struct pos_sem* GetNative() { return &sem_; }

private:
    struct pos_sem sem_;
    bool taken_by_construction_;
};

} // namespace poski
#endif // POSKI_CPP_OS_SEMAPHORE_H
