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

/*
 * Function: RequestBuffer and CancelBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RequestBuffer by cs and ps
 *                  2. call CancelBuffer both
 *                  3. check ret
 */
HWTEST_F(ConsumerSurfaceTest, RequestAndCancle001, Function | MediumTest | Level2)
{
    sptr<SurfaceBuffer> buffer;
    int releaseFence = -1;
    GSError ret = ps->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = ps->CancelBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: SetUserData
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetUserData many times
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetUserData001, Function | MediumTest | Level2)
{
    GSError ret;

    std::string strs[SURFACE_MAX_USER_DATA_COUNT];
    constexpr int32_t stringLengthMax = 32;
    char str[stringLengthMax] = {};
    for (int i = 0; i < SURFACE_MAX_USER_DATA_COUNT; i++) {
        auto secRet = snprintf_s(str, sizeof(str), sizeof(str) - 1, "%d", i);
        ASSERT_GT(secRet, 0);

        strs[i] = str;
        ret = cs->SetUserData(strs[i], "magic");
        ASSERT_EQ(ret, OHOS::GSERROR_OK);
    }

    ret = cs->SetUserData("-1", "error");
    ASSERT_NE(ret, OHOS::GSERROR_OK);

    std::string retStr;
    for (int i = 0; i < SURFACE_MAX_USER_DATA_COUNT; i++) {
        retStr = cs->GetUserData(strs[i]);
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
HWTEST_F(ConsumerSurfaceTest, UserDataChangeListen001, Function | MediumTest | Level2)
{
    sptr<IConsumerSurface> csTestUserData = IConsumerSurface::Create();
    GSError ret1 = OHOS::GSERROR_INVALID_ARGUMENTS;
    GSError ret2 = OHOS::GSERROR_INVALID_ARGUMENTS;
    auto func1 = [&ret1](const std::string& key, const std::string& value) {
        ret1 = OHOS::GSERROR_OK;
    };
    auto func2 = [&ret2](const std::string& key, const std::string& value) {
        ret2 = OHOS::GSERROR_OK;
    };
    csTestUserData->RegisterUserDataChangeListener("func1", func1);
    csTestUserData->RegisterUserDataChangeListener("func2", func2);
    csTestUserData->RegisterUserDataChangeListener("func3", nullptr);
    ASSERT_EQ(csTestUserData->RegisterUserDataChangeListener("func2", func2), OHOS::GSERROR_INVALID_ARGUMENTS);

    if (csTestUserData->SetUserData("Regist", "OK") == OHOS::GSERROR_OK) {
        ASSERT_EQ(ret1, OHOS::GSERROR_OK);
        ASSERT_EQ(ret2, OHOS::GSERROR_OK);
    }

    ret1 = OHOS::GSERROR_INVALID_ARGUMENTS;
    ret2 = OHOS::GSERROR_INVALID_ARGUMENTS;
    csTestUserData->UnRegisterUserDataChangeListener("func1");
    ASSERT_EQ(csTestUserData->UnRegisterUserDataChangeListener("func1"), OHOS::GSERROR_INVALID_ARGUMENTS);

    if (csTestUserData->SetUserData("UnRegist", "INVALID") == OHOS::GSERROR_OK) {
        ASSERT_EQ(ret1, OHOS::GSERROR_INVALID_ARGUMENTS);
        ASSERT_EQ(ret2, OHOS::GSERROR_OK);
    }

    ret1 = OHOS::GSERROR_INVALID_ARGUMENTS;
    ret2 = OHOS::GSERROR_INVALID_ARGUMENTS;
    csTestUserData->ClearUserDataChangeListener();
    csTestUserData->RegisterUserDataChangeListener("func1", func1);
    if (csTestUserData->SetUserData("Clear", "OK") == OHOS::GSERROR_OK) {
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
HWTEST_F(ConsumerSurfaceTest, UserDataChangeListen002, Function | MediumTest | Level2)
{
    sptr<IConsumerSurface> csTestUserData = IConsumerSurface::Create();

    auto func = [&csTestUserData](const std::string& FuncName) {
        constexpr int32_t RegisterListenerNum = 1000;
        std::vector<GSError> ret(RegisterListenerNum, OHOS::GSERROR_INVALID_ARGUMENTS);
        std::string strs[RegisterListenerNum];
        constexpr int32_t stringLengthMax = 32;
        char str[stringLengthMax] = {};
        for (int i = 0; i < RegisterListenerNum; i++) {
            auto secRet = snprintf_s(str, sizeof(str), sizeof(str) - 1, "%s%d", FuncName.c_str(), i);
            ASSERT_GT(secRet, 0);
            strs[i] = str;
            ASSERT_EQ(csTestUserData->RegisterUserDataChangeListener(strs[i], [i, &ret]
            (const std::string& key, const std::string& value) {
                ret[i] = OHOS::GSERROR_OK;
            }), OHOS::GSERROR_OK);
        }

        if (csTestUserData->SetUserData("Regist", FuncName) == OHOS::GSERROR_OK) {
            for (int i = 0; i < RegisterListenerNum; i++) {
                ASSERT_EQ(ret[i], OHOS::GSERROR_OK);
            }
        }

        for (int i = 0; i < RegisterListenerNum; i++) {
            csTestUserData->UnRegisterUserDataChangeListener(strs[i]);
        }
    };

    std::thread t1(func, "thread1");
    std::thread t2(func, "thread2");
    t1.join();
    t2.join();
}

/*
 * Function: RegisterConsumerListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RegisterConsumerListener
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, ConsumerListener001, Function | MediumTest | Level2)
{
    class TestConsumerListener : public IBufferConsumerListener {
    public:
        void OnBufferAvailable() override
        {
            sptr<SurfaceBuffer> buffer;
            int32_t flushFence;

            cs->AcquireBuffer(buffer, flushFence, timestamp, damage);
            int32_t *p = (int32_t*)buffer->GetVirAddr();
            if (p != nullptr) {
                for (int32_t i = 0; i < 128; i++) {
                    ASSERT_EQ(p[i], i);
                }
            }

            cs->ReleaseBuffer(buffer, -1);
        }
    };

    sptr<IBufferConsumerListener> listener = new TestConsumerListener();
    GSError ret = cs->RegisterConsumerListener(listener);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SurfaceBuffer> buffer;
    int releaseFence = -1;
    ret = ps->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);

    int32_t *p = (int32_t*)buffer->GetVirAddr();
    if (p != nullptr) {
        for (int32_t i = 0; i < 128; i++) {
            p[i] = i;
        }
    }

    GraphicTransformType tranformType = ps->GetTransform();
    ret = ps->FlushBuffer(buffer, -1, flushConfig);
    ASSERT_EQ(ps->SetTransform(GraphicTransformType::GRAPHIC_ROTATE_180), OHOS::GSERROR_OK);
    GraphicTransformType bufferTranformType = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    ASSERT_EQ(cs->GetSurfaceBufferTransformType(nullptr, &bufferTranformType), OHOS::SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(cs->GetSurfaceBufferTransformType(buffer, nullptr), OHOS::SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(cs->GetSurfaceBufferTransformType(buffer, &bufferTranformType), OHOS::GSERROR_OK);
    ASSERT_EQ(bufferTranformType, tranformType);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    listener->OnTunnelHandleChange();
    listener->OnGoBackground();
    listener->OnCleanCache();
    listener->OnTransformChange();
    TestConsumerListenerClazz* listenerClazz = new TestConsumerListenerClazz();
    listenerClazz->OnTunnelHandleChange();
    listenerClazz->OnGoBackground();
    listenerClazz->OnCleanCache();
    listenerClazz->OnTransformChange();
    delete listenerClazz;
}

/*
 * Function: RegisterConsumerListener, RequestBuffer and FlushBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RegisterConsumerListener
 *                  2. call RequestBuffer
 *                  3. call FlushBuffer
 *                  4. check ret
 */
HWTEST_F(ConsumerSurfaceTest, ConsumerListener002, Function | MediumTest | Level2)
{
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    GSError ret = cs->RegisterConsumerListener(listener);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SurfaceBuffer> buffer;
    int releaseFence = -1;
    ret = ps->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);

    int32_t *p = (int32_t*)buffer->GetVirAddr();
    if (p != nullptr) {
        for (int32_t i = 0; i < requestConfig.width * requestConfig.height; i++) {
            p[i] = i;
        }
    }
    ASSERT_EQ(ps->SetTransform(GraphicTransformType::GRAPHIC_ROTATE_90), OHOS::GSERROR_OK);

    ret = ps->FlushBuffer(buffer, -1, flushConfig);
    GraphicTransformType bufferTranformType = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    ASSERT_EQ(cs->GetSurfaceBufferTransformType(buffer, &bufferTranformType), OHOS::GSERROR_OK);
    ASSERT_EQ(bufferTranformType, GraphicTransformType::GRAPHIC_ROTATE_90);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(ps->SetTransform(GraphicTransformType::GRAPHIC_ROTATE_NONE), OHOS::GSERROR_OK);

    sptr<OHOS::SyncFence> flushFence;
    ret = cs->AcquireBuffer(buffer, flushFence, timestamp, damage);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);

    bool isInCache = false;
    ASSERT_EQ(cs->IsSurfaceBufferInCache(buffer->GetSeqNum(), isInCache), OHOS::GSERROR_OK);
    ASSERT_EQ(isInCache, true);
    ASSERT_EQ(cs->IsSurfaceBufferInCache(0xFFFFFFFF, isInCache), OHOS::GSERROR_OK);
    ASSERT_EQ(isInCache, false);
}

/*
 * Function: RegisterConsumerListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RegisterConsumerListener with nullptr params
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, ConsumerListener003, Function | MediumTest | Level2)
{
    GSError ret = surface_->RegisterConsumerListener(nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    ret = surface_->RegisterConsumerListener(listener);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: RegisterConsumerListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RegisterConsumerListener with nullptr params
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, ConsumerListener004, Function | MediumTest | Level2)
{
    GSError ret = surface_->RegisterConsumerListener(nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    TestConsumerListenerClazz* listenerClazz = new TestConsumerListenerClazz();
    ret = surface_->RegisterConsumerListener(listenerClazz);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    delete listenerClazz;
}

/*
 * Function: UnregisterConsumerListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call UnregisterConsumerListener with consumer_ is nullptr
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, ConsumerListener005, Function | MediumTest | Level2)
{
    GSError ret = surface_->UnregisterConsumerListener();
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: UnregisterConsumerListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call UnregisterConsumerListener with consumer_ is normal
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, ConsumerListener006, Function | MediumTest | Level2)
{
    sptr<BufferQueue> queue = new BufferQueue("test");
    surface_->consumer_ = new BufferQueueConsumer(queue);
    ASSERT_NE(surface_->consumer_, nullptr);
    GSError ret = surface_->UnregisterConsumerListener();
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: RegisterReleaseListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RegisterReleaseListener with nullptr param
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, ReleaseListener001, Function | MediumTest | Level2)
{
    OnReleaseFunc onBufferRelease = nullptr;
    GSError ret = surface_->RegisterReleaseListener(onBufferRelease);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: RegisterReleaseListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RegisterReleaseListener with consumer_ is normal
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, ReleaseListener002, Function | MediumTest | Level2)
{
    sptr<BufferQueue> queue = new BufferQueue("test");
    surface_->consumer_ = new BufferQueueConsumer(queue);
    ASSERT_NE(surface_->consumer_, nullptr);

    OnReleaseFunc onBufferRelease = nullptr;
    GSError ret = surface_->RegisterReleaseListener(onBufferRelease);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: RegisterReleaseListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RegisterReleaseListener
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, ReleaseListener003, Function | MediumTest | Level2)
{
    OnReleaseFuncWithFence func = nullptr;
    GSError ret = surface_->RegisterReleaseListener(func);
    ASSERT_EQ(ret, OHOS::GSERROR_NOT_SUPPORT);
}

/*
 * Function: UnRegisterReleaseListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call UnRegisterReleaseListener
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, ReleaseListener004, Function | MediumTest | Level2)
{
    GSError ret = surface_->UnRegisterReleaseListener();
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: RegisterDeleteBufferListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RegisterDeleteBufferListener with consumer_ is nullptr
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, DeleteBufferListener001, Function | MediumTest | Level2)
{
    OnDeleteBufferFunc func = nullptr;
    GSError ret = surface_->RegisterDeleteBufferListener(func, false);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: RegisterDeleteBufferListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RegisterDeleteBufferListener with consumer_ is normal
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, DeleteBufferListener002, Function | MediumTest | Level2)
{
    sptr<BufferQueue> queue = new BufferQueue("test");
    surface_->consumer_ = new BufferQueueConsumer(queue);
    ASSERT_NE(surface_->consumer_, nullptr);

    OnDeleteBufferFunc func = nullptr;
    GSError ret = surface_->RegisterDeleteBufferListener(func, false);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);

    ret = surface_->RegisterDeleteBufferListener(func, true);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: RegisterDeleteBufferListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RegisterDeleteBufferListener with consumer_ is normal
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, DeleteBufferListener003, Function | MediumTest | Level2)
{
    sptr<BufferQueue> queue = new BufferQueue("test");
    surface_->consumer_ = new BufferQueueConsumer(queue);
    ASSERT_NE(surface_->consumer_, nullptr);
    OnDeleteBufferFunc func = deleteBuffer;
    GSError ret = surface_->RegisterDeleteBufferListener(func, false);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(surface_->hasRegistercallBackForRT_, true);
}

/*
 * Function: RegisterDeleteBufferListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RegisterDeleteBufferListener with consumer_ is normal
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, DeleteBufferListener004, Function | MediumTest | Level2)
{
    sptr<BufferQueue> queue = new BufferQueue("test");
    surface_->consumer_ = new BufferQueueConsumer(queue);
    ASSERT_NE(surface_->consumer_, nullptr);

    surface_->hasRegistercallBackForRT_ = true;
    OnDeleteBufferFunc func = deleteBuffer;
    GSError ret = surface_->RegisterDeleteBufferListener(func, false);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: RegisterDeleteBufferListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RegisterDeleteBufferListener with consumer_ is normal
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, DeleteBufferListener005, Function | MediumTest | Level2)
{
    sptr<BufferQueue> queue = new BufferQueue("test");
    surface_->consumer_ = new BufferQueueConsumer(queue);
    ASSERT_NE(surface_->consumer_, nullptr);

    OnDeleteBufferFunc func = deleteBuffer;
    GSError ret = surface_->RegisterDeleteBufferListener(func, true);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(surface_->hasRegistercallBackForRedraw_, true);
}

/*
 * Function: RegisterDeleteBufferListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RegisterDeleteBufferListener with consumer_ is normal
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, DeleteBufferListener006, Function | MediumTest | Level2)
{
    sptr<BufferQueue> queue = new BufferQueue("test");
    surface_->consumer_ = new BufferQueueConsumer(queue);
    ASSERT_NE(surface_->consumer_, nullptr);

    surface_->hasRegistercallBackForRedraw_ = true;
    OnDeleteBufferFunc func = deleteBuffer;
    GSError ret = surface_->RegisterDeleteBufferListener(func, true);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: RegisterUserDataChangeListener
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RegisterUserDataChangeListener with nullptr param
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, UserDataChangeListener001, Function | MediumTest | Level2)
{
    GSError ret = surface_->RegisterUserDataChangeListener("test", nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: GoBackground
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call GoBackground with consumer_ is nullptr
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, GoBackground001, Function | MediumTest | Level2)
{
    GSError ret = surface_->GoBackground();
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: GoBackground
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call GoBackground with consumer_ is normal
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, GoBackground002, Function | MediumTest | Level2)
{
    sptr<BufferQueue> queue = new BufferQueue("test");
    surface_->consumer_ = new BufferQueueConsumer(queue);
    ASSERT_NE(surface_->consumer_, nullptr);

    GSError ret = surface_->GoBackground();
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    surface_->producer_ = new BufferQueueProducer(queue);
    ASSERT_NE(surface_->producer_, nullptr);

    ret = surface_->GoBackground();
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: GetUniqueId
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call GetUniqueId with producer_ is nullptr
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, GetUniqueId001, Function | MediumTest | Level2)
{
    uint64_t ret = surface_->GetUniqueId();
    ASSERT_EQ(ret, 0);
}

/*
 * Function: Dump
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call Dump
 *                  2. check ret
 */
HWTEST_F(ConsumerSurfaceTest, Dump001, Function | MediumTest | Level2)
{
    std::string result;
    surface_->Dump(result);
    sptr<BufferQueue> queue = new BufferQueue("test");
    surface_->consumer_ = new BufferQueueConsumer(queue);
    ASSERT_NE(surface_->consumer_, nullptr);
    surface_->Dump(result);
}

/*
 * Function: SetScalingMode and GetScalingMode
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetScalingMode with abnormal parameters and check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetScalingMode001, Function | MediumTest | Level2)
{
    ScalingMode scalingMode = ScalingMode::SCALING_MODE_SCALE_TO_WINDOW;
    GSError ret = cs->SetScalingMode(-1, scalingMode);
    ASSERT_EQ(ret, OHOS::GSERROR_NO_ENTRY);

    ret = cs->GetScalingMode(-1, scalingMode);
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
HWTEST_F(ConsumerSurfaceTest, SetAndGetScalingMode002, Function | MediumTest | Level1)
{
    ScalingMode scalingMode = ScalingMode::SCALING_MODE_SCALE_TO_WINDOW;
    sptr<SurfaceBuffer> buffer;
    int releaseFence = -1;
    GSError ret = ps->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);

    uint32_t sequence = buffer->GetSeqNum();
    ret = cs->SetScalingMode(sequence, scalingMode);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ScalingMode scalingModeGet = ScalingMode::SCALING_MODE_FREEZE;
    ret = cs->GetScalingMode(sequence, scalingModeGet);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(scalingMode, scalingModeGet);

    ret = ps->CancelBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: SetScalingMode and GetScalingMode
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetScalingMode with abnormal parameters and check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetScalingMode003, Function | MediumTest | Level2)
{
    ScalingMode scalingMode = ScalingMode::SCALING_MODE_SCALE_TO_WINDOW;
    GSError ret = cs->SetScalingMode(scalingMode);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: SetScalingMode and GetScalingMode
 * Type: Function
 * Rank: Important(1)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetScalingMode with nullptr and check ret
 *                  2. call GetScalingMode with nullptr and check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetScalingMode004, Function | MediumTest | Level1)
{
    ScalingMode scalingMode = ScalingMode::SCALING_MODE_SCALE_TO_WINDOW;
    GSError ret = surface_->SetScalingMode(firstSeqnum, scalingMode);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);

    ret = surface_->SetScalingMode(scalingMode);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);

    ret = surface_->GetScalingMode(firstSeqnum, scalingMode);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: QueryMetaDataType
 * Type: Function
 * Rank: Important(1)
 * EnvConditions: N/A
 * CaseDescription: 1. call QueryMetaDataType and check ret
 */
HWTEST_F(ConsumerSurfaceTest, MetaDataType001, Function | MediumTest | Level1)
{
    HDRMetaDataType type = HDRMetaDataType::HDR_META_DATA;
    GSError ret = cs->QueryMetaDataType(firstSeqnum, type);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(type, HDRMetaDataType::HDR_NOT_USED);
}

/*
 * Function: QueryMetaDataType
 * Type: Function
 * Rank: Important(1)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetMetaData with normal parameters and check ret
 *                  2. call QueryMetaDataType and check ret
 */
HWTEST_F(ConsumerSurfaceTest, MetaDataType002, Function | MediumTest | Level1)
{
    std::vector<GraphicHDRMetaData> metaData;
    GraphicHDRMetaData data = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .value = 1,
    };
    metaData.push_back(data);
    GSError ret = cs->SetMetaData(firstSeqnum, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    HDRMetaDataType type = HDRMetaDataType::HDR_NOT_USED;
    ret = cs->QueryMetaDataType(firstSeqnum, type);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(type, HDRMetaDataType::HDR_META_DATA);
}

/*
 * Function: QueryMetaDataType
 * Type: Function
 * Rank: Important(1)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetMetaDataSet with normal parameters and check ret
 *                  2. call QueryMetaDataType and check ret
 */
HWTEST_F(ConsumerSurfaceTest, MetaDataType003, Function | MediumTest | Level1)
{
    GraphicHDRMetadataKey key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_HDR10_PLUS;
    std::vector<uint8_t> metaData;
    uint8_t data = 1;
    metaData.push_back(data);
    GSError ret = cs->SetMetaDataSet(firstSeqnum, key, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    HDRMetaDataType type = HDRMetaDataType::HDR_NOT_USED;
    ret = cs->QueryMetaDataType(firstSeqnum, type);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(type, HDRMetaDataType::HDR_META_DATA_SET);
}

/*
 * Function: QueryMetaDataType
 * Type: Function
 * Rank: Important(1)
 * EnvConditions: N/A
 * CaseDescription: 1. call QueryMetaDataType with nullptr and check ret
 */
HWTEST_F(ConsumerSurfaceTest, MetaDataType004, Function | MediumTest | Level1)
{
    HDRMetaDataType type = HDRMetaDataType::HDR_META_DATA;
    GSError ret = surface_->QueryMetaDataType(firstSeqnum, type);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: GetHdrWhitePointBrightness and GetSdrWhitePointBrightness
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call GetHdrWhitePointBrightness with nullptr and check ret
 *                  2. call GetSdrWhitePointBrightness with nullptr and check ret
 */
HWTEST_F(ConsumerSurfaceTest, GetHdrWhitePointBrightness001, Function | MediumTest | Level2)
{
    float ret = surface_->GetHdrWhitePointBrightness();
    ASSERT_EQ(static_cast<int32_t>(ret), 0);
    ret = surface_->GetSdrWhitePointBrightness();
    ASSERT_EQ(static_cast<int32_t>(ret), 0);
}

/*
 * Function: SetMetaData and GetMetaData
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetMetaData with abnormal parameters and check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetMetaData001, Function | MediumTest | Level2)
{
    std::vector<GraphicHDRMetaData> metaData;
    GSError ret = cs->SetMetaData(firstSeqnum, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: SetMetaData and GetMetaData
 * Type: Function
 * Rank: Important(1)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetMetaData with normal parameters and check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetMetaData002, Function | MediumTest | Level1)
{
    std::vector<GraphicHDRMetaData> metaData;
    GraphicHDRMetaData data = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .value = 100,  // for test
    };
    metaData.push_back(data);
    GSError ret = cs->SetMetaData(firstSeqnum, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: SetMetaData and GetMetaData
 * Type: Function
 * Rank: Important(1)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetMetaData with normal parameters and check ret
 *                  2. call GetMetaData and check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetMetaData003, Function | MediumTest | Level1)
{
    std::vector<GraphicHDRMetaData> metaData;
    GraphicHDRMetaData data = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .value = 100,  // for test
    };
    metaData.push_back(data);

    sptr<SurfaceBuffer> buffer;
    int releaseFence = -1;
    GSError ret = ps->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);

    uint32_t sequence = buffer->GetSeqNum();
    ret = cs->SetMetaData(sequence, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    std::vector<GraphicHDRMetaData> metaDataGet;
    ret = cs->GetMetaData(sequence, metaDataGet);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(metaData[0].key, metaDataGet[0].key);
    ASSERT_EQ(metaData[0].value, metaDataGet[0].value);

    ret = cs->GetMetaData(sequence + 1, metaDataGet);
    ASSERT_EQ(ret, OHOS::GSERROR_NO_ENTRY);

    ret = ps->CancelBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: SetMetaData and GetMetaData
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetMetaData and GetMetaData with nullptr and check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetMetaData004, Function | MediumTest | Level2)
{
    std::vector<GraphicHDRMetaData> metaData;
    GSError ret = surface_->SetMetaData(firstSeqnum, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ret = surface_->GetMetaData(firstSeqnum, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: SetMetaDataSet and GetMetaDataSet
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetMetaDataSet with abnormal parameters and check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetMetaDataSet001, Function | MediumTest | Level2)
{
    GraphicHDRMetadataKey key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_HDR10_PLUS;
    std::vector<uint8_t> metaData;
    GSError ret = cs->SetMetaDataSet(firstSeqnum, key, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: SetMetaDataSet and GetMetaDataSet
 * Type: Function
 * Rank: Important(1)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetMetaDataSet with normal parameters and check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetMetaDataSet002, Function | MediumTest | Level1)
{
    GraphicHDRMetadataKey key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_HDR10_PLUS;
    std::vector<uint8_t> metaData;
    uint8_t data = 10;  // for test
    metaData.push_back(data);
    GSError ret = cs->SetMetaDataSet(firstSeqnum, key, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: SetMetaDataSet and GetMetaDataSet
 * Type: Function
 * Rank: Important(1)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetMetaDataSet with normal parameters and check ret
 *                  2. call GetMetaDataSet and check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetMetaDataSet003, Function | MediumTest | Level1)
{
    GraphicHDRMetadataKey key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_HDR10_PLUS;
    std::vector<uint8_t> metaData;
    uint8_t data = 10;  // for test
    metaData.push_back(data);

    sptr<SurfaceBuffer> buffer;
    int releaseFence = -1;
    GSError ret = ps->RequestBuffer(buffer, releaseFence, requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);

    uint32_t sequence = buffer->GetSeqNum();
    ret = cs->SetMetaDataSet(sequence, key, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    GraphicHDRMetadataKey keyGet = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X;
    std::vector<uint8_t> metaDataGet;
    ret = cs->GetMetaDataSet(sequence, keyGet, metaDataGet);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(key, keyGet);
    ASSERT_EQ(metaData[0], metaDataGet[0]);

    ret = cs->GetMetaDataSet(sequence + 1, keyGet, metaDataGet);
    ASSERT_EQ(ret, OHOS::GSERROR_NO_ENTRY);

    ret = ps->CancelBuffer(buffer);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: SetMetaDataSet and GetMetaDataSet
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetMetaDataSet and GetMetaDataSet with nullptr and check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetMetaDataSet004, Function | MediumTest | Level2)
{
    GraphicHDRMetadataKey key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_HDR10_PLUS;
    std::vector<uint8_t> metaData;
    GSError ret = surface_->SetMetaDataSet(firstSeqnum, key, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ret = surface_->GetMetaDataSet(firstSeqnum, key, metaData);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: SetTunnelHandle and GetTunnelHandle
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetTunnelHandle with abnormal parameters and check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetTunnelHandle001, Function | MediumTest | Level2)
{
    GraphicExtDataHandle *handle = nullptr;
    GSError ret = cs->SetTunnelHandle(handle);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: SetTunnelHandle and GetTunnelHandle
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetTunnelHandle with abnormal parameters and check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetTunnelHandle002, Function | MediumTest | Level2)
{
    GraphicExtDataHandle *handle = nullptr;
    handle = new GraphicExtDataHandle();
    handle->fd = -1;
    handle->reserveInts = 0;
    GSError ret = cs->SetTunnelHandle(handle);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
 * Function: SetTunnelHandle and GetTunnelHandle
 * Type: Function
 * Rank: Important(1)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetTunnelHandle with normal parameters and check ret
 *                  2. call GetTunnelHandle and check ret
* @tc.require: issueI5GMZN issueI5IWHW
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetTunnelHandle003, Function | MediumTest | Level1)
{
    GraphicExtDataHandle *handle = static_cast<GraphicExtDataHandle *>(
        malloc(sizeof(GraphicExtDataHandle) + sizeof(int32_t)));
    handle->fd = -1;
    handle->reserveInts = 1;
    handle->reserve[0] = 0;
    GSError ret = cs->SetTunnelHandle(handle);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = cs->SetTunnelHandle(handle);
    ASSERT_EQ(ret, OHOS::GSERROR_NO_ENTRY);

    sptr<SurfaceTunnelHandle> handleGet = nullptr;
    handleGet = cs->GetTunnelHandle();
    ASSERT_NE(handleGet, nullptr);
    ASSERT_EQ(handle->fd, handleGet->GetHandle()->fd);
    ASSERT_EQ(handle->reserveInts, handleGet->GetHandle()->reserveInts);
    ASSERT_EQ(handle->reserve[0], handleGet->GetHandle()->reserve[0]);
    free(handle);
}

/*
 * Function: SetTunnelHandle and GetTunnelHandle
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetTunnelHandle and GetTunnelHandle with nullptr and check ret
 */
HWTEST_F(ConsumerSurfaceTest, SetAndGetTunnelHandle004, Function | MediumTest | Level2)
{
    GraphicExtDataHandle *handle = nullptr;
    GSError ret = surface_->SetTunnelHandle(handle);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    sptr<SurfaceTunnelHandle> tunnelHandle = surface_->GetTunnelHandle();
    ASSERT_EQ(tunnelHandle, nullptr);
}
}
