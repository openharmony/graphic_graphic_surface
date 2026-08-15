/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "hwsched_reporter.h"

using namespace testing;
using namespace testing::ext;

namespace {
    static const int32_t FRT_HWSCHED_PID = 2048;
    static const int32_t FRT_HWSCHED_PID_2 = 2049;
    static const int32_t FRT_HWSCHED_INVALID_PID = -1;
    static const int32_t FRT_SCENE_HWSCHED = 2;
}

namespace OHOS::Rosen {
class HwschedReporterTest : public testing::Test {
public:
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    void SetUp() override {}
    void TearDown() override {}
};

/*
* Function: HwschedReporter construction
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: HwschedReporter constructed with correct sceneType
 */
HWTEST_F(HwschedReporterTest, Construct001, Function | MediumTest | Level2)
{
    HwschedReporter reporter;
    ASSERT_TRUE(reporter.GetSceneType() == FRT_SCENE_HWSCHED);
    ASSERT_TRUE(reporter.libraryInfo_.soName == "libhwsched_client.z.so");
    ASSERT_TRUE(reporter.libraryInfo_.symName == "ReportFrameInfo");
}

/*
* Function: HwschedReporter::Activate / IsActive
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. IsActive returns false before Activate
                   2. IsActive returns true after Activate
                   3. IsActive returns false Deactivate
 */
HWTEST_F(HwschedReporterTest, ActivateAndIsActive001, Function | MediumTest | Level2)
{
    HwschedReporter reporter;
    ASSERT_TRUE(!reporter.IsActive());
    
    reporter.Activate(FRT_HWSCHED_PID);
    ASSERT_TRUE(reporter.IsActive());

    reporter.Deactivate();
    ASSERT_TRUE(!reporter.IsActive());
}

/*
* Function: HwschedReporter::IsActiveWithPid
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. IsActiveWithPid returns false for invalid pid
                   2. IsActiveWithPid returns true after Activate
                   3. IsActiveWithPid returns false Deactivate
 */
HWTEST_F(HwschedReporterTest, IsActiveWithPid001, Function | MediumTest | Level2)
{
    HwschedReporter reporter;
    ASSERT_TRUE(!reporter.IsActiveWithPid(FRT_HWSCHED_INVALID_PID));
    ASSERT_TRUE(!reporter.IsActiveWithPid(0));
    ASSERT_TRUE(!reporter.IsActiveWithPid(FRT_HWSCHED_PID));

    reporter.Activate(FRT_HWSCHED_PID);
    ASSERT_TRUE(reporter.IsActiveWithPid(FRT_HWSCHED_PID));


    reporter.Deactivate();
    ASSERT_TRUE(!reporter.IsActiveWithPid(FRT_HWSCHED_PID));
}

/*
* Function: HwschedReporter::multi-PID
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. Multiple PIDS can be activated
                   2. isActiveWithPid returns true for each
                   3. Deactivate clears all
 */
HWTEST_F(HwschedReporterTest, MultiPid001, Function | MediumTest | Level2)
{
    HwschedReporter reporter;
    reporter.Activate(FRT_HWSCHED_PID);
    reporter.Activate(FRT_HWSCHED_PID_2);
    ASSERT_TRUE(reporter.IsActiveWithPid(FRT_HWSCHED_PID));
    ASSERT_TRUE(reporter.IsActiveWithPid(FRT_HWSCHED_PID_2));
    ASSERT_TRUE(reporter.IsActive());

    reporter.Deactivate();
    ASSERT_TRUE(!reporter.IsActiveWithPid(FRT_HWSCHED_PID));
    ASSERT_TRUE(!reporter.IsActiveWithPid(FRT_HWSCHED_PID_2));
    ASSERT_TRUE(!reporter.IsActive());
}

/*
* Function: HwschedReporter::IsActiveWithPid cache
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: Second call with pid hits activePid_ cache
 */
HWTEST_F(HwschedReporterTest, ActivePidCache001, Function | MediumTest | Level2)
{
    HwschedReporter reporter;
    reporter.Activate(FRT_HWSCHED_PID);
    
    // First call loads into activePid_ cache
    ASSERT_TRUE(reporter.IsActiveWithPid(FRT_HWSCHED_PID));
    // Second call should hit cache (activePid_ == pid)
    ASSERT_TRUE(reporter.IsActiveWithPid(FRT_HWSCHED_PID));

    reporter.Deactivate();
}

/*
* Function: HwschedReporter::Report empty buffMsg
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: Report with empty buffMsg returns without crash
 */
HWTEST_F(HwschedReporterTest, ReportEmptyBuffer001, Function | MediumTest | Level2)
{
    HwschedReporter reporter;
    reporter.Activate(FRT_HWSCHED_PID);
    
    // empty bufferMsg, should not crash
    reporter.Report("LayerName", 0, "");
    ASSERT_TRUE(reporter.IsActive());
    reporter.Deactivate();
}

/*
* Function: HwschedReporter::Report library not loaded
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: Report when library not loaded does not crash
 */
HWTEST_F(HwschedReporterTest, ReportNoLibrary001, Function | MediumTest | Level2)
{
    HwschedReporter reporter;
    reporter.Activate(FRT_HWSCHED_PID);
    
    // empty bufferMsg, should not crash
    reporter.Report("LayerName", 1024, "{\"test\":\"msg\"}");
    ASSERT_TRUE(reporter.IsActive());
    reporter.Deactivate();
}

/*
* Function: HwschedReporter::LoadLibrary / CloseLibrary
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: LoadLibrary and CloseLibrary do not crash
 */
HWTEST_F(HwschedReporterTest, LoadLibraryAndClose001, Function | MediumTest | Level2)
{
    HwschedReporter reporter;
    reporter.LoadLibrary();
    reporter.CloseLibrary();
    ASSERT_TRUE(!reporter.libraryInfo_.isLoaded);
    ASSERT_TRUE(reporter.libraryInfo_.soHandle == nullptr);
}

/*
* Function: HwschedReporter::Deactivate
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. Deactivate clears pidSet and activePid
                   2. Deactivate calls CloseLibrary
 */
HWTEST_F(HwschedReporterTest, Deactivate001, Function | MediumTest | Level2)
{
    HwschedReporter reporter;
    reporter.Activate(FRT_HWSCHED_PID);
    reporter.Activate(FRT_HWSCHED_PID_2);
    ASSERT_TRUE(reporter.IsActive());

    reporter.Deactivate();
    ASSERT_TRUE(!reporter.IsActive());
    ASSERT_TRUE(!reporter.IsActiveWithPid(FRT_HWSCHED_PID));
    ASSERT_TRUE(!reporter.IsActiveWithPid(FRT_HWSCHED_PID_2));
    ASSERT_TRUE(reporter.activePid_.load() == -1);
}

    
} // namespace OHOS::Rosen
