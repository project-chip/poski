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

#ifndef POSKI_CPP_OS_MUTEX_H
#define POSKI_CPP_OS_MUTEX_H

#include "poski/osal/os_mutex.h"
#include <assert.h>
#include <stdint.h>

namespace poski {

class OsMutex {
public:
    // Constructor initializes and optionally locks the mutex immediately (RAII)
    explicit OsMutex(bool lock_on_construction = false, pos_time_t timeout = POS_TIME_FOREVER) 
        : lock_depth_(0) {
        pos_error_t err = pos_mutex_init(&mutex_);
        assert(err == POS_OK);
        if (lock_on_construction) {
            err = pos_mutex_lock(&mutex_, timeout);
            assert(err == POS_OK);
            lock_depth_ = 1;
        }
    }
    
    // Destructor recursively releases the lock automatically if held
    ~OsMutex() {
        while (lock_depth_ > 0) {
            pos_error_t err = pos_mutex_unlock(&mutex_);
            assert(err == POS_OK);
            lock_depth_--;
        }
    }
    
    // Prevent copy and assignment
    OsMutex(const OsMutex&) = delete;
    OsMutex& operator=(const OsMutex&) = delete;

    pos_error_t Lock(pos_time_t timeout = POS_TIME_FOREVER) {
        pos_error_t err = pos_mutex_lock(&mutex_, timeout);
        if (err == POS_OK) {
            lock_depth_++;
        }
        return err;
    }

    pos_error_t Unlock() {
        assert(lock_depth_ > 0);
        lock_depth_--; // Decrement first while holding the lock
        
        pos_error_t err = pos_mutex_unlock(&mutex_);
        if (err != POS_OK) {
            // Revert if unlock failed
            lock_depth_++;
        }
        return err;
    }

    struct pos_mutex* GetNative() { return &mutex_; }

private:
    struct pos_mutex mutex_;
    uint32_t lock_depth_;
};

} // namespace poski
#endif // POSKI_CPP_OS_MUTEX_H
