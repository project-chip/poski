/*
 *    Copyright (c) 2026 Project CHIP Authors
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

/**
 * @file
 *   OSAL Critical Section and short-duration atomic interrupt-masking API.
 *
 *   Provides nestable interrupt lock/unlock primitives for protecting short
 *   critical sections across both task and ISR contexts, plus ISR context
 *   detection (`pos_crit_in_isr`).
 */

#ifndef POSKI_OS_CRIT_H
#define POSKI_OS_CRIT_H

#include "poski/osal/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque saved interrupt state returned by `pos_crit_enter()` and
 *        passed to `pos_crit_exit()`.
 */
typedef uint32_t pos_crit_state_t;

/**
 * @brief Enter a critical section by masking interrupts (or acquiring the
 *        critical section lock on hosted targets).
 *
 * Calls to `pos_crit_enter()` and `pos_crit_exit()` may be nested; each enter
 * must be paired with a corresponding `pos_crit_exit(state)` in LIFO order.
 *
 * @return Saved interrupt/critical state token to pass to `pos_crit_exit()`.
 */
pos_crit_state_t pos_crit_enter(void);

/**
 * @brief Exit a critical section and restore the previous interrupt state.
 *
 * @param state Saved interrupt state returned by the matching `pos_crit_enter()`.
 */
void pos_crit_exit(pos_crit_state_t state);

/**
 * @brief Return true if currently executing inside a critical section.
 */
bool pos_crit_is_active(void);

/**
 * @brief Return true if the caller is currently executing in an hardware
 *        Interrupt Service Routine (ISR) context.
 */
bool pos_crit_in_isr(void);

/* Atomic critical-section aliases */
typedef pos_crit_state_t pos_atomic_state_t;

static inline pos_atomic_state_t pos_atomic_enter(void)
{
    return pos_crit_enter();
}

static inline void pos_atomic_exit(pos_atomic_state_t state)
{
    pos_crit_exit(state);
}

static inline bool pos_atomic_is_active(void)
{
    return pos_crit_is_active();
}

static inline bool pos_atomic_in_isr(void)
{
    return pos_crit_in_isr();
}

#ifdef __cplusplus
}
#endif

#endif /* POSKI_OS_CRIT_H */
