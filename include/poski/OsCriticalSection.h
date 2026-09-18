/*
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

#ifndef POSKI_CPP_OS_CRITICAL_SECTION_H
#define POSKI_CPP_OS_CRITICAL_SECTION_H

#include "poski/osal/os_crit.h"

namespace poski {

/**
 * @brief RAII scope guard that enters a critical section (disabling interrupts)
 *        on construction and restores the saved interrupt state on destruction.
 */
class OsCriticalSection {
public:
    OsCriticalSection() : state_(pos_crit_enter()) {}
    ~OsCriticalSection() { pos_crit_exit(state_); }

    OsCriticalSection(const OsCriticalSection&) = delete;
    OsCriticalSection& operator=(const OsCriticalSection&) = delete;

    static bool IsActive() { return pos_crit_is_active(); }
    static bool InIsr() { return pos_crit_in_isr(); }

private:
    pos_crit_state_t state_;
};

using OsAtomicGuard = OsCriticalSection;

} // namespace poski

#endif // POSKI_CPP_OS_CRITICAL_SECTION_H
