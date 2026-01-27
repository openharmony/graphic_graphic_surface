/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include <fcntl.h>
#include <map>
#include <surface.h>
#include <sys/mman.h>
#include <gtest/gtest.h>

#include "buffer_consumer_listener.h"
#include "buffer_extra_data_impl.h"
#include "buffer_producer_listener.h"
#define PRIVATE PUBLIC
#include "buffer_queue.h"
#undef PRIVATE
#include "consumer_surface.h"
#include "producer_surface_delegator.h"
#include "remote_object_mock.h"
#include "sync_fence.h"
#include "surface_buffer_impl.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class BufferQueueTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

    static inline BufferRequestConfig requestConfig = {
        .width = 0x100,
        .height = 0x100,
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    static inline BufferFlushConfigWithDamages flushConfig = {
        .damages = {
            {
                .w = 0x100,
                .h = 0x100,
            }
        },
    };
    static inline int64_t timestamp = 0;
    static inline std::vector<Rect> damages = {};
    static inline sptr<BufferQueue> bq = nullptr;
    static inline std::map<int32_t, sptr<SurfaceBuffer>> cache;
    static inline sptr<BufferExtraData> bedata = nullptr;
    static inline sptr<ProducerSurfaceDelegator> surfaceDelegator = nullptr;
    static inline sptr<IConsumerSurface> csurface1 = nullptr;
};

void BufferQueueTest::SetUpTestCase()
{
    bq = new BufferQueue("test");
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    bq->RegisterConsumerListener(listener);
    bedata = new OHOS::BufferExtraDataImpl;
    csurface1 = IConsumerSurface::Create();
}

void BufferQueueTest::TearDownTestCase()
{
    bq = nullptr;
    bedata = nullptr;
    csurface1 = nullptr;
    surfaceDelegator = nullptr;
    for (auto it : cache) {
        it.second = nullptr;
    }
    cache.clear();
    sleep(2);
}

/*
* Function: GetUsedSize
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetUsedSize and check ret
 */
HWTEST_F(BufferQueueTest, GetUsedSize001, TestSize.Level0)
{
    uint32_t usedSize = bq->GetUsedSize();
    ASSERT_NE(usedSize, -1);
}

/*
* Function: SetQueueSize and GetQueueSize
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetQueueSize for default
*                  2. call SetQueueSize
*                  3. call SetQueueSize again with abnormal input
*                  4. check ret and call GetQueueSize
 */
HWTEST_F(BufferQueueTest, QueueSize001, TestSize.Level0)
{
    ASSERT_EQ(bq->GetQueueSize(), (uint32_t)SURFACE_DEFAULT_QUEUE_SIZE);

    GSError ret = bq->SetQueueSize(2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->SetQueueSize(SURFACE_MAX_QUEUE_SIZE + 1);
    ASSERT_NE(ret, OHOS::GSERROR_OK);

    ASSERT_EQ(bq->GetQueueSize(), 2u);
    BufferQueue *bqTmp = new BufferQueue("testTmp");
    EXPECT_EQ(bqTmp->SetQueueSize(1), GSERROR_OK);
    bqTmp = nullptr;
}

/*
* Function: SetQueueSize and GetQueueSize
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetQueueSize 2 times both with abnormal input
*                  2. call GetQueueSize
*                  3. check ret
 */
HWTEST_F(BufferQueueTest, QueueSize002, TestSize.Level0)
{
    GSError ret = bq->SetQueueSize(-1);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(bq->GetQueueSize(), 2u);

    ret = bq->SetQueueSize(0);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(bq->GetQueueSize(), 2u);
}

/*
* Function: RequestBuffer, FlushBuffer, AcquireBuffer and ReleaseBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer and FlushBuffer
*                  2. call AcquireBuffer and ReleaseBuffer
*                  3. check ret
 */
HWTEST_F(BufferQueueTest, ReqCanFluAcqRel001, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;

    // first request
    ASSERT_EQ(bq->freeList_.size(), 0);
    ASSERT_EQ(bq->dirtyList_.size(), 0);
    ASSERT_EQ(bq->GetUsedSize(), 0);
    ASSERT_EQ(bq->bufferQueueSize_, 2);
    ASSERT_EQ(bq->detachReserveSlotNum_, 0);

    auto start = std::chrono::high_resolution_clock::now();
    auto ret = bq->RequestBuffer(requestConfig, bedata, retval);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "RequestBuffer costs: " << duration.count() << "ms" << std::endl;

    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(retval.buffer, nullptr);
    ASSERT_GE(retval.sequence, 0);

    // add cache
    cache[retval.sequence] = retval.buffer;

    // buffer queue will map
    uint8_t *addr1 = reinterpret_cast<uint8_t*>(retval.buffer->GetVirAddr());
    ASSERT_NE(addr1, nullptr);
    addr1[0] = 5;

    bq->sptrCSurfaceDelegator_ = nullptr;
    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bq->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->AcquireBuffer(retval.buffer, retval.fence, timestamp, damages);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(retval.buffer, nullptr);

    uint8_t *addr2 = reinterpret_cast<uint8_t*>(retval.buffer->GetVirAddr());
    ASSERT_NE(addr2, nullptr);
    if (addr2 != nullptr) {
        ASSERT_EQ(addr2[0], 5u);
    }

    sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
    ret = bq->ReleaseBuffer(retval.buffer, releaseFence);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer and CancelBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer
*                  2. call CancelBuffer
*                  3. check ret
 */
HWTEST_F(BufferQueueTest, ReqCanFluAcqRel002, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;

    // not first request
    ASSERT_EQ(bq->freeList_.size(), 1);
    ASSERT_EQ(bq->dirtyList_.size(), 0);
    ASSERT_EQ(bq->GetUsedSize(), 1);
    ASSERT_EQ(bq->bufferQueueSize_, 2);
    ASSERT_EQ(bq->detachReserveSlotNum_, 0);
    auto start = std::chrono::high_resolution_clock::now();
    GSError ret = bq->RequestBuffer(requestConfig, bedata, retval);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "RequestBuffer costs: " << duration.count() << "ms" << std::endl;

    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval.sequence, 0);
    ASSERT_EQ(retval.buffer, nullptr);

    ret = bq->CancelBuffer(retval.sequence, bedata);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer and CancelBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer
*                  2. call CancelBuffer 2 times
*                  3. check ret
 */
HWTEST_F(BufferQueueTest, ReqCanFluAcqRel003, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;

    // not first request
    GSError ret = bq->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval.sequence, 0);
    ASSERT_EQ(retval.buffer, nullptr);

    ret = bq->CancelBuffer(retval.sequence, bedata);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->CancelBuffer(retval.sequence, bedata);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer and FlushBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer
*                  2. call FlushBuffer 2 times
*                  3. check ret
 */
HWTEST_F(BufferQueueTest, ReqCanFluAcqRel004, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;

    // not first request
    GSError ret = bq->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval.sequence, 0);
    ASSERT_EQ(retval.buffer, nullptr);

    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bq->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
}

/*
* Function: AcquireBuffer and ReleaseBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call AcquireBuffer
*                  2. call ReleaseBuffer 2 times
*                  3. check ret
 */
HWTEST_F(BufferQueueTest, ReqCanFluAcqRel005, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer;

    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    GSError ret = bq->AcquireBuffer(buffer, acquireFence, timestamp, damages);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ASSERT_EQ(buffer->GetSyncFence(), nullptr);
    buffer->SetAndMergeSyncFence(SyncFence::INVALID_FENCE);
    ASSERT_NE(buffer->GetSyncFence(), nullptr);
    ASSERT_FALSE(buffer->GetSyncFence()->IsValid());
    sptr<SyncFence> ReleaseFence = SyncFence::INVALID_FENCE;
    ret = bq->ReleaseBuffer(buffer, ReleaseFence);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->ReleaseBuffer(buffer, ReleaseFence);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer, and CancelBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer and CancelBuffer by different retval
*                  2. check ret
 */
HWTEST_F(BufferQueueTest, ReqCanFluAcqRel006, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval1;
    IBufferProducer::RequestBufferReturnValue retval2;
    IBufferProducer::RequestBufferReturnValue retval3;
    GSError ret;

    // not alloc
    ret = bq->RequestBuffer(requestConfig, bedata, retval1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval1.sequence, 0);
    ASSERT_EQ(retval1.buffer, nullptr);

    // alloc
    ret = bq->RequestBuffer(requestConfig, bedata, retval2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval2.sequence, 0);
    ASSERT_NE(retval2.buffer, nullptr);

    cache[retval2.sequence] = retval2.buffer;

    // no buffer
    ASSERT_EQ(bq->freeList_.size(), 0);
    ASSERT_EQ(bq->dirtyList_.size(), 0);
    ASSERT_EQ(bq->GetUsedSize(), 2u);
    ASSERT_EQ(bq->bufferQueueSize_, 2);
    ASSERT_EQ(bq->detachReserveSlotNum_, 0);
    auto start = std::chrono::high_resolution_clock::now();
    ret = bq->RequestBuffer(requestConfig, bedata, retval3);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "RequestBuffer costs: " << duration.count() << "ms" << std::endl;

    ASSERT_NE(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(retval3.buffer, nullptr);

    ret = bq->CancelBuffer(retval1.sequence, bedata);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->CancelBuffer(retval2.sequence, bedata);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->CancelBuffer(retval3.sequence, bedata);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer, ReleaseBuffer and FlushBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer
*                  2. call ReleaseBuffer
*                  3. call FlushBuffer
*                  4. check ret
 */
HWTEST_F(BufferQueueTest, ReqCanFluAcqRel007, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;

    // not alloc
    GSError ret = bq->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval.sequence, 0);
    ASSERT_EQ(retval.buffer, nullptr);

    retval.buffer = cache[retval.sequence];
    retval.buffer->SetAndMergeSyncFence(new SyncFence(0));
    sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
    ret = bq->ReleaseBuffer(retval.buffer, releaseFence);
    ASSERT_NE(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bq->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: AcquireBuffer, FlushBuffer and ReleaseBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call AcquireBuffer
*                  2. call FlushBuffer
*                  3. call ReleaseBuffer
*                  4. check ret
 */
HWTEST_F(BufferQueueTest, ReqCanFluAcqRel008, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;

    // acq from last test
    GSError ret = bq->AcquireBuffer(buffer, acquireFence, timestamp, damages);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    uint32_t sequence;
    for (auto it = cache.begin(); it != cache.end(); it++) {
        if (it->second.GetRefPtr() == buffer.GetRefPtr()) {
            sequence = it->first;
        }
    }
    ASSERT_GE(sequence, 0);

    ret = bq->FlushBuffer(sequence, bedata, acquireFence, flushConfig);
    ASSERT_NE(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
    ret = bq->ReleaseBuffer(buffer, releaseFence);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer and CancelBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer
*                  2. call CancelBuffer
*                  3. check retval and ret
 */
HWTEST_F(BufferQueueTest, ReqCanFluAcqRel009, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    BufferRequestConfig deleteconfig = requestConfig;
    deleteconfig.width = 1921;

    GSError ret = bq->RequestBuffer(deleteconfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(retval.deletingBuffers.size(), 1u);
    ASSERT_GE(retval.sequence, 0);
    ASSERT_NE(retval.buffer, nullptr);

    ret = bq->CancelBuffer(retval.sequence, bedata);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer, FlushBuffer, AcquireBuffer and ReleaseBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer and FlushBuffer
*                  2. call AcquireBuffer and ReleaseBuffer
*                  3. check ret
 */
HWTEST_F(BufferQueueTest, ReqCanFluAcqRel0010, Function | MediumTest | Level2)
{
    IBufferProducer::RequestBufferReturnValue retval;
    IBufferProducer::RequestBufferReturnValue retval1;
    IBufferProducer::RequestBufferReturnValue retval2;

    GSError ret = bq->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval.sequence, 0);
    ASSERT_EQ(retval.buffer, nullptr);

    ret = bq->RequestBuffer(requestConfig, bedata, retval1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval1.sequence, 0);
    ASSERT_NE(retval1.buffer, nullptr);

    ASSERT_EQ(bq->freeList_.size(), 0);
    ASSERT_EQ(bq->GetUsedSize(), 2u);
    ASSERT_EQ(bq->bufferQueueSize_, 2);
    ASSERT_EQ(bq->detachReserveSlotNum_, 0);
    ASSERT_EQ(bq->dirtyList_.size(), 0);

    bq->SetRequestBufferNoblockMode(true);
    bool noblock = false;
    bq->GetRequestBufferNoblockMode(noblock);
    ASSERT_TRUE(noblock);
    auto start = std::chrono::high_resolution_clock::now();
    ret = bq->RequestBuffer(requestConfig, bedata, retval2);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "RequestBuffer costs: " << duration.count() << "ms" << std::endl;

    ASSERT_EQ(ret, OHOS::GSERROR_NO_BUFFER);

    ret = bq->CancelBuffer(retval.sequence, bedata);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->CancelBuffer(retval1.sequence, bedata);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer and CancelBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer
*                  2. call CancelBuffer
*                  3. check retval and ret
 */
HWTEST_F(BufferQueueTest, ReqCanFluAcqRel0011, Function | MediumTest | Level2)
{
    IBufferProducer::RequestBufferReturnValue retval;
    IBufferProducer::RequestBufferReturnValue retval1;
    IBufferProducer::RequestBufferReturnValue retval2;

    GSError ret = bq->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval.sequence, 0);
    ASSERT_EQ(retval.buffer, nullptr);

    ret = bq->RequestBuffer(requestConfig, bedata, retval1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval1.sequence, 0);
    ASSERT_EQ(retval1.buffer, nullptr);

    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bq->FlushBuffer(retval1.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    // no buffer
    ASSERT_EQ(bq->freeList_.size(), 0);
    ASSERT_EQ(bq->GetUsedSize(), 2u);
    ASSERT_EQ(bq->bufferQueueSize_, 2);
    ASSERT_EQ(bq->detachReserveSlotNum_, 0);
    ASSERT_EQ(bq->dirtyList_.size(), 1);

    auto start = std::chrono::high_resolution_clock::now();
    ret = bq->RequestBuffer(requestConfig, bedata, retval2);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "RequestBuffer costs: " << duration.count() << "ms" << std::endl;

    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(retval2.buffer, nullptr);

    ret = bq->CancelBuffer(retval.sequence, bedata);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    acquireFence = SyncFence::INVALID_FENCE;
    ret = bq->FlushBuffer(retval2.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->AcquireBuffer(retval2.buffer, retval.fence, timestamp, damages);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
    ret = bq->ReleaseBuffer(retval2.buffer, releaseFence);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    bq->SetRequestBufferNoblockMode(false);
    bool noblock = true;
    bq->GetRequestBufferNoblockMode(noblock);
    ASSERT_FALSE(noblock);
}

/*
* Function: RequestBuffer and CancelBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer
*                  2. call CancelBuffer
*                  3. check ret
 */
HWTEST_F(BufferQueueTest, ReqCanFluAcqRel0012, Function | MediumTest | Level2)
{
    IBufferProducer::RequestBufferReturnValue retval;

    for (auto i = 0; i != 1000; i++) {
        GSError ret = bq->RequestBuffer(requestConfig, bedata, retval);
        ASSERT_EQ(ret, OHOS::GSERROR_OK);

        ret = bq->CancelBuffer(retval.sequence, bedata);
        ASSERT_EQ(ret, OHOS::GSERROR_OK);
    }
}

/*
 * Function: GetLastConsumeTime
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call GetLastConsumeTime
 *                  2. check lastConsumeTime
 */
HWTEST_F(BufferQueueTest, GetLastConsumeTimeTest, TestSize.Level0)
{
    int64_t lastConsumeTime = 0;
    bq->GetLastConsumeTime(lastConsumeTime);
    std::cout << "lastConsumeTime = " << lastConsumeTime << std::endl;
    ASSERT_NE(lastConsumeTime, 0);
}

/*
* Function: SetDesiredPresentTimestampAndUiTimestamp
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetDesiredPresentTimestampAndUiTimestamp with different parameter and check ret
*                  2. call SetDesiredPresentTimestampAndUiTimestamp with empty parameter and check ret
*                  3. repeatly call SetDesiredPresentTimestampAndUiTimestamp with different parameter and check ret
 */
HWTEST_F(BufferQueueTest, SetDesiredPresentTimestampAndUiTimestamp001, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    BufferRequestConfig config = requestConfig;
    config.width = 1921;
    GSError ret = bq->RequestBuffer(config, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval.sequence, 0);

    // call SetDesiredPresentTimestampAndUiTimestamp with different uiTimestamp and desireTimestamp, check ret
    int64_t desiredPresentTimestamp = 0;
    uint64_t uiTimestamp = 2;
    bq->SetDesiredPresentTimestampAndUiTimestamp(retval.sequence, desiredPresentTimestamp, uiTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].desiredPresentTimestamp, uiTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].timestamp, uiTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].isAutoTimestamp, false);

    desiredPresentTimestamp = -1;
    bq->SetDesiredPresentTimestampAndUiTimestamp(retval.sequence, desiredPresentTimestamp, uiTimestamp);
    ASSERT_GT(bq->bufferQueueCache_[retval.sequence].desiredPresentTimestamp, 0);
    ASSERT_NE(bq->bufferQueueCache_[retval.sequence].desiredPresentTimestamp, uiTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].timestamp, uiTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].isAutoTimestamp, true);

    desiredPresentTimestamp = 1;
    bq->SetDesiredPresentTimestampAndUiTimestamp(retval.sequence, desiredPresentTimestamp, uiTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].desiredPresentTimestamp, desiredPresentTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].timestamp, uiTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].isAutoTimestamp, false);

    // call SetDesiredPresentTimestampAndUiTimestamp with empty uiTimestamp and desireTimestamp, check ret
    desiredPresentTimestamp = 0;
    uiTimestamp = 0;
    bq->SetDesiredPresentTimestampAndUiTimestamp(retval.sequence, desiredPresentTimestamp, uiTimestamp);
    ASSERT_NE(bq->bufferQueueCache_[retval.sequence].desiredPresentTimestamp, desiredPresentTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].timestamp, uiTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].isAutoTimestamp, true);

    //repeatly call SetDesiredPresentTimestampAndUiTimestamp with different uiTimestamp and desireTimestamp, check ret
    desiredPresentTimestamp = 0;
    uiTimestamp = 2;
    bq->SetDesiredPresentTimestampAndUiTimestamp(retval.sequence, desiredPresentTimestamp, uiTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].desiredPresentTimestamp, uiTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].timestamp, uiTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].isAutoTimestamp, false);

    desiredPresentTimestamp = -1;
    bq->SetDesiredPresentTimestampAndUiTimestamp(retval.sequence, desiredPresentTimestamp, uiTimestamp);
    ASSERT_GT(bq->bufferQueueCache_[retval.sequence].desiredPresentTimestamp, 0);
    ASSERT_NE(bq->bufferQueueCache_[retval.sequence].desiredPresentTimestamp, uiTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].timestamp, uiTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].isAutoTimestamp, true);

    desiredPresentTimestamp = 1;
    bq->SetDesiredPresentTimestampAndUiTimestamp(retval.sequence, desiredPresentTimestamp, uiTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].desiredPresentTimestamp, desiredPresentTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].timestamp, uiTimestamp);
    ASSERT_EQ(bq->bufferQueueCache_[retval.sequence].isAutoTimestamp, false);

    ret = bq->CancelBuffer(retval.sequence, bedata);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. set BufferRequestConfig with abnormal value
*                  2. call RequestBuffer
*                  3. check ret
 */
HWTEST_F(BufferQueueTest, RequestBuffer001, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    BufferRequestConfig config = requestConfig;
    config.width = -1;

    GSError ret = bq->RequestBuffer(config, bedata, retval);
    ASSERT_EQ(ret, OHOS::SURFACE_ERROR_UNKOWN);
}

/*
* Function: RequestBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. set BufferRequestConfig with abnormal value
*                  2. call RequestBuffer
*                  3. check ret
 */
HWTEST_F(BufferQueueTest, RequestBuffer002, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    BufferRequestConfig config = requestConfig;
    config.height = -1;

    GSError ret = bq->RequestBuffer(config, bedata, retval);
    ASSERT_EQ(ret, OHOS::SURFACE_ERROR_UNKOWN);
}

/*
* Function: RequestBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. set BufferRequestConfig with abnormal value
*                  2. call RequestBuffer
*                  3. check ret
 */
HWTEST_F(BufferQueueTest, RequestBuffer006, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    BufferRequestConfig config = requestConfig;
    config.format = -1;

    GSError ret = bq->RequestBuffer(config, bedata, retval);
    ASSERT_EQ(ret, OHOS::SURFACE_ERROR_UNKOWN);
}

/*
* Function: QueryIfBufferAvailable
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call QueryIfBufferAvailable and check ret
 */
HWTEST_F(BufferQueueTest, QueryIfBufferAvailable001, TestSize.Level0)
{
    bq->CleanCache(false, nullptr);
    bool ret = bq->QueryIfBufferAvailable();
    ASSERT_EQ(ret, true);

    GSError reqRet = OHOS::GSERROR_OK;
    IBufferProducer::RequestBufferReturnValue retval;
    BufferRequestConfig config = requestConfig;
    while (reqRet != OHOS::GSERROR_NO_BUFFER) {
        reqRet = bq->RequestBuffer(config, bedata, retval);
    }

    ret = bq->QueryIfBufferAvailable();
    ASSERT_EQ(ret, false);
}

/*
* Function: GetName
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetName and check ret
 */
HWTEST_F(BufferQueueTest, GetName001, TestSize.Level0)
{
    std::string name("na");
    GSError ret = bq->GetName(name);
    ASSERT_EQ(ret, GSERROR_OK);
    ASSERT_NE(name, "na");
}

/*
* Function: RegisterConsumerListener
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RegisterConsumerListener and check ret
 */
HWTEST_F(BufferQueueTest, RegisterConsumerListener001, TestSize.Level0)
{
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    GSError ret = bq->RegisterConsumerListener(listener);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: SetDefaultWidthAndHeight
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetDefaultWidthAndHeight and check ret
 */
HWTEST_F(BufferQueueTest, SetDefaultWidthAndHeight001, TestSize.Level0)
{
    int width = 0;
    int height = 0;
    GSError ret = bq->SetDefaultWidthAndHeight(width, height);
    ASSERT_EQ(ret, GSERROR_INVALID_ARGUMENTS);

    width = 1;
    ret = bq->SetDefaultWidthAndHeight(width, height);
    ASSERT_EQ(ret, GSERROR_INVALID_ARGUMENTS);

    width = 80;
    height = 80;
    ret = bq->SetDefaultWidthAndHeight(width, height);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: GetDefaultWidth and GetDefaultHeight
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetDefaultWidth and check ret
 */
HWTEST_F(BufferQueueTest, GetDefaultWidth001, TestSize.Level0)
{
    int32_t width = 80;
    int32_t height = 80;
    GSError ret = bq->SetDefaultWidthAndHeight(width, height);
    ASSERT_EQ(ret, GSERROR_OK);

    ASSERT_EQ(width, bq->GetDefaultWidth());
    ASSERT_EQ(height, bq->GetDefaultHeight());
}

/*
* Function: SetDefaultUsage and GetDefaultUsage
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetDefaultUsage and check ret
 */
HWTEST_F(BufferQueueTest, SetDefaultUsage001, TestSize.Level0)
{
    uint64_t usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA;
    GSError ret = bq->SetDefaultUsage(usage);
    ASSERT_EQ(ret, GSERROR_OK);
    ASSERT_EQ(usage, bq->GetDefaultUsage());
}

/*
* Function: CleanCache
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call CleanCache and check ret
 */
HWTEST_F(BufferQueueTest, CleanCache001, TestSize.Level0)
{
    GSError ret = bq->CleanCache(false, nullptr);
    ASSERT_EQ(ret, GSERROR_OK);
}
/*
* Function: AttachBufferUpdateStatus
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call AttachBufferUpdateStatus and check ret
 */
HWTEST_F(BufferQueueTest, AttachBufferUpdateStatus, TestSize.Level0)
{
    uint32_t sequence = 2;
    int32_t timeOut = 6;
    std::mutex mutex_;
    std::unique_lock<std::mutex> lock(mutex_);
    auto mapIter = bq->bufferQueueCache_.find(sequence);
    GSError ret = bq->AttachBufferUpdateStatus(lock, sequence, timeOut, mapIter);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: AttachBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call AttachBuffer, DetachBuffer and check ret
 */
HWTEST_F(BufferQueueTest, AttachBufferAndDetachBuffer001, TestSize.Level0)
{
    bq->CleanCache(false, nullptr);
    int32_t timeOut = 6;
    IBufferProducer::RequestBufferReturnValue retval;
    GSError ret = bq->AttachBuffer(retval.buffer, timeOut);
    ASSERT_EQ(ret, GSERROR_INVALID_OPERATING);
    EXPECT_EQ(bq->DetachBuffer(retval.buffer), GSERROR_INVALID_ARGUMENTS);
    sptr<SurfaceBuffer> buffer = nullptr;
    EXPECT_EQ(bq->DetachBuffer(buffer), GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: AttachBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call AttachBuffer, DetachBuffer and check ret
 */
HWTEST_F(BufferQueueTest, AttachBufferAndDetachBuffer002, TestSize.Level0)
{
    bq->CleanCache(false, nullptr);
    int32_t timeOut = 6;
    EXPECT_EQ(bq->SetQueueSize(SURFACE_MAX_QUEUE_SIZE), GSERROR_OK);
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    ASSERT_NE(buffer, nullptr);
    GSError ret = bq->AttachBuffer(buffer, timeOut);
    sptr<SurfaceBuffer> buffer1 = SurfaceBuffer::Create();
    EXPECT_EQ(bq->GetUsedSize(), 1);
    ASSERT_EQ(ret, GSERROR_OK);
    EXPECT_EQ(bq->AttachBuffer(buffer1, timeOut), GSERROR_OK);
    bq->sourceType_ = OHSurfaceSource::OH_SURFACE_SOURCE_LOWPOWERVIDEO;
    bq->AttachBuffer(buffer, timeOut);
    ASSERT_EQ(ret, GSERROR_OK);
    ret= bq->DetachBuffer(buffer);
    EXPECT_EQ(ret, GSERROR_NO_ENTRY);
    EXPECT_EQ(bq->DetachBuffer(buffer1), GSERROR_NO_ENTRY);
    {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        EXPECT_EQ(bq->AllocBuffer(buffer1, nullptr, requestConfig, lock), GSERROR_OK);
    }
    EXPECT_EQ(bq->DetachBuffer(buffer1), GSERROR_OK);
}

/*
* Function: RegisterDeleteBufferListener
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RegisterDeleteBufferListener and check ret
 */
HWTEST_F(BufferQueueTest, RegisterDeleteBufferListener001, TestSize.Level0)
{
    GSError ret = bq->RegisterDeleteBufferListener(nullptr, true);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: QueueAndDequeueDelegator
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RegisterSurfaceDelegator and check ret
*                  2. call RequestBuffer and check ret (this will call DelegatorDequeueBuffer)
*                  3. call FlushBuffer and check ret (this will call DelegatorQueueBuffer)
 */
HWTEST_F(BufferQueueTest, QueueAndDequeueDelegator001, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    auto ret = bq->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, GSERROR_OK);
    ASSERT_GE(retval.sequence, 0);

    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bq->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: SetSurfaceSourceType and GetSurfaceSourceType
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetSurfaceSourceType and check ret
*                  2. call GetSurfaceSourceType and check the value
*/
HWTEST_F(BufferQueueTest, SurfaceSourceType001, TestSize.Level0)
{
    OHSurfaceSource sourceType = OHSurfaceSource::OH_SURFACE_SOURCE_VIDEO;
    GSError ret = bq->SetSurfaceSourceType(sourceType);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(sourceType, bq->GetSurfaceSourceType());
}

/*
* Function: SetSurfaceAppFrameworkType
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetSurfaceAppFrameworkType and check ret
*/
HWTEST_F(BufferQueueTest, SetSurfaceAppFrameworkType001, TestSize.Level0)
{
    std::string type = "";
    GSError ret = bq->SetSurfaceAppFrameworkType(type);
    ASSERT_EQ(ret, OHOS::GSERROR_NO_ENTRY);

    std::string type1 = "AAAAABBBBBCCCCCDDDDDEEEEEFFFFFGGGGGAAAAABBBBBCCCCCDDDDDEEEEEFFFFFGGGGG";
    ret = bq->SetSurfaceAppFrameworkType(type1);
    ASSERT_EQ(ret, OHOS::GSERROR_OUT_OF_RANGE);

    std::string type2 = "test";
    ret = bq->SetSurfaceAppFrameworkType(type2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: GetSurfaceAppFrameworkType
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetSurfaceAppFrameworkType and check value
*/
HWTEST_F(BufferQueueTest, GetSurfaceAppFrameworkType001, TestSize.Level0)
{
    std::string type = "test";
    GSError ret = bq->SetSurfaceAppFrameworkType(type);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(bq->GetSurfaceAppFrameworkType(), "test");
}

/*
* Function: SetGlobalAlpha and GetGlobalAlpha
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetGlobalAlpha and check value
*                  2. call GetGlobalAlpha and check value
*/
HWTEST_F(BufferQueueTest, SetGlobalAlpha001, TestSize.Level0)
{
    int32_t alpha = 255;
    GSError ret = bq->SetGlobalAlpha(alpha);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    int32_t resultAlpha = -1;
    ret = bq->GetGlobalAlpha(resultAlpha);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(resultAlpha, alpha);
}

/*
* Function: SetFrameGravity and GetGlobalAlpha
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetFrameGravity and check value
*                  2. call GetGlobalAlpha and check value
*/
HWTEST_F(BufferQueueTest, SetFrameGravity001, TestSize.Level0)
{
    int32_t frameGravity = 15;
    GSError ret = bq->SetFrameGravity(frameGravity);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    int32_t resultFrameGravity = -1;
    ret = bq->GetFrameGravity(resultFrameGravity);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(resultFrameGravity, frameGravity);
}

/*
* Function: SetFixedRotation and GetFixedRotation
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetFixedRotation and check value
*                  2. call GetFixedRotation and check value
*/
HWTEST_F(BufferQueueTest, SetFixedRotation001, TestSize.Level0)
{
    int32_t fixedRotation = 1;
    GSError ret = bq->SetFixedRotation(fixedRotation);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    int32_t resultFixedRotation = -1;
    ret = bq->GetFixedRotation(resultFixedRotation);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(resultFixedRotation, fixedRotation);
}

/*
* Function: GetLastFlushedDesiredPresentTimeStamp
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetLastFlushedDesiredPresentTimeStamp and check value
*/
HWTEST_F(BufferQueueTest, GetLastFlushedDesiredPresentTimeStamp001, TestSize.Level0)
{
    int64_t timeStampValue = 100000;
    bq->lastFlushedDesiredPresentTimeStamp_ = timeStampValue;
    int64_t result = 0;
    GSError ret = bq->GetLastFlushedDesiredPresentTimeStamp(result);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(result, timeStampValue);
}

/*
* Function: GetFrontDesiredPresentTimeStamp
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetFrontDesiredPresentTimeStamp and check value
*/
HWTEST_F(BufferQueueTest, GetFrontDesiredPresentTimeStamp001, Function | MediumTest | Level2)
{
    int64_t result = 0;
    bool isAutoTimeStamp = false;
    bq->dirtyList_.clear();
    GSError ret = bq->GetFrontDesiredPresentTimeStamp(result, isAutoTimeStamp);
    ASSERT_EQ(ret, OHOS::GSERROR_NO_BUFFER);
    ASSERT_EQ(result, 0);
}


/*
* Function: GetFrontDesiredPresentTimeStamp
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetFrontDesiredPresentTimeStamp and check value
*/
HWTEST_F(BufferQueueTest, GetFrontDesiredPresentTimeStamp002, Function | MediumTest | Level2)
{
    int64_t timeStampValue = 100;
    bool isAutoTimeStamp = false;
    int64_t result = -1;
    bq->dirtyList_.clear();
    bq->bufferQueueCache_.clear();
    uint32_t seqId = 100;
    bq->dirtyList_.push_back(seqId);
    bq->bufferQueueCache_[seqId].desiredPresentTimestamp = timeStampValue;
    bq->bufferQueueCache_[seqId].isAutoTimestamp = isAutoTimeStamp;
    GSError ret = bq->GetFrontDesiredPresentTimeStamp(result, isAutoTimeStamp);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(result, timeStampValue);
}

/*
* Function: GetFrontDesiredPresentTimeStamp
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetFrontDesiredPresentTimeStamp and check value
*/
HWTEST_F(BufferQueueTest, GetFrontDesiredPresentTimeStamp003, Function | MediumTest | Level2)
{
    std::unique_lock<std::mutex> lock;
    int64_t timeStampValue1 = 100;
    bool isAutoTimeStamp1 = false;
    int64_t timeStampValue2 = 200;
    bool isAutoTimeStamp2 = true;
    bq->ClearLocked(lock);
    uint32_t seqId1 = 100;
    uint32_t seqId2 = 200;
    bq->dirtyList_.push_back(seqId1);
    bq->dirtyList_.push_back(seqId2);
    bq->bufferQueueCache_[seqId1].desiredPresentTimestamp = timeStampValue1;
    bq->bufferQueueCache_[seqId1].isAutoTimestamp = isAutoTimeStamp1;
    bq->bufferQueueCache_[seqId2].desiredPresentTimestamp = timeStampValue2;
    bq->bufferQueueCache_[seqId2].isAutoTimestamp = isAutoTimeStamp2;
    int64_t result = -1;
    bool isAutoTimeStamp = false;
    GSError ret = bq->GetFrontDesiredPresentTimeStamp(result, isAutoTimeStamp);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(result, timeStampValue1);
}

/*
* Function: GetBufferSupportFastCompose
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetBufferSupportFastCompose and check value
*/
HWTEST_F(BufferQueueTest, GetBufferSupportFastCompose001, TestSize.Level0)
{
    bool supportFastCompose = true;
    bq->bufferSupportFastCompose_ = supportFastCompose;
    bool result = false;
    GSError ret = bq->GetBufferSupportFastCompose(result);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(result, supportFastCompose);
}

/*
* Function: RegisterProducerPropertyListener
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RegisterProducerPropertyListener and check value
*/
HWTEST_F(BufferQueueTest, RegisterProducerPropertyListener001, TestSize.Level0)
{
    bool producerId = 6;
    OnReleaseFunc onBufferRelease = nullptr;
    sptr<IProducerListener> listener = new BufferReleaseProducerListener(onBufferRelease);
    std::map<uint64_t, sptr<IProducerListener>> propertyChangeListeners_ = {{producerId, listener}};
    GSError ret = bq->RegisterProducerPropertyListener(listener, producerId);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: UnRegisterProducerPropertyListener
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call UnRegisterProducerPropertyListener and check value
*/
HWTEST_F(BufferQueueTest, UnRegisterProducerPropertyListener001, TestSize.Level0)
{
    bool producerId = 6;
    OnReleaseFunc onBufferRelease = nullptr;
    sptr<IProducerListener> listener = new BufferReleaseProducerListener(onBufferRelease);
    std::map<uint64_t, sptr<IProducerListener>> propertyChangeListeners_ = {{producerId, listener}};
    GSError ret = bq->UnRegisterProducerPropertyListener(producerId);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: SetTransformHint
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetTransformHint and check value
*/
HWTEST_F(BufferQueueTest, SetTransformHint001, TestSize.Level0)
{
    GraphicTransformType transformHint = GraphicTransformType::GRAPHIC_ROTATE_90;
    uint64_t producerId = 0;
    EXPECT_EQ(bq->SetTransformHint(transformHint, producerId), GSERROR_OK);
    transformHint = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    ASSERT_EQ(bq->SetTransformHint(transformHint, producerId), OHOS::GSERROR_OK);
}

/*
 * Function: PreAllocBuffers
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetUp: producer calls PreAllocBuffers and check the ret
 *                  2. operation: producer sends invalid width/height/format/allocBufferCount config to bufferQueue
 *                  3. result: bufferQueue return GSERROR_INVALID_ARGUMENTS
 */
HWTEST_F(BufferQueueTest, PreAllocBuffers001, TestSize.Level0)
{
    BufferQueue *bqTmp = new BufferQueue("testTmp");
    EXPECT_EQ(bqTmp->SetQueueSize(3), GSERROR_OK);
    BufferRequestConfig requestConfigTmp = {
        .width = 0x0,
        .height = 0x100,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
    };
    uint32_t allocBufferCount = 3;
    GSError ret = bqTmp->PreAllocBuffers(requestConfigTmp, allocBufferCount);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ((bqTmp->bufferQueueCache_).size(), 0);

    requestConfigTmp = {
        .width = 0x100,
        .height = 0x0,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
    };
    ret = bqTmp->PreAllocBuffers(requestConfigTmp, allocBufferCount);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ((bqTmp->bufferQueueCache_).size(), 0);

    requestConfigTmp = {
        .width = 0x100,
        .height = 0x100,
        .format = GRAPHIC_PIXEL_FMT_BUTT,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
    };
    ret = bqTmp->PreAllocBuffers(requestConfigTmp, allocBufferCount);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ((bqTmp->bufferQueueCache_).size(), 0);

    requestConfigTmp = {
        .width = 0x100,
        .height = 0x100,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
    };
    allocBufferCount = 0;
    ret = bqTmp->PreAllocBuffers(requestConfigTmp, allocBufferCount);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ((bqTmp->bufferQueueCache_).size(), 0);
    bqTmp = nullptr;
}

/*
 * Function: PreAllocBuffers
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetUp: producer calls PreAllocBuffers and check the ret
 *                  2. operation: producer sends valid width/height/format/allocBufferCount config to bufferQueue
 *                  3. result: bufferQueue return GSERROR_OK
 */
HWTEST_F(BufferQueueTest, PreAllocBuffers002, TestSize.Level0)
{
    BufferQueue *bqTmp = new BufferQueue("testTmp");
    EXPECT_EQ(bqTmp->SetQueueSize(3), GSERROR_OK);
    BufferRequestConfig requestConfigTmp = {
        .width = 0x100,
        .height = 0x100,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
    };
    uint32_t allocBufferCount = 3;
    GSError ret = bqTmp->PreAllocBuffers(requestConfigTmp, allocBufferCount);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ((bqTmp->bufferQueueCache_).size(), 3);
    bqTmp = nullptr;
}

/*
 * Function: PreAllocBuffers
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetUp: producer calls PreAllocBuffers and check the ret
 *                  2. operation: producer sends valid width/height/format/allocBufferCount and larger allocBufferCount
 *                       than empty bufferQueueSize config to bufferQueue
 *                  3. result: bufferQueue return GSERROR_OK
 */
HWTEST_F(BufferQueueTest, PreAllocBuffers003, TestSize.Level0)
{
    BufferQueue *bqTmp = new BufferQueue("testTmp");
    EXPECT_EQ(bqTmp->SetQueueSize(1), GSERROR_OK);
    BufferRequestConfig requestConfigTmp = {
        .width = 0x100,
        .height = 0x100,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
    };
    uint32_t allocBufferCount = 3;
    GSError ret = bqTmp->PreAllocBuffers(requestConfigTmp, allocBufferCount);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ((bqTmp->bufferQueueCache_).size(), 1);
    bqTmp = nullptr;
}

/*
 * Function: PreAllocBuffers
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetUp: producer calls PreAllocBuffers and check the ret
 *                  2. operation: producer sends all usage type
 *                  3. result: bufferQueue return GSERROR_OK
 */
HWTEST_F(BufferQueueTest, PreAllocBuffers004, TestSize.Level0)
{
    BufferQueue *bqTmp = new BufferQueue("testTmp");
    EXPECT_EQ(bqTmp->SetQueueSize(1), GSERROR_OK);
    BufferRequestConfig requestConfigTmp = {
        .width = 0x100,
        .height = 0x100,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
    };

    uint32_t allocBufferCount = 3;
    std::vector<SurfaceBufferUsage> bufferUsages = {
        SurfaceBufferUsage::BUFFER_USAGE_CPU_READ,
        SurfaceBufferUsage::BUFFER_USAGE_CPU_WRITE,
        SurfaceBufferUsage::BUFFER_USAGE_MEM_MMZ,
        SurfaceBufferUsage::BUFFER_USAGE_MEM_DMA,
        SurfaceBufferUsage::BUFFER_USAGE_MEM_SHARE,
        SurfaceBufferUsage::BUFFER_USAGE_MEM_MMZ_CACHE,
        SurfaceBufferUsage::BUFFER_USAGE_MEM_FB,
        SurfaceBufferUsage::BUFFER_USAGE_ASSIGN_SIZE,
        SurfaceBufferUsage::BUFFER_USAGE_HW_RENDER,
        SurfaceBufferUsage::BUFFER_USAGE_HW_TEXTURE,
        SurfaceBufferUsage::BUFFER_USAGE_HW_COMPOSER,
        SurfaceBufferUsage::BUFFER_USAGE_PROTECTED,
        SurfaceBufferUsage::BUFFER_USAGE_CAMERA_READ,
        SurfaceBufferUsage::BUFFER_USAGE_CAMERA_WRITE,
        SurfaceBufferUsage::BUFFER_USAGE_VIDEO_ENCODER,
        SurfaceBufferUsage::BUFFER_USAGE_VIDEO_DECODER,
        SurfaceBufferUsage::BUFFER_USAGE_CPU_READ_OFTEN,
        SurfaceBufferUsage::BUFFER_USAGE_CPU_HW_BOTH,
        SurfaceBufferUsage::BUFFER_USAGE_ALIGNMENT_512,
        SurfaceBufferUsage::BUFFER_USAGE_DRM_REDRAW,
        SurfaceBufferUsage::BUFFER_USAGE_GRAPHIC_2D_ACCEL,
        SurfaceBufferUsage::BUFFER_USAGE_PREFER_NO_PADDING
    };

    for (const auto& bufferUsage : bufferUsages) {
        // 遍历不同的usage
        requestConfigTmp.usage = bufferUsage;
        GSError ret = bqTmp->PreAllocBuffers(requestConfigTmp, allocBufferCount);
        ASSERT_EQ(ret, OHOS::GSERROR_OK);
        ASSERT_EQ((bqTmp->bufferQueueCache_).size(), 1);
        bqTmp = nullptr;      
    }
}

/*
 * Function: PreAllocBuffers
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetUp: producer calls PreAllocBuffers and check the ret
 *                  2. operation: producer sends valid width/height/format and larger allocBufferCount
 *                       than max bufferQueueSize config to bufferQueue
 *                  3. result: bufferQueue return GSERROR_OK
 */
HWTEST_F(BufferQueueTest, PreAllocBuffers005, TestSize.Level0)
{
    BufferQueue *bqTmp = new BufferQueue("testTmp");
    // max queue size为64
    EXPECT_EQ(bqTmp->SetQueueSize(64), GSERROR_OK);
    BufferRequestConfig requestConfigTmp = {
        .width = 0x100,
        .height = 0x100,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
    };
    uint32_t allocBufferCount = 65;
    GSError ret = bqTmp->PreAllocBuffers(requestConfigTmp, allocBufferCount);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ((bqTmp->bufferQueueCache_).size(), 64);
    bqTmp = nullptr;
}

/*
 * Function: MarkBufferReclaimableByIdLocked
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call MarkBufferReclaimableByIdLocked and check value
 */
HWTEST_F(BufferQueueTest, MarkBufferReclaimableByIdLocked001, TestSize.Level0)
{
    for (const auto &[id, ele] : bq->bufferQueueCache_) {
        printf("seq: %u\n", id);
        bq->MarkBufferReclaimableByIdLocked(id);
        ASSERT_GT(bq->bufferQueueCache_.size(), 0);
    }
}

/*
 * Function: SetupNewBufferLockedAndAllocBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetupNewBufferLocked and AllocBuffer check ret
 */
HWTEST_F(BufferQueueTest, SetupNewBufferLockedAndAllocBuffer001, TestSize.Level0)
{
    BufferRequestConfig requestConfig = {
        .width = 0x100,
        .height = 0x100,
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    BufferRequestConfig updateConfig = {
        .width = 0,
        .height = 0,
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    IBufferProducer::RequestBufferReturnValue retval;
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    EXPECT_EQ(bq->SetupNewBufferLocked(buffer, bedata, updateConfig, requestConfig,
        retval, lock), SURFACE_ERROR_UNKOWN);
}

/*
 * Function: Multiple requests for buffer in blocking mode, other threads release buffer and reuse buffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. requestThread multiple requests for buffer in blocking mode
 *                  2. releaseThread release buffer
 *                  3. requestThread reuse buffer
 *                  4. check ret
 */
HWTEST_F(BufferQueueTest, ReqBufferWithBlockModeAndReuseBuffer001, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    IBufferProducer::RequestBufferReturnValue retval1;
    IBufferProducer::RequestBufferReturnValue retval2;
    IBufferProducer::RequestBufferReturnValue retval3;

    BufferRequestConfig requestConfigTest = {
        .width = 0x100,
        .height = 0x100,
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 5000,
    };
    std::mutex mtx;
    std::condition_variable cv;
    bool conditionMet = false;
    std::atomic<bool> stopRequest(false);
    std::atomic<bool> stopRelease(false);
    sptr<BufferQueue> bqTest = new BufferQueue("test");
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    bqTest->RegisterConsumerListener(listener);
    // Create thread for requesting buffer
    std::thread requestThread([&]() {
        while (!stopRequest) {
            GSError ret = bqTest->RequestBuffer(requestConfigTest, bedata, retval);
            ASSERT_EQ(ret, OHOS::GSERROR_OK);
            ASSERT_GE(retval.sequence, 0);
            ASSERT_NE(retval.buffer, nullptr);

            ret = bqTest->RequestBuffer(requestConfigTest, bedata, retval1);
            ASSERT_EQ(ret, OHOS::GSERROR_OK);
            ASSERT_GE(retval1.sequence, 0);
            ASSERT_NE(retval1.buffer, nullptr);

            ret = bqTest->RequestBuffer(requestConfigTest, bedata, retval2);
            ASSERT_EQ(ret, OHOS::GSERROR_OK);
            ASSERT_GE(retval2.sequence, 0);
            ASSERT_NE(retval2.buffer, nullptr);

            ASSERT_EQ(bqTest->freeList_.size(), 0);
            ASSERT_EQ(bqTest->GetUsedSize(), 3U);
            ASSERT_EQ(bqTest->bufferQueueSize_, 3U);
            ASSERT_EQ(bqTest->detachReserveSlotNum_, 0);
            ASSERT_EQ(bqTest->dirtyList_.size(), 0);

            conditionMet = true;
            cv.notify_one();
            auto start = std::chrono::high_resolution_clock::now();
            ret = bqTest->RequestBuffer(requestConfigTest, bedata, retval3);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "RequestBuffer costs: " << duration.count() << "ms" << std::endl;
            ASSERT_LT(duration.count(), 1500); // Confirm blockage 1500 means timeout value
            ASSERT_GT(duration.count(), 800); // Confirm blockage 800 means timeout value
            ASSERT_EQ(ret, OHOS::GSERROR_OK);
            ASSERT_EQ(retval3.buffer, nullptr);
        }
    });

    // Create a thread to release the buffer
    std::thread releaseThread([&]() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() { return conditionMet || stopRelease; });
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (conditionMet) {
            sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
            GSError ret = bqTest->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
            ASSERT_EQ(ret, OHOS::GSERROR_OK);
            ASSERT_EQ(bqTest->freeList_.size(), 0);
            ASSERT_EQ(bqTest->dirtyList_.size(), 1);
            ret = bqTest->AcquireBuffer(retval.buffer, retval.fence, timestamp, damages);
            ASSERT_EQ(ret, OHOS::GSERROR_OK);
            ASSERT_NE(retval.buffer, nullptr);
            ASSERT_EQ(bqTest->freeList_.size(), 0);
            ASSERT_EQ(bqTest->dirtyList_.size(), 0);
            sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
            ret = bqTest->ReleaseBuffer(retval.buffer, releaseFence);
            ASSERT_EQ(ret, OHOS::GSERROR_OK);
            ASSERT_EQ(bqTest->freeList_.size(), 1);
            ASSERT_EQ(bqTest->dirtyList_.size(), 0);
        }
    });
    // Stop the threads after 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));
    stopRequest = true;
    stopRelease = true;
    cv.notify_one();

    requestThread.join();
    releaseThread.join();
}

/*
 * Function: Multiple requests for buffer in blocking mode, other threads set status wrong
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. requestThread multiple requests for buffer in blocking mode
 *                  2. releaseThread set status wrong
 *                  3. requestThread return GSERROR_NO_CONSUMER
 */
HWTEST_F(BufferQueueTest, ReqBufferWithBlockModeAndStatusWrong001, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    IBufferProducer::RequestBufferReturnValue retval1;
    IBufferProducer::RequestBufferReturnValue retval2;
    IBufferProducer::RequestBufferReturnValue retval3;

    BufferRequestConfig requestConfigTest = {
        .width = 0x100,
        .height = 0x100,
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 5000,
    };
    std::mutex mtx;
    std::condition_variable cv;
    bool conditionMet = false;
    std::atomic<bool> stopRequest(false);
    std::atomic<bool> stopRelease(false);
    sptr<BufferQueue> bqTest = new BufferQueue("test");
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    bqTest->RegisterConsumerListener(listener);
    // Create thread for requesting buffer
    std::thread requestThread([&]() {
        while (!stopRequest) {
            uint32_t i = 0;
            // 5:首次RequestBuffer概率性失败，设置重试
            while (bqTest->RequestBuffer(requestConfigTest, bedata, retval) != OHOS::GSERROR_OK && i < 5) {
                i++;
            }
            ASSERT_GE(retval.sequence, 0);
            ASSERT_NE(retval.buffer, nullptr);

            GSError ret = bqTest->RequestBuffer(requestConfigTest, bedata, retval1);
            ASSERT_EQ(ret, OHOS::GSERROR_OK);
            ASSERT_GE(retval1.sequence, 0);
            ASSERT_NE(retval1.buffer, nullptr);

            ret = bqTest->RequestBuffer(requestConfigTest, bedata, retval2);
            ASSERT_EQ(ret, OHOS::GSERROR_OK);
            ASSERT_GE(retval2.sequence, 0);
            ASSERT_NE(retval2.buffer, nullptr);

            ASSERT_EQ(bqTest->freeList_.size(), 0);
            ASSERT_EQ(bqTest->GetUsedSize(), 3U);
            ASSERT_EQ(bqTest->bufferQueueSize_, 3U);
            ASSERT_EQ(bqTest->detachReserveSlotNum_, 0);
            ASSERT_EQ(bqTest->dirtyList_.size(), 0);

            conditionMet = true;
            cv.notify_one();
            auto start = std::chrono::high_resolution_clock::now();
            ret = bqTest->RequestBuffer(requestConfigTest, bedata, retval3);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "RequestBuffer costs: " << duration.count() << "ms" << std::endl;
            ASSERT_LT(duration.count(), 1500); // Confirm blockage 1500 means timeout value
            ASSERT_GT(duration.count(), 800); // Confirm blockage 800 means timeout value
            ASSERT_EQ(ret, OHOS::GSERROR_NO_CONSUMER);
            ASSERT_EQ(retval3.buffer, nullptr);
        }
    });

    // Create a thread to set status wrong
    std::thread releaseThread([&]() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() { return conditionMet || stopRelease; });
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (conditionMet) {
            bqTest->SetStatus(false);
        }
    });
    // Stop the threads after 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));
    stopRequest = true;
    stopRelease = true;
    cv.notify_one();

    requestThread.join();
    releaseThread.join();
}

/*
 * Function: AcquireLppBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call AcquireLppBuffer
 */
HWTEST_F(BufferQueueTest, AcquireLppBuffer001, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = nullptr;
    sptr<SyncFence> acquireFence = SyncFence::InvalidFence();
    int64_t timestamp = 0;
    std::vector<Rect> damage;
    sptr<BufferQueue> tmpBq = new BufferQueue("test");
 
    tmpBq->sourceType_ = OHSurfaceSource::OH_SURFACE_SOURCE_VIDEO;
    tmpBq->lppSlotInfo_ = nullptr;
    ASSERT_EQ(tmpBq->AcquireLppBuffer(buffer, acquireFence, timestamp, damage), OHOS::GSERROR_TYPE_ERROR);
 
    tmpBq->sourceType_ = OHSurfaceSource::OH_SURFACE_SOURCE_LOWPOWERVIDEO;
    tmpBq->lppSlotInfo_ = nullptr;
    ASSERT_EQ(tmpBq->AcquireLppBuffer(buffer, acquireFence, timestamp, damage), OHOS::GSERROR_TYPE_ERROR);
 
    tmpBq->lppSlotInfo_ = new LppSlotInfo{.readOffset = -1, .writeOffset = -1,
        .slot = {{.seqId = 100, .timestamp = 1000, .crop = {1, 2, 3, 4}}},
        .frameRate = 30, .isStopShbDraw = false};
    ASSERT_EQ(tmpBq->AcquireLppBuffer(buffer, acquireFence, timestamp, damage), OHOS::GSERROR_INVALID_ARGUMENTS);

    uint32_t seqId1 = 100;
    uint32_t seqId2 = 101;
    uint32_t seqId3 = 102;
    BufferElement ele = {
        .buffer = nullptr, .state = BUFFER_STATE_ACQUIRED, .isDeleting = false,
        .config = {}, .fence = SyncFence::InvalidFence()};
    tmpBq->bufferQueueCache_[seqId1] = ele;
    tmpBq->bufferQueueCache_[seqId2] = ele;
    tmpBq->bufferQueueCache_[seqId3] = ele;
    BufferSlot* slot = new BufferSlot();
    tmpBq->lppFenceMap_[seqId1] = slot;
    tmpBq->lppFenceMap_[seqId2] = slot;
    tmpBq->lppFenceMap_[seqId3] = slot;
    tmpBq->lppSlotInfo_->readOffset = 0;
    tmpBq->lppSlotInfo_->writeOffset = 1;
    ASSERT_EQ(tmpBq->AcquireLppBuffer(buffer, acquireFence, timestamp, damage), OHOS::GSERROR_NO_BUFFER);

    tmpBq->lppFenceMap_.clear();
    tmpBq->bufferQueueCache_.clear();
    tmpBq->lppSlotInfo_->writeOffset = 0;
    tmpBq->lastLppWriteOffset_ = 0;
    tmpBq->lastLppWriteTimestamp_ = 1000;
    ASSERT_EQ(tmpBq->AcquireLppBuffer(buffer, acquireFence, timestamp, damage), OHOS::GSERROR_NO_BUFFER);

    tmpBq->lastLppWriteTimestamp_ = 0;
    ASSERT_EQ(tmpBq->AcquireLppBuffer(buffer, acquireFence, timestamp, damage), OHOS::GSERROR_NO_BUFFER);

    tmpBq->lastLppWriteTimestamp_ = 0;
    ele.buffer = SurfaceBuffer::Create();
    tmpBq->bufferQueueCache_[100] = ele;
    delete slot;
    slot = nullptr;
    delete tmpBq->lppSlotInfo_;
    tmpBq->lppSlotInfo_ = nullptr;
}
/*
 * Function: SetLppShareFd
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call SetLppShareFd
 */
HWTEST_F(BufferQueueTest, SetLppShareFd001, TestSize.Level0)
{
    sptr<BufferQueue> tmpBq = new BufferQueue("test");
    int fd = -1;
    bool state = true;
    tmpBq->sourceType_ = OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
    ASSERT_EQ(tmpBq->SetLppShareFd(fd, state), OHOS::GSERROR_TYPE_ERROR);

    tmpBq->sourceType_ = OHSurfaceSource::OH_SURFACE_SOURCE_LOWPOWERVIDEO;
    tmpBq->lppSlotInfo_ = nullptr;
    ASSERT_EQ(tmpBq->SetLppShareFd(fd, state), OHOS::GSERROR_INVALID_ARGUMENTS);

    state = false;
    ASSERT_EQ(tmpBq->SetLppShareFd(fd, state), OHOS::GSERROR_OK);
}
 
/*
 * Function: SetLppShareFd
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call SetLppShareFd
 */
HWTEST_F(BufferQueueTest, SetLppShareFd002, TestSize.Level0)
{
    sptr<BufferQueue> tmpBq = new BufferQueue("test");
    bool state = true;
    int fd = open("/dev/lpptest", O_RDWR | O_CREAT, static_cast<mode_t>(0600));
    ASSERT_NE(fd, -1);
    int64_t fileSize = static_cast<int64_t>(tmpBq->GetUniqueId() & 0xFFFFFFFF) * (0x10000);
    ASSERT_NE(ftruncate(fd, fileSize), -1);
 
    tmpBq->lppSlotInfo_ = nullptr;
    tmpBq->sourceType_ = OHSurfaceSource::OH_SURFACE_SOURCE_LOWPOWERVIDEO;
    ASSERT_EQ(tmpBq->SetLppShareFd(fd, state), OHOS::GSERROR_OK);
    state = false;
    ASSERT_EQ(tmpBq->SetLppShareFd(fd, state), OHOS::GSERROR_OK);
    close(fd);

    state = true;
    fd = open("/dev/lpptest", O_RDWR | O_CREAT, static_cast<mode_t>(0600));
    ASSERT_NE(fd, -1);
    ASSERT_NE(ftruncate(fd, fileSize), -1);
    tmpBq->lppSlotInfo_ = nullptr;
    tmpBq->sourceType_ = OHSurfaceSource::OH_SURFACE_SOURCE_LOWPOWERVIDEO;
    ASSERT_EQ(tmpBq->SetLppShareFd(fd, state), OHOS::GSERROR_OK);
    ASSERT_EQ(tmpBq->SetLppShareFd(-1, state), OHOS::GSERROR_INVALID_ARGUMENTS);
    close(fd);
}
/*
 * Function: FlushLppBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call FlushLppBuffer
 */
HWTEST_F(BufferQueueTest, FlushLppBuffer001, TestSize.Level0)
{
    sptr<BufferQueue> tmpBq = new BufferQueue("test");
    ASSERT_NE(tmpBq, nullptr);
    tmpBq->FlushLppBuffer();
    ASSERT_TRUE(tmpBq->dirtyList_.empty());
    tmpBq->sourceType_ = OHSurfaceSource::OH_SURFACE_SOURCE_LOWPOWERVIDEO;
    tmpBq->lppSlotInfo_ = new LppSlotInfo{.readOffset = 0,
        .writeOffset = 1,
        .slot = {{.seqId = 100, .timestamp = 1000, .crop = {1, 2, 3, 4}}},
        .frameRate = 30,
        .isStopShbDraw = false};
    tmpBq->isRsDrawLpp_ = false;
    BufferElement ele = {
        .buffer = SurfaceBuffer::Create(), .state = BUFFER_STATE_ACQUIRED, .isDeleting = false,
        .config = {}, .fence = SyncFence::InvalidFence()};
    tmpBq->bufferQueueCache_[100] = ele;
    tmpBq->FlushLppBuffer();
    ASSERT_TRUE(tmpBq->dirtyList_.empty());
}
/*
 * Function: SetLppDrawSource
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call SetLppDrawSource
 */
HWTEST_F(BufferQueueTest, SetLppDrawSource001, TestSize.Level0)
{
    sptr<BufferQueue> tmpBq = new BufferQueue("test");
    bool isShbDrawLpp = false;
    bool isRsDrawLpp = false;
    tmpBq->sourceType_ = OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
    ASSERT_EQ(tmpBq->SetLppDrawSource(isShbDrawLpp, isRsDrawLpp), OHOS::GSERROR_TYPE_ERROR);

    tmpBq->sourceType_ = OHSurfaceSource::OH_SURFACE_SOURCE_LOWPOWERVIDEO;
    tmpBq->lppSlotInfo_ = nullptr;
    ASSERT_EQ(tmpBq->SetLppDrawSource(isShbDrawLpp, isRsDrawLpp), OHOS::GSERROR_TYPE_ERROR);

    tmpBq->lppSlotInfo_ = new LppSlotInfo{.readOffset = 0,
        .writeOffset = 1,
        .slot = {{.seqId = 100, .timestamp = 1000, .crop = {1, 2, 3, 4}}},
        .frameRate = 30,
        .isStopShbDraw = false};
    tmpBq->lppSkipCount_ = 11;
    ASSERT_EQ(tmpBq->SetLppDrawSource(isShbDrawLpp, isRsDrawLpp), OHOS::GSERROR_OUT_OF_RANGE);

    tmpBq->lppSkipCount_ = 0;
    ASSERT_EQ(tmpBq->SetLppDrawSource(isShbDrawLpp, isRsDrawLpp), OHOS::GSERROR_OK);

    isShbDrawLpp = true;
    tmpBq->lppSlotInfo_->isStopShbDraw = true;
    ASSERT_EQ(tmpBq->SetLppDrawSource(isShbDrawLpp, isRsDrawLpp), OHOS::GSERROR_NO_CONSUMER);

    tmpBq->lppSlotInfo_->isStopShbDraw = false;
    tmpBq->lastLppWriteOffset_ = 0;
    tmpBq->lastRsToShbWriteOffset_ = 0;
    ASSERT_EQ(tmpBq->SetLppDrawSource(isShbDrawLpp, isRsDrawLpp), OHOS::GSERROR_NO_CONSUMER);

    tmpBq->lastRsToShbWriteOffset_ = -1;
    ASSERT_EQ(tmpBq->SetLppDrawSource(isShbDrawLpp, isRsDrawLpp), OHOS::GSERROR_NO_CONSUMER);

    tmpBq->lastRsToShbWriteOffset_ = 0;
    tmpBq->lastLppWriteOffset_ = 3;
    ASSERT_EQ(tmpBq->SetLppDrawSource(isShbDrawLpp, isRsDrawLpp), OHOS::GSERROR_OK);
}

/*
 * Function: SetAlphaType
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call SetAlphaType
 */
HWTEST_F(BufferQueueTest, SetAlphaTypeTest, TestSize.Level0)
{
    sptr<BufferQueue> tmpBq = new BufferQueue("test");
    GraphicAlphaType alphaType = static_cast<GraphicAlphaType>(-1);
    ASSERT_EQ(tmpBq->SetAlphaType(alphaType), OHOS::GSERROR_INVALID_ARGUMENTS);

    alphaType = static_cast<GraphicAlphaType>(0);
    ASSERT_EQ(tmpBq->SetAlphaType(alphaType), OHOS::GSERROR_OK);

    alphaType = static_cast<GraphicAlphaType>(1);
    ASSERT_EQ(tmpBq->SetAlphaType(alphaType), OHOS::GSERROR_OK);

    alphaType = static_cast<GraphicAlphaType>(2);
    ASSERT_EQ(tmpBq->SetAlphaType(alphaType), OHOS::GSERROR_OK);

    alphaType = static_cast<GraphicAlphaType>(3);
    ASSERT_EQ(tmpBq->SetAlphaType(alphaType), OHOS::GSERROR_OK);

    alphaType = static_cast<GraphicAlphaType>(4);
    ASSERT_EQ(tmpBq->SetAlphaType(alphaType), OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: GetAlphaType
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call GetAlphaType
 */
HWTEST_F(BufferQueueTest, GetAlphaTypeTest, TestSize.Level0)
{
    sptr<BufferQueue> tmpBq = new BufferQueue("test");
    GraphicAlphaType tmpAlphaType = static_cast<GraphicAlphaType>(1);
    ASSERT_EQ(tmpBq->SetAlphaType(tmpAlphaType), OHOS::GSERROR_OK);

    GraphicAlphaType alphaType;
    ASSERT_EQ(tmpBq->GetAlphaType(alphaType), OHOS::GSERROR_OK);
    ASSERT_EQ(alphaType, GraphicAlphaType::GRAPHIC_ALPHATYPE_OPAQUE);
}

/*
 * Function: SetIsPriorityAlloc
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetIsPriorityAlloc and check value
 */
HWTEST_F(BufferQueueTest, SetIsPriorityAlloc001, TestSize.Level0)
{
    BufferQueue *bqTmp = new BufferQueue("testTmp");
    ASSERT_EQ(bqTmp->SetQueueSize(1), GSERROR_OK);

    bqTmp->SetIsPriorityAlloc(true);
    ASSERT_EQ(bqTmp->isPriorityAlloc_, true);
    bqTmp->SetIsPriorityAlloc(false);
    bqTmp = nullptr;
}

/*
 * Function: SetIsPriorityAlloc
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetIsPriorityAlloc and check value
 */
HWTEST_F(BufferQueueTest, SetIsPriorityAlloc002, TestSize.Level0)
{
    sptr<BufferQueue> bqTest = new BufferQueue("test");
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    bqTest->RegisterConsumerListener(listener);

    IBufferProducer::RequestBufferReturnValue retval;
    IBufferProducer::RequestBufferReturnValue retval1;
    IBufferProducer::RequestBufferReturnValue retval2;
    IBufferProducer::RequestBufferReturnValue retval3;

    BufferRequestConfig requestConfigTest = {
        .width = 0x100,
        .height = 0x100,
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 5000,
    };
    bqTest->SetQueueSize(3);
    bqTest->SetIsPriorityAlloc(true);
    // fisrt request buffer
    GSError ret = bqTest->RequestBuffer(requestConfigTest, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval.sequence, 0);
    ASSERT_NE(retval.buffer, nullptr);
    ASSERT_EQ(bqTest->GetUsedSize(), 1);

    ret = bqTest->RequestBuffer(requestConfigTest, bedata, retval1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval1.sequence, 0);
    ASSERT_NE(retval1.buffer, nullptr);
    ASSERT_EQ(bqTest->GetUsedSize(), 2);
    // used buffer queue size is max
    ret = bqTest->RequestBuffer(requestConfigTest, bedata, retval2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval2.sequence, 0);
    ASSERT_NE(retval2.buffer, nullptr);
    ASSERT_EQ(bqTest->GetUsedSize(), 3);
    
    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bqTest->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(bqTest->GetUsedSize(), 3);

    ret = bqTest->FlushBuffer(retval1.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(bqTest->GetUsedSize(), 3);

    ret = bqTest->FlushBuffer(retval2.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(bqTest->GetUsedSize(), 3);

    ret = bqTest->AcquireBuffer(retval.buffer, retval.fence, timestamp, damages);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(retval.buffer, nullptr);

    uint8_t *addr2 = reinterpret_cast<uint8_t*>(retval.buffer->GetVirAddr());
    ASSERT_NE(addr2, nullptr);

    sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
    ret = bqTest->ReleaseBuffer(retval.buffer, releaseFence);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    // reuse
    ret = bqTest->RequestBuffer(requestConfigTest, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval.sequence, 0);
    ASSERT_EQ(retval.buffer, nullptr);
    ASSERT_EQ(bqTest->GetUsedSize(), 3);

    ret = bqTest->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(bqTest->GetUsedSize(), 3);
}

/*
 * Function: SetIsPriorityAlloc
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetIsPriorityAlloc and check value
 */
HWTEST_F(BufferQueueTest, SetIsPriorityAlloc003, TestSize.Level0)
{
    sptr<BufferQueue> bqTest = new BufferQueue("test");
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    bqTest->RegisterConsumerListener(listener);

    IBufferProducer::RequestBufferReturnValue retval;
    IBufferProducer::RequestBufferReturnValue retval1;
    IBufferProducer::RequestBufferReturnValue retval2;
    IBufferProducer::RequestBufferReturnValue retval3;

    BufferRequestConfig requestConfigTest = {
        .width = 0x100,
        .height = 0x100,
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 5000,
    };
    bqTest->SetQueueSize(3);
    bqTest->SetIsPriorityAlloc(false);
    // fisrt request buffer
    GSError ret = bqTest->RequestBuffer(requestConfigTest, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval.sequence, 0);
    ASSERT_NE(retval.buffer, nullptr);
    ASSERT_EQ(bqTest->GetUsedSize(), 1);

    ret = bqTest->RequestBuffer(requestConfigTest, bedata, retval1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval1.sequence, 0);
    ASSERT_NE(retval1.buffer, nullptr);
    ASSERT_EQ(bqTest->GetUsedSize(), 2);
    // used buffer queue size is max
    ret = bqTest->RequestBuffer(requestConfigTest, bedata, retval2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval2.sequence, 0);
    ASSERT_NE(retval2.buffer, nullptr);
    ASSERT_EQ(bqTest->GetUsedSize(), 3);
    
    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bqTest->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(bqTest->GetUsedSize(), 3);

    ret = bqTest->FlushBuffer(retval1.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(bqTest->GetUsedSize(), 3);

    ret = bqTest->FlushBuffer(retval2.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(bqTest->GetUsedSize(), 3);

    ret = bqTest->AcquireBuffer(retval.buffer, retval.fence, timestamp, damages);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(retval.buffer, nullptr);

    uint8_t *addr2 = reinterpret_cast<uint8_t*>(retval.buffer->GetVirAddr());
    ASSERT_NE(addr2, nullptr);

    sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
    ret = bqTest->ReleaseBuffer(retval.buffer, releaseFence);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    // reuse
    ret = bqTest->RequestBuffer(requestConfigTest, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval.sequence, 0);
    ASSERT_EQ(retval.buffer, nullptr);
    ASSERT_EQ(bqTest->GetUsedSize(), 3);

    ret = bqTest->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(bqTest->GetUsedSize(), 3);
}

/*
 * Function: CheckLppFenceLocked
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call CheckLppFenceLocked
 */
HWTEST_F(BufferQueueTest, CheckLppFenceLocked001, TestSize.Level0)
{
    sptr<BufferQueue> tmpBq = new BufferQueue("test");
    tmpBq->lppSlotInfo_ = nullptr;
    ASSERT_EQ(tmpBq->CheckLppFenceLocked(), false);

    tmpBq->lppSlotInfo_ = new LppSlotInfo{.readOffset = -1, .writeOffset = -1,
        .slot = {{.seqId = 100, .timestamp = 1000, .isRsUsing = true, .crop = {1, 2, 3, 4}}},
        .frameRate = 30, .isStopShbDraw = false};
    tmpBq->lppFenceMap_[100] = nullptr;
    ASSERT_EQ(tmpBq->CheckLppFenceLocked(), true);

    tmpBq->lppFenceMap_[100] =  &(tmpBq->lppSlotInfo_->slot[0]);
    BufferElement ele = {
        .buffer = SurfaceBuffer::Create(), .state = BUFFER_STATE_ACQUIRED, .isDeleting = false,
        .config = {}, .fence = SyncFence::InvalidFence()};
    tmpBq->bufferQueueCache_[100] = ele;
    ASSERT_EQ(tmpBq->CheckLppFenceLocked(), true);

    ele.state = BUFFER_STATE_FLUSHED;
    ASSERT_EQ(tmpBq->CheckLppFenceLocked(), true);
}
/*
 * Function: SetLppBufferConfig
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call SetLppBufferConfig
 */
HWTEST_F(BufferQueueTest, SetLppBufferConfig001, TestSize.Level0)
{
    sptr<BufferQueue> tmpBq = new BufferQueue("test");
    tmpBq->transform_ = GraphicTransformType::GRAPHIC_ROTATE_270;
    sptr<SurfaceBuffer> buffer = nullptr;
    std::vector<Rect> damages = {{.x = 0, .y = 0, .w = 0, .h = 0}};
    BufferSlot slot;
    tmpBq->SetLppBufferConfig(buffer, damages, slot);
    ASSERT_EQ(damages[0].w, 0);

    buffer = SurfaceBuffer::Create();
    tmpBq->SetLppBufferConfig(buffer, damages, slot);
    ASSERT_EQ(buffer->GetSurfaceBufferTransform(), GraphicTransformType::GRAPHIC_ROTATE_270);
}

/*
 * Function: RequestBuffersForListenerLocked
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call RequestBuffersForListenerLocked
 */
HWTEST_F(BufferQueueTest, RequestBuffersForListenerLocked001, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    bq->CleanCache(false, nullptr);
    bq->isOnReleaseBufferWithSequenceAndFence_ = true;
    bq->SetIsPriorityAlloc(true);
    std::vector<std::pair<uint32_t, sptr<SyncFence>>> requestBuffersAndFences;
    {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        bq->RequestBuffersForListenerLocked(requestBuffersAndFences, lock);
        ASSERT_EQ(requestBuffersAndFences.size(), 0);
        auto ret = bq->RequestBufferLocked(requestConfig, bedata, retval, lock, true);
        ASSERT_EQ(ret, OHOS::GSERROR_NO_BUFFER);
        bq->RegisterProducerReleaseListener(nullptr, true);
        ret = bq->SetProducerCacheCleanFlagLocked(true, lock);
        ASSERT_EQ(ret, OHOS::GSERROR_OK);

        ret = bq->RequestBuffer(requestConfig, bedata, retval);
        ASSERT_EQ(ret, OHOS::GSERROR_OK);

        sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
        ret = bq->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
        ASSERT_EQ(ret, OHOS::GSERROR_OK);

        ret = bq->AcquireBuffer(retval.buffer, retval.fence, timestamp, damages);
        ASSERT_EQ(ret, OHOS::GSERROR_OK);

        sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
        ret = bq->ReleaseBuffer(retval.buffer, releaseFence);
        ASSERT_EQ(ret, OHOS::GSERROR_OK);
    }
    bq->UnRegisterProducerReleaseListener();
    bq->CleanCache(false, nullptr);
}

/*
 * Function: RequestBuffersForListenerLocked
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call RequestBuffersForListenerLocked
 */
HWTEST_F(BufferQueueTest, RequestBuffersForListenerLocked002, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    bq->CleanCache(false, nullptr);

    auto ret = bq->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bq->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->AcquireBuffer(retval.buffer, retval.fence, timestamp, damages);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
    ret = bq->ReleaseBuffer(retval.buffer, releaseFence);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    bq->isOnReleaseBufferWithSequenceAndFence_ = true;
    bq->SetIsPriorityAlloc(true);
    std::vector<std::pair<uint32_t, sptr<SyncFence>>> requestBuffersAndFences;
    {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        BufferRequestConfig requestConfigTmp = {
            .width = 0x200,
            .height = 0x200,
            .strideAlignment = 0x8,
            .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
            .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
            .timeout = 0,
        };
        ret = bq->ReuseBuffer(requestConfigTmp, bedata, retval, lock, true);
        ASSERT_EQ(ret, OHOS::GSERROR_NO_BUFFER);
        bq->producerCacheClean_ = true;
        ret = bq->ReuseBuffer(requestConfig, bedata, retval, lock, true);
        ASSERT_EQ(ret, OHOS::GSERROR_NO_BUFFER);
        bq->producerCacheClean_ = false;
    }

    bq->UnRegisterProducerReleaseListener();
    bq->CleanCache(false, nullptr);
}

/*
 * Function: ReleaseBufferLocked
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call ReleaseBufferLocked
 */
HWTEST_F(BufferQueueTest, RequestBuffersForListenerLocked003, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    sptr<IRemoteObject> obj = nullptr;
    sptr<IProducerListener> listener = new ProducerListenerProxy(obj);
    bq->RegisterProducerReleaseListener(listener, true);

    std::vector<std::pair<uint32_t, sptr<SyncFence>>> requestBuffersAndFences;
    auto ret = bq->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    requestBuffersAndFences.push_back(std::make_pair(retval.sequence, retval.fence));
    requestBuffersAndFences.push_back(std::make_pair(0, nullptr));
    bq->OnReleaseBufferWithSequenceAndFence(listener, requestBuffersAndFences);
    auto iter = bq->bufferQueueCache_.find(retval.sequence);
    ASSERT_NE(iter, bq->bufferQueueCache_.end());
    ASSERT_EQ(iter->second.state, 0);
    bq->UnRegisterProducerReleaseListener();
    bq->CleanCache(false, nullptr);
}

/*
 * Function: ReleaseBufferLocked
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call ReleaseBufferLocked
 */
HWTEST_F(BufferQueueTest, RequestBuffersForListenerLocked004, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval1;
    IBufferProducer::RequestBufferReturnValue retval2;
    IBufferProducer::RequestBufferReturnValue retval3;
    sptr<BufferExtraData> bedata1;
    sptr<BufferExtraData> bedata2;
    sptr<BufferExtraData> bedata3;
    auto ret = bq->RequestBuffer(requestConfig, bedata1, retval1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ret = bq->RequestBuffer(requestConfig, bedata2, retval2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    BufferRequestConfig requestConfigTmp = {
        .width = 0x200,
        .height = 0x200,
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    ret = bq->RequestBuffer(requestConfigTmp, bedata3, retval3);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ret = bq->CancelBuffer(retval1.sequence, bedata1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ret = bq->CancelBuffer(retval2.sequence, bedata2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ret = bq->CancelBuffer(retval3.sequence, bedata3);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    bq->freeList_.push_back(0);
    std::vector<std::pair<uint32_t, sptr<SyncFence>>> requestBuffersAndFences;
    {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        bq->RequestBuffersForListenerLocked(requestBuffersAndFences, lock);
        ASSERT_EQ(requestBuffersAndFences.size(), 3);
    }
    bq->UnRegisterProducerReleaseListener();
    bq->CleanCache(false, nullptr);
}

/*
 * Function: ReleaseBufferLocked
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call ReleaseBufferLocked
 */
HWTEST_F(BufferQueueTest, ReleaseBufferLocked001, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    sptr<SyncFence> fence = nullptr;
    sptr<SurfaceBuffer> buffer = new SurfaceBufferImpl();
    auto ret = bq->ReleaseBufferLocked(buffer, fence, lock);
    ASSERT_EQ(ret, OHOS::SURFACE_ERROR_BUFFER_NOT_INCACHE);

    ret = bq->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bq->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->AcquireBuffer(retval.buffer, retval.fence, timestamp, damages);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
    ret = bq->ReleaseBuffer(retval.buffer, releaseFence);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/**
 * Function: IsCached
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call IsCached
 */
HWTEST_F(BufferQueueTest, IsCached001, TestSize.Level0)
{
    sptr<BufferQueue> tmpBq = new BufferQueue("test");
    auto buffer = SurfaceBuffer::Create();
    auto bufferSeqNum = buffer->GetSeqNum();
    EXPECT_FALSE(tmpBq->IsCached(bufferSeqNum));

    BufferElement ele = {
        .buffer = buffer,
        .state = BUFFER_STATE_ACQUIRED,
        .isDeleting = false,
        .config = {},
        .fence = SyncFence::InvalidFence()};
    tmpBq->bufferQueueCache_[bufferSeqNum] = ele;
    EXPECT_TRUE(tmpBq->IsCached(bufferSeqNum));
}

/*
 * Function: SetDropFrameLevel001
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: Test SetDropFrameLevel with valid value
 */
HWTEST_F(BufferQueueTest, SetDropFrameLevel001, TestSize.Level0)
{
    GSError ret = bq->SetDropFrameLevel(2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(bq->dropFrameLevel_, 2);
}

/*
 * Function: SetDropFrameLevel002
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: Test SetDropFrameLevel with zero (no drop)
 */
HWTEST_F(BufferQueueTest, SetDropFrameLevel002, TestSize.Level0)
{
    GSError ret = bq->SetDropFrameLevel(0);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(bq->dropFrameLevel_, 0);
}

/*
 * Function: SetDropFrameLevel003
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: Test SetDropFrameLevel with negative value (should fail)
 */
HWTEST_F(BufferQueueTest, SetDropFrameLevel003, TestSize.Level0)
{
    GSError ret = bq->SetDropFrameLevel(-1);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: DropBuffersByLevel001
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: Test DropBuffersByLevel with level=0 (no drop)
 */
HWTEST_F(BufferQueueTest, DropBuffersByLevel001, TestSize.Level0)
{
    // Clean up state from previous tests
    bq->CleanCache(false, nullptr);

    // Ensure queue size is sufficient for test
    bq->SetQueueSize(SURFACE_MAX_QUEUE_SIZE);

    IBufferProducer::RequestBufferReturnValue retval1;
    IBufferProducer::RequestBufferReturnValue retval2;
    sptr<BufferExtraData> bedata1;
    sptr<BufferExtraData> bedata2;

    // Produce 2 buffers
    auto ret = bq->RequestBuffer(requestConfig, bedata1, retval1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bq->FlushBuffer(retval1.sequence, bedata1, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->RequestBuffer(requestConfig, bedata2, retval2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ret = bq->FlushBuffer(retval2.sequence, bedata2, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    // Set drop level to 0 (no drop)
    bq->SetDropFrameLevel(0);
    ASSERT_EQ(bq->dirtyList_.size(), 2);

    // Call DropBuffersByLevel
    std::vector<BufferAndFence> dropBuffers;
    bq->DropBuffersByLevel(dropBuffers);

    // Should not drop any buffers
    ASSERT_EQ(dropBuffers.size(), 0);
    ASSERT_EQ(bq->dirtyList_.size(), 2);

    // Cleanup
    bq->CleanCache(false, nullptr);
}

/*
 * Function: DropBuffersByLevel002
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: Test DropBuffersByLevel with level=1 (keep latest 1, drop 1)
 */
HWTEST_F(BufferQueueTest, DropBuffersByLevel002, TestSize.Level0)
{
    // Clean up state from previous tests
    bq->CleanCache(false, nullptr);

    // Ensure queue size is sufficient for test
    bq->SetQueueSize(SURFACE_MAX_QUEUE_SIZE);

    IBufferProducer::RequestBufferReturnValue retval1;
    IBufferProducer::RequestBufferReturnValue retval2;
    sptr<BufferExtraData> bedata1;
    sptr<BufferExtraData> bedata2;

    // Produce 2 buffers
    auto ret = bq->RequestBuffer(requestConfig, bedata1, retval1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bq->FlushBuffer(retval1.sequence, bedata1, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->RequestBuffer(requestConfig, bedata2, retval2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ret = bq->FlushBuffer(retval2.sequence, bedata2, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ASSERT_EQ(bq->dirtyList_.size(), 2);

    // Set drop level to 1 (keep latest 1)
    bq->SetDropFrameLevel(1);

    // Call DropBuffersByLevel
    std::vector<BufferAndFence> dropBuffers;
    bq->DropBuffersByLevel(dropBuffers);

    // Should drop 1 buffer (oldest)
    ASSERT_EQ(dropBuffers.size(), 1);
    ASSERT_EQ(bq->dirtyList_.size(), 1);

    // Cleanup
    bq->ReleaseDropBuffers(dropBuffers);
    bq->CleanCache(false, nullptr);
}

/*
 * Function: DropBuffersByLevel003
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: Test DropBuffersByLevel with level >= available count (no drop)
 */
HWTEST_F(BufferQueueTest, DropBuffersByLevel003, TestSize.Level0)
{
    // Clean up state from previous tests
    bq->CleanCache(false, nullptr);

    // Ensure queue size is sufficient for test
    bq->SetQueueSize(SURFACE_MAX_QUEUE_SIZE);

    IBufferProducer::RequestBufferReturnValue retval1;
    IBufferProducer::RequestBufferReturnValue retval2;
    sptr<BufferExtraData> bedata1;
    sptr<BufferExtraData> bedata2;

    // Produce 2 buffers
    auto ret = bq->RequestBuffer(requestConfig, bedata1, retval1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bq->FlushBuffer(retval1.sequence, bedata1, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->RequestBuffer(requestConfig, bedata2, retval2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ret = bq->FlushBuffer(retval2.sequence, bedata2, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ASSERT_EQ(bq->dirtyList_.size(), 2);

    // Set drop level to 5 (greater than available count)
    bq->SetDropFrameLevel(5);

    // Call DropBuffersByLevel
    std::vector<BufferAndFence> dropBuffers;
    bq->DropBuffersByLevel(dropBuffers);

    // Should not drop any buffers
    ASSERT_EQ(dropBuffers.size(), 0);
    ASSERT_EQ(bq->dirtyList_.size(), 2);

    // Cleanup
    bq->CleanCache(false, nullptr);
}

/*
 * Function: DropBuffersByLevel004
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: Test DropBuffersByLevel with multiple buffers
 */
HWTEST_F(BufferQueueTest, DropBuffersByLevel004, TestSize.Level0)
{
    // Clean up state from previous tests
    bq->CleanCache(false, nullptr);

    // Ensure queue size is sufficient for test (need 4 buffers)
    bq->SetQueueSize(SURFACE_MAX_QUEUE_SIZE);

    IBufferProducer::RequestBufferReturnValue retval1;
    IBufferProducer::RequestBufferReturnValue retval2;
    IBufferProducer::RequestBufferReturnValue retval3;
    IBufferProducer::RequestBufferReturnValue retval4;
    sptr<BufferExtraData> bedata1;
    sptr<BufferExtraData> bedata2;
    sptr<BufferExtraData> bedata3;
    sptr<BufferExtraData> bedata4;

    // Produce 4 buffers
    auto ret = bq->RequestBuffer(requestConfig, bedata1, retval1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bq->FlushBuffer(retval1.sequence, bedata1, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->RequestBuffer(requestConfig, bedata2, retval2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ret = bq->FlushBuffer(retval2.sequence, bedata2, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->RequestBuffer(requestConfig, bedata3, retval3);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ret = bq->FlushBuffer(retval3.sequence, bedata3, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->RequestBuffer(requestConfig, bedata4, retval4);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ret = bq->FlushBuffer(retval4.sequence, bedata4, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ASSERT_EQ(bq->dirtyList_.size(), 4);

    // Set drop level to 2 (keep latest 2)
    bq->SetDropFrameLevel(2);

    // Call DropBuffersByLevel
    std::vector<BufferAndFence> dropBuffers;
    bq->DropBuffersByLevel(dropBuffers);

    // Should drop 2 oldest buffers
    ASSERT_EQ(dropBuffers.size(), 2);
    ASSERT_EQ(bq->dirtyList_.size(), 2);

    // Verify remaining buffers are the latest 2
    auto it = bq->dirtyList_.begin();
    ASSERT_EQ(*it, retval3.sequence);
    ++it;
    ASSERT_EQ(*it, retval4.sequence);

    // Cleanup
    bq->ReleaseDropBuffers(dropBuffers);
    bq->CleanCache(false, nullptr);
}
}