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
#include "sync_fence.h"

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
    EXPECT_EQ(frameSched_.setFrameParamFunc_, nullptr);
    frameSched_.SetFrameParam(0, 0, 0, 0);
}

/*
 * Function: SendFenceId
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SendFenceId
 *                  2. check ret
 */
HWTEST_F(FrameSchedTest, SendFenceId001, Function | MediumTest | Level2)
{
    Rosen::FrameSched frameSched_;
    uint32_t fenceId = 1;
    frameSched_.sendFenceIdFunc_ = nullptr;
    frameSched_.SendFenceId(fenceId);
    EXPECT_EQ(frameSched_.sendFenceIdFunc_, nullptr);
    frameSched_.LoadLibrary();
    frameSched_.SendFenceId(fenceId);
    EXPECT_NE(frameSched_.sendFenceIdFunc_, nullptr);
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
    uint32_t fenceId = 1;
    frameSched_.monitorGpuStartFunc_ = nullptr;
    frameSched_.MonitorGpuStart(fenceId);
    EXPECT_EQ(frameSched_.monitorGpuStartFunc_, nullptr);
    frameSched_.LoadLibrary();
    frameSched_.MonitorGpuStart(fenceId);
    EXPECT_NE(frameSched_.monitorGpuStartFunc_, nullptr);
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
    frameSched_.monitorGpuEndFunc_ = nullptr;
    frameSched_.MonitorGpuEnd();
    EXPECT_EQ(frameSched_.monitorGpuEndFunc_, nullptr);
    frameSched_.LoadLibrary();
    frameSched_.MonitorGpuEnd();
    EXPECT_NE(frameSched_.monitorGpuEndFunc_, nullptr);
}

/*
* Function: IsScbScene
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call IsScbScene
*                  2. check ret
*/
HWTEST_F(FrameSchedTest, IsScbScene001, Function | MediumTest | Level2)
{
    Rosen::FrameSched frameSched_;
    frameSched_.isScbSceneFunc_ = nullptr;
    frameSched_.schedSoLoaded_ = false;
    frameSched_.IsScbScene();
    EXPECT_EQ(frameSched_.isScbSceneFunc_, nullptr);
    frameSched_.schedSoLoaded_ = true;
    frameSched_.IsScbScene();
    EXPECT_EQ(frameSched_.isScbSceneFunc_, nullptr);
    frameSched_.LoadLibrary();
    frameSched_.schedSoLoaded_ = false;
    frameSched_.IsScbScene();
    EXPECT_NE(frameSched_.isScbSceneFunc_, nullptr);
    frameSched_.schedSoLoaded_ = true;
    frameSched_.IsScbScene();
    EXPECT_NE(frameSched_.isScbSceneFunc_, nullptr);
}

/*
* Function: SyncMerge
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SyncMerge
*                  2. check ret
*/
HWTEST_F(FrameSchedTest, SyncMergeTest001, Function | MediumTest | Level2)
{
    int32_t newFenceFd = -1;
    ASSERT_EQ(SyncFence::SyncMerge("SyncMergeTest001", 1, -1, newFenceFd), -1);
    ASSERT_EQ(newFenceFd, -1);

    sptr<SyncFence> fence = new SyncFence(0);
    ASSERT_EQ(fence->GetStatus(), FenceStatus::ERROR);

    sptr<SyncFence> fence1 = new SyncFence(-1);
    EXPECT_EQ(fence1->GetStatus(), ERROR);
}

/*
* Function: LoadLibrary
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call LoadLibrary
*                  2. check ret
*/
HWTEST_F(FrameSchedTest, LoadLibrary001, Function | MediumTest | Level2)
{
    auto frameSched = new Rosen::FrameSched();
    frameSched->schedSoLoaded_ = false;
    bool res = frameSched->LoadLibrary();
    EXPECT_TRUE(res);

    frameSched->schedSoLoaded_ = true;
    res = frameSched->LoadLibrary();
    EXPECT_TRUE(res);

    delete frameSched;
}

/*
* Function: CloseLibrary
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call CloseLibrary
*                  2. check ret
*/
HWTEST_F(FrameSchedTest, CloseLibrary001, Function | MediumTest | Level2)
{
    auto frameSched = new Rosen::FrameSched();
    frameSched->schedHandle_ = nullptr;
    frameSched->schedSoLoaded_ = true;
    frameSched->CloseLibrary();
    EXPECT_FALSE(frameSched->schedSoLoaded_);
    delete frameSched;
}

/*
* Function: LoadSymbol
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call LoadSymbol
*                  2. check ret
*/
HWTEST_F(FrameSchedTest, LoadSymbol001, Function | MediumTest | Level2)
{
    auto frameSched = new Rosen::FrameSched();
    frameSched->schedSoLoaded_ = false;
    auto res = frameSched->LoadSymbol("LoadSymbol001");
    EXPECT_EQ(res, nullptr);
    delete frameSched;
}

/*
* Function: Init
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call Init
*                  2. check ret
*/
HWTEST_F(FrameSchedTest, Init001, Function | MediumTest | Level2)
{
    auto frameSched = new Rosen::FrameSched();
    frameSched->initFunc_ = nullptr;
    frameSched->Init();
    EXPECT_NE(frameSched->initFunc_, nullptr);
    delete frameSched;
}
}