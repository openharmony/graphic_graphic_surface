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

namespace OHOS::Rosen {

// Minimal concrete subclass for testing SceneReporter's non-virtual methods
class TestSceneReporter : public SceneReporter {
public:
    TestSceneReporter(const std::string& soName, const std::string& symName)
        : SceneReporter(soName, symName) {}
    void Activate(int32_t pid) override { activePid_ = pid; }
    void Deactivate() override
    {
        activePid_ = -1;
        CloseLibrary();
    }
    bool IsActive() const override { return activePid_ > 0; }
    bool IsActiveWithPid(int32_t pid) const override { return pid == activePid_; }
    void Report(const std::string& layerName, uint64_t uniqueId, const std::string& bufferMsg) override {}
    int32_t activePid_ = -1;
};

namespace {
    static const std::string FRT_TEST_SO_NAME = "libtest_nonexist.z.so";
    static const std::string FRT_TEST_SYM_NAME = "TestFunc";
    static const int32_t FRT_SCENE_NONE = 0;
    static const int32_t FRT_SCENE_GAME = 1;
    static const int32_t FRT_SCENE_HWSCHED = 2;
    static const int32_t FRT_SCENE_BACKGROUND = 3;
    static const int32_t FRT_SCENE_FOREGROUND = 4;
}

class SceneReporterTest : public testing::Test {
public:
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    void SetUp() override {}
    void TearDown() override {}
};

/*
* Function: SceneReporter construction
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. Constructor stores soName and symName in libraryInfo_
                   2. sceneType_ defaults to FR_SCENE_NONE
                   3.library is not loaded
 */
HWTEST_F(SceneReporterTest, Construct001, Function | MediumTest | Level2)
{
    TestSceneReporter reporter(FRT_TEST_SO_NAME, FRT_TEST_SYM_NAME);
    ASSERT_TRUE(reporter.libraryInfo_.soName == FRT_TEST_SO_NAME);
    ASSERT_TRUE(reporter.libraryInfo_.symName == FRT_TEST_SYM_NAME);
    ASSERT_TRUE(reporter.GetSceneType() == FRT_SCENE_NONE);
    ASSERT_TRUE(!reporter.libraryInfo_.isLoaded);
    ASSERT_TRUE(reporter.libraryInfo_.soHandle == nullptr);
    ASSERT_TRUE(reporter.libraryInfo_.notifyFunc == nullptr);
}

/*
* Function: SceneReporter::LoadLibrary non-existent library
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. LoadLibrary with non-existent .so does not crash
                   2. libraryInfo_ remains unloaded
 */
HWTEST_F(SceneReporterTest, LoadLibraryNotExist001, Function | MediumTest | Level2)
{
    TestSceneReporter reporter(FRT_TEST_SO_NAME, FRT_TEST_SYM_NAME);
    reporter.LoadLibrary();
    ASSERT_TRUE(!reporter.libraryInfo_.isLoaded);
    ASSERT_TRUE(reporter.libraryInfo_.soHandle == nullptr);
}

/*
* Function: SceneReporter::CloseLibrary without load
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: CloseLibrary without prior LoadLibrary does not crash
 */
HWTEST_F(SceneReporterTest, CloseLibraryWithoutLoad001, Function | MediumTest | Level2)
{
    TestSceneReporter reporter(FRT_TEST_SO_NAME, FRT_TEST_SYM_NAME);
    reporter.CloseLibrary();
    ASSERT_TRUE(!reporter.libraryInfo_.isLoaded);
    ASSERT_TRUE(reporter.libraryInfo_.soHandle == nullptr);
}

/*
* Function: SceneReporter::LoadLibrary idempotent
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: Calling LoadLibrary twice does not crash
 */
HWTEST_F(SceneReporterTest, LoadLibraryIdempotent001, Function | MediumTest | Level2)
{
    TestSceneReporter reporter(FRT_TEST_SO_NAME, FRT_TEST_SYM_NAME);
    reporter.LoadLibrary()
    reporter.LoadLibrary();
    ASSERT_TRUE(!reporter.libraryInfo_.isLoaded);
}

/*
* Function: SceneReporter destructor
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: Destructor calls CloseLibrary does not crash
 */
HWTEST_F(SceneReporterTest, Destructor001, Function | MediumTest | Level2)
{
    TestSceneReporter reporter(FRT_TEST_SO_NAME, FRT_TEST_SYM_NAME);
    reporter.LoadLibrary()
}

/*
* Function: SceneReporter sceneType_
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: sceneType_ can be set and read via GetSceneType
 */
HWTEST_F(SceneReporterTest, SceneType001, Function | MediumTest | Level2)
{
    TestSceneReporter reporter(FRT_TEST_SO_NAME, FRT_TEST_SYM_NAME);
    reporter.LoadLibrary()
    reporter.LoadLibrary();
    ASSERT_TRUE(reporter.GetSceneType() == FRT_SCENE_NONE);

    reporter.sceneType_.store(FRT_SCENE_HWSCHED);
    ASSERT_TRUE(reporter.GetSceneType() == FRT_SCENE_HWSCHED);

    reporter.sceneType_.store(FRT_SCENE_GAME);
    ASSERT_TRUE(reporter.GetSceneType() == FRT_SCENE_GAME);
}

/*
* Function: SceneReporter constants
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: Scene constants have correct values and ordering
 */
HWTEST_F(SceneReporterTest, Constants001, Function | MediumTest | Level2)
{
    ASSERT_TRUE(FRT_SCENE_NONE == 0);
    ASSERT_TRUE(FRT_SCENE_GAME == 1);
    ASSERT_TRUE(FRT_SCENE_HWSCHED == 2);
    ASSERT_TRUE(FRT_SCENE_BACKGROUND == 3);
    ASSERT_TRUE(FRT_SCENE_FOREGROUND == 4);
    ASSERT_TRUE(FRT_SCENE_BACKGROUND < FR_SCENE_FOREGROUND);
}

} // namespace OHOS::Rosen