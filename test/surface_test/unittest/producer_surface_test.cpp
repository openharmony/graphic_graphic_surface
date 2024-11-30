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
#include <securec.h>
#include <sys/time.h>

#include "buffer_consumer_listener.h"
#include "consumer_surface.h"
#include "metadata_helper.h"
#include "native_window.h"
#include "producer_surface.h"
#include "producer_surface_delegator.h"
#include "surface.h"
#include "surface_buffer_impl.h"
#include "sync_fence.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::HDI::Display::Graphic::Common::V1_0;

namespace OHOS::Rosen {
class ProducerSurfaceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
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
    static inline int64_t timestamp = 0;
    static inline Rect damage = {};
    static inline sptr<IConsumerSurface> csurf = nullptr;
    static inline sptr<IBufferProducer> producer = nullptr;
    static inline sptr<Surface> pSurface = nullptr;
    static inline sptr<ProducerSurfaceDelegator> surfaceDelegator = nullptr;
    static inline uint32_t firstSeqnum = 0;

    static inline GSError OnBufferRelease(sptr<SurfaceBuffer> &buffer)
    {
        return GSERROR_OK;
    }
    sptr<ProducerSurface> surface_ = nullptr;
    sptr<ProducerSurface> surfaceMd_ = nullptr;
};

void ProducerSurfaceTest::SetUpTestCase()
{
    csurf = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    csurf->RegisterConsumerListener(listener);
    producer = csurf->GetProducer();
    pSurface = Surface::CreateSurfaceAsProducer(producer);
    pSurface->RegisterReleaseListener(OnBufferRelease);
}

void ProducerSurfaceTest::TearDownTestCase()
{
    pSurface->UnRegisterReleaseListener();
    csurf = nullptr;
    producer = nullptr;
    pSurface = nullptr;
}

void ProducerSurfaceTest::SetUp()
{
    surface_ = new ProducerSurface(producer);
    ASSERT_NE(surface_, nullptr);
    surface_->producer_ = nullptr;
    
    surfaceMd_ = new ProducerSurface(producer);
    ASSERT_NE(surfaceMd_, nullptr);
    surfaceMd_->producer_ = nullptr;
}

void ProducerSurfaceTest::TearDown()
{
    surface_ = nullptr;
}

/*
* Function: ProducerSurface
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. check pSurface
 */
HWTEST_F(ProducerSurfaceTest, ProducerSurfaceCheck, Function | MediumTest | Level2)
{
    ASSERT_NE(pSurface, nullptr);
}

/*
* Function: GetProducerInitInfo
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. check pSurface
 */
HWTEST_F(ProducerSurfaceTest, GetProducerInitInfoTest, Function | MediumTest | Level2)
{
    ProducerInitInfo info;
    GSError ret = surface_->GetProducerInitInfo(info);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: RequestBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer with nullptr params
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, RequestBufferTest001, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = nullptr;
    sptr<SyncFence> releaseFence = nullptr;
    GSError ret = surface_->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: RequestBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer with nullptr params
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, RequestBufferTest002, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = nullptr;
    int releaseFence = -1;
    GSError ret = surface_->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: RequestBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer with nullptr params
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, RequestBufferTest003, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    sptr<SyncFence> releaseFence = nullptr;
    GSError ret = surface_->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: RequestBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer with producer_ is nullptr
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, RequestBufferTest004, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    int releaseFence = -1;
    GSError ret = surface_->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: RequestBuffers
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffers with producer_ is nullptr
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, RequestBuffersTest, Function | MediumTest | Level2)
{
    std::vector<sptr<SurfaceBuffer>> sfbuffers;
    std::vector<sptr<SyncFence>> releaseFences;
    GSError ret = surface_->RequestBuffers(sfbuffers, releaseFences, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: FlushBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call FlushBuffer with nullptr params
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, FlushBufferTest001, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = nullptr;
    GSError ret = surface_->FlushBuffer(buffer, SyncFence::INVALID_FENCE, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: FlushBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call FlushBuffer with producer_ is nullptr
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, FlushBufferTest002, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    GSError ret = surface_->FlushBuffer(buffer, SyncFence::INVALID_FENCE, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: FlushBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call FlushBuffer with nullptr params
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, FlushBufferTest003, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    GSError ret = surface_->FlushBuffer(buffer, nullptr, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: FlushBuffers
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call FlushBuffers with producer_ is nullptr
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, FlushBuffersTest, Function | MediumTest | Level2)
{
    std::vector<sptr<SurfaceBuffer>> buffers;
    buffers.push_back(SurfaceBuffer::Create());
    std::vector<sptr<SyncFence>> flushFences;
    std::vector<BufferFlushConfigWithDamages> configs;
    GSError ret = surface_->FlushBuffers(buffers, flushFences, configs);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: GetLastFlushedBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetLastFlushedBuffer with producer_ is nullptr
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, GetLastFlushedBufferTest, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    sptr<SyncFence> fence;
    float matrix[16];
    GSError ret = surface_->GetLastFlushedBuffer(buffer, fence, matrix, false);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: CancelBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call CancelBuffer with producer_ is nullptr
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, CancelBufferTest, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    GSError ret = surface_->CancelBuffer(buffer);
    ASSERT_EQ(ret, OHOS::SURFACE_ERROR_UNKOWN);
}

/*
* Function: AttachBufferToQueue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call AttachBufferToQueue with producer_ is nullptr
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, AttachBufferToQueueTest, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    GSError ret = surface_->AttachBufferToQueue(buffer);
    ASSERT_EQ(ret, OHOS::SURFACE_ERROR_UNKOWN);
}

/*
* Function: DetachBufferFromQueue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call DetachBufferFromQueue with producer_ is nullptr
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, DetachBufferFromQueueTest, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    GSError ret = surface_->DetachBufferFromQueue(buffer);
    ASSERT_EQ(ret, OHOS::SURFACE_ERROR_UNKOWN);
}

/*
* Function: DetachBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call DetachBuffer with producer_ is nullptr
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, DetachBufferTest, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    GSError ret = surface_->DetachBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: SetQueueSize and GetQueueSize
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetQueueSize and get default value
*                  2. call SetQueueSize
*                  3. call SetQueueSize again with abnormal value
*                  4. call GetQueueSize
*                  5. check ret
 */
HWTEST_F(ProducerSurfaceTest, GetAndSetQueueSizeTest001, Function | MediumTest | Level2)
{
    ASSERT_EQ(pSurface->GetQueueSize(), (uint32_t)SURFACE_DEFAULT_QUEUE_SIZE);
    GSError ret = pSurface->SetQueueSize(2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->SetQueueSize(SURFACE_MAX_QUEUE_SIZE + 1);
    ASSERT_NE(ret, OHOS::GSERROR_OK);

    ASSERT_EQ(pSurface->GetQueueSize(), 2u);
}

/*
* Function: SetQueueSize and GetQueueSize
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetQueueSize with producer_ is nullptr and check ret
*                  2. call SetQueueSize with producer_ is nullptr and check ret
 */
HWTEST_F(ProducerSurfaceTest, GetAndSetQueueSizeTest002, Function | MediumTest | Level2)
{
    uint32_t queueSize = surface_->GetQueueSize();
    ASSERT_EQ(queueSize, 0);
    GSError ret = surface_->SetQueueSize(queueSize);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: GetQueueSize and SetQueueSize
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetQueueSize
*                  2. call SetQueueSize 2 times
*                  3. check ret
 */
HWTEST_F(ProducerSurfaceTest, GetAndSetQueueSizeTest003, Function | MediumTest | Level2)
{
    sptr<ConsumerSurface> cs = static_cast<ConsumerSurface*>(csurf.GetRefPtr());
    sptr<BufferQueueProducer> bqp = static_cast<BufferQueueProducer*>(cs->GetProducer().GetRefPtr());
    ASSERT_EQ(bqp->GetQueueSize(), 2u);

    GSError ret = pSurface->SetQueueSize(1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->SetQueueSize(2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: GetDefaultWidth, GetDefaultHeight and SetDefaultWidthAndHeight
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetDefaultWidthAndHeight with producer_ is nullptr and check ret
*                  2. call GetDefaultWidth with producer_ is nullptr and check ret
*                  3. call GetDefaultHeight with producer_ is nullptr and check ret
 */
HWTEST_F(ProducerSurfaceTest, DefaultWidthAndHeightTest, Function | MediumTest | Level2)
{
    GSError ret = surface_->SetDefaultWidthAndHeight(0, 0);
    ASSERT_EQ(ret, OHOS::GSERROR_NOT_SUPPORT);
    int32_t width = surface_->GetDefaultWidth();
    ASSERT_EQ(width, -1);  // -1 is default width
    int32_t height = surface_->GetDefaultHeight();
    ASSERT_EQ(height, -1);  // -1 is default height
}

/*
* Function: SetTransformHint and GetTransformHint
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetTransformHint with producer_ is nullptr and check ret
*                  2. call GetTransformHint with producer_ is nullptr and check ret
 */
HWTEST_F(ProducerSurfaceTest, GetAndSetTransformHintTest, Function | MediumTest | Level2)
{
    GSError ret = surface_->SetTransformHint(GraphicTransformType::GRAPHIC_ROTATE_NONE);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    GraphicTransformType type = surface_->GetTransformHint();
    ASSERT_EQ(type, GraphicTransformType::GRAPHIC_ROTATE_NONE);
}

/*
* Function: SetDefaultUsage and GetDefaultUsage
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetDefaultUsage with producer_ is nullptr and check ret
*                  2. call GetDefaultUsage with producer_ is nullptr and check ret
 */
HWTEST_F(ProducerSurfaceTest, SetAndGetDefaultUsageTest, Function | MediumTest | Level2)
{
    GSError ret = surface_->SetDefaultUsage(0);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    uint64_t usage = surface_->GetDefaultUsage();
    ASSERT_EQ(usage, 0);
}

/*
* Function: RequestBuffer and FlushBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer
*                  2. call FlushBuffer
*                  3. check ret
 */
HWTEST_F(ProducerSurfaceTest, RequestAndFlushBufferTest001, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer;

    int releaseFence = -1;
    GSError ret = pSurface->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);
    firstSeqnum = buffer->GetSeqNum();

    ret = pSurface->FlushBuffer(buffer, -1, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
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
HWTEST_F(ProducerSurfaceTest, RequestAndFlushBufferTest002, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer;
    int releaseFence = -1;

    GSError ret = pSurface->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);

    ret = pSurface->FlushBuffer(buffer, -1, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->FlushBuffer(buffer, -1, flushConfig);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
}

/*
* Function: AcquireBuffer and ReleaseBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call AcquireBuffer and ReleaseBuffer many times
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, AcquireAndReleaseBufferTest001, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer;
    int32_t flushFence;

    GSError ret = csurf->AcquireBuffer(buffer, flushFence, timestamp, damage);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = csurf->ReleaseBuffer(buffer, -1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = csurf->AcquireBuffer(buffer, flushFence, timestamp, damage);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = csurf->ReleaseBuffer(buffer, -1);
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
HWTEST_F(ProducerSurfaceTest, RequestAndCancelBufferTest001, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer;

    int releaseFence = -1;
    GSError ret = pSurface->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->CancelBuffer(buffer);
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
HWTEST_F(ProducerSurfaceTest, RequestAndCancelBufferTest002, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer;

    int releaseFence = -1;
    GSError ret = pSurface->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->CancelBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->CancelBuffer(buffer);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer and CancelBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer and CancelBuffer many times
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, RequestAndCancelBufferTest003, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer;
    sptr<SurfaceBuffer> buffer1;
    sptr<SurfaceBuffer> buffer2;
    int releaseFence = -1;

    GSError ret = pSurface->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->RequestBuffer(buffer1, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->RequestBuffer(buffer2, releaseFence, requestConfig);
    ASSERT_NE(ret, OHOS::GSERROR_OK);

    ret = pSurface->CancelBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->CancelBuffer(buffer1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->CancelBuffer(buffer2);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer, ReleaseBuffer and CancelBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer
*                  2. call ReleaseBuffer
*                  3. call CancelBuffer
*                  4. check ret
 */
HWTEST_F(ProducerSurfaceTest, RequestAndCancelBufferTest004, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer;

    int releaseFence = -1;
    GSError ret = pSurface->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->CancelBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: SetUserData and GetUserData
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetUserData and GetUserData many times
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, SetAndGetUserDataTest, Function | MediumTest | Level2)
{
    GSError ret;

    std::string strs[SURFACE_MAX_USER_DATA_COUNT];
    constexpr int32_t stringLengthMax = 32;
    char str[stringLengthMax] = {};
    for (int i = 0; i < SURFACE_MAX_USER_DATA_COUNT; i++) {
        auto secRet = snprintf_s(str, sizeof(str), sizeof(str) - 1, "%d", i);
        ASSERT_GT(secRet, 0);

        strs[i] = str;
        ret = pSurface->SetUserData(strs[i], "magic");
        ASSERT_EQ(ret, OHOS::GSERROR_OK);
    }

    ret = pSurface->SetUserData("-1", "error");
    ASSERT_NE(ret, OHOS::GSERROR_OK);

    std::string retStr;
    for (int i = 0; i < SURFACE_MAX_USER_DATA_COUNT; i++) {
        retStr = pSurface->GetUserData(strs[i]);
        ASSERT_EQ(retStr, "magic");
    }
}

/*
* Function: RegisterUserDataChangeListener
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. RegisterUserDataChangeListener
*                  2. SetUserData
*                  3. check ret
 */
HWTEST_F(ProducerSurfaceTest, RegisterUserDataChangeListenerTest001, Function | MediumTest | Level2)
{
    sptr<IConsumerSurface> csurfTestUserData = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listenerTestUserData = new BufferConsumerListener();
    csurfTestUserData->RegisterConsumerListener(listenerTestUserData);
    sptr<IBufferProducer> producerTestUserData = csurf->GetProducer();
    sptr<Surface> pSurfaceTestUserData = Surface::CreateSurfaceAsProducer(producerTestUserData);

    GSError ret1 = OHOS::GSERROR_INVALID_ARGUMENTS;
    GSError ret2 = OHOS::GSERROR_INVALID_ARGUMENTS;
    auto func1 = [&ret1](const std::string& key, const std::string& value) {
        ret1 = OHOS::GSERROR_OK;
    };
    auto func2 = [&ret2](const std::string& key, const std::string& value) {
        ret2 = OHOS::GSERROR_OK;
    };
    pSurfaceTestUserData->RegisterUserDataChangeListener("func1", func1);
    pSurfaceTestUserData->RegisterUserDataChangeListener("func2", func2);
    pSurfaceTestUserData->RegisterUserDataChangeListener("func3", nullptr);
    ASSERT_EQ(pSurfaceTestUserData->RegisterUserDataChangeListener("func2", func2), OHOS::GSERROR_INVALID_ARGUMENTS);

    if (pSurfaceTestUserData->SetUserData("Regist", "OK") == OHOS::GSERROR_OK) {
        ASSERT_EQ(ret1, OHOS::GSERROR_OK);
        ASSERT_EQ(ret2, OHOS::GSERROR_OK);
    }

    ret1 = OHOS::GSERROR_INVALID_ARGUMENTS;
    ret2 = OHOS::GSERROR_INVALID_ARGUMENTS;
    pSurfaceTestUserData->UnRegisterUserDataChangeListener("func1");
    ASSERT_EQ(pSurfaceTestUserData->UnRegisterUserDataChangeListener("func1"), OHOS::GSERROR_INVALID_ARGUMENTS);

    if (pSurfaceTestUserData->SetUserData("UnRegist", "INVALID") == OHOS::GSERROR_OK) {
        ASSERT_EQ(ret1, OHOS::GSERROR_INVALID_ARGUMENTS);
        ASSERT_EQ(ret2, OHOS::GSERROR_OK);
    }

    ret1 = OHOS::GSERROR_INVALID_ARGUMENTS;
    ret2 = OHOS::GSERROR_INVALID_ARGUMENTS;
    pSurfaceTestUserData->ClearUserDataChangeListener();
    pSurfaceTestUserData->RegisterUserDataChangeListener("func1", func1);
    if (pSurfaceTestUserData->SetUserData("Clear", "OK") == OHOS::GSERROR_OK) {
        ASSERT_EQ(ret1, OHOS::GSERROR_OK);
        ASSERT_EQ(ret2, OHOS::GSERROR_INVALID_ARGUMENTS);
    }
}

/*
* Function: RegisterUserDataChangeListener
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. RegisterUserDataChangeListener
*                  2. SetUserData
*                  3. check ret
 */
HWTEST_F(ProducerSurfaceTest, RegisterUserDataChangeListenerTest002, Function | MediumTest | Level2)
{
    sptr<IConsumerSurface> csurfTestUserData = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listenerTestUserData = new BufferConsumerListener();
    csurfTestUserData->RegisterConsumerListener(listenerTestUserData);
    sptr<IBufferProducer> producerTestUserData = csurf->GetProducer();
    sptr<Surface> pSurfaceTestUserData = Surface::CreateSurfaceAsProducer(producerTestUserData);

    auto func = [&pSurfaceTestUserData](const std::string& FuncName) {
        constexpr int32_t RegisterListenerNum = 1000;
        std::vector<GSError> ret(RegisterListenerNum, OHOS::GSERROR_INVALID_ARGUMENTS);
        std::string strs[RegisterListenerNum];
        constexpr int32_t stringLengthMax = 32;
        char str[stringLengthMax] = {};
        for (int i = 0; i < RegisterListenerNum; i++) {
            auto secRet = snprintf_s(str, sizeof(str), sizeof(str) - 1, "%s%d", FuncName.c_str(), i);
            ASSERT_GT(secRet, 0);
            strs[i] = str;
            ASSERT_EQ(pSurfaceTestUserData->RegisterUserDataChangeListener(strs[i], [i, &ret]
            (const std::string& key, const std::string& value) {
                ret[i] = OHOS::GSERROR_OK;
            }), OHOS::GSERROR_OK);
        }

        if (pSurfaceTestUserData->SetUserData("Regist", FuncName) == OHOS::GSERROR_OK) {
            for (int i = 0; i < RegisterListenerNum; i++) {
                ASSERT_EQ(ret[i], OHOS::GSERROR_OK);
            }
        }

        for (int i = 0; i < RegisterListenerNum; i++) {
            pSurfaceTestUserData->UnRegisterUserDataChangeListener(strs[i]);
        }
    };

    std::thread t1(func, "thread1");
    std::thread t2(func, "thread2");
    t1.join();
    t2.join();
}

/*
* Function: GetUniqueId
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetUniqueId
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, GetUniqueIdTest, Function | MediumTest | Level2)
{
    uint64_t uniqueId = pSurface->GetUniqueId();
    ASSERT_NE(uniqueId, 0);
}

/*
* Function: SetTransform and GetTransform
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetTransform with default and check ret
 */
HWTEST_F(ProducerSurfaceTest, SetTransformTest001, Function | MediumTest | Level2)
{
    GSError ret = pSurface->SetTransform(GraphicTransformType::GRAPHIC_ROTATE_NONE);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: SetTransform and GetTransform
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call SetTransform with other parameters and check ret
 */
HWTEST_F(ProducerSurfaceTest, SetTransformTest002, Function | MediumTest | Level1)
{
    GSError ret = pSurface->SetTransform(GraphicTransformType::GRAPHIC_ROTATE_90);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: SetTransform and GetTransform
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call SetTransform with other parameters and check ret
 */
HWTEST_F(ProducerSurfaceTest, SetTransformTest003, Function | MediumTest | Level1)
{
    GSError ret = pSurface->SetTransform(GraphicTransformType::GRAPHIC_ROTATE_180);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: SetTransform and GetTransform
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call SetTransform with other parameters and check ret
 */
HWTEST_F(ProducerSurfaceTest, SetTransformTest004, Function | MediumTest | Level1)
{
    GSError ret = pSurface->SetTransform(GraphicTransformType::GRAPHIC_ROTATE_270);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}
}
