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

#include "sync_fence_tracker.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class SyncFenceTrackerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
};

void SyncFenceTrackerTest::SetUpTestCase()
{
}

void SyncFenceTrackerTest::TearDownTestCase()
{
}

/*
* Function: TrackFence
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call TrackFence
*                  2. check ret
*/
HWTEST_F(SyncFenceTrackerTest, TrackFenceTest001, Function | MediumTest | Level2)
{
    auto tracker = new SyncFenceTracker("TrackFenceTest001");
    sptr<SyncFence> fence = new SyncFence(0);
    tracker->TrackFence(nullptr, true);
    EXPECT_EQ(tracker->fencesQueued_.load(), 0);
   
    tracker->TrackFence(fence, true);
    EXPECT_EQ(tracker->fencesQueued_.load(), 1);

    tracker->isGpuFence_ = true;
    tracker->TrackFence(fence, true);
    EXPECT_EQ(tracker->fencesQueued_.load(), 2);

    tracker->TrackFence(fence, false);
    EXPECT_EQ(tracker->fencesQueued_.load(), 2);

    tracker->isGpuFence_ = false;
    tracker->TrackFence(fence, true);
    EXPECT_EQ(tracker->fencesQueued_.load(), 3);

    tracker->TrackFence(fence, false);
    EXPECT_EQ(tracker->fencesQueued_.load(), 4);
    delete tracker;
}

/*
* Function: TrackFence
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call TrackFence
*                  2. check ret
*/
HWTEST_F(SyncFenceTrackerTest, TrackFenceTest002, Function | MediumTest | Level2)
{
    auto tracker = new SyncFenceTracker("Acquire Fence");
    sptr<SyncFence> fence = new SyncFence(0);
    tracker->TrackFence(nullptr, true);
    EXPECT_EQ(tracker->fencesQueued_.load(), 0);
 
    tracker->isGpuFreq_ = true;
    tracker->TrackFence(fence, true);
    EXPECT_EQ(tracker->fencesQueued_.load(), 1);
 
    tracker->isGpuFreq_ = false;
    tracker->TrackFence(fence, false);
    EXPECT_EQ(tracker->fencesQueued_.load(), 1);
    delete tracker;
}

/*
* Function: CheckGpuSubhealthEventLimit
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call CheckGpuSubhealthEventLimit
*                  2. check ret
*/
HWTEST_F(SyncFenceTrackerTest, CheckGpuSubhealthEventLimit001, Function | MediumTest | Level2)
{
    auto tracker = new SyncFenceTracker("CheckGpuSubhealthEventLimit001");
    tracker->gpuSubhealthEventNum_ = 0;
    ASSERT_EQ(tracker->CheckGpuSubhealthEventLimit(), true);
    tracker->gpuSubhealthEventDay_ = 0;
    ASSERT_EQ(tracker->CheckGpuSubhealthEventLimit(), true);
    tracker->gpuSubhealthEventNum_ = 1;
    tracker->gpuSubhealthEventDay_ = 0xFFFFFFF;
    ASSERT_EQ(tracker->CheckGpuSubhealthEventLimit(), true);
    tracker->gpuSubhealthEventDay_ = 0xFFFFFFF;
    tracker->gpuSubhealthEventNum_ = 0xFFFFFFF;
    ASSERT_EQ(tracker->CheckGpuSubhealthEventLimit(), false);
    delete tracker;
}

/*
* Function: GetFrameRate
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetFrameRate
*                  2. check ret
*/
HWTEST_F(SyncFenceTrackerTest, GetFrameRate001, Function | MediumTest | Level2)
{
    auto tracker = new SyncFenceTracker("GetFrameRate001");
    int64_t frameRate = tracker->GetFrameRate();
    EXPECT_EQ(frameRate, 0);

    for (int64_t i = 0; i < 2; i++) {
        tracker->frameStartTimes_->push(1);
    }
    frameRate = tracker->GetFrameRate();
    EXPECT_EQ(frameRate, 0);

    tracker->frameStartTimes_->push(2);
    frameRate = tracker->GetFrameRate();
    EXPECT_EQ(frameRate, 2000);

    delete tracker;
}

/*
* Function: ReportEventGpuSubhealth
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call ReportEventGpuSubhealth
*                  2. check ret
*/
HWTEST_F(SyncFenceTrackerTest, ReportEventGpuSubhealth001, Function | MediumTest | Level2)
{
    auto tracker = new SyncFenceTracker("ReportEventGpuSubhealth001");
    EXPECT_NE(tracker->handler_, nullptr);
    tracker->ReportEventGpuSubhealth(0);
    tracker->handler_ = nullptr;
    tracker->ReportEventGpuSubhealth(0);
    delete tracker;
}

/*
* Function: WaitFence
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call WaitFence
*                  2. check ret
*/
HWTEST_F(SyncFenceTrackerTest, WaitFence001, Function | MediumTest | Level2)
{
    auto tracker = new SyncFenceTracker("WaitFence001");
    auto fence = SyncFence::INVALID_FENCE;

    tracker->isGpuFence_ = true;
    tracker->isGpuEnable_ = true;
    int32_t retCode = tracker->WaitFence(fence);
    EXPECT_EQ(retCode, -1);

    tracker->isGpuFence_ = true;
    tracker->isGpuEnable_ = false;
    tracker->WaitFence(fence);
    EXPECT_EQ(retCode, -1);

    tracker->isGpuFence_ = false;
    tracker->isGpuEnable_ = true;
    tracker->WaitFence(fence);
    EXPECT_EQ(retCode, -1);

    tracker->isGpuFence_ = false;
    tracker->isGpuEnable_ = false;
    tracker->WaitFence(fence);
    EXPECT_EQ(retCode, -1);

    delete tracker;
}
}