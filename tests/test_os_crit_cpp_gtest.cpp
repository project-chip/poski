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

#include <gtest/gtest.h>
#include <poski/OsCriticalSection.h>

TEST(OsCriticalSectionCpp, ScopedNesting) {
    EXPECT_FALSE(poski::OsCriticalSection::IsActive());
    EXPECT_FALSE(poski::OsCriticalSection::InIsr());

    {
        poski::OsCriticalSection outer;
        EXPECT_TRUE(poski::OsCriticalSection::IsActive());
        {
            poski::OsAtomicGuard inner;
            EXPECT_TRUE(poski::OsCriticalSection::IsActive());
        }
        EXPECT_TRUE(poski::OsCriticalSection::IsActive());
    }

    EXPECT_FALSE(poski::OsCriticalSection::IsActive());
}
