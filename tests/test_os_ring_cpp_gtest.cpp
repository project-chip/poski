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

#include <gtest/gtest.h>
#include <poski/OsRing.h>

TEST(OsRingCpp, BasicPushPop) {
    poski::OsRing ring(sizeof(uint32_t), 4);
    EXPECT_TRUE(ring.empty());
    EXPECT_FALSE(ring.full());
    
    uint32_t val = 42;
    ring.push_back(&val);
    EXPECT_FALSE(ring.empty());
    EXPECT_EQ(ring.size(), 1);
    
    uint32_t* out = (uint32_t*)ring.front();
    EXPECT_EQ(*out, 42);
    
    ring.pop_front();
    EXPECT_TRUE(ring.empty());
}

TEST(OsRingCpp, FullPowerOfTwo) {
    poski::OsRing ring(sizeof(uint8_t), 4);
    uint8_t val = 1;
    
    ring.push_back(&val);
    ring.push_back(&val);
    ring.push_back(&val);
    EXPECT_FALSE(ring.full());
    
    ring.push_back(&val);
    EXPECT_TRUE(ring.full());
}
