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

#ifndef POSKI_OS_ZEPHYR_OS_UTILS_H
#define POSKI_OS_ZEPHYR_OS_UTILS_H

#include <poski/osal/osal.h>
#include <assert.h>

#define SuccessOrQuit(ERR, MSG) \
  assert((MSG, ERR == 0))

#define VerifyOrQuit(TST, MSG) \
  assert((MSG, TST))

#define SuccessOrExit(aStatus) \
  do {                         \
    if ((aStatus) != 0) {      \
      goto exit;               \
    }                          \
  } while (false)

#define VerifyOrExit(aCondition, aAction) \
  do {                                    \
    if (!(aCondition)) {                  \
      aAction;                            \
      goto exit;                          \
    }                                     \
  } while (false)

pos_error_t map_zephyr_to_osal_error(int ret);

#endif // POSKI_OS_ZEPHYR_OS_UTILS_H
