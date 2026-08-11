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

#ifndef POSKI_OS_PANIC_H
#define POSKI_OS_PANIC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Halts execution immediately upon unrecoverable kernel/driver failure.
 * @param msg Diagnostic string describing the failure condition (may be NULL).
 */
__attribute__((noreturn)) void pos_panic(const char *msg);

#ifdef __cplusplus
}
#endif

#endif /* POSKI_OS_PANIC_H */
