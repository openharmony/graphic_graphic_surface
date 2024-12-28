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
#include "hebc_white_list.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class HebcWhilteListTest : public testing::Test {
public:
    static void SetUpTestSuite(void);
    static void TearDownTestSuite(void);
    void SetUp() override;
    void TearDown() override;
};

void HebcWhilteListTest::SetUpTestSuite(void) {}

void HebcWhilteListTest::TearDownTestSuite(void) {}

void HebcWhilteListTest::SetUp() {}

void HebcWhilteListTest::TearDown() {}

/*
 * Function: Check
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call Init
 *                  2. call Check
 *                  3. check ret
 */
HWTEST_F(HebcWhilteListTest, Check001, Function | MediumTest | Level2)
{
    HebcWhiteList::GetInstance().Init();
    std::string appName = "";
    HebcWhiteList::GetInstance().GetApplicationName(appName);
    bool isInHebcList = HebcWhiteList::GetInstance().Check(appName);
    ASSERT_EQ(false, isInHebcList);
}

/*
 * Function: GetConfigAbsolutePath
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetup: new hebcWhiteList
 *                  2. operation: GetConfigAbsolutePath
 *                  3. result: return empty path
 */
HWTEST_F(HebcWhilteListTest, GetConfigAbsolutePath001, Function | MediumTest | Level2)
{
    HebcWhiteList hebcWhiteList;
    std::string result = hebcWhiteList.GetConfigAbsolutePath();
    ASSERT_EQ(false, result.empty());
}

/*
 * Function: ParseJson
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetup: new hebcWhiteList and set json empty
 *                  2. operation: ParseJson
 *                  3. result: return empty json
 */
HWTEST_F(HebcWhilteListTest, ParseJson001, Function | MediumTest | Level2)
{
    HebcWhiteList hebcWhiteList;
    std::string emptyJson = "";
    ASSERT_EQ(true, hebcWhiteList.ParseJson(emptyJson));
}

/*
 * Function: ParseJson
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetup: new hebcWhiteList and set invalid json
 *                  2. operation: ParseJson
 *                  3. result: return invalid json
 */
HWTEST_F(HebcWhilteListTest, ParseJson002, Function | MediumTest | Level2)
{
    HebcWhiteList hebcWhiteList;
    std::string invalidJson = "invalid json";
    ASSERT_EQ(false, hebcWhiteList.ParseJson(invalidJson));
}

/*
 * Function: ParseJson
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetup: new hebcWhiteList and set no hebc json
 *                  2. operation: ParseJson
 *                  3. result: return noHebc json
 */
HWTEST_F(HebcWhilteListTest, ParseJson003, Function | MediumTest | Level2)
{
    HebcWhiteList hebcWhiteList;
    std::string noHebcJson = "{}";
    ASSERT_EQ(true, hebcWhiteList.ParseJson(noHebcJson));
}

/*
 * Function: ParseJson
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetup: new hebcWhiteList and set valid json
 *                  2. operation: ParseJson
 *                  3. result: return Parsonjson true and hebclist size is 2
 */
HWTEST_F(HebcWhilteListTest, ParseJson004, Function | MediumTest | Level2)
{
    HebcWhiteList hebcWhiteList;
    std::string validJson = "{\"HEBC\":{\"AppName\":[\"app1\",\"app2\"]}}";
    ASSERT_EQ(true, hebcWhiteList.ParseJson(validJson));
    ASSERT_EQ(2, hebcWhiteList.hebcList_.size());
}
} // namespace OHOS::Rosen