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

#ifndef POSKI_OS_QUEUE_H
#define POSKI_OS_QUEUE_H

#include "poski/osal/os_types.h"

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
pos_error_t pos_queue_init(struct pos_queue * msgq, size_t msg_size, size_t max_msgs);

/**
 * @brief Receive a message from a message queue.
 *
 * This routine receives a message from message queue @a q in a "first in,
 * first out" manner.
 *
 * @note Can be called by ISRs, but @a timeout must be set to POS_TIME_NO_WAIT.
 *
 * @param msgq Address of the message queue.
 * @param data Address of area to hold the received message.
 * @param timeout Waiting period to receive the message (in milliseconds),
 *                or one of the special values POS_TIME_NO_WAIT and POS_TIME_FOREVER.
 *
 * @retval POS_OK Message received.
 * @retval POS_EBUSY Returned without waiting.
 * @retval POS_TIMEOUT Waiting period timed out.
 */
pos_error_t pos_queue_get(struct pos_queue * msgq, void * data, pos_time_t timeout);

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
 * @retval POS_OK Message sent.
 * @retval POS_EBUSY Returned without waiting or queue purged.
 * @retval POS_TIMEOUT Waiting period timed out.
 */
pos_error_t pos_queue_put(struct pos_queue * msgq, void * data);

/**
 * @brief Returns whether the given message queue is initialized and valid.
 *
 * @param msgq Address of the message queue.
 *
 * @retval POS_OK Queue is valid.
 * @retval POS_EINVAL Queue is not valid.
 */
int pos_queue_inited(const struct pos_queue * msgq);

bool pos_queue_is_empty(struct pos_queue * msgq);

void pos_queue_set_signal_cb(struct pos_queue * msgq, pos_signal_fn signal_cb, void * data);

#ifdef __cplusplus
}
#endif

#endif /* POSKI_OS_QUEUE_H */
