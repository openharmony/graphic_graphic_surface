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
#include <securec.h>
#include <gtest/gtest.h>
#include <surface.h>
#include <consumer_surface.h>
#include <producer_surface.h>
#include "buffer_consumer_listener.h"
#include <native_window.h>
#include "sync_fence.h"
#include "producer_surface_delegator.h"

using namespace testing;
using namespace testing::ext;

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
HWTEST_F(ProducerSurfaceTest, ProducerSurface001, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, GetProducerInitInfo001, Function | MediumTest | Level2)
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
* CaseDescription: 1. call RequestBuffer with producer_ is nullptr
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, RequestBuffer001, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
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
HWTEST_F(ProducerSurfaceTest, RequestBuffer002, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, RequestBuffer003, Function | MediumTest | Level2)
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
* CaseDescription: 1. call RequestBuffer with nullptr params
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, RequestBuffer004, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = nullptr;
    sptr<SyncFence> releaseFence = nullptr;
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
HWTEST_F(ProducerSurfaceTest, RequestBuffers001, Function | MediumTest | Level2)
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
* CaseDescription: 1. call FlushBuffer with producer_ is nullptr
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, FlushBuffer001, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, FlushBuffer002, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    GSError ret = surface_->FlushBuffer(buffer, nullptr, flushConfig);
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
HWTEST_F(ProducerSurfaceTest, FlushBuffer003, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = nullptr;
    GSError ret = surface_->FlushBuffer(buffer, SyncFence::INVALID_FENCE, flushConfig);
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
HWTEST_F(ProducerSurfaceTest, FlushBuffers001, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, GetLastFlushedBuffer001, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, CancelBuffer001, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    GSError ret = surface_->CancelBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: AttachBufferToQueue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call AttachBufferToQueue with producer_ is nullptr
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, AttachBufferToQueue001, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, DetachBufferFromQueue001, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, DetachBuffer001, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, QueueSize001, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, QueueSize002, Function | MediumTest | Level2)
{
    uint32_t queueSize = surface_->GetQueueSize();
    ASSERT_EQ(queueSize, 0);
    GSError ret = surface_->SetQueueSize(queueSize);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
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
HWTEST_F(ProducerSurfaceTest, DefaultWidthAndHeight001, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, TransformHint001, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, DefaultUsage001, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, ReqCanFluAcqRel001, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, ReqCanFluAcqRel002, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, ReqCanFluAcqRel003, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, ReqCanFluAcqRel004, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, ReqCanFluAcqRel005, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, ReqCanFluAcqRel006, Function | MediumTest | Level2)
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
* Function: GetQueueSize and SetQueueSize
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetQeueSize
*                  2. call SetQueueSize 2 times
*                  3. check ret
 */
HWTEST_F(ProducerSurfaceTest, SetQueueSizeDeleting001, Function | MediumTest | Level2)
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
* Function: RequestBuffer, ReleaseBuffer and CancelBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer
*                  2. call ReleaseBuffer
*                  3. call CancelBuffer
*                  4. check ret
 */
HWTEST_F(ProducerSurfaceTest, ReqCanFluAcqRel007, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, UserData001, Function | MediumTest | Level2)
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
* Function: UserDataChangeListen
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. RegisterUserDataChangeListen
*                  2. SetUserData
*                  3. check ret
 */
HWTEST_F(ProducerSurfaceTest, UserDataChangeListen001, Function | MediumTest | Level2)
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
* Function: UserDataChangeListen
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. RegisterUserDataChangeListen
*                  2. SetUserData
*                  3. check ret
 */
HWTEST_F(ProducerSurfaceTest, UserDataChangeListen002, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, UniqueId001, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, transform001, Function | MediumTest | Level2)
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
HWTEST_F(ProducerSurfaceTest, transform002, Function | MediumTest | Level1)
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
HWTEST_F(ProducerSurfaceTest, transform003, Function | MediumTest | Level1)
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
HWTEST_F(ProducerSurfaceTest, transform004, Function | MediumTest | Level1)
{
    GSError ret = pSurface->SetTransform(GraphicTransformType::GRAPHIC_ROTATE_270);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: SetTransform and GetTransform
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call SetTransform with producer_ is nullptr and check ret
*                  2. call GetTransform with producer_ is nullptr and check ret
 */
HWTEST_F(ProducerSurfaceTest, transform005, Function | MediumTest | Level1)
{
    GSError ret = surface_->SetTransform(GraphicTransformType::GRAPHIC_ROTATE_270);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    GraphicTransformType type = surface_->GetTransform();
    ASSERT_EQ(type, GraphicTransformType::GRAPHIC_ROTATE_BUTT);
}

/*
* Function: IsSupportedAlloc
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call IsSupportedAlloc with abnormal parameters and check ret
 */
HWTEST_F(ProducerSurfaceTest, isSupportedAlloc001, Function | MediumTest | Level2)
{
    std::vector<BufferVerifyAllocInfo> infos;
    std::vector<bool> supporteds;
    GSError ret = pSurface->IsSupportedAlloc(infos, supporteds);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: IsSupportedAlloc
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call IsSupportedAlloc with abnormal parameters and check ret
 */
HWTEST_F(ProducerSurfaceTest, isSupportedAlloc002, Function | MediumTest | Level2)
{
    std::vector<BufferVerifyAllocInfo> infos;
    std::vector<bool> supporteds;
    GSError ret = pSurface->IsSupportedAlloc(infos, supporteds);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);

    BufferVerifyAllocInfo info = {
        .width = 0x100,
        .height = 0x100,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
    };
    infos.push_back(info);
    info.format = GRAPHIC_PIXEL_FMT_YCRCB_420_SP;
    infos.push_back(info);
    info.format = GRAPHIC_PIXEL_FMT_YUV_422_I;
    infos.push_back(info);

    ret = pSurface->IsSupportedAlloc(infos, supporteds);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: IsSupportedAlloc
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call IsSupportedAlloc with normal parameters and check ret
 */
HWTEST_F(ProducerSurfaceTest, isSupportedAlloc003, Function | MediumTest | Level1)
{
    std::vector<BufferVerifyAllocInfo> infos;
    std::vector<bool> supporteds;
    BufferVerifyAllocInfo info = {
        .width = 0x100,
        .height = 0x100,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
    };
    infos.push_back(info);
    info.format = GRAPHIC_PIXEL_FMT_YCRCB_420_SP;
    infos.push_back(info);
    info.format = GRAPHIC_PIXEL_FMT_YUV_422_I;
    infos.push_back(info);

    supporteds.push_back(false);
    supporteds.push_back(false);
    supporteds.push_back(false);

    GSError ret = pSurface->IsSupportedAlloc(infos, supporteds);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);  // mock data result
    ASSERT_EQ(supporteds[0], true);  // mock data result
    ASSERT_EQ(supporteds[1], true);  // mock data result
    ASSERT_EQ(supporteds[2], false);  // mock data result
}

/*
* Function: IsSupportedAlloc
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call IsSupportedAlloc with producer_ is nullptr and check ret
 */
HWTEST_F(ProducerSurfaceTest, isSupportedAlloc004, Function | MediumTest | Level2)
{
    std::vector<BufferVerifyAllocInfo> infos;
    std::vector<bool> supporteds;
    GSError ret = surface_->IsSupportedAlloc(infos, supporteds);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: SetScalingMode and GetScalingMode
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetScalingMode with abnormal parameters and check ret
 */
HWTEST_F(ProducerSurfaceTest, scalingMode001, Function | MediumTest | Level2)
{
    ScalingMode scalingMode = ScalingMode::SCALING_MODE_SCALE_TO_WINDOW;
    GSError ret = pSurface->SetScalingMode(-1, scalingMode);
    ASSERT_EQ(ret, OHOS::GSERROR_NO_ENTRY);
}

/*
* Function: SetScalingMode and GetScalingMode
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call SetScalingMode with normal parameters and check ret
*                  2. call GetScalingMode and check ret
 */
HWTEST_F(ProducerSurfaceTest, scalingMode002, Function | MediumTest | Level1)
{
    ScalingMode scalingMode = ScalingMode::SCALING_MODE_SCALE_TO_WINDOW;
    sptr<SurfaceBuffer> buffer;
    int releaseFence = -1;
    GSError ret = pSurface->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);

    uint32_t sequence = buffer->GetSeqNum();
    ret = pSurface->SetScalingMode(sequence, scalingMode);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->CancelBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: SetScalingMode003
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetScalingMode with abnormal parameters and check ret
 */
HWTEST_F(ProducerSurfaceTest, scalingMode003, Function | MediumTest | Level2)
{
    ScalingMode scalingMode = ScalingMode::SCALING_MODE_SCALE_TO_WINDOW;
    GSError ret = pSurface->SetScalingMode(scalingMode);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: SetScalingMode and GetScalingMode
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetScalingMode with producer_ is nullptr and check ret
*                  2. call GetScalingMode and check ret
 */
HWTEST_F(ProducerSurfaceTest, scalingMode004, Function | MediumTest | Level2)
{
    ScalingMode scalingMode = ScalingMode::SCALING_MODE_SCALE_TO_WINDOW;
    GSError ret = surface_->SetScalingMode(scalingMode);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ret = surface_->SetScalingMode(0, scalingMode);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ret = surface_->GetScalingMode(0, scalingMode);
    ASSERT_EQ(ret, OHOS::GSERROR_NOT_SUPPORT);
}

/*
* Function: SetMetaData and GetMetaData
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetMetaData with abnormal parameters and check ret
 */
HWTEST_F(ProducerSurfaceTest, metaData001, Function | MediumTest | Level2)
{
    std::vector<GraphicHDRMetaData> metaData;
    GSError ret = pSurface->SetMetaData(firstSeqnum, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: SetMetaData and GetMetaData
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetMetaData with abnormal parameters and check ret
 */
HWTEST_F(ProducerSurfaceTest, metaData002, Function | MediumTest | Level2)
{
    std::vector<GraphicHDRMetaData> metaData;
    GraphicHDRMetaData data = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .value = 100,  // for test
    };
    metaData.push_back(data);
    GSError ret = pSurface->SetMetaData(-1, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_NO_ENTRY);
}

/*
* Function: SetMetaData and GetMetaData
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call SetMetaData with normal parameters and check ret
*                  2. call GetMetaData and check ret
 */
HWTEST_F(ProducerSurfaceTest, metaData003, Function | MediumTest | Level1)
{
    std::vector<GraphicHDRMetaData> metaData;
    GraphicHDRMetaData data = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .value = 100,  // for test
    };
    metaData.push_back(data);
    sptr<SurfaceBuffer> buffer;
    int releaseFence = -1;
    GSError ret = pSurface->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);

    uint32_t sequence = buffer->GetSeqNum();
    ret = pSurface->SetMetaData(sequence, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->CancelBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: SetMetaData and GetMetaData
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetMetaData with producer_ is nullptr and check ret
*                  2. call GetMetaData and check ret
 */
HWTEST_F(ProducerSurfaceTest, metaData004, Function | MediumTest | Level2)
{
    std::vector<GraphicHDRMetaData> metaData;
    GSError ret = surface_->SetMetaData(0, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    GraphicHDRMetaData data = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .value = 100,  // 100 metaData value for test
    };
    metaData.push_back(data);
    ret = surface_->SetMetaData(0, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ret = surface_->GetMetaData(0, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_NOT_SUPPORT);
}

/*
* Function: SetMetaDataSet and GetMetaDataSet
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetMetaDataSet with abnormal parameters and check ret
 */
HWTEST_F(ProducerSurfaceTest, metaDataSet001, Function | MediumTest | Level2)
{
    GraphicHDRMetadataKey key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_HDR10_PLUS;
    std::vector<uint8_t> metaData;

    GSError ret = pSurface->SetMetaDataSet(firstSeqnum, key, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: SetMetaDataSet and GetMetaDataSet
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetMetaDataSet with abnormal parameters and check ret
 */
HWTEST_F(ProducerSurfaceTest, metaDataSet002, Function | MediumTest | Level2)
{
    GraphicHDRMetadataKey key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_HDR10_PLUS;
    std::vector<uint8_t> metaData;

    uint8_t data = 10;  // for test
    metaData.push_back(data);
    GSError ret = pSurface->SetMetaDataSet(-1, key, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_NO_ENTRY);
}

/*
* Function: SetMetaDataSet and GetMetaDataSet
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call SetMetaDataSet with normal parameters and check ret
*                  2. call GetMetaDataSet and check ret
 */
HWTEST_F(ProducerSurfaceTest, metaDataSet003, Function | MediumTest | Level1)
{
    GraphicHDRMetadataKey key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_HDR10_PLUS;
    std::vector<uint8_t> metaData;
    uint8_t data = 10;  // for test
    metaData.push_back(data);

    sptr<SurfaceBuffer> buffer;
    int releaseFence = -1;
    GSError ret = pSurface->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);

    uint32_t sequence = buffer->GetSeqNum();
    ret = pSurface->SetMetaDataSet(sequence, key, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->CancelBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: SetMetaDataSet and GetMetaDataSet
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetMetaDataSet with producer_ is nullptr and check ret
*                  2. call GetMetaDataSet and check ret
 */
HWTEST_F(ProducerSurfaceTest, metaDataSet004, Function | MediumTest | Level2)
{
    GraphicHDRMetadataKey key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_HDR10_PLUS;
    std::vector<uint8_t> metaData;

    uint8_t data = 10;  // metaData value for test
    metaData.push_back(data);
    GSError ret = surface_->SetMetaDataSet(0, key, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ret = surface_->GetMetaDataSet(0, key, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_NOT_SUPPORT);
}

/*
* Function: SetTunnelHandle and GetTunnelHandle
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetTunnelhandle with producer_ is nullptr and check ret
 */
HWTEST_F(ProducerSurfaceTest, tunnelHandle001, Function | MediumTest | Level2)
{
    GraphicExtDataHandle *handle = nullptr;
    handle = static_cast<GraphicExtDataHandle *>(malloc(sizeof(GraphicExtDataHandle) + sizeof(int32_t) * 1));
    handle->fd = -1;
    handle->reserveInts = 1;
    handle->reserve[0] = 0;
    GSError ret = surface_->SetTunnelHandle(handle);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);

    sptr<SurfaceTunnelHandle> handleGet = surface_->GetTunnelHandle();
    ASSERT_EQ(handleGet, nullptr);
    free(handle);
}

/*
* Function: SetTunnelHandle and GetTunnelHandle
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetTunnelhandle with normal parameters and check ret
 */
HWTEST_F(ProducerSurfaceTest, tunnelHandle002, Function | MediumTest | Level2)
{
    GraphicExtDataHandle *handle = nullptr;
    handle = static_cast<GraphicExtDataHandle *>(malloc(sizeof(GraphicExtDataHandle) + sizeof(int32_t) * 1));
    handle->fd = -1;
    handle->reserveInts = 1;
    handle->reserve[0] = 0;
    GSError ret = pSurface->SetTunnelHandle(handle);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->SetTunnelHandle(handle);
    ASSERT_EQ(ret, OHOS::GSERROR_NO_ENTRY);
    free(handle);
}

/*
* Function: connect
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call connect and check ret
 */
HWTEST_F(ProducerSurfaceTest, connect001, Function | MediumTest | Level1)
{
    GSError ret = pSurface->Connect();
    ASSERT_EQ(ret, OHOS::SURFACE_ERROR_CONSUMER_IS_CONNECTED);
}

/*
* Function: disconnect
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call Disconnect and check ret
 */
HWTEST_F(ProducerSurfaceTest, disconnect001, Function | MediumTest | Level1)
{
    GSError ret = pSurface->Disconnect();
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: connect
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call connect and check ret
 */
HWTEST_F(ProducerSurfaceTest, connect002, Function | MediumTest | Level1)
{
    GSError ret = pSurface->Connect();
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: disconnect
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call Disconnect and check ret
 */
HWTEST_F(ProducerSurfaceTest, disconnect002, Function | MediumTest | Level1)
{
    GSError ret = pSurface->Disconnect();
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: connect
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call connect with producer_ is nullptr and check ret
 */
HWTEST_F(ProducerSurfaceTest, connect003, Function | MediumTest | Level1)
{
    GSError ret = surface_->Connect();
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: disconnect
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call Disconnect with producer_ is nullptr and check ret
 */
HWTEST_F(ProducerSurfaceTest, disconnect003, Function | MediumTest | Level1)
{
    GSError ret = surface_->Disconnect();
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: SetPresentTimestamp and GetPresentTimestamp
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetPresentTimestamp with producer_ is nullptr and check ret
*                  2. call SetPresentTimestamp and check ret
 */
HWTEST_F(ProducerSurfaceTest, presentTimestamp001, Function | MediumTest | Level2)
{
    GraphicPresentTimestampType type = GraphicPresentTimestampType::GRAPHIC_DISPLAY_PTS_UNSUPPORTED;
    int64_t time = 0;

    GSError ret = surface_->GetPresentTimestamp(firstSeqnum, type, time);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    GraphicPresentTimestamp timestamp;
    ret = surface_->SetPresentTimestamp(firstSeqnum, timestamp);
    ASSERT_EQ(ret, OHOS::GSERROR_NOT_SUPPORT);
}

/*
* Function: SetPresentTimestamp and GetPresentTimestamp
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetPresentTimestamp with normal parameters and check ret
* @tc.require: issueI5I57K
 */
HWTEST_F(ProducerSurfaceTest, presentTimestamp002, Function | MediumTest | Level2)
{
    GraphicPresentTimestampType type = GraphicPresentTimestampType::GRAPHIC_DISPLAY_PTS_UNSUPPORTED;
    int64_t time = 0;

    GSError ret = pSurface->GetPresentTimestamp(firstSeqnum, type, time);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: SetPresentTimestamp and GetPresentTimestamp
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetPresentTimestamp with normal parameters and check ret
* @tc.require: issueI5I57K
 */
HWTEST_F(ProducerSurfaceTest, presentTimestamp003, Function | MediumTest | Level2)
{
    GraphicPresentTimestampType type = GraphicPresentTimestampType::GRAPHIC_DISPLAY_PTS_DELAY;
    int64_t time = 0;
    GSError ret = pSurface->GetPresentTimestamp(-1, type, time);
    ASSERT_EQ(ret, OHOS::GSERROR_NO_ENTRY);
}

/*
* Function: SetPresentTimestamp and GetPresentTimestamp
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call SetPresentTimestamp and check ret
* @tc.require: issueI5I57K
 */
HWTEST_F(ProducerSurfaceTest, presentTimestamp004, Function | MediumTest | Level1)
{
    sptr<SurfaceBuffer> buffer;
    int releaseFence = -1;
    GSError ret = pSurface->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);

    uint32_t sequence = buffer->GetSeqNum();
    GraphicPresentTimestampType type = GraphicPresentTimestampType::GRAPHIC_DISPLAY_PTS_DELAY;
    int64_t time = 0;
    ret = pSurface->GetPresentTimestamp(sequence, type, time);
    ASSERT_EQ(ret, OHOS::GSERROR_NO_ENTRY);

    ret = pSurface->CancelBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: SetWptrNativeWindowToPSurface
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. SetWptrNativeWindowToPSurface and check ret
* @tc.require: issueI7WYIY
 */
HWTEST_F(ProducerSurfaceTest, SetWptrNativeWindowToPSurface001, Function | MediumTest | Level1)
{
    struct NativeWindow nativeWindow;
    GSError ret = pSurface->SetWptrNativeWindowToPSurface(&nativeWindow);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: AttachBuffer
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. AttachBuffer and check ret
* @tc.require: issueI7WYIY
 */
HWTEST_F(ProducerSurfaceTest, AttachBuffer001, Function | MediumTest | Level1)
{
    GSError ret = pSurface->CleanCache();
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    ASSERT_NE(buffer, nullptr);
    sptr<SyncFence> fence = SyncFence::INVALID_FENCE;
    int32_t timeOut = 5;
    ret = pSurface->AttachBuffer(buffer, timeOut);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: AttachBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call AttachBuffer with producer_ is nullptr
*                  2. check ret
 */
HWTEST_F(ProducerSurfaceTest, AttachBuffer002, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    GSError ret = surface_->AttachBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ret = surface_->AttachBuffer(buffer, 0);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: RegisterSurfaceDelegator000
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. RegisterSurfaceDelegator and check ret
* @tc.require: issueI7WYIY
 */
HWTEST_F(ProducerSurfaceTest, RegisterSurfaceDelegator001, Function | MediumTest | Level1)
{
    GSError ret = pSurface->CleanCache();
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ret = pSurface->RegisterSurfaceDelegator(nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: CleanCache001
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. CleanCache and check ret
 */
HWTEST_F(ProducerSurfaceTest, CleanCache001, Function | MediumTest | Level2)
{
    GSError ret = pSurface->CleanCache(true);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: CleanCache
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. CleanCache with producer_ is nullptr and check ret
 */
HWTEST_F(ProducerSurfaceTest, CleanCache002, Function | MediumTest | Level2)
{
    GSError ret = surface_->CleanCache(true);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: GoBackground
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. GoBackground with producer_ is nullptr and check ret
 */
HWTEST_F(ProducerSurfaceTest, GoBackground001, Function | MediumTest | Level2)
{
    GSError ret = surface_->GoBackground();
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: SetSurfaceSourceType and GetSurfaceSourceType
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetSurfaceSourceType and check ret
*                  2. call GetSurfaceSourceType and check ret
*/
HWTEST_F(ProducerSurfaceTest, SurfaceSourceType001, Function | MediumTest | Level2)
{
    OHSurfaceSource sourceType = OHSurfaceSource::OH_SURFACE_SOURCE_VIDEO;
    GSError ret = pSurface->SetSurfaceSourceType(sourceType);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(pSurface->GetSurfaceSourceType(), OH_SURFACE_SOURCE_VIDEO);
}

/*
* Function: SetSurfaceSourceType and GetSurfaceSourceType
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetSurfaceSourceType with producer_ is nullptr and check ret
*                  2. call GetSurfaceSourceType with producer_ is nullptr and check ret
*/
HWTEST_F(ProducerSurfaceTest, SurfaceSourceType002, Function | MediumTest | Level2)
{
    OHSurfaceSource sourceType = OHSurfaceSource::OH_SURFACE_SOURCE_VIDEO;
    GSError ret = surface_->SetSurfaceSourceType(sourceType);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(surface_->GetSurfaceSourceType(), OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT);
}

/*
* Function: SetSurfaceAppFrameworkType and GetSurfaceAppFrameworkType
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetSurfaceAppFrameworkType and check ret
*                  2. call GetSurfaceAppFrameworkType and check ret
*/
HWTEST_F(ProducerSurfaceTest, SurfaceAppFrameworkType001, Function | MediumTest | Level2)
{
    std::string type = "test";
    GSError ret = pSurface->SetSurfaceAppFrameworkType(type);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(pSurface->GetSurfaceAppFrameworkType(), "test");
}

/*
* Function: SetSurfaceAppFrameworkType and GetSurfaceAppFrameworkType
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetSurfaceAppFrameworkType with producer_ is nullptr and check ret
*                  2. call GetSurfaceAppFrameworkType with producer_ is nullptr and check ret
*/
HWTEST_F(ProducerSurfaceTest, SurfaceAppFrameworkType002, Function | MediumTest | Level2)
{
    std::string type = "test";
    GSError ret = surface_->SetSurfaceAppFrameworkType(type);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(surface_->GetSurfaceAppFrameworkType(), "");
}

/*
* Function: RegisterReleaseListener and UnRegisterReleaseListener
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RegisterReleaseListener with producer_ is nullptr and check ret
*                  2. call UnRegisterReleaseListener with producer_ is nullptr and check ret
*/
HWTEST_F(ProducerSurfaceTest, ReleaseListener001, Function | MediumTest | Level2)
{
    GSError ret = surface_->RegisterReleaseListener(OnBufferRelease);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    OnReleaseFuncWithFence releaseFuncWithFence;
    ret = surface_->RegisterReleaseListener(releaseFuncWithFence);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ret = surface_->UnRegisterReleaseListener();
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: RegisterUserDataChangeListener
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RegisterUserDataChangeListener with nullptr param
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceTest, RegisterUserDataChangeListener001, Function | MediumTest | Level2)
{
    GSError ret = surface_->RegisterUserDataChangeListener("test", nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: IsRemote
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call IsRemote with producer_ is nullptr
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceTest, IsRemote001, Function | MediumTest | Level2)
{
    bool ret = surface_->IsRemote();
    ASSERT_EQ(ret, false);
}

/*
* Function: RequestBuffersAndFlushBuffers
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffers and FlushBuffers
* @tc.require: issueI5GMZN issueI5IWHW
 */
HWTEST_F(ProducerSurfaceTest, RequestBuffersAndFlushBuffers, Function | MediumTest | Level1)
{
    pSurface->SetQueueSize(12);
    std::vector<sptr<SurfaceBuffer>> sfbuffers;
    std::vector<sptr<SyncFence>> releaseFences;
    EXPECT_EQ(OHOS::GSERROR_OK, pSurface->RequestBuffers(sfbuffers, releaseFences, requestConfig));
    for (size_t i = 0; i < sfbuffers.size(); ++i) {
        EXPECT_NE(nullptr, sfbuffers[i]);
    }
    std::cout << sfbuffers.size() << std::endl;
    uint32_t num = static_cast<uint32_t>(sfbuffers.size());
    std::vector<sptr<SyncFence>> flushFences;
    std::vector<BufferFlushConfigWithDamages> configs;
    flushFences.resize(num);
    configs.reserve(num);
    auto handleConfig = [](BufferFlushConfigWithDamages &config) -> void {
        config.damages.reserve(1);
        OHOS::Rect damage = {
            .x = 0,
            .y = 0,
            .w = 0x100,
            .h = 0x100
        };
        config.damages.emplace_back(damage);
        config.timestamp = 0;
    };
    for (uint32_t i = 0; i < num; ++i) {
        flushFences[i] = new SyncFence(-1);
        BufferFlushConfigWithDamages config;
        handleConfig(config);
        configs.emplace_back(config);
    }
    flushFences[0] = nullptr;
    EXPECT_EQ(OHOS::GSERROR_INVALID_ARGUMENTS, pSurface->FlushBuffers(sfbuffers, flushFences, configs));
    flushFences[0] = new SyncFence(-1);
    EXPECT_EQ(OHOS::GSERROR_OK, pSurface->FlushBuffers(sfbuffers, flushFences, configs));
    sptr<SurfaceBuffer> buffer;
    int32_t flushFence;
    for (uint32_t i = 0; i < num; ++i) {
        GSError ret = csurf->AcquireBuffer(buffer, flushFence, timestamp, damage);
        ASSERT_EQ(ret, OHOS::GSERROR_OK);
        ret = csurf->ReleaseBuffer(buffer, -1);
        ASSERT_EQ(ret, OHOS::GSERROR_OK);
    }
    EXPECT_EQ(OHOS::GSERROR_NO_BUFFER, csurf->AcquireBuffer(buffer, flushFence, timestamp, damage));
    pSurface->SetQueueSize(2);
}

/*
* Function: AcquireLastFlushedBuffer and ReleaseLastFlushedBuffer
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call AcquireLastFlushedBuffer OK
*                  2. call AcquireLastFlushedBuffer FAIL
*                  3. call ReleaseLastFlushedBuffer
 */
HWTEST_F(ProducerSurfaceTest, AcquireLastFlushedBuffer001, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer;
    int releaseFence = -1;
    EXPECT_EQ(producer->SetQueueSize(3), OHOS::GSERROR_OK);
    GSError ret = pSurface->RequestBuffer(buffer, releaseFence, requestConfig);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->FlushBuffer(buffer, -1, flushConfig);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    int32_t flushFence;
    ret = csurf->AcquireBuffer(buffer, flushFence, timestamp, damage);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);
    ret = csurf->ReleaseBuffer(buffer, -1);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SurfaceBuffer> buffer1 = nullptr;
    sptr<SyncFence> fence = nullptr;
    float matrix[16];

    ret = pSurface->AcquireLastFlushedBuffer(buffer1, fence, matrix, 16, false);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);
    EXPECT_EQ(buffer->GetSeqNum(), buffer1->GetSeqNum());

    ret = pSurface->AcquireLastFlushedBuffer(buffer1, fence, matrix, 16, false);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_BUFFER_STATE_INVALID);

    sptr<SurfaceBuffer> buffer2;
    ret = pSurface->RequestBuffer(buffer2, releaseFence, requestConfig);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SurfaceBuffer> buffer3;
    ret = pSurface->RequestBuffer(buffer3, releaseFence, requestConfig);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SurfaceBuffer> buffer4;
    ret = pSurface->RequestBuffer(buffer4, releaseFence, requestConfig);
    EXPECT_EQ(ret, OHOS::GSERROR_NO_BUFFER);

    ret = pSurface->ReleaseLastFlushedBuffer(buffer1);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->RequestBuffer(buffer4, releaseFence, requestConfig);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->FlushBuffer(buffer2, -1, flushConfig);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->FlushBuffer(buffer3, -1, flushConfig);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->FlushBuffer(buffer4, -1, flushConfig);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    ret = pSurface->ReleaseLastFlushedBuffer(buffer2);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_BUFFER_STATE_INVALID);

    EXPECT_EQ(pSurface->CleanCache(), OHOS::GSERROR_OK);
}

/*
* Function: AcquireLastFlushedBuffer and ReleaseLastFlushedBuffer
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call AcquireLastFlushedBuffer FAIL
*                  3. call ReleaseLastFlushedBuffer FAIL
 */
HWTEST_F(ProducerSurfaceTest, AcquireLastFlushedBuffer002, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer1 = nullptr;
    sptr<SyncFence> fence = nullptr;
    float matrix[16];
    GSError ret = surface_->AcquireLastFlushedBuffer(buffer1, fence, matrix, 16, false);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ret = surface_->ReleaseLastFlushedBuffer(buffer1);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ret = pSurface->ReleaseLastFlushedBuffer(nullptr);
    EXPECT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: SetHdrWhitePointBrightness and SetSdrWhitePointBrightness
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetHdrWhitePointBrightness with producer_ is nullptr and check ret
*                  2. call SetSdrWhitePointBrightness with producer_ is nullptr and check ret
*/
HWTEST_F(ProducerSurfaceTest, WhitePointBrightness001, Function | MediumTest | Level2)
{
    GSError ret = surface_->SetHdrWhitePointBrightness(0);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ret = surface_->SetSdrWhitePointBrightness(0);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: ReleaseLastFlushedBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call ReleaseLastFlushedBuffer with buffer is nullptr and check ret
*                  2. call ReleaseLastFlushedBuffer with producer_ is nullptr and check ret
*/
HWTEST_F(ProducerSurfaceTest, ReleaseLastFlushedBuffer001, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer = nullptr;
    GSError ret = surface_->ReleaseLastFlushedBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    buffer = SurfaceBuffer::Create();
    ret = surface_->ReleaseLastFlushedBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}
}
