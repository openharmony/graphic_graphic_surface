/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include <surface.h>
#include <buffer_queue_consumer.h>
#include "buffer_consumer_listener.h"
#include "buffer_extra_data_impl.h"
#include "sync_fence.h"
#include "consumer_surface.h"
#include "producer_surface_delegator.h"
#include "buffer_client_producer.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class BufferQueueConsumerTest : public testing::Test {
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
    static inline sptr<BufferQueueConsumer> bqc = nullptr;
    static inline sptr<BufferQueueConsumer> byt = nullptr;
    static inline sptr<BufferExtraData> bedata = nullptr;
    static inline sptr<ProducerSurfaceDelegator> surfaceDelegator = nullptr;
    static inline sptr<IConsumerSurface> csurface = nullptr;
    static inline sptr<IBufferProducer> bufferProducer = nullptr;
    static inline sptr<Surface> surface = nullptr;
};

void BufferQueueConsumerTest::SetUpTestCase()
{
    bq = new BufferQueue("test");
    bqc = new BufferQueueConsumer(bq);
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    bqc->RegisterConsumerListener(listener);
    bedata = new BufferExtraDataImpl;
    csurface = IConsumerSurface::Create();
    surfaceDelegator = ProducerSurfaceDelegator::Create();

    bufferProducer = csurface->GetProducer();
    surface = Surface::CreateSurfaceAsProducer(bufferProducer);
}

void BufferQueueConsumerTest::TearDownTestCase()
{
    bq = nullptr;
    bqc = nullptr;
}

/*
 * Function: AcquireBuffer and ReleaseBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RequestBuffer and FlushBuffer
 *                  2. call AcquireBuffer and ReleaseBuffer
 *                  3. check ret
 */
HWTEST_F(BufferQueueConsumerTest, AcqRel001, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    GSError ret = bq->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval.sequence, 0);
    ASSERT_NE(retval.buffer, nullptr);

    uint8_t *addr1 = reinterpret_cast<uint8_t*>(retval.buffer->GetVirAddr());
    ASSERT_NE(addr1, nullptr);

    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bq->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bqc->AcquireBuffer(retval.buffer, retval.fence, timestamp, damages);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
    ret = bqc->ReleaseBuffer(retval.buffer, releaseFence);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: AcquireBuffer and ReleaseBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RequestBuffer and FlushBuffer
 * 2. call AcquireBuffer and ReleaseBuffer
 * 3. call ReleaseBuffer again
 * 4. check ret
 */
HWTEST_F(BufferQueueConsumerTest, AcqRel002, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    GSError ret = bq->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_GE(retval.sequence, 0);
    ASSERT_EQ(retval.buffer, nullptr);

    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bq->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SurfaceBuffer>& buffer = retval.buffer;
    ret = bqc->AcquireBuffer(buffer, retval.fence, timestamp, damages);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
    ret = bqc->ReleaseBuffer(buffer, releaseFence);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bqc->ReleaseBuffer(buffer, releaseFence);
    ASSERT_NE(ret, OHOS::GSERROR_OK);

    int32_t timeOut = 1;
    ret = bqc->AttachBuffer(buffer, timeOut);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: GetLastConsumeTime
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call GetLastConsumeTime
 *                  2. check lastConsumeTime
 */
HWTEST_F(BufferQueueConsumerTest, GetLastConsumeTimeTest, TestSize.Level0)
{
    int64_t lastConsumeTime = 0;
    bqc->GetLastConsumeTime(lastConsumeTime);
    std::cout << "lastConsumeTime = " << lastConsumeTime << std::endl;
    ASSERT_NE(lastConsumeTime, 0);
}

/*
 * Function: AttachBuffer001
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. check bufferQueue_
 *    2. call AttachBuffer
 */
HWTEST_F(BufferQueueConsumerTest, AttachBuffer001, TestSize.Level0)
{
    int32_t timeOut = 0;
    IBufferProducer::RequestBufferReturnValue retval;
    sptr<SurfaceBuffer> &buffer = retval.buffer;

    sptr<BufferQueue> bufferqueue = nullptr;
    auto bqcTmp = new BufferQueueConsumer(bufferqueue);

    GSError ret = bqcTmp->AttachBuffer(buffer, timeOut);
    ASSERT_EQ(ret, GSERROR_INVALID_ARGUMENTS);
}
/* Function: RegisterSurfaceDelegator
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. check ret
 *    2. call RegisterSurfaceDelegator
 */
HWTEST_F(BufferQueueConsumerTest, RegisterSurfaceDelegator001, TestSize.Level0)
{
    sptr<BufferQueue> bufferqueue = nullptr;
    auto bqcTmp = new BufferQueueConsumer(bufferqueue);

    sptr<ProducerSurfaceDelegator> surfaceDelegator = ProducerSurfaceDelegator::Create();
    GSError ret = bqcTmp->RegisterSurfaceDelegator(surfaceDelegator->AsObject(), surface);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
}

HWTEST_F(BufferQueueConsumerTest, RegisterSurfaceDelegator002, TestSize.Level0)
{
    sptr<ProducerSurfaceDelegator> surfaceDelegator = ProducerSurfaceDelegator::Create();
    GSError ret = bqc->RegisterSurfaceDelegator(surfaceDelegator->AsObject(), surface);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

HWTEST_F(BufferQueueConsumerTest, RegisterSurfaceDelegator003, TestSize.Level0)
{
    sptr<ProducerSurfaceDelegator> surfaceDelegator = ProducerSurfaceDelegator::Create();
    GSError ret = bqc->RegisterSurfaceDelegator(surfaceDelegator->AsObject(), nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

HWTEST_F(BufferQueueConsumerTest, AddBranchCoverage001, TestSize.Level0)
{
    sptr<BufferQueue> bufferQueue = nullptr;
    sptr<BufferQueueConsumer> consumer = new BufferQueueConsumer(bufferQueue);

    sptr<SurfaceBuffer> buffer = nullptr;
    sptr<SyncFence> fence = nullptr;
    int64_t timestamp = 0;
    std::vector<Rect> damages;
    sptr<IRemoteObject> client = nullptr;
    sptr<Surface> cSurface = nullptr;
    sptr<IBufferConsumerListener> listener = nullptr;
    IBufferConsumerListenerClazz *listenerClass = nullptr;
    OnReleaseFunc func;
    OnDeleteBufferFunc deleteFunc;
    bool isForUniRedraw = false;
    std::string result;
    ScalingMode scalingMode;
    HDRMetaDataType type;
    std::vector<GraphicHDRMetaData> metaData;
    GraphicHDRMetadataKey key;
    std::vector<uint8_t> metaData1;
    GraphicPresentTimestamp timestamp1;
    bool isInCache = false;
    int32_t alpha = -1;
    int32_t frameGravity = -1;
    int32_t fixedRotation = -1;
    int64_t lastFlushedDesiredPresentTimeStamp = -1;
    bool bufferSupportFastCompose = false;
    int64_t frontDesiredPresentTimeStamp = -1;
    bool isAutoTimeStamp = false;
    GraphicAlphaType alphaType = GraphicAlphaType::GRAPHIC_ALPHATYPE_UNKNOWN;
    ASSERT_EQ(consumer->AcquireBuffer(buffer, fence, timestamp, damages), OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(consumer->ReleaseBuffer(buffer, fence), OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(consumer->AttachBufferToQueue(buffer), OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(consumer->DetachBufferFromQueue(buffer, false), OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(consumer->AttachBuffer(buffer), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(consumer->DetachBuffer(buffer), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(consumer->RegisterSurfaceDelegator(client, cSurface), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(consumer->QueryIfBufferAvailable(), false);
    ASSERT_EQ(consumer->RegisterConsumerListener(listener), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(consumer->RegisterConsumerListener(listenerClass), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(consumer->RegisterReleaseListener(func), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(consumer->RegisterDeleteBufferListener(deleteFunc, isForUniRedraw), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(consumer->UnregisterConsumerListener(), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(consumer->SetDefaultWidthAndHeight(0, 0), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(consumer->SetDefaultUsage(0), OHOS::GSERROR_INVALID_ARGUMENTS);
    consumer->Dump(result);
    consumer->DumpCurrentFrameLayer();
    ASSERT_EQ(consumer->GetTransform(), GraphicTransformType::GRAPHIC_ROTATE_BUTT);
    ASSERT_EQ(consumer->GetScalingMode(0, scalingMode), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(consumer->QueryMetaDataType(0, type), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(consumer->GetMetaData(0, metaData), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(consumer->GetMetaDataSet(0, key, metaData1), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(consumer->GetTunnelHandle(), nullptr);
    ASSERT_EQ(consumer->SetPresentTimestamp(0, timestamp1), OHOS::GSERROR_INVALID_ARGUMENTS);
    consumer->SetBufferHold(0);
    ASSERT_EQ(consumer->OnConsumerDied(), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(consumer->GoBackground(), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(consumer->GetHdrWhitePointBrightness(), OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(consumer->GetSdrWhitePointBrightness(), OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(consumer->IsSurfaceBufferInCache(0, isInCache), OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(consumer->GetGlobalAlpha(alpha), OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(consumer->GetFrameGravity(frameGravity), OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(consumer->GetFixedRotation(fixedRotation), OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(consumer->GetLastFlushedDesiredPresentTimeStamp(
        lastFlushedDesiredPresentTimeStamp), OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(consumer->GetBufferSupportFastCompose(bufferSupportFastCompose), OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(consumer->SetMaxQueueSize(1), OHOS::SURFACE_ERROR_UNKOWN);
    uint32_t maxQueueSize;
    ASSERT_EQ(consumer->GetMaxQueueSize(maxQueueSize), OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(consumer->GetFrontDesiredPresentTimeStamp(
        frontDesiredPresentTimeStamp, isAutoTimeStamp), OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(consumer->SetDropBufferMode(true), OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(consumer->GetAlphaType(alphaType), OHOS::SURFACE_ERROR_UNKOWN);
}

/*
 * Function: AcquireBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call AcquireBuffer and check ret
 */
HWTEST_F(BufferQueueConsumerTest, AcquireBufferByReturnValue, TestSize.Level0)
{
    sptr<ProducerSurfaceDelegator> surfaceDelegator = ProducerSurfaceDelegator::Create();
    GSError ret = bqc->RegisterSurfaceDelegator(surfaceDelegator->AsObject(), nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    bqc->bufferQueue_ = nullptr;
    IConsumerSurface::AcquireBufferReturnValue returnValue;
    ret = bqc->AcquireBuffer(returnValue);
    ASSERT_EQ(ret, OHOS::SURFACE_ERROR_UNKOWN);
}

/*
 * Function: ReleaseBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call ReleaseBuffer and check ret
 */
HWTEST_F(BufferQueueConsumerTest, ReleaseBufferBySeq, TestSize.Level0)
{
    bqc->bufferQueue_ = nullptr;
    uint32_t sequence = 0;
    sptr<SyncFence> fence = nullptr;
    GSError ret = bqc->ReleaseBuffer(sequence, fence);
    ASSERT_EQ(ret, OHOS::SURFACE_ERROR_UNKOWN);
}

/*
 * Function: SetIsActiveGame
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetIsActiveGame and check ret
 */
HWTEST_F(BufferQueueConsumerTest, SetIsActiveGame001, TestSize.Level0)
{
    bqc->bufferQueue_ = nullptr;
    bqc->SetIsActiveGame(false);
    // bqc->bufferQueue_ is not nullptr
    bqc->bufferQueue_ = new BufferQueue("test");
    bqc->SetIsActiveGame(false);
    bqc->bufferQueue_ = nullptr;
}

/*
 * Function: SetLppDrawSource
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: set lpp draw source
 */
HWTEST_F(BufferQueueConsumerTest, SetLppDrawSource001, TestSize.Level0)
{
    bool isShbDrawLpp = false;
    bool isRsDrawLpp = false;
    sptr<BufferQueue> bufferQueue = nullptr;
    sptr<BufferQueueConsumer> consumer = new BufferQueueConsumer(bufferQueue);
    ASSERT_EQ(consumer->SetLppDrawSource(isShbDrawLpp, isRsDrawLpp), OHOS::SURFACE_ERROR_UNKOWN);

    consumer->bufferQueue_ = new BufferQueue("test");
    ASSERT_EQ(consumer->SetLppDrawSource(isShbDrawLpp, isRsDrawLpp), OHOS::GSERROR_TYPE_ERROR);
}

/*
 * Function: GetAlphaType
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: get alpha type
 */
HWTEST_F(BufferQueueConsumerTest, GetAlphaTypeTest, TestSize.Level0)
{
    sptr<BufferQueue> bufferQueue = new BufferQueue("test");
    sptr<BufferQueueConsumer> consumer = new BufferQueueConsumer(bufferQueue);
    GraphicAlphaType alphaType;
    ASSERT_EQ(consumer->GetAlphaType(alphaType), OHOS::GSERROR_OK);
}

/*
 * Function: AcquireBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: acquire lpp buffer
 */
HWTEST_F(BufferQueueConsumerTest, AcqRel003, TestSize.Level0)
{
    sptr<BufferQueue> bufferQueue = new BufferQueue("test");
    sptr<BufferQueueConsumer> consumer = new BufferQueueConsumer(bufferQueue);
    sptr<SurfaceBuffer> buffer = nullptr;
    sptr<SyncFence> acquireFence = SyncFence::InvalidFence();
    int64_t timestamp = 0;
    std::vector<Rect> damages;
    ASSERT_EQ(consumer->AcquireBuffer(buffer, acquireFence, timestamp, damages, true), OHOS::GSERROR_TYPE_ERROR);
}

/*
 * Function: SetIsPriorityAlloc
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetIsPriorityAlloc and check ret
 */
HWTEST_F(BufferQueueConsumerTest, SetIsPriorityAlloc001, TestSize.Level0)
{
    bqc->bufferQueue_ = nullptr;
    // bqc->bufferQueue_ is not nullptr
    bqc->bufferQueue_ = new BufferQueue("test");
    bqc->SetIsPriorityAlloc(true);
    ASSERT_EQ(bqc->bufferQueue_->isPriorityAlloc_, true);
    bqc->SetIsPriorityAlloc(false);
    ASSERT_EQ(bqc->bufferQueue_->isPriorityAlloc_, false);
    bqc->bufferQueue_ = nullptr;
}
}
