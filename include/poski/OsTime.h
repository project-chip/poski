/*
 *
 *    Copyright (c) 2026 Google LLC
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

#ifndef POSKI_CPP_OS_TIME_H
#define POSKI_CPP_OS_TIME_H

#include "poski/osal/os_time.h"

namespace poski {

class OsTime {
public:
    static pos_time_t Get() { return pos_time_get(); }
    static pos_time_t GetMs() { return pos_time_get_ms(); }
    static pos_time_t MsToTicks(pos_time_t ms) { return pos_time_ms_to_ticks(ms); }
    static pos_time_t TicksToMs(pos_time_t ticks) { return pos_time_ticks_to_ms(ticks); }
};

} // namespace poski
#endif // POSKI_CPP_OS_TIME_H
