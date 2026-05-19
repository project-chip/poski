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
#include <poski/osal/osal.h>
#include "os_utils.h"

pos_error_t map_zephyr_to_osal_error(int ret)
{
    pos_error_t err;

    switch (ret)
    {
    case 0:
        err = POS_OK;
        break;
    case -ENOMEM:
        err = POS_ENOMEM;
        break;
    case -EAGAIN:
        err = POS_TIMEOUT;
        break;
    case -EBUSY:
        err = POS_EBUSY;
        break;
    case -EINVAL:
        err = POS_EINVAL;
        break;
    default:
        err = POS_ERROR;
        break;
    }

    return err;
}
