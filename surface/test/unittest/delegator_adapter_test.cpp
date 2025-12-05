/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <cerrno>
#include <dlfcn.h>
#include "buffer_queue.h"
#include "delegator_adapter.h"
#include "delegator_mock.h"
#include "file_ex.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class DelegatorAdapterTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
};

void DelegatorAdapterTest::SetUpTestCase()
{
}

void DelegatorAdapterTest::TearDownTestCase()
{
}

/*
* Function: SetQueueSize and GetQueueSize
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetQueueSize with producer_ is nullptr
*                  2. call SetQueueSize with producer_ is nullptr
*                  3. check ret
*/
HWTEST_F(DelegatorAdapterTest, init, TestSize.Level0)
{
    g_dlopenNull = false;
    g_producerSurfaceDelegatorCreateNull = true;
    g_producerSurfaceDelegatorDestroyNull = true;
    g_producerSurfaceDelegatorSetSurfaceNull = true;
    g_producerSurfaceDelegatorSetClientNull = true;
    g_producerSurfaceDelegatorReleaseBufferNull = true;
    g_consumerSurfaceDelegatorCreateNull = true;
    g_consumerSurfaceDelegatorSetClientNull = true;
    g_consumerSurfaceDelegatorSetSurfaceNull = true;
    g_consumerSurfaceDelegatorDequeueBufferNull = true;
    g_consumerSurfaceDelegatorQueueBufferNull = true;
    g_consumerSurfaceDelegatorDestroyNull = true;
    auto &delegatorAdapter_ = DelegatorAdapter::GetInstance();
    ASSERT_NE(delegatorAdapter_.handle_, nullptr);
    ASSERT_EQ(delegatorAdapter_.funcMap_.size(), 0);
    delegatorAdapter_.handle_ = reinterpret_cast<void *>(0x14895);
}
}