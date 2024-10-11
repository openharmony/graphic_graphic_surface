/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include "scoped_bytrace.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class ScopedBytraceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
};

void ScopedBytraceTest::SetUpTestCase()
{
}

void ScopedBytraceTest::TearDownTestCase()
{
}

/*
* Function: ScopedBytraceTest001
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call ScopedBytrace
*                  2. check ret
*/
HWTEST_F(ScopedBytraceTest, ScopedBytraceTest001, Function | MediumTest | Level2)
{
    ScopedBytrace trace("ScopedBytraceTest001");
    trace.End();
    trace.End();

    ScopedBytrace trace1("ScopedBytraceTest001");
}

/*
* Function: ScopedBytraceTest002
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call ScopedDebugTrace
*                  2. check ret
*/
HWTEST_F(ScopedBytraceTest, ScopedDebugTraceTest001, Function | MediumTest | Level2)
{
    ScopedDebugTrace::debugTraceEnabled_ = true;
    ScopedDebugTrace trace("ScopedDebugTraceTest001");

    ScopedDebugTrace::debugTraceEnabled_ = false;
    ScopedDebugTrace trace1("ScopedDebugTraceTest001");
}
}