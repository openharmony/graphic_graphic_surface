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
} // namespace OHOS::Rosen