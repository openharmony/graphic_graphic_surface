/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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
#include <hilog/log.h>
#include "buffer_handle.h"

#include "buffer_handle_utils.h"
#include "buffer_handle_parcel.h"

using namespace testing::ext;

namespace OHOS {
class BufferHandleTest : public testing::Test {
public:
    static constexpr HiviewDFX::HiLogLabel LOG_LABEL = { LOG_CORE, 0xD001400, "graphicutils" };

    static void SetUpTestCase()
    {}

    static void TearDownTestCase()
    {}
};

/*
* Function: BufferHandleTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: AllocateBufferHandle
*/
HWTEST_F(BufferHandleTest, AllocateBufferHandle, Function | SmallTest | Level2)
{
    uint32_t fds = 1025, ints = 1025;
    uint32_t buffer_handle_reserve_max_size = 1024;
    ASSERT_EQ(nullptr, AllocateBufferHandle(fds, ints));
    ASSERT_EQ(nullptr, AllocateBufferHandle(fds, buffer_handle_reserve_max_size));
    ASSERT_EQ(nullptr, AllocateBufferHandle(buffer_handle_reserve_max_size, ints));
    BufferHandle *handle = AllocateBufferHandle(buffer_handle_reserve_max_size, buffer_handle_reserve_max_size);
    ASSERT_NE(nullptr, handle);
    ASSERT_EQ(0, FreeBufferHandle(handle));
}

/*
* Function: BufferHandleTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: AllocateBufferHandle
*/
HWTEST_F(BufferHandleTest, AllocateBufferHandle001, Function | SmallTest | Level2)
{
    uint32_t fds = 1025, ints = 1025;
    uint32_t testsize = 100;
    auto test1 = AllocateBufferHandle(fds, ints);
    ASSERT_EQ(nullptr, test1);

    auto test2 = AllocateBufferHandle(fds, testsize);
    ASSERT_EQ(nullptr, test2);

    auto test3 = AllocateBufferHandle(testsize, ints);
    ASSERT_EQ(nullptr, test3);

    auto test4 = AllocateBufferHandle(testsize, testsize);
    ASSERT_NE(nullptr, test4);
    ASSERT_EQ(0, FreeBufferHandle(test4));
}

/*
* Function: BufferHandleTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: FreeBufferHandle
*/
HWTEST_F(BufferHandleTest, FreeBufferHandle, Function | SmallTest | Level2)
{
    uint32_t buffer_handle_reserve_max_size = 1024;
    ASSERT_EQ(0, FreeBufferHandle(nullptr));
    BufferHandle *handle = AllocateBufferHandle(buffer_handle_reserve_max_size, buffer_handle_reserve_max_size);
    ASSERT_NE(nullptr, handle);
    ASSERT_EQ(0, FreeBufferHandle(handle));
}

/*
* Function: BufferHandleTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: FreeBufferHandle
*/
HWTEST_F(BufferHandleTest, FreeBufferHandle001, Function | SmallTest | Level2)
{
    uint32_t testsize = 100;
    auto test1 = FreeBufferHandle(nullptr);
    ASSERT_EQ(0, test1);

    BufferHandle *handle = AllocateBufferHandle(testsize, testsize);
    ASSERT_NE(nullptr, handle);
    handle->fd = 0;
    handle->reserve[0] = 0;
    auto test2 = FreeBufferHandle(handle);
    ASSERT_EQ(0, test2);
}

/*
* Function: BufferHandleTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: OHOS::WriteBufferHandle
*/
HWTEST_F(BufferHandleTest, WriteBufferHandle, Function | SmallTest | Level2)
{
    MessageParcel parcel;
    uint32_t buffer_handle_reserve_max_size = 1024;
    BufferHandle *handle = AllocateBufferHandle(buffer_handle_reserve_max_size, buffer_handle_reserve_max_size);
    ASSERT_NE(nullptr, handle);

    ASSERT_EQ(false, WriteBufferHandle(parcel, *handle));

    ASSERT_EQ(0, FreeBufferHandle(handle));
}

/*
* Function: BufferHandleTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: OHOS::ReadBufferHandle
*/
HWTEST_F(BufferHandleTest, ReadBufferHandle, Function | SmallTest | Level2)
{
    MessageParcel parcel;

    EXPECT_EQ(nullptr, ReadBufferHandle(parcel));
}
}