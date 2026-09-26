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
 * This uses the FreeRTOS port layer's own ISR query, never the CPU directly.
 * FreeRTOS has no query that every port implements, so this takes:
 *
 *   - portCHECK_IF_IN_ISR(), if the port defines it (ESP-IDF v5.1+, RP2040,
 *     XCORE.AI), else
 *   - xPortIsInsideInterrupt(), which the GCC, IAR and Keil Cortex-M ports
 *     provide (ARM_CM0 since FreeRTOS V10.6.0).
 *
 * xPortIsInsideInterrupt() is a function rather than a macro in most ports, so
 * it cannot be probed with `#if defined(...)`.  A port with neither query
 * therefore fails to build here, instead of silently sending ISR calls down
 * task-level kernel paths.  Its platform supplies one, typically by defining
 * portCHECK_IF_IN_ISR() in FreeRTOSConfig.h, as config/posix does for the
 * POSIX simulator.
 */
static inline bool pos_hw_in_isr(void)
{
#if defined(portCHECK_IF_IN_ISR)
    return portCHECK_IF_IN_ISR() != pdFALSE;
#else
    return xPortIsInsideInterrupt() != pdFALSE;
#endif
}

#endif /* _OS_HW_H */
