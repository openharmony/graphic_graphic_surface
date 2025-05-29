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
#include <consumer_surface.h>
#include <native_window.h>
#include "buffer_consumer_listener.h"
#include "sync_fence.h"
#include <message_option.h>
#include <message_parcel.h>
#include "transact_surface_delegator_stub.h"
#include "consumer_surface.h"
#include "producer_surface_delegator.h"
#include "buffer_queue_producer.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class ProducerSurfaceDelegatorTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

    static inline sptr<IConsumerSurface> csurf = nullptr;
    static inline sptr<ProducerSurfaceDelegator> qwe = nullptr;
    static inline sptr<IBufferProducer> producer = nullptr;
    static inline sptr<Surface> pSurface = nullptr;
    static inline sptr<SurfaceBuffer> pBuffer = nullptr;
    static inline sptr<SurfaceBuffer> cBuffer = nullptr;
};

class IRemoteObjectMocker : public IRemoteObject {
public:
    IRemoteObjectMocker() : IRemoteObject{u"IRemoteObjectMocker"} {}
    ~IRemoteObjectMocker() {}
    int32_t GetObjectRefCount() { return 0; }

    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
    {
        return 0;
    }

    bool IsProxyObject() const
    {
        return true;
    }

    bool CheckObjectLegality() const
    {
        return true;
    }

    bool AddDeathRecipient(const sptr<DeathRecipient> &recipient)
    {
        return true;
    }

    bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient)
    {
        return true;
    }

    sptr<IRemoteBroker> AsInterface()
    {
        return nullptr;
    }

    int Dump(int fd, const std::vector<std::u16string> &args)
    {
        return 0;
    }
};

void ProducerSurfaceDelegatorTest::SetUpTestCase()
{
    csurf = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    csurf->RegisterConsumerListener(listener);
    producer = csurf->GetProducer();
    pSurface = Surface::CreateSurfaceAsProducer(producer);
    qwe = new ProducerSurfaceDelegator();
    sptr<IRemoteObjectMocker> remoteObjectMocker = new IRemoteObjectMocker();
    qwe->SetClient(remoteObjectMocker);
    pBuffer = SurfaceBuffer::Create();
}

void ProducerSurfaceDelegatorTest::TearDownTestCase()
{
    pSurface->UnRegisterReleaseListener();
    qwe = nullptr;
    csurf = nullptr;
    producer = nullptr;
    pSurface = nullptr;
    pBuffer = nullptr;
}

/*
* Function: QueueBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call QueueBuffer
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, QueueBuffer002, TestSize.Level0)
{
    int32_t slot = 1;
    int32_t acquireFence = 3;
    sptr<Surface> aSurface = Surface::CreateSurfaceAsProducer(producer);
    qwe->SetSurface(aSurface);
    GSError ret = qwe->QueueBuffer(slot, acquireFence);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: DequeueBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call DequeueBuffer
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, DequeueBuffer002, TestSize.Level0)
{
    int32_t slot = 1;
    qwe->SetSurface(pSurface);
    GSError ret = qwe->DequeueBuffer(slot, pBuffer);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: QueueBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call QueueBuffer
*                  2. check SendMessage ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, QueueBuffer003, TestSize.Level0)
{
    int32_t slot = 1;
    int32_t acquireFence = 3;
    qwe->SetSurface(pSurface);
    GSError ret = qwe->QueueBuffer(slot, acquireFence);
    ASSERT_EQ(ret, GSERROR_OK);
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
    sptr<SyncFence> fence = SyncFence::INVALID_FENCE;
    GSError ret = qwe->ReleaseBuffer(pBuffer, fence);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: DetachBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call DetachBuffer
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, DetachBuffer001, TestSize.Level0)
{
    int32_t slot = -1;
    GSError ret = qwe->DetachBuffer(slot);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: CancelBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call CancelBuffer
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, CancelBuffer001, TestSize.Level0)
{
    int32_t slot = -1;
    int32_t fenceFd = -1;
    GSError ret = qwe->CancelBuffer(slot, fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: ClearBufferSlot
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call ClearBufferSlot
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, ClearBufferSlot001, TestSize.Level0)
{
    int32_t slot = -1;
    GSError ret = qwe->ClearBufferSlot(slot);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: OnRemoteRequest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OnRemoteRequest
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, OnRemoteRequest001, TestSize.Level0)
{
    uint32_t code = 1; // QUEUEBUFFER
    MessageParcel reply;
    MessageOption option;
    MessageParcel dataQueue;
    dataQueue.WriteInt32(10);
    dataQueue.WriteFileDescriptor(20);
    int ret2 = qwe->OnRemoteRequest(code, dataQueue, reply, option);
    ASSERT_EQ(ret2, ERR_NONE);
}

/*
* Function: ClearAllBuffers
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call ClearAllBuffers
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, ClearAllBuffers001, TestSize.Level0)
{
    GSError ret = qwe->ClearAllBuffers();
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: AddBufferLocked
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call AddBufferLocked
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, AddBufferLocked001, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE({
        qwe->AddBufferLocked(nullptr, 0);
    });
}

/*
* Function: GetBufferLocked
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetBufferLocked
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, GetBufferLocked001, TestSize.Level0)
{
    ASSERT_EQ(qwe->GetBufferLocked(0), nullptr);
}

/*
* Function: GetSlotLocked
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetSlotLocked
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, GetSlotLocked001, TestSize.Level0)
{
    ASSERT_EQ(qwe->GetSlotLocked(nullptr), 0);
}

/*
* Function: RetryFlushBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RetryFlushBuffer
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, RetryFlushBuffer001, TestSize.Level0)
{
    BufferFlushConfig config = {
        .damage = {
            .x = 0,
            .y = 0,
            .w = 0,
            .h = 0,
        },
        .timestamp = 0
    };
    GSError ret = qwe->RetryFlushBuffer(pBuffer, 0, config);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: OnSetDataspace
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OnSetDataspace
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, SetDataspace001, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteUint32(1);
    ASSERT_EQ(qwe->OnSetDataspace(data, reply), 0);
}

/*
* Function: HasSlotInSet
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call HasSlotInSet
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, HasSlotInSet, TestSize.Level0)
{
    sptr<ProducerSurfaceDelegator> delegator = new ProducerSurfaceDelegator();
    bool ret = delegator->HasSlotInSet(0);
    ASSERT_EQ(ret, false);
    delegator->InsertSlotIntoSet(0);
    ret = delegator->HasSlotInSet(0);
    ASSERT_EQ(ret, true);
}

/*
* Function: InsertSlotIntoSet
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call InsertSlotIntoSet
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, InsertSlotIntoSet, TestSize.Level0)
{
    sptr<ProducerSurfaceDelegator> delegator = new ProducerSurfaceDelegator();
    delegator->InsertSlotIntoSet(0);
    delegator->InsertSlotIntoSet(1);
    ASSERT_EQ(delegator->HasSlotInSet(0), true);
    ASSERT_EQ(delegator->HasSlotInSet(1), true);
    ASSERT_EQ(delegator->HasSlotInSet(2), false);
}

/*
* Function: EraseSlotFromSet
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call EraseSlotFromSet
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, EraseSlotFromSet, TestSize.Level0)
{
    sptr<ProducerSurfaceDelegator> delegator = new ProducerSurfaceDelegator();
    delegator->InsertSlotIntoSet(0);
    ASSERT_EQ(delegator->HasSlotInSet(0), true);
    delegator->EraseSlotFromSet(0);
    ASSERT_EQ(delegator->HasSlotInSet(0), false);
}

/*
* Function: OnNdkFlushBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OnNdkFlushBuffer
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, OnNdkFlushBuffer, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    ASSERT_EQ(qwe->OnNdkFlushBuffer(data, reply), GSERROR_OK);
}

/*
* Function: OnQueueBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OnQueueBuffer
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, OnQueueBuffer, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    ASSERT_EQ(qwe->OnQueueBuffer(data, reply), 0);
}

/*
* Function: NdkFlushBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call NdkFlushBuffer
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, NdkFlushBuffer, TestSize.Level0)
{
    sptr<SyncFence> fence = SyncFence::INVALID_FENCE;
    ASSERT_EQ(qwe->NdkFlushBuffer(pBuffer, 0, fence), GSERROR_OK);
}

/*
* Function: NdkConvertBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call NdkConvertBuffer
*                  2. check ret
*/
HWTEST_F(ProducerSurfaceDelegatorTest, NdkConvertBuffer, TestSize.Level0)
{
    MessageParcel data;
    ASSERT_EQ(qwe->NdkConvertBuffer(data, 0, 0), nullptr);
}
} // namespace OHOS::Rosen
