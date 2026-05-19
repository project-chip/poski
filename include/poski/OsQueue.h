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

#ifndef POSKI_CPP_OS_QUEUE_H
#define POSKI_CPP_OS_QUEUE_H

#include "poski/osal/os_queue.h"
#include <assert.h>
#include <stddef.h>

namespace poski {

template <typename T, size_t QueueSize>
class OsQueue {
public:
    OsQueue() : queue_{}, initialized_(false), init_status_(POS_EINVAL) {
        init_status_ = pos_queue_init(&queue_, sizeof(T), QueueSize);
        initialized_ = (init_status_ == POS_OK);
        assert(init_status_ == POS_OK);
    }

    ~OsQueue() {
        if (!initialized_) {
            return;
        }
        pos_error_t err = pos_queue_deinit(&queue_);
        assert(err == POS_OK);
        if (err == POS_OK) {
            initialized_ = false;
        }
    }
    OsQueue(const OsQueue&) = delete;
    OsQueue& operator=(const OsQueue&) = delete;

    pos_error_t Pop(T& item, pos_time_t timeout = POS_TIME_FOREVER) {
        if (!initialized_) {
            return POS_EINVAL;
        }
        return pos_queue_get(&queue_, &item, timeout);
    }

    pos_error_t Push(const T& item) {
        if (!initialized_) {
            return POS_EINVAL;
        }
        T mutable_item = item;
        return pos_queue_put(&queue_, &mutable_item);
    }

    bool IsEmpty() {
        if (!initialized_) {
            return false;
        }
        return pos_queue_is_empty(&queue_);
    }

    bool Inited() {
        return initialized_ && pos_queue_inited(&queue_);
    }

    void SetSignalCallback(pos_signal_fn signal_cb, void * data) {
        if (!initialized_) {
            return;
        }
        pos_queue_set_signal_cb(&queue_, signal_cb, data);
    }

    struct pos_queue* GetNative() { return initialized_ ? &queue_ : NULL; }

private:
    struct pos_queue queue_;
    bool initialized_;
    pos_error_t init_status_;
};

} // namespace poski
#endif // POSKI_CPP_OS_QUEUE_H
