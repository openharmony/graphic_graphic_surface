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
#include <securec.h>
#include <gtest/gtest.h>
#include <surface.h>
#include <buffer_queue_producer.h>
#include <consumer_surface.h>
#include "buffer_consumer_listener.h"
#include "sync_fence.h"
#include "producer_surface_delegator.h"
#include "buffer_queue_consumer.h"
#include "buffer_queue.h"
#include "v1_1/buffer_handle_meta_key_type.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class ConsumerSurfaceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    static void deleteBuffer(int32_t bufferId);
    void SetUp() override;
    void TearDown() override;

    static inline BufferRequestConfig requestConfig = {
        .width = 0x100,
        .height = 0x100,
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    static inline BufferFlushConfig flushConfig = {
        .damage = {
            .w = 0x100,
            .h = 0x100,
        },
    };
    static inline BufferFlushConfigWithDamages flushConfigWithDamages = {
        .damages = {
            { .x = 0x100, .y = 0x100, .w = 0x100, .h = 0x100, },
            { .x = 0x200, .y = 0x200, .w = 0x200, .h = 0x200, },
        },
        .timestamp = 0x300,
    };
    static inline int64_t timestamp = 0;
    static inline Rect damage = {};
    static inline std::vector<Rect> damages = {};
    static inline sptr<IConsumerSurface> cs = nullptr;
    static inline sptr<Surface> ps = nullptr;
    static inline sptr<BufferQueue> bq = nullptr;
    static inline sptr<ProducerSurfaceDelegator> surfaceDelegator = nullptr;
    static inline sptr<BufferQueueConsumer> consumer_ = nullptr;
    static inline uint32_t firstSeqnum = 0;
    sptr<ConsumerSurface> surface_ = nullptr;
};

void ConsumerSurfaceTest::SetUpTestCase()
{
    cs = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    cs->RegisterConsumerListener(listener);
    auto p = cs->GetProducer();
    bq = new BufferQueue("test");
    ps = Surface::CreateSurfaceAsProducer(p);
    surfaceDelegator = ProducerSurfaceDelegator::Create();
}

void ConsumerSurfaceTest::TearDownTestCase()
{
    cs = nullptr;
}

void ConsumerSurfaceTest::deleteBuffer(int32_t bufferId)
{
}

void ConsumerSurfaceTest::SetUp()
{
    surface_ = new ConsumerSurface("test");
    ASSERT_NE(surface_, nullptr);
    ASSERT_EQ(surface_->producer_, nullptr);
    ASSERT_EQ(surface_->consumer_, nullptr);
}

void ConsumerSurfaceTest::TearDown()
{
    surface_ = nullptr;
}

class TestConsumerListenerClazz : public IBufferConsumerListenerClazz {
public:
    void OnBufferAvailable() override
    {
    }
};

/*
 * Function: GetProducer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. get ConsumerSurface and GetProducer
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, GetProducer001, Function | MediumTest | Level2)
{
    ASSERT_NE(cs, nullptr);
    sptr<ConsumerSurface> qs = static_cast<ConsumerSurface*>(cs.GetRefPtr());
    ASSERT_NE(qs->GetProducer(), nullptr);
}

/*
 * Function: ConsumerSurface ctor
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new ConsumerSurface, only one nullptr for consumer_ and producer_
 *                  2. dtor and check ret
 */
HWTEST_F(ConsumerSurfaceTest, ConsumerSurfaceCtor001, Function | MediumTest | Level2)
{
    sptr<BufferQueue> queue = new BufferQueue("test");
    surface_->producer_ = new BufferQueueProducer(queue);

    ASSERT_NE(surface_->producer_, nullptr);
    ASSERT_EQ(surface_->consumer_, nullptr);
}

/*
 * Function: ConsumerSurface ctor
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new ConsumerSurface, only one nullptr for consumer_ and producer_
 *                  2. dtor and check ret
 */
HWTEST_F(ConsumerSurfaceTest, ConsumerSurfaceCtor002, Function | MediumTest | Level2)
{
    sptr<BufferQueue> queue = new BufferQueue("test");
    surface_->consumer_ = new BufferQueueConsumer(queue);
    ASSERT_NE(surface_->consumer_, nullptr);
}

/*
 * Function: SetDefaultWidthAndHeight, GetDefaultWidth and GetDefaultHeight
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetDefaultWidthAndHeight with consumer_ is nullptr
 *                  2. call GetDefaultWidth with producer_ is nullptr
 *                  3. call GetDefaultHeight with producer_ is nullptr
 *                  4. check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetDefaultWidthAndHeight001, Function | MediumTest | Level2)
{
    ASSERT_NE(surface_, nullptr);
    int32_t width = 0;
    int32_t height = 0;
    GSError ret = surface_->SetDefaultWidthAndHeight(width, height);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);

    width = surface_->GetDefaultWidth();
    ASSERT_EQ(width, -1);

    height = surface_->GetDefaultHeight();
    ASSERT_EQ(height, -1);
}

/*
 * Function: SetDefaultWidthAndHeight, GetDefaultWidth and GetDefaultHeight
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetDefaultWidthAndHeight with noraml value
 *                  2. call GetDefaultWidth
 *                  3. call GetDefaultHeight
 *                  4. check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetDefaultWidthAndHeight002, Function | MediumTest | Level2)
{
    ASSERT_NE(cs, nullptr);
    int32_t width = 100;  // 100 test value for width
    int32_t height = 100;  // 100 test value for height
    GSError ret = cs->SetDefaultWidthAndHeight(width, height);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    int32_t value = cs->GetDefaultWidth();
    ASSERT_EQ(value, width);

    value = cs->GetDefaultHeight();
    ASSERT_EQ(value, height);
}

/*
* Function: SetDefaultUsage and GetDefaultUsage
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetDefaultUsage with consumer_ is nullptr
 *                  2. call GetDefaultUsage with producer_ is nullptr
 *                  3. check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetDefaultUsage001, Function | MediumTest | Level2)
{
    ASSERT_NE(surface_, nullptr);
    GSError ret = surface_->SetDefaultUsage(0);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);

    uint64_t value = surface_->GetDefaultUsage();
    ASSERT_EQ(value, 0);
}

/*
 * Function: SetDefaultUsage and GetDefaultUsage
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call GetDefaultUsage with normal
 *                  2. call SetDefaultUsage with normal
 *                  3. call GetDefaultUsage agagin
 *                  4. check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetDefaultUsage002, Function | MediumTest | Level2)
{
    ASSERT_NE(cs, nullptr);
    int32_t usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA;
    uint32_t value = cs->GetDefaultUsage();
    ASSERT_EQ(value, 0);

    GSError ret = cs->SetDefaultUsage(usage);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    value = cs->GetDefaultUsage();
    ASSERT_EQ(value, usage);
}

/*
 * Function: SetQueueSize and GetQueueSize
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call GetQueueSize and get default value
 *                  2. call SetQueueSize
 *                  3. call SetQueueSize again with abnormal value
 *                  4. call GetQueueSize for BufferQueueProducer and BufferQueue
 *                  5. check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetQueueSize001, Function | MediumTest | Level2)
{
    ASSERT_NE(cs, nullptr);
    ASSERT_EQ(cs->GetQueueSize(), static_cast<uint32_t>(SURFACE_DEFAULT_QUEUE_SIZE));

    GSError ret = cs->SetQueueSize(2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(cs->GetQueueSize(), 2u);

    ret = cs->SetQueueSize(SURFACE_MAX_QUEUE_SIZE + 1);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(cs->GetQueueSize(), 2u);
}

/*
 * Function: SetQueueSize and GetQueueSize
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call GetQueueSize
 *                  2. call SetQueueSize 2 times
 *                  3. check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetQueueSize002, Function | MediumTest | Level2)
{
    sptr<ConsumerSurface> qs = static_cast<ConsumerSurface*>(cs.GetRefPtr());
    sptr<BufferQueueProducer> bqp = static_cast<BufferQueueProducer*>(qs->GetProducer().GetRefPtr());
    ASSERT_EQ(bqp->GetQueueSize(), 2u);

    GSError ret = cs->SetQueueSize(1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = cs->SetQueueSize(2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
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
HWTEST_F(ConsumerSurfaceTest, SetAndGetQueueSize003, Function | MediumTest | Level2)
{
    ASSERT_NE(surface_, nullptr);
    uint32_t size = surface_->GetQueueSize();
    ASSERT_EQ(size, 0);

    GSError ret = surface_->SetQueueSize(1);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: AcquireBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call AcquireBuffer with consumer_ is nullptr
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, AcquireBuffer001, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    sptr<OHOS::SyncFence> fence;
    GSError ret = surface_->AcquireBuffer(buffer, fence, timestamp, damage);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: AcquireBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call AcquireBuffer with nullptr params
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, AcquireBuffer002, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = nullptr;
    sptr<OHOS::SyncFence> fence;
    GSError ret = surface_->AcquireBuffer(buffer, fence, timestamp, damage);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);

    buffer = SurfaceBuffer::Create();
    fence = nullptr;
    ret = surface_->AcquireBuffer(buffer, fence, timestamp, damage);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: ReleaseBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call ReleaseBuffer with consumer_ is nullptr
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, ReleaseBuffer001, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    sptr<OHOS::SyncFence> fence;
    GSError ret = surface_->ReleaseBuffer(buffer, fence);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: ReleaseBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call ReleaseBuffer with nullptr params
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, ReleaseBuffer002, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = nullptr;
    sptr<OHOS::SyncFence> fence;
    GSError ret = surface_->ReleaseBuffer(buffer, fence);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: DetachBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call DetachBuffer with consumer_ is nullptr
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, DetachBuffer001, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer;
    GSError ret = surface_->DetachBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: DetachBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call DetachBuffer with nullptr params
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, DetachBuffer002, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = nullptr;
    GSError ret = surface_->DetachBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: RequestBuffer and FlushBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RequestBuffer by cs and ps
 *                  2. call FlushBuffer both
 *                  3. check ret
 */
HWTEST_F(ConsumerSurfaceTest, RequestAndFlush001, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer;
    int releaseFence = -1;
    GSError ret = ps->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);

    firstSeqnum = buffer->GetSeqNum();
    ret = ps->FlushBuffer(buffer, -1, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = ps->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ret = ps->FlushBuffer(buffer, -1, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: AcquireBuffer and ReleaseBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call AcquireBuffer
 *                  2. call ReleaseBuffer
 *                  3. check ret
 */
HWTEST_F(ConsumerSurfaceTest, AcquireAndRelease001, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer;
    int32_t flushFence;
    GSError ret = cs->AcquireBuffer(buffer, flushFence, timestamp, damage);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);

    ret = cs->ReleaseBuffer(buffer, -1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
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
HWTEST_F(ConsumerSurfaceTest, AcquireAndRelease002, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer;
    int32_t flushFence;
    GSError ret = cs->AcquireBuffer(buffer, flushFence, timestamp, damage);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);

    ret = cs->ReleaseBuffer(buffer, -1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = cs->ReleaseBuffer(buffer, -1);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
}
}
