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

#ifndef CHIP_OS_QUEUE_H
#define CHIP_OS_QUEUE_H

#include "chip/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize a message queue.
 *
 * This routine initializes a message queue object, prior to its first use.
 *
 * Message queues deliver complete copies of fixed size messages from a producer
 * to a consumer in a thread-safe manner.
 *
 * @param msgq Address of the message queue.
 * @param msg_size Message size (in bytes).
 * @param max_msgs Maximum number of messages that can be queued.
 *
 * @return N/A
 */
chip_os_error_t chip_os_queue_init(struct chip_os_queue * msgq, size_t msg_size, size_t max_msgs);

/**
 * @brief Receive a message from a message queue.
 *
 * This routine receives a message from message queue @a q in a "first in,
 * first out" manner.
 *
 * @note Can be called by ISRs, but @a timeout must be set to CHIP_OS_TIME_NO_WAIT.
 *
 * @param msgq Address of the message queue.
 * @param data Address of area to hold the received message.
 * @param timeout Waiting period to receive the message (in milliseconds),
 *                or one of the special values CHIP_OS_TIME_NO_WAIT and CHIP_OS_TIME_FOREVER.
 *
 * @retval CHIP_OS_OK Message received.
 * @retval CHIP_OS_EBUSY Returned without waiting.
 * @retval CHIP_OS_TIMEOUT Waiting period timed out.
 */
chip_os_error_t chip_os_queue_get(struct chip_os_queue * msgq, void * data, chip_os_time_t timeout);

/**
 * @brief Send a message to a message queue.
 *
 * This routine sends a message to message queue @a q.
 *
 * @note Can be called by ISRs.
 *
 * @param msgq Address of the message queue.
 * @param data Pointer to the message.
 *
 * @retval CHIP_OS_OK Message sent.
 * @retval CHIP_OS_EBUSY Returned without waiting or queue purged.
 * @retval CHIP_OS_TIMEOUT Waiting period timed out.
 */
chip_os_error_t chip_os_queue_put(struct chip_os_queue * msgq, void * data);

/**
 * @brief Returns whether the given message queue is initialized and valid.
 *
 * @param msgq Address of the message queue.
 *
 * @retval CHIP_OS_OK Queue is valid.
 * @retval CHIP_OS_EINVAL Queue is not valid.
 */
int chip_os_queue_inited(const struct chip_os_queue * msgq);

bool chip_os_queue_is_empty(struct chip_os_queue * msgq);

void chip_os_queue_set_signal_cb(struct chip_os_queue * msgq, chip_os_signal_fn signal_cb, void * data);

#ifdef __cplusplus
}
#endif

#endif /* CHIP_OS_QUEUE_H */
