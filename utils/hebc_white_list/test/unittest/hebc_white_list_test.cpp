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
class HebcWhiteListTest : public testing::Test {
public:
    static void SetUpTestSuite(void);
    static void TearDownTestSuite(void);
    void SetUp() override;
    void TearDown() override;
};

void HebcWhiteListTest::SetUpTestSuite(void) {}

void HebcWhiteListTest::TearDownTestSuite(void) {}

void HebcWhiteListTest::SetUp() {}

void HebcWhiteListTest::TearDown() {}

/*
 * Function: Check
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call Init
 *                  2. call Check
 *                  3. check ret
 */
HWTEST_F(HebcWhiteListTest, Check001, Function | MediumTest | Level2)
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
HWTEST_F(HebcWhiteListTest, GetConfigAbsolutePath001, Function | MediumTest | Level2)
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
HWTEST_F(HebcWhiteListTest, ParseJson001, Function | MediumTest | Level2)
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
HWTEST_F(HebcWhiteListTest, ParseJson002, Function | MediumTest | Level2)
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
HWTEST_F(HebcWhiteListTest, ParseJson003, Function | MediumTest | Level2)
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
HWTEST_F(HebcWhiteListTest, ParseJson004, Function | MediumTest | Level2)
{
    HebcWhiteList hebcWhiteList;
    std::string validJson = "{\"HEBC\":{\"AppName\":[\"app1\",\"app2\"]}}";
    ASSERT_EQ(true, hebcWhiteList.ParseJson(validJson));
    ASSERT_EQ(2, hebcWhiteList.hebcList_.size());
    EXPECT_EQ(hebcWhiteList.hebcList_[0], "app1");
    EXPECT_EQ(hebcWhiteList.hebcList_[1], "app2");
}

/*
 * Function: ParseJson
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetup: new hebcWhiteList and set json with other key
 *                  2. operation: ParseJson
 *                  3. result: return Parsonjson true and hebclist size is empty
 */
HWTEST_F(HebcWhiteListTest, ParseJsonOtherKey, Function | MediumTest | Level2)
{
    HebcWhiteList wl;
    std::string json = R"({"OtherKey":123})";
    EXPECT_TRUE(wl.ParseJson(json));
    EXPECT_TRUE(wl.hebcList_.empty());
}

/*
 * Function: ParseJson
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetup: new hebcWhiteList
 *                     set json with HEBC key but no AppName
 *                  2. operation: ParseJson
 *                  3. result: return Parsonjson true
 *                     hebclist size is empty
 */
HWTEST_F(HebcWhiteListTest, ParseJsonHEBCEmpty, Function | MediumTest | Level2)
{
    HebcWhiteList wl;
    std::string json = R"({"HEBC":{}})";
    EXPECT_TRUE(wl.ParseJson(json));
    EXPECT_TRUE(wl.hebcList_.empty());
}

/*
 * Function: ParseJson
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetup: new hebcWhiteList
 *                     set json with HEBC key
 *                     this AppName is max size
 *                  2. operation: ParseJson
 *                  3. result: return Parsonjson true
 *                     hebclist size is max size
 */
HWTEST_F(HebcWhiteListTest, ParseJsonAppNameIsArrayExceedMax, Function | MediumTest | Level2)
{
    HebcWhiteList wl;
    std::string json = R"({"HEBC":{"AppName":[)";
    for (int i = 0; i < HebcWhiteList::MAX_HEBC_WHITELIST_NUMBER + 2; ++i) {
        json += "\"a\",";
    }
    json.pop_back(); // delete ending comma
    json += "]}}";
    EXPECT_TRUE(wl.ParseJson(json));
    auto list = wl.hebcList_;
    EXPECT_LE(list.size(), HebcWhiteList::MAX_HEBC_WHITELIST_NUMBER);
}

/*
 * Function: ParseJson
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetup: new hebcWhiteList
 *                     set json with HEBC key
 *                     this AppName is array contains nonString and string
 *                  2. operation: ParseJson
 *                  3. result: return Parsonjson true
 *                     hebclist size is json list's size
 *                     AppName only Contains String values
 */
HWTEST_F(HebcWhiteListTest, ParseJsonAppNameIsArrayContainsNonString, Function | MediumTest | Level2)
{
    HebcWhiteList wl;
    std::string json = R"({"HEBC":{"AppName":["a",123,"b"]}})";
    EXPECT_TRUE(wl.ParseJson(json));
    auto list = wl.hebcList_;
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list[0], "a");
    EXPECT_EQ(list[1], "b");
}

/*
 * Function: ParseJson
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetup: new hebcWhiteList
 *                     set json with HEBC key
 *                     this AppName is string
 *                  2. operation: ParseJson
 *                  3. result: return Parsonjson true
 *                     hebclist size is 1
 *                     AppName is set value
 */
HWTEST_F(HebcWhiteListTest, ParseJsonAppNameIsString, Function | MediumTest | Level2)
{
    HebcWhiteList wl;
    std::string json = R"({"HEBC":{"AppName":"abc"}})";
    EXPECT_TRUE(wl.ParseJson(json));
    auto list = wl.hebcList_;
    EXPECT_EQ(list.size(), 1);
    EXPECT_EQ(list[0], "abc");
}

/*
 * Function: ParseJson
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetup: new hebcWhiteList
 *                     set json with HEBC key
 *                     this AppName is nonString
 *                  2. operation: ParseJson
 *                  3. result: return Parsonjson true
 *                     hebclist size is empty
 */
HWTEST_F(HebcWhiteListTest, ParseJsonAppNameIsOtherType, Function | MediumTest | Level2)
{
    HebcWhiteList wl;
    std::string json = R"({"HEBC":{"AppName":123}})";
    EXPECT_TRUE(wl.ParseJson(json));
    EXPECT_TRUE(wl.hebcList_.empty());
}
} // namespace OHOS::Rosen