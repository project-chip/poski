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
 * Return true when called from an ISR context.  Uses FreeRTOS's
 * own ARMv7-M port helper (defined in portmacro.h as a single
 * mrs of IPSR), so this works on every M-class port without a
 * PAL or vendor-CMSIS dependency.
 */
static inline bool pos_hw_in_isr(void)
{
    return xPortIsInsideInterrupt() == pdTRUE;
}

#endif /* _OS_HW_H */
