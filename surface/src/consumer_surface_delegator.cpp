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

#include <message_option.h>
#include <message_parcel.h>
#include "consumer_surface_delegator.h"
#include "buffer_log.h"
#include "sync_fence.h"
#include "delegator_adapter.h"

namespace OHOS {
sptr<ConsumerSurfaceDelegator> ConsumerSurfaceDelegator::Create()
{
    return sptr<ConsumerSurfaceDelegator>(new ConsumerSurfaceDelegator());
}

ConsumerSurfaceDelegator::ConsumerSurfaceDelegator()
{
    auto& delegatorAdapter = DelegatorAdapter::GetInstance();
    auto consumerDelegator = delegatorAdapter.GetFunc<ConsumerDelegatorFunc>(
        FunctionFlags::CONSUMER_CREATE_FUNC);
    if (consumerDelegator != nullptr) {
        mDelegator_ = reinterpret_cast<uintptr_t>(consumerDelegator());
    }
}

ConsumerSurfaceDelegator::~ConsumerSurfaceDelegator()
{
    auto& delegatorAdapter = DelegatorAdapter::GetInstance();
    auto consumerDelegatorDestroy = delegatorAdapter.GetFunc<ConsumerDestroyFunc>(
        FunctionFlags::CONSUMER_DESTROY_FUNC);
    if (consumerDelegatorDestroy != nullptr && mDelegator_ != 0) {
        consumerDelegatorDestroy(mDelegator_);
    }
}

void ConsumerSurfaceDelegator::SetSurface(sptr<Surface> surface)
{
    if (surface == nullptr) {
        BLOGE("surface is nullptr");
        return;
    }
    auto& delegatorAdapter = DelegatorAdapter::GetInstance();
    auto setSurfaceFunc = delegatorAdapter.GetFunc<SetConsumerSurfaceFunc>(
        FunctionFlags::SET_CONSUMER_SURFACE_FUNC);
    if (setSurfaceFunc != nullptr && mDelegator_ != 0) {
        setSurfaceFunc(mDelegator_, surface);
    }
}

bool ConsumerSurfaceDelegator::SetClient(sptr<IRemoteObject> client)
{
    if (client == nullptr) {
        BLOGE("client is nullptr");
        return false;
    }
    auto& delegatorAdapter = DelegatorAdapter::GetInstance();
    auto setClientFunc = delegatorAdapter.GetFunc<SetConsumerClientFunc>(
        FunctionFlags::SET_CONSUMER_CLIENT_FUNC);
    if (setClientFunc != nullptr && mDelegator_ != 0) {
        return setClientFunc(mDelegator_, client);
    }
    BLOGE("remote SetClient is nullptr");
    return false;
}

GSError ConsumerSurfaceDelegator::DequeueBuffer(const BufferRequestConfig& config, sptr<BufferExtraData>& bedata,
                                                struct IBufferProducer::RequestBufferReturnValue& retval)
{
    auto& delegatorAdapter = DelegatorAdapter::GetInstance();
    auto dequeueBufferFunc = delegatorAdapter.GetFunc<ConsumerDeQueueBuffer>(
        FunctionFlags::CONSUMER_DEQUEUE_BUFFER_FUNC);
    if (dequeueBufferFunc != nullptr && mDelegator_ != 0) {
        return dequeueBufferFunc(mDelegator_, config, bedata, retval);
    }
    BLOGE("%{public}s error, DequeueBufferFunc:%{public}d mDelegator:%{public}d",
        __func__, dequeueBufferFunc != nullptr, mDelegator_ != 0);
    return GSERROR_BINDER;
}

GSError ConsumerSurfaceDelegator::QueueBuffer(sptr<SurfaceBuffer>& buffer, int32_t fenceFd)
{
    if (buffer == nullptr) {
        BLOGE("buffer is nullptr");
        return GSERROR_INVALID_ARGUMENTS;
    }
    auto& delegatorAdapter = DelegatorAdapter::GetInstance();
    auto queueBufferFunc = delegatorAdapter.GetFunc<ConsumerQueueBuffer>(
        FunctionFlags::CONSUMER_QUEUE_BUFFER_FUNC);
    if (queueBufferFunc != nullptr && mDelegator_ != 0) {
        return queueBufferFunc(mDelegator_, buffer, fenceFd);
    }
    BLOGE("%{public}s error, QueueBufferFunc:%{public}d mDelegator:%{public}d",
        __func__, queueBufferFunc != nullptr, mDelegator_ != 0);
    return GSERROR_BINDER;
}

} // namespace OHOS