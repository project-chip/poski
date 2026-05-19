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

#include <gtest/gtest.h>
#include <stdlib.h>

#ifdef WRAP_C_MAIN
extern "C" int TEST_MAIN(int argc, char **argv);
#else
int TEST_MAIN();
#endif

TEST(OsalGtest, TEST_NAME) {
#ifdef WRAP_C_MAIN
    EXPECT_EXIT({ exit(TEST_MAIN(0, nullptr)); }, ::testing::ExitedWithCode(0), "");
#else
    EXPECT_EXIT({ exit(TEST_MAIN()); }, ::testing::ExitedWithCode(0), "");
#endif
}
