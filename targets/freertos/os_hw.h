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

#include "FreeRTOS.h"

/*
 * Return true when called from an ISR context.
 *
 * This asks the FreeRTOS port layer, never the CPU directly.  FreeRTOS has no
 * ISR query that every port implements, so the first one available is used:
 *
 *   1. POS_FREERTOS_IN_ISR(), if defined (e.g. in FreeRTOSConfig.h) for a port
 *      with neither query below: e.g. `xPortInIsrContext()` on ESP-IDF before
 *      v5.1, `(uxInterruptNesting != 0)` on PIC32, or `0` on the POSIX
 *      simulator, which has no interrupt context.
 *   2. portCHECK_IF_IN_ISR(), defined by ESP-IDF (v5.1+), RP2040 and XCORE.AI.
 *   3. xPortIsInsideInterrupt(), provided by the GCC, IAR and Keil Cortex-M
 *      ports (ARM_CM0 since FreeRTOS V10.6.0).
 *
 * xPortIsInsideInterrupt() is a function rather than a macro in most ports, so
 * it cannot be probed with `#if defined(...)`.  A port that lacks it therefore
 * fails to build here, instead of silently sending ISR calls down task-level
 * kernel paths.
 */
static inline bool pos_hw_in_isr(void)
{
#if defined(POS_FREERTOS_IN_ISR)
    return POS_FREERTOS_IN_ISR() != 0;
#elif defined(portCHECK_IF_IN_ISR)
    return portCHECK_IF_IN_ISR() != 0;
#else
    return xPortIsInsideInterrupt() != pdFALSE;
#endif
}

#endif /* _OS_HW_H */
