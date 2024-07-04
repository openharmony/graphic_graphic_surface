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

#include "acquire_fence_manager.h"
#include "sync_fence.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class AcquireFenceTrackerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
};

void AcquireFenceTrackerTest::SetUpTestCase()
{
}

void AcquireFenceTrackerTest::TearDownTestCase()
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
HWTEST_F(AcquireFenceTrackerTest, TrackFence001, Function | MediumTest | Level2)
{
    sptr<SyncTimeline> syncTimeline_ = new SyncTimeline();
    int32_t fd = syncTimeline_->GenerateFence("Acquire Fence", 20);
    sptr<SyncFence> syncFence = new SyncFence(fd);
    bool traceTag = true;
    AcquireFenceTracker::TrackFence((const sptr<SyncFence>&)syncFence, traceTag);
}

/*
* Function: SetBlurSize
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetBlurSize
*                  2. check ret
*/
HWTEST_F(AcquireFenceTrackerTest, blurSize001, Function | MediumTest | Level2)
{
    int32_t blurSize = 10;
    AcquireFenceTracker::SetBlurSize(blurSize);
}

/*
* Function: SetContainerNodeNum
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetContainerNodeNum
*                  2. check ret
*/
HWTEST_F(AcquireFenceTrackerTest, SetContainerNodeNum001, Function | MediumTest | Level2)
{
    int containerNodeNum = 1000;
    AcquireFenceTracker::SetContainerNodeNum(containerNodeNum);
}
}