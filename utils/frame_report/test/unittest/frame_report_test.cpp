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
#include "frame_report.h"

using namespace testing;
using namespace testing::ext;

namespace {
    static const int32_t FRT_GAME_ERROR_PID = -1;
    static const int32_t FRT_GAME_PID = 1024;
    static const int32_t FRT_GAME_PID_NOT = 0;
    static const int32_t FRT_GAME_ERROR_UNIQUEID = -1;
    static const int32_t FRT_GAME_UNIQUEID = 1024;
    static const int32_t FRT_GAME_UNIQUEID_NOT = 1023;
    static const int32_t FRT_GAME_ERROR_STATE = -1;
    static const int32_t FRT_GAME_BACKGROUND = 0;
    static const int32_t FRT_GAME_FOREGROUND = 1;
    static const int32_t FRT_GAME_SCHED = 2;
    static const int64_t FRT_GAME_BUFFER_TIME = 2048;
    static std::string FRT_SURFACE_NAME_EMPTY = "";
    static std::string FRT_SURFACE_NAME = "SurfaceTEST";
    static const uint64_t FRT_UNIQUEID = 0L;
}

namespace OHOS::Rosen {
class FrameReportTest : public testing::Test {
public:
    static void SetUpTestSuite(void);
    static void TearDownTestSuite(void);
    void SetUp();
    void TearDown();
};

void FrameReportTest::SetUpTestSuite(void) {}

void FrameReportTest::TearDownTestSuite(void) {}

void FrameReportTest::SetUp() {}

void FrameReportTest::TearDown() {}

/*
* Function: SetGameScene
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetGameScene
*                  2. check ret
 */
HWTEST_F(FrameReportTest, SetGameScene001, Function | MediumTest | Level2)
{
    Rosen::FrameReport::GetInstance().SetGameScene(FRT_GAME_PID, FRT_GAME_BACKGROUND);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().activelyPid_.load() == FR_DEFAULT_PID);

    Rosen::FrameReport::GetInstance().SetGameScene(FRT_GAME_PID, FRT_GAME_FOREGROUND);

    Rosen::FrameReport::GetInstance().SetGameScene(FRT_GAME_PID, FRT_GAME_SCHED);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().activelyPid_.load() == FRT_GAME_PID);

    Rosen::FrameReport::GetInstance().SetGameScene(FRT_GAME_PID, FRT_GAME_BACKGROUND);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().activelyPid_.load() == FR_DEFAULT_PID);

    Rosen::FrameReport::GetInstance().SetGameScene(FRT_GAME_PID, FRT_GAME_ERROR_STATE);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().activelyPid_.load() == FR_DEFAULT_PID);
}

/*
* Function: HasGameScene
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call HasGameScene
*                  2. check ret
 */
HWTEST_F(FrameReportTest, HasGameScene001, Function | MediumTest | Level2)
{
    Rosen::FrameReport::GetInstance().SetGameScene(FRT_GAME_PID, FRT_GAME_BACKGROUND);
    bool result = Rosen::FrameReport::GetInstance().HasGameScene();
    ASSERT_TRUE(!result);

    Rosen::FrameReport::GetInstance().SetGameScene(FRT_GAME_PID, FRT_GAME_SCHED);
    result = Rosen::FrameReport::GetInstance().HasGameScene();
    ASSERT_TRUE(result);
}

/*
* Function: IsActiveGameWithPid
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call IsActiveGameWithPid
*                  2. check ret
 */
HWTEST_F(FrameReportTest, IsActiveGameWithPid001, Function | MediumTest | Level2)
{
    bool result = Rosen::FrameReport::GetInstance().IsActiveGameWithPid(FRT_GAME_ERROR_PID);
    ASSERT_TRUE(!result);

    result = Rosen::FrameReport::GetInstance().IsActiveGameWithPid(FRT_GAME_PID);
    ASSERT_TRUE(result);
}

/*
* Function: IsActiveGameWithUniqueId
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call IsActiveGameWithUniqueId
*                  2. check ret
 */
HWTEST_F(FrameReportTest, IsActiveGameWithUniqueId001, Function | MediumTest | Level2)
{
    bool result = Rosen::FrameReport::GetInstance().IsActiveGameWithUniqueId(FRT_GAME_ERROR_UNIQUEID);
    ASSERT_TRUE(!result);

    result = Rosen::FrameReport::GetInstance().IsActiveGameWithUniqueId(FR_DEFAULT_UNIQUEID);
    ASSERT_TRUE(!result);

    result = Rosen::FrameReport::GetInstance().IsActiveGameWithUniqueId(FRT_GAME_UNIQUEID);
    ASSERT_TRUE(!result);
}

/*
* Function: SetLastSwapBufferTime
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetLastSwapBufferTime
*                  2. check ret
 */
HWTEST_F(FrameReportTest, SetLastSwapBufferTime001, Function | MediumTest | Level2)
{
    Rosen::FrameReport::GetInstance().SetLastSwapBufferTime(FRT_GAME_BUFFER_TIME);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().lastSwapBufferTime_.load() == FRT_GAME_BUFFER_TIME);
}

/*
* Function: SetDequeueBufferTime
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetDequeueBufferTime
*                  2. check ret
 */
HWTEST_F(FrameReportTest, SetDequeueBufferTime001, Function | MediumTest | Level2)
{
    Rosen::FrameReport::GetInstance().SetDequeueBufferTime(FRT_SURFACE_NAME_EMPTY, FRT_GAME_BUFFER_TIME);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().dequeueBufferTime_.load() == FRT_GAME_BUFFER_TIME);
    
    Rosen::FrameReport::GetInstance().SetDequeueBufferTime(FRT_SURFACE_NAME, FRT_GAME_BUFFER_TIME);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().dequeueBufferTime_.load() == FRT_GAME_BUFFER_TIME);
}

/*
* Function: SetQueueBufferTime
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetQueueBufferTime
*                  2. check ret
 */
HWTEST_F(FrameReportTest, SetQueueBufferTime001, Function | MediumTest | Level2)
{
    Rosen::FrameReport::GetInstance().SetQueueBufferTime(FRT_GAME_UNIQUEID, FRT_SURFACE_NAME_EMPTY,
                                                         FRT_GAME_BUFFER_TIME);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().queueBufferTime_.load() == FRT_GAME_BUFFER_TIME);
    
    Rosen::FrameReport::GetInstance().SetQueueBufferTime(FRT_GAME_UNIQUEID, FRT_SURFACE_NAME, FRT_GAME_BUFFER_TIME);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().queueBufferTime_.load() == FRT_GAME_BUFFER_TIME);
}

/*
* Function: SetAcquireBufferSysTime
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetAcquireBufferSysTime
*                  2. check ret
 */
HWTEST_F(FrameReportTest, SetAcquireBufferSysTime001, Function | MediumTest | Level2)
{
    Rosen::FrameReport::GetInstance().SetGameScene(FRT_GAME_PID_NOT, FRT_GAME_SCHED);
    // NOT GAME
    Rosen::FrameReport::GetInstance().SetAcquireBufferSysTime();
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().acquireBufferSysTime_.load() == 0);

    Rosen::FrameReport::GetInstance().SetGameScene(FRT_GAME_PID, FRT_GAME_SCHED);
    // IS GAME
    Rosen::FrameReport::GetInstance().SetAcquireBufferSysTime();
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().acquireBufferSysTime_.load() != 0);
}

/*
* Function: SetPendingBufferNum
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetPendingBufferNum
*                  2. check ret
 */
HWTEST_F(FrameReportTest, SetPendingBufferNum001, Function | MediumTest | Level2)
{
    Rosen::FrameReport::GetInstance().SetQueueBufferTime(FRT_GAME_UNIQUEID, FRT_SURFACE_NAME_EMPTY,
                                                         FRT_GAME_BUFFER_TIME);
    Rosen::FrameReport::GetInstance().SetPendingBufferNum(FRT_GAME_UNIQUEID_NOT, FRT_SURFACE_NAME_EMPTY,
                                                          FRT_GAME_BUFFER_TIME);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().pendingBufferNum_.load() != FRT_GAME_BUFFER_TIME);
    
    Rosen::FrameReport::GetInstance().SetPendingBufferNum(FRT_GAME_UNIQUEID, FRT_SURFACE_NAME, FRT_GAME_BUFFER_TIME);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().pendingBufferNum_.load() == FRT_GAME_BUFFER_TIME);
}

/*
* Function: Report
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call Report
 */
HWTEST_F(FrameReportTest, Report001, Function | MediumTest | Level2)
{
    Rosen::FrameReport::GetInstance().Report(FRT_SURFACE_NAME_EMPTY);
    Rosen::FrameReport::GetInstance().Report(FRT_SURFACE_NAME);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().pendingBufferNum_.load() == FRT_GAME_BUFFER_TIME);
}

/*
* Function: CloseLibrary
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call CloseLibrary
 */
HWTEST_F(FrameReportTest, CloseLibrary001, Function | MediumTest | Level2)
{
    Rosen::FrameReport::GetInstance().CloseLibrary();
    EXPECT_EQ(Rosen::FrameReport::GetInstance().notifyFrameInfoFunc_, nullptr);
    EXPECT_EQ(Rosen::FrameReport::GetInstance().gameSoHandle_, nullptr);
    EXPECT_EQ(Rosen::FrameReport::GetInstance().isGameSoLoaded_, false);
}

/*
* Function: DeletePidInfo
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call DeletePidInfo
*                  2. check ret
 */
HWTEST_F(FrameReportTest, DeletePidInfo001, Function | MediumTest | Level2)
{
    Rosen::FrameReport::GetInstance().DeletePidInfo();
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().activelyPid_.load() == FR_DEFAULT_PID);
}

/*
* Function: NotifyFrameInfo
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call NotifyFrameInfo
*                  2. check ret
 */
HWTEST_F(FrameReportTest, NotifyFrameInfo001, Function | MediumTest | Level2)
{
    Rosen::FrameReport::GetInstance().notifyFrameInfoFunc_ = nullptr;
    Rosen::FrameReport::GetInstance().NotifyFrameInfo(FRT_GAME_PID, FRT_SURFACE_NAME, FRT_GAME_BUFFER_TIME,
                                                      FRT_SURFACE_NAME_EMPTY, FRT_UNIQUEID);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().notifyFrameInfoFunc_ == nullptr);
}

/*
* Function: NotifyFrameInfo002
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call NotifyFrameInfo
*                  2. check ret
*/
HWTEST_F(FrameReportTest, NotifyFrameInfo002, Function | MediumTest | Level2)
{
    Rosen::FrameReport::GetInstance().notifyFrameInfoFunc_ = [](int32_t pid,
                                                                 const std::string &layerName,
                                                                 int64_t timestamp,
                                                                 const std::string &bufferMsg,
                                                                 uint64_t uniqueId) noexcept { return uniqueId == 0; };
    Rosen::FrameReport::GetInstance().NotifyFrameInfo(
        FRT_GAME_PID, FRT_SURFACE_NAME, FRT_GAME_BUFFER_TIME, FRT_SURFACE_NAME_EMPTY, FRT_UNIQUEID);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().activelyPid_.load() == FR_DEFAULT_PID);
    Rosen::FrameReport::GetInstance().notifyFrameInfoFunc_ = nullptr;
}

/*
* Function: SetFlushBufferSequence001
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetFlushBufferSequence
*                  2. check flushBufferSysTime_ and flushBufferSequence_ are set correctly
*/
HWTEST_F(FrameReportTest, SetFlushBufferSequence001, Function | MediumTest | Level2)
{
    static const uint32_t FRT_FLUSH_SEQUENCE = 100;

    Rosen::FrameReport::GetInstance().SetFlushBufferSequence(FRT_FLUSH_SEQUENCE);

    ASSERT_TRUE(Rosen::FrameReport::GetInstance().flushBufferSysTime_.load() > 0);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().flushBufferSequence_.load() == FRT_FLUSH_SEQUENCE);
}

/*
* Function: SetAcquireBufferSeqWithUniqueId001
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetAcquireBufferSeqWithUniqueId with non-matching uniqueId
*                  2. check acquireBufferSysTime_ and acquireBufferSequence_ are not changed
*/
HWTEST_F(FrameReportTest, SetAcquireBufferSeqWithUniqueId001, Function | MediumTest | Level2)
{
    static const uint32_t FRT_ACQUIRE_SEQUENCE = 200;

    Rosen::FrameReport::GetInstance().SetGameScene(FRT_GAME_PID, FRT_GAME_SCHED);
    Rosen::FrameReport::GetInstance().SetAcquireBufferSeqWithUniqueId(FRT_GAME_UNIQUEID_NOT, FRT_ACQUIRE_SEQUENCE);

    ASSERT_TRUE(Rosen::FrameReport::GetInstance().acquireBufferSysTime_.load() == 0);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().acquireBufferSequence_.load() == 0);
}

/*
* Function: SetAcquireBufferSeqWithUniqueId002
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetAcquireBufferSeqWithUniqueId with matching uniqueId
*                  2. check acquireBufferSysTime_ and acquireBufferSequence_ are set correctly
*/
HWTEST_F(FrameReportTest, SetAcquireBufferSeqWithUniqueId002, Function | MediumTest | Level2)
{
    static const uint32_t FRT_ACQUIRE_SEQUENCE = 201;

    Rosen::FrameReport::GetInstance().SetGameScene(FRT_GAME_PID, FRT_GAME_SCHED);
    Rosen::FrameReport::GetInstance().SetQueueBufferTime(FRT_GAME_UNIQUEID, FRT_SURFACE_NAME, FRT_GAME_BUFFER_TIME);
    Rosen::FrameReport::GetInstance().SetAcquireBufferSeqWithUniqueId(FRT_GAME_UNIQUEID, FRT_ACQUIRE_SEQUENCE);

    ASSERT_TRUE(Rosen::FrameReport::GetInstance().acquireBufferSysTime_.load() > 0);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().acquireBufferSequence_.load() == FRT_ACQUIRE_SEQUENCE);
}

/*
* Function: SetPresentTimeWithUniqueId001
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetPresentTimeWithUniqueId with non-matching uniqueId
*                  2. check presentFenceSysTime_, presentFenceSequence_ and lastReleaseSysTime_ are not changed
*/
HWTEST_F(FrameReportTest, SetPresentTimeWithUniqueId001, Function | MediumTest | Level2)
{
    static const int64_t FRT_PRESENT_FENCE_TIME = 3000;
    static const uint32_t FRT_PRESENT_SEQUENCE = 300;

    Rosen::FrameReport::GetInstance().SetGameScene(FRT_GAME_PID, FRT_GAME_SCHED);
    Rosen::FrameReport::GetInstance().SetPresentTimeWithUniqueId(FRT_GAME_UNIQUEID_NOT, FRT_PRESENT_FENCE_TIME,
                                                                  FRT_PRESENT_SEQUENCE);

    ASSERT_TRUE(Rosen::FrameReport::GetInstance().presentFenceSysTime_.load() == 0);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().presentFenceSequence_.load() == 0);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().lastReleaseSysTime_.load() == 0);
}

/*
* Function: SetPresentTimeWithUniqueId002
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetPresentTimeWithUniqueId with matching uniqueId and normal timestamp
*                  2. check presentFenceSysTime_, presentFenceSequence_ and lastReleaseSysTime_ are set correctly
*/
HWTEST_F(FrameReportTest, SetPresentTimeWithUniqueId002, Function | MediumTest | Level2)
{
    static const int64_t FRT_PRESENT_FENCE_TIME = 3001;
    static const uint32_t FRT_PRESENT_SEQUENCE = 301;

    Rosen::FrameReport::GetInstance().SetGameScene(FRT_GAME_PID, FRT_GAME_SCHED);
    Rosen::FrameReport::GetInstance().SetQueueBufferTime(FRT_GAME_UNIQUEID, FRT_SURFACE_NAME, FRT_GAME_BUFFER_TIME);
    Rosen::FrameReport::GetInstance().SetPresentTimeWithUniqueId(FRT_GAME_UNIQUEID, FRT_PRESENT_FENCE_TIME,
                                                                  FRT_PRESENT_SEQUENCE);

    ASSERT_TRUE(Rosen::FrameReport::GetInstance().presentFenceSysTime_.load() == FRT_PRESENT_FENCE_TIME);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().presentFenceSequence_.load() == FRT_PRESENT_SEQUENCE);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().lastReleaseSysTime_.load() > 0);
}

/*
* Function: SetPresentTimeWithUniqueId003
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetPresentTimeWithUniqueId with FENCE_PENDING_TIMESTAMP
*                  2. check presentFenceSysTime_ uses lastReleaseSysTime_ when FENCE_PENDING_TIMESTAMP is passed
*/
HWTEST_F(FrameReportTest, SetPresentTimeWithUniqueId003, Function | MediumTest | Level2)
{
    static const int64_t FRT_PENDING_TIMESTAMP = INT64_MAX;
    static const uint32_t FRT_PRESENT_SEQUENCE = 302;

    Rosen::FrameReport::GetInstance().SetGameScene(FRT_GAME_PID, FRT_GAME_SCHED);
    Rosen::FrameReport::GetInstance().SetQueueBufferTime(FRT_GAME_UNIQUEID, FRT_SURFACE_NAME, FRT_GAME_BUFFER_TIME);

    // First call to set lastReleaseSysTime_
    Rosen::FrameReport::GetInstance().SetPresentTimeWithUniqueId(FRT_GAME_UNIQUEID, 1000, 301);
    int64_t firstReleaseTime = Rosen::FrameReport::GetInstance().lastReleaseSysTime_.load();

    // Second call with FENCE_PENDING_TIMESTAMP should use lastReleaseSysTime_
    Rosen::FrameReport::GetInstance().SetPresentTimeWithUniqueId(FRT_GAME_UNIQUEID, FRT_PENDING_TIMESTAMP,
                                                                  FRT_PRESENT_SEQUENCE);

    ASSERT_TRUE(Rosen::FrameReport::GetInstance().presentFenceSysTime_.load() == firstReleaseTime);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().presentFenceSequence_.load() == FRT_PRESENT_SEQUENCE);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().lastReleaseSysTime_.load() > firstReleaseTime);
}

} // namespace OHOS::Rosen