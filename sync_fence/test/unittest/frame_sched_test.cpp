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

#include "frame_sched.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class FrameSchedTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
};

void FrameSchedTest::SetUpTestCase()
{
}

void FrameSchedTest::TearDownTestCase()
{
}

/*
* Function: SetFrameParam
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetFrameParam
*                  2. check ret
*/
HWTEST_F(FrameSchedTest, SetFrameParam001, Function | MediumTest | Level2)
{
    Rosen::FrameSched frameSched_;
    frameSched_.SetFrameParam(0, 0, 0, 0);
}

/*
* Function: MonitorGpuStart
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call MonitorGpuStart
*                  2. check ret
*/
HWTEST_F(FrameSchedTest, MonitorGpuStart001, Function | MediumTest | Level2)
{
    Rosen::FrameSched frameSched_;
    frameSched_.MonitorGpuStart();
}

/*
* Function: MonitorGpuEnd
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call MonitorGpuEnd
*                  2. check ret
*/
HWTEST_F(FrameSchedTest, MonitorGpuEnd001, Function | MediumTest | Level2)
{
    Rosen::FrameSched frameSched_;
    frameSched_.MonitorGpuEnd();
}
}