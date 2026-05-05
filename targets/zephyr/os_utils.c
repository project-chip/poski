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

#include <errno.h>
#include <chip/osal.h>
#include "os_utils.h"

chip_os_error_t map_zephyr_to_osal_error(int ret)
{
    chip_os_error_t err;

    switch (ret)
    {
    case 0:
        err = CHIP_OS_OK;
        break;
    case -ENOMEM:
        err = CHIP_OS_ENOMEM;
        break;
    case -EAGAIN:
        err = CHIP_OS_TIMEOUT;
        break;
    case -EBUSY:
        err = CHIP_OS_EBUSY;
        break;
    case -EINVAL:
        err = CHIP_OS_EINVAL;
        break;
    default:
        err = CHIP_OS_ERROR;
        break;
    }

    return err;
}
