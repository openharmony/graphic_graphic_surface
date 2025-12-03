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

#include <buffer_extra_data_impl.h>
#include <gtest/gtest.h>
#include <message_option.h>
#include <message_parcel.h>
#include <native_window.h>
#include <securec.h>
#include <surface.h>

#include "buffer_queue_producer.h"
#include "delegator_mock.h"
#define PRIVATE PUBLIC
#include "buffer_queue.h"
#include "delegator_adapter.h"
#undef PRIVATE
#include "consumer_surface.h"
#include "consumer_surface_delegator.h"
#include "iconsumer_surface.h"
#include "ibuffer_consumer_listener.h"
#include "producer_surface.h"
#include "remote_object_mock.h"
#include "sync_fence.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class ConsumerSurfaceDelegatorTest : public testing::Test {
public:
    void SetUp();
    void TearDown();

    static inline sptr<BufferExtraData> bedata = nullptr;
    static inline sptr<SurfaceBuffer> buffer = nullptr;
    static inline BufferRequestConfig requestConfig = {
        .width = 0x100,
        .height = 0x100,
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
};

void ConsumerSurfaceDelegatorTest::SetUp()
{
    bedata = new OHOS::BufferExtraDataImpl;
    buffer = SurfaceBuffer::Create();
    DelegatorAdapter::GetInstance().funcMap_.clear();
}

void ConsumerSurfaceDelegatorTest::TearDown()
{
    bedata = nullptr;
    buffer = nullptr;
    DelegatorAdapter::GetInstance().funcMap_.clear();
}

/*
* Function: QueueBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call QueueBuffer
*                  2. check ret
 */
HWTEST_F(ConsumerSurfaceDelegatorTest, QueueBuffer, TestSize.Level0)
{
    sptr<ConsumerSurfaceDelegator> consumerDelegator = ConsumerSurfaceDelegator::Create();
    int32_t fenceFd = 3;
    sptr<SurfaceBuffer> bufferTemp = nullptr;
    GSError ret = consumerDelegator->QueueBuffer(bufferTemp, fenceFd);
    ASSERT_EQ(ret, GSERROR_INVALID_ARGUMENTS);

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::CONSUMER_QUEUE_BUFFER_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorQueueBuffer);
    consumerDelegator->mDelegator_ = 0x123456;
    GSError ret1 = consumerDelegator->QueueBuffer(buffer, fenceFd);
    ASSERT_EQ(ret1, GSERROR_OK);

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::CONSUMER_QUEUE_BUFFER_FUNC] = nullptr;
    GSError ret2 = consumerDelegator->QueueBuffer(buffer, fenceFd);
    ASSERT_EQ(ret2, GSERROR_BINDER);

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::CONSUMER_QUEUE_BUFFER_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorQueueBuffer);
    consumerDelegator->mDelegator_ = 0;
    GSError ret3 = consumerDelegator->QueueBuffer(buffer, fenceFd);
    ASSERT_EQ(ret3, GSERROR_BINDER);
    consumerDelegator = nullptr;
}

/*
* Function: QueueBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call QueueBuffer
*                  2. check ret
 */
HWTEST_F(ConsumerSurfaceDelegatorTest, QueueBuffer001, TestSize.Level0)
{
    sptr<ConsumerSurfaceDelegator> consumerDelegator = ConsumerSurfaceDelegator::Create();
    int32_t fenceFd = 3;

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::CONSUMER_QUEUE_BUFFER_FUNC] = nullptr;
    GSError ret2 = consumerDelegator->QueueBuffer(buffer, fenceFd);
    ASSERT_EQ(ret2, GSERROR_BINDER);

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::CONSUMER_QUEUE_BUFFER_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorQueueBuffer);
    consumerDelegator->mDelegator_ = 0;
    GSError ret3 = consumerDelegator->QueueBuffer(buffer, fenceFd);
    ASSERT_EQ(ret3, GSERROR_BINDER);
    consumerDelegator = nullptr;
}

/*
* Function: DequeueBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call DequeueBuffer
*                  2. check ret
 */
HWTEST_F(ConsumerSurfaceDelegatorTest, DequeueBuffer, TestSize.Level0)
{
    g_dlopenNull = true;
    sptr<ConsumerSurfaceDelegator> consumerDelegator = ConsumerSurfaceDelegator::Create();
    IBufferProducer::RequestBufferReturnValue retval;

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::CONSUMER_DEQUEUE_BUFFER_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorDequeueBuffer);
    consumerDelegator->mDelegator_ = 0x123456;
    GSError ret1 = consumerDelegator->DequeueBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret1, GSERROR_OK);

    DelegatorAdapter::GetInstance().funcMap_.erase(FunctionFlags::CONSUMER_DEQUEUE_BUFFER_FUNC);
    GSError ret2 = consumerDelegator->DequeueBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret2, GSERROR_BINDER);
    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::CONSUMER_DEQUEUE_BUFFER_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorDequeueBuffer);

    consumerDelegator->mDelegator_ = 0;
    GSError ret3 = consumerDelegator->DequeueBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret3, GSERROR_BINDER);
    consumerDelegator = nullptr;
    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::CONSUMER_DEQUEUE_BUFFER_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorDequeueBuffer);
}

/*
* Function: DequeueBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call DequeueBuffer
*                  2. check ret
 */
HWTEST_F(ConsumerSurfaceDelegatorTest, DequeueBuffer001, TestSize.Level0)
{
    sptr<ConsumerSurfaceDelegator> consumerDelegator = ConsumerSurfaceDelegator::Create();
    IBufferProducer::RequestBufferReturnValue retval;

    DelegatorAdapter::GetInstance().funcMap_.erase(FunctionFlags::CONSUMER_DEQUEUE_BUFFER_FUNC);
    GSError ret2 = consumerDelegator->DequeueBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret2, GSERROR_BINDER);
    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::CONSUMER_DEQUEUE_BUFFER_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorDequeueBuffer);

    consumerDelegator->mDelegator_ = 0;
    GSError ret3 = consumerDelegator->DequeueBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret3, GSERROR_BINDER);
    consumerDelegator = nullptr;
    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::CONSUMER_DEQUEUE_BUFFER_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorDequeueBuffer);
}

/*
* Function: DequeueBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call DequeueBuffer
*                  2. check ret
 */
HWTEST_F(ConsumerSurfaceDelegatorTest, DequeueBuffer002, TestSize.Level0)
{
    sptr<ConsumerSurfaceDelegator> consumerDelegator = ConsumerSurfaceDelegator::Create();
    IBufferProducer::RequestBufferReturnValue retval;

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::CONSUMER_DEQUEUE_BUFFER_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorDequeueBuffer);
    consumerDelegator->mDelegator_ = 0x123456;
    GSError ret3 = consumerDelegator->DequeueBuffer(requestConfig, bedata, retval);
    ASSERT_NE(ret3, GSERROR_BINDER);
    consumerDelegator = nullptr;
}

/*
* Function: SetClient
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetClient
*                  2. check ret
 */
HWTEST_F(ConsumerSurfaceDelegatorTest, SetClient, TestSize.Level0)
{
    sptr<ConsumerSurfaceDelegator> consumerDelegator = ConsumerSurfaceDelegator::Create();
    sptr<IRemoteObject> clientTemp = nullptr;
    bool ret = consumerDelegator->SetClient(clientTemp);
    ASSERT_EQ(ret, false);

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::SET_CONSUMER_CLIENT_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorSetClient);
    consumerDelegator->mDelegator_ = 0x123456;
    bool ret1 = consumerDelegator->SetClient(new OHOS::Rosen::IRemoteObjectMocker());
    ASSERT_NE(ret1, false);

    consumerDelegator->mDelegator_ = 0;
    ret1 = consumerDelegator->SetClient(new OHOS::Rosen::IRemoteObjectMocker());
    ASSERT_EQ(ret1, false);

    DelegatorAdapter::GetInstance().funcMap_.erase(FunctionFlags::SET_CONSUMER_CLIENT_FUNC);
    ret1 = consumerDelegator->SetClient(new OHOS::Rosen::IRemoteObjectMocker());
    ASSERT_EQ(ret1, false);
    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::SET_CONSUMER_CLIENT_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorSetClient);
}

/*
* Function: SetClient001
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetClient001
*                  2. check ret
 */
HWTEST_F(ConsumerSurfaceDelegatorTest, SetClient001, TestSize.Level0)
{
    sptr<ConsumerSurfaceDelegator> consumerDelegator = ConsumerSurfaceDelegator::Create();
    sptr<IRemoteObject> clientTemp = nullptr;
    bool ret = consumerDelegator->SetClient(clientTemp);
    ASSERT_EQ(ret, false);

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::SET_CONSUMER_CLIENT_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorSetClient);
    consumerDelegator->mDelegator_ = 0;
    bool ret1 = consumerDelegator->SetClient(new OHOS::Rosen::IRemoteObjectMocker());
    ASSERT_EQ(ret1, false);

    DelegatorAdapter::GetInstance().funcMap_.erase(FunctionFlags::SET_CONSUMER_CLIENT_FUNC);
    ret1 = consumerDelegator->SetClient(new OHOS::Rosen::IRemoteObjectMocker());
    ASSERT_EQ(ret1, false);
    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::SET_CONSUMER_CLIENT_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorSetClient);
}

/*
* Function: SetSurface
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetSurface
*                  2. check ret
 */
HWTEST_F(ConsumerSurfaceDelegatorTest, SetSurface, TestSize.Level0)
{
    sptr<ConsumerSurfaceDelegator> consumerDelegator = ConsumerSurfaceDelegator::Create();
    sptr<Surface> surface = nullptr;
    ASSERT_NO_FATAL_FAILURE({consumerDelegator->SetSurface(surface);});

    sptr<IConsumerSurface> csurf = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    csurf->RegisterConsumerListener(listener);
    sptr<IBufferProducer> producer = csurf->GetProducer();
    surface = new ProducerSurface(producer);
    ASSERT_NO_FATAL_FAILURE({consumerDelegator->SetSurface(surface);});
    consumerDelegator = nullptr;
}

/*
* Function: SetSurface
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetSurface
*                  2. check ret
 */
HWTEST_F(ConsumerSurfaceDelegatorTest, SetSurface001, TestSize.Level0)
{
    g_dlopenNull = true;
    sptr<ConsumerSurfaceDelegator> consumerDelegator = ConsumerSurfaceDelegator::Create();

    sptr<IConsumerSurface> csurf = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    csurf->RegisterConsumerListener(listener);
    sptr<IBufferProducer> producer = csurf->GetProducer();
    sptr<Surface> surface = new ProducerSurface(producer);
    ASSERT_NO_FATAL_FAILURE({consumerDelegator->SetSurface(surface);});

    consumerDelegator->mDelegator_ = 0;
    ASSERT_NO_FATAL_FAILURE({consumerDelegator->SetSurface(surface);});
    consumerDelegator = nullptr;
}

/*
* Function: SetSurface
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetSurface
*                  2. check ret
 */
HWTEST_F(ConsumerSurfaceDelegatorTest, SetSurface002, TestSize.Level0)
{
    sptr<ConsumerSurfaceDelegator> consumerDelegator = ConsumerSurfaceDelegator::Create();
    sptr<IConsumerSurface> csurf = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    csurf->RegisterConsumerListener(listener);
    sptr<IBufferProducer> producer = csurf->GetProducer();
    sptr<Surface> surface = new ProducerSurface(producer);

    auto &delegatorAdapter = DelegatorAdapter::GetInstance();
    delegatorAdapter.funcMap_[FunctionFlags::SET_CONSUMER_SURFACE_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorSetSurface);
    consumerDelegator->mDelegator_ = 0;
    ASSERT_NO_FATAL_FAILURE({consumerDelegator->SetSurface(surface);});

    delegatorAdapter.funcMap_.erase(FunctionFlags::SET_CONSUMER_SURFACE_FUNC);
    ASSERT_NO_FATAL_FAILURE({consumerDelegator->SetSurface(surface);});
}

/*
* Function: SetSurface
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetSurface
*                  2. check ret
 */
HWTEST_F(ConsumerSurfaceDelegatorTest, SetSurface003, TestSize.Level0)
{
    BufferRequestConfig requestConfig = {
        .width = 0x100,
        .height = 0x100,
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    sptr<BufferQueue> bq = new BufferQueue("test");
    IBufferProducer::RequestBufferReturnValue retval;
    sptr<BufferExtraData> bedata = new OHOS::BufferExtraDataImpl();

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::SET_CONSUMER_CLIENT_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorSetClient);
    sptr<Surface> surface = nullptr;
    sptr<IRemoteObjectMocker> remoteObjectMocker = new IRemoteObjectMocker();
    auto ret = bq->RegisterSurfaceDelegator(remoteObjectMocker, surface);
    sptr<ConsumerSurfaceDelegator> consumerDelegator = ConsumerSurfaceDelegator::Create();
    bq->sptrCSurfaceDelegator_ = consumerDelegator;
    ret = bq->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_NE(ret, GSERROR_OK);
}

/*
* Function: ~ConsumerSurfaceDelegator
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call ~ConsumerSurfaceDelegator()
*                  2. check ret
 */
HWTEST_F(ConsumerSurfaceDelegatorTest, ConsumerSurfaceDelegator, TestSize.Level0)
{
    ConsumerSurfaceDelegator* surfaceDelegator = new ConsumerSurfaceDelegator();
    ASSERT_NE(surfaceDelegator, nullptr);
    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::CONSUMER_DESTROY_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorDestroy);
    surfaceDelegator->mDelegator_ = 0x1234566;
    ASSERT_NO_FATAL_FAILURE({delete(surfaceDelegator);});

    ConsumerSurfaceDelegator* surfaceDelegator1 = new ConsumerSurfaceDelegator();
    ASSERT_NE(surfaceDelegator1, nullptr);
    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::CONSUMER_DESTROY_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorDestroy);
    surfaceDelegator1->mDelegator_ = 0;
    ASSERT_NO_FATAL_FAILURE({delete(surfaceDelegator1);});

    ConsumerSurfaceDelegator* surfaceDelegator2 = new ConsumerSurfaceDelegator();
    ASSERT_NE(surfaceDelegator2, nullptr);
    DelegatorAdapter::GetInstance().funcMap_.erase(FunctionFlags::CONSUMER_DESTROY_FUNC);
    surfaceDelegator2->mDelegator_ = 0;
    ASSERT_NO_FATAL_FAILURE({delete(surfaceDelegator2);});
    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::CONSUMER_DESTROY_FUNC] =
        reinterpret_cast<void *>(MockConsumerSurfaceDelegatorDestroy);
}

}
