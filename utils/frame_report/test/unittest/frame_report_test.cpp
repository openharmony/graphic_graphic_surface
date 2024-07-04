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
    static const int32_t FRT_GAME_ERROR_UNIQUEID = -1;
    static const int32_t FRT_GAME_UNIQUEID = 1024;
    static const int32_t FRT_GAME_ERROR_STATE = -1;
    static const int32_t FRT_GAME_BACKGROUND = 0;
    static const int32_t FRT_GAME_FOREGROUND = 1;
    static const int32_t FRT_GAME_SCHED = 2;
    static const int64_t FRT_GAME_BUFFER_TIME = 2048;
    static const int64_t FRT_GAME_BUFFER_TIME_DEFAULT = 0;
    static std::string FRT_SURFACE_NAME_EMPTY = "";
    static std::string FRT_SURFACE_NAME = "SurfaceTEST";
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
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().isGameSoLoaded_ == true);

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
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().dequeueBufferTime_.load() == FRT_GAME_BUFFER_TIME_DEFAULT);
    
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
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().queueBufferTime_.load() == FRT_GAME_BUFFER_TIME_DEFAULT);
    
    Rosen::FrameReport::GetInstance().SetQueueBufferTime(FRT_GAME_UNIQUEID, FRT_SURFACE_NAME, FRT_GAME_BUFFER_TIME);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().queueBufferTime_.load() == FRT_GAME_BUFFER_TIME);
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
    Rosen::FrameReport::GetInstance().SetPendingBufferNum(FRT_SURFACE_NAME_EMPTY, FRT_GAME_BUFFER_TIME);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().pendingBufferNum_.load() == FRT_GAME_BUFFER_TIME_DEFAULT);
    
    Rosen::FrameReport::GetInstance().SetPendingBufferNum(FRT_SURFACE_NAME, FRT_GAME_BUFFER_TIME);
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
}

/*
* Function: ReportCommitTime
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call ReportCommitTime
 */
HWTEST_F(FrameReportTest, ReportCommitTime001, Function | MediumTest | Level2)
{
    Rosen::FrameReport::GetInstance().ReportCommitTime(FRT_GAME_BUFFER_TIME);
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
                                                      FRT_SURFACE_NAME_EMPTY);
    ASSERT_TRUE(Rosen::FrameReport::GetInstance().notifyFrameInfoFunc_ == nullptr);
}

} // namespace OHOS::Rosen