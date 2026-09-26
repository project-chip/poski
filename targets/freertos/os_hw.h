/*
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

#ifndef _OS_HW_H
#define _OS_HW_H

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"

/*
 * Return true when called from an ISR context.
 *
 * On ARMv6-M / ARMv7-M / ARMv8-M the IPSR register holds the number of the
 * active exception (0 in thread mode).  Reading it directly works on every
 * M-profile port (including ARM_CM0, which has no xPortIsInsideInterrupt())
 * without a PAL or vendor-CMSIS dependency.  Note that xPortIsInsideInterrupt()
 * is an inline function rather than a macro, so it cannot be probed with
 * `#if defined(...)`.
 *
 * Hosted simulation ports (e.g. the FreeRTOS POSIX port) have no interrupt
 * context, so they always report task context.
 */
static inline bool pos_hw_in_isr(void)
{
#if defined(__ARM_ARCH_PROFILE) && (__ARM_ARCH_PROFILE == 'M')
    uint32_t ipsr;
    __asm volatile("mrs %0, ipsr" : "=r"(ipsr)::"memory");
    return ipsr != 0;
#else
    return false;
#endif
}

#endif /* _OS_HW_H */
