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
#include "producer_surface_delegator.h"
#include "buffer_log.h"
#include "sync_fence.h"
#include "delegator_adapter.h"

namespace OHOS {

sptr<ProducerSurfaceDelegator> ProducerSurfaceDelegator::Create()
{
    return sptr<ProducerSurfaceDelegator>(new ProducerSurfaceDelegator());
}

ProducerSurfaceDelegator::ProducerSurfaceDelegator()
{
    auto& delegatorAdapter = DelegatorAdapter::GetInstance();
    auto createFunc = delegatorAdapter.GetFunc<CreateFunc>(
        FunctionFlags::PRODUCER_CREATE_FUNC);
    if (createFunc != nullptr) {
        mDelegator_ = reinterpret_cast<uintptr_t>(createFunc());
    } else {
        BLOGE("remote Delegator createFunc is nullptr");
    }
}

ProducerSurfaceDelegator::~ProducerSurfaceDelegator()
{
    auto& delegatorAdapter = DelegatorAdapter::GetInstance();
    auto destroyFunc = delegatorAdapter.GetFunc<DestroyFunc>(
        FunctionFlags::PRODUCER_DESTROY_FUNC);
    if (destroyFunc != nullptr && mDelegator_ != 0) {
        destroyFunc(mDelegator_);
    } else {
        BLOGE("%{public}s error, ~ProducerSurfaceDelegator:%{public}d mDelegator:%{public}d",
            __func__, destroyFunc != nullptr, mDelegator_ != 0);
    }
    mDelegator_ = 0;
}

void ProducerSurfaceDelegator::SetSurface(sptr<Surface> surface)
{
    if (surface == nullptr) {
        BLOGE("surface is nullptr");
        return;
    }
    auto& delegatorAdapter = DelegatorAdapter::GetInstance();
    auto setSurfaceFunc = delegatorAdapter.GetFunc<SetProducerSurfaceFunc>(
        FunctionFlags::SET_PRODUCER_SURFACE_FUNC);
    if (setSurfaceFunc != nullptr && mDelegator_ != 0) {
        setSurfaceFunc(mDelegator_, surface);
    }
}

bool ProducerSurfaceDelegator::SetClient(sptr<IRemoteObject> client)
{
    if (client == nullptr) {
        BLOGE("client is nullptr");
        return false;
    }
    auto& delegatorAdapter = DelegatorAdapter::GetInstance();
    auto setClientFunc = delegatorAdapter.GetFunc<SetProducerClientFunc>(
        FunctionFlags::SET_PRODUCER_CLIENT_FUNC);
    if (setClientFunc != nullptr && mDelegator_ != 0) {
        return setClientFunc(mDelegator_, client);
    }
    BLOGE("%{public}s error, SetClient:%{public}d mDelegator:%{public}d",
        __func__, setClientFunc != nullptr, mDelegator_ != 0);
    return false;
}

GSError ProducerSurfaceDelegator::ReleaseBuffer(const sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence)
{
    if (buffer == nullptr || fence == nullptr) {
        BLOGE("buffer or fence is nullptr");
        return GSERROR_INVALID_ARGUMENTS;
    }
    auto& delegatorAdapter = DelegatorAdapter::GetInstance();
    auto releaseBufferfunc = delegatorAdapter.GetFunc<ReleaseBufferFunc>(
        FunctionFlags::PRODUCER_RELEASE_BUFFER_FUNC);
    if (releaseBufferfunc != nullptr && mDelegator_ != 0) {
        return releaseBufferfunc(mDelegator_, buffer, fence);
    }
    BLOGE("%{public}s error, ReleaseBuffer:%{public}d mDelegator:%{public}d",
        __func__, releaseBufferfunc != nullptr, mDelegator_ != 0);
    return GSERROR_BINDER;
}

} // namespace OHOS
