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
#include <consumer_surface.h>
#include <gtest/gtest.h>
#include <message_option.h>
#include <message_parcel.h>
#include <native_window.h>
#include <securec.h>
#include <surface.h>

#include "buffer_consumer_listener.h"
#include "buffer_queue_producer.h"
#include "consumer_surface.h"
#include "delegator_mock.h"
#include "iconsumer_surface.h"
#include "ibuffer_consumer_listener.h"
#include "producer_surface_delegator.h"
#include "producer_surface.h"
#include "remote_object_mock.h"
#include "sync_fence.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class ProducerSurfaceDelegatorTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
};

void ProducerSurfaceDelegatorTest::SetUpTestCase()
{
}

void ProducerSurfaceDelegatorTest::TearDownTestCase()
{
}

/*
 * Function: ReleaseBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call ReleaseBuffer
 *                  2. check ret
 */
HWTEST_F(ProducerSurfaceDelegatorTest, ReleaseBuffer, TestSize.Level0)
{
    g_dlopenNull = true;
    sptr<ProducerSurfaceDelegator> producerSurfaceDelegator = new ProducerSurfaceDelegator();

    sptr<SurfaceBuffer> pBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> fence = SyncFence::INVALID_FENCE;
    sptr<SurfaceBuffer> bufferTemp = nullptr;
    GSError ret = producerSurfaceDelegator->ReleaseBuffer(bufferTemp, fence);
    ASSERT_EQ(ret, GSERROR_INVALID_ARGUMENTS);

    sptr<SyncFence> fenceTemp = nullptr;
    ret = producerSurfaceDelegator->ReleaseBuffer(pBuffer, fenceTemp);
    ASSERT_EQ(ret, GSERROR_INVALID_ARGUMENTS);

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::PRODUCER_RELEASE_BUFFER_FUNC] =
        reinterpret_cast<void *>(MockProducerSurfaceDelegatorReleaseBuffer);
    producerSurfaceDelegator->mDelegator_ = 0x123465;
    ret = producerSurfaceDelegator->ReleaseBuffer(pBuffer, fence);
    ASSERT_EQ(ret, GSERROR_OK);

    DelegatorAdapter::GetInstance().funcMap_.erase(FunctionFlags::PRODUCER_RELEASE_BUFFER_FUNC);
    ret = producerSurfaceDelegator->ReleaseBuffer(pBuffer, fence);
    ASSERT_EQ(ret, GSERROR_BINDER);

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::PRODUCER_RELEASE_BUFFER_FUNC] =
        reinterpret_cast<void *>(MockProducerSurfaceDelegatorReleaseBuffer);
    producerSurfaceDelegator->mDelegator_ = 0;
    ret = producerSurfaceDelegator->ReleaseBuffer(pBuffer, fence);
    ASSERT_EQ(ret, GSERROR_BINDER);
}

/*
 * Function: ReleaseBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call ReleaseBuffer
 *                  2. check ret
 */
HWTEST_F(ProducerSurfaceDelegatorTest, ReleaseBuffer001, TestSize.Level0)
{
    g_dlopenNull = true;
    sptr<ProducerSurfaceDelegator> producerSurfaceDelegator = new ProducerSurfaceDelegator();

    sptr<SurfaceBuffer> pBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> fence = SyncFence::INVALID_FENCE;

    DelegatorAdapter::GetInstance().funcMap_.erase(FunctionFlags::PRODUCER_RELEASE_BUFFER_FUNC);
    auto ret = producerSurfaceDelegator->ReleaseBuffer(pBuffer, fence);
    ASSERT_EQ(ret, GSERROR_BINDER);

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::PRODUCER_RELEASE_BUFFER_FUNC] =
        reinterpret_cast<void *>(MockProducerSurfaceDelegatorReleaseBuffer);
    producerSurfaceDelegator->mDelegator_ = 0;
    ret = producerSurfaceDelegator->ReleaseBuffer(pBuffer, fence);
    ASSERT_EQ(ret, GSERROR_BINDER);
}

/*
 * Function: SetClient
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetClient
 *                  2. check ret
 */
HWTEST_F(ProducerSurfaceDelegatorTest, SetClient, TestSize.Level0)
{
    sptr<ProducerSurfaceDelegator> producerSurfaceDelegator = new ProducerSurfaceDelegator();
    sptr<IRemoteObject> clientTemp = nullptr;
    bool ret = producerSurfaceDelegator->SetClient(clientTemp);
    ASSERT_EQ(ret, false);

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::SET_PRODUCER_CLIENT_FUNC] =
        reinterpret_cast<void *>(MockProducerSurfaceDelegatorSetClient);
    producerSurfaceDelegator->mDelegator_ = 0x1128;

    ret = producerSurfaceDelegator->SetClient(new OHOS::Rosen::IRemoteObjectMocker());
    ASSERT_NE(ret, false);

    producerSurfaceDelegator->mDelegator_ = 0;
    ret = producerSurfaceDelegator->SetClient(new OHOS::Rosen::IRemoteObjectMocker());
    ASSERT_EQ(ret, false);
}

/*
 * Function: SetClient
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetClient
 *                  2. check ret
 */
HWTEST_F(ProducerSurfaceDelegatorTest, SetClient001, TestSize.Level0)
{
    sptr<ProducerSurfaceDelegator> producerSurfaceDelegator = new ProducerSurfaceDelegator();
    sptr<IRemoteObject> clientTemp = nullptr;

    DelegatorAdapter::GetInstance().funcMap_.erase(FunctionFlags::SET_PRODUCER_CLIENT_FUNC);
    bool ret = producerSurfaceDelegator->SetClient(new OHOS::Rosen::IRemoteObjectMocker());
    ASSERT_EQ(ret, false);

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::SET_PRODUCER_CLIENT_FUNC] =
        reinterpret_cast<void *>(MockProducerSurfaceDelegatorSetClient);
    producerSurfaceDelegator->mDelegator_ = 0;
    ret = producerSurfaceDelegator->SetClient(new OHOS::Rosen::IRemoteObjectMocker());
    ASSERT_EQ(ret, false);
}

/*
 * Function: SetSurface
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetSurface
 *                  2. check ret
 */
HWTEST_F(ProducerSurfaceDelegatorTest, SetSurface, TestSize.Level0)
{
    sptr<ProducerSurfaceDelegator> producerSurfaceDelegator = new ProducerSurfaceDelegator();
    sptr<Surface> surface = nullptr;
    ASSERT_NO_FATAL_FAILURE({producerSurfaceDelegator->SetSurface(surface);});

    sptr<IConsumerSurface> csurf = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    csurf->RegisterConsumerListener(listener);
    sptr<IBufferProducer> producer = csurf->GetProducer();
    surface = new ProducerSurface(producer);
    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::SET_PRODUCER_SURFACE_FUNC] =
        reinterpret_cast<void *>(MockProducerSurfaceDelegatorSetSurface);
    producerSurfaceDelegator->mDelegator_ = 0x1121;
    ASSERT_NO_FATAL_FAILURE({producerSurfaceDelegator->SetSurface(surface);});

    DelegatorAdapter::GetInstance().funcMap_.erase(FunctionFlags::SET_PRODUCER_SURFACE_FUNC);
    ASSERT_NO_FATAL_FAILURE({producerSurfaceDelegator->SetSurface(surface);});

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::SET_PRODUCER_SURFACE_FUNC] =
        reinterpret_cast<void *>(MockProducerSurfaceDelegatorSetSurface);
    producerSurfaceDelegator->mDelegator_ = 0;
    ASSERT_NO_FATAL_FAILURE({producerSurfaceDelegator->SetSurface(surface);});
}

/*
 * Function: ~ProducerSurfaceDelegator
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call ~ProducerSurfaceDelegator
 *                  2. check ret
 */
HWTEST_F(ProducerSurfaceDelegatorTest, ProducerSurfaceDelegator, TestSize.Level0)
{
    ProducerSurfaceDelegator* surfaceDelegator = new ProducerSurfaceDelegator();
    ASSERT_NE(surfaceDelegator, nullptr);
    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::PRODUCER_DESTROY_FUNC] =
        reinterpret_cast<void *>(MockProducerSurfaceDelegatorDestroy);
    surfaceDelegator->mDelegator_ = 0x1234566;
    ASSERT_NO_FATAL_FAILURE({delete(surfaceDelegator);});

    ProducerSurfaceDelegator* surfaceDelegator1 = new ProducerSurfaceDelegator();
    ASSERT_NE(surfaceDelegator1, nullptr);
    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::PRODUCER_DESTROY_FUNC] =
        reinterpret_cast<void *>(MockProducerSurfaceDelegatorDestroy);
    surfaceDelegator1->mDelegator_ = 0;
    ASSERT_NO_FATAL_FAILURE({delete(surfaceDelegator1);});

    ProducerSurfaceDelegator* surfaceDelegator2 = new ProducerSurfaceDelegator();
    ASSERT_NE(surfaceDelegator2, nullptr);
    DelegatorAdapter::GetInstance().funcMap_.erase(FunctionFlags::PRODUCER_DESTROY_FUNC);
    surfaceDelegator2->mDelegator_ = 0;
    ASSERT_NO_FATAL_FAILURE({delete(surfaceDelegator2);});

    DelegatorAdapter::GetInstance().funcMap_[FunctionFlags::PRODUCER_DESTROY_FUNC] =
        reinterpret_cast<void *>(MockProducerSurfaceDelegatorDestroy);
}

} // namespace OHOS::Rosen
