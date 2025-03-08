/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_SURFACE_BUFFER_PRODUCER_LISTENER_H
#define INTERFACES_INNERKITS_SURFACE_BUFFER_PRODUCER_LISTENER_H

#include <refbase.h>
#include "buffer_utils.h"
#include "iremote_broker.h"
#include "buffer_log.h"
#include "surface_buffer.h"
#include "ibuffer_producer_listener.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "message_option.h"
#include "sync_fence.h"

namespace OHOS {
class ProducerListenerProxy : public IRemoteProxy<IProducerListener> {
public:
    explicit ProducerListenerProxy(const sptr<IRemoteObject>& impl) : IRemoteProxy<IProducerListener>(impl) {};
    virtual ~ProducerListenerProxy() noexcept = default;
    GSError OnBufferReleased() override
    {
        MessageOption option;
        MessageParcel arguments;
        MessageParcel reply;
        if (!arguments.WriteInterfaceToken(IProducerListener::GetDescriptor())) {
            BLOGE("write interface token failed");
            return GSERROR_BINDER;
        }
        option.SetFlags(MessageOption::TF_ASYNC);
        int32_t ret = Remote()->SendRequest(IProducerListener::ON_BUFFER_RELEASED, arguments, reply, option);
        if (ret != ERR_NONE) {
            return GSERROR_BINDER;
        }
        return GSERROR_OK;
    }

    GSError OnBufferReleasedWithFence(const sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence) override
    {
        MessageOption option;
        MessageParcel arguments;
        MessageParcel reply;
        if (!arguments.WriteInterfaceToken(IProducerListener::GetDescriptor())) {
            BLOGE("write interface token failed");
            return GSERROR_BINDER;
        }
        WriteSurfaceBufferImpl(arguments, buffer->GetSeqNum(), buffer);
        arguments.WriteBool(fence != nullptr);
        if (fence != nullptr) {
            fence->WriteToMessageParcel(arguments);
        }
        option.SetFlags(MessageOption::TF_ASYNC);
        int32_t ret = Remote()->SendRequest(IProducerListener::ON_BUFFER_RELEASED_WITH_FENCE, arguments, reply, option);
        if (ret != ERR_NONE) {
            BLOGE("Remote SendRequest fail, ret = %{public}d", ret);
            return GSERROR_BINDER;
        }
        return GSERROR_OK;
    }

    GSError OnPropertyChange(SurfaceProperty property) override
    {
        MessageOption option;
        MessageParcel arguments;
        MessageParcel reply;
        if(!arguments.WriteInterfaceToken(IProducerListener::GetDescriptor())){
            BLOGE("write interface token failed");
            return GSERROR_BINDER;
        }

        WriteSurfaceProperty(arguments,property);
        option.SetFlags(MessageOption::TF_ASYNC);
        int32_t ret = Remote()->SendRequest(IProducerListener::ON_PROPERTY_CHANGE, arguments， reply, option);
        if(ret != ERR_NONE){
            return GSERROR_BINDER;
        }
        return GSERROR_OK;
    }

    void ResetReleaseFunc() override {}
private:
    static inline BrokerDelegator<ProducerListenerProxy> delegator_;
};

class ProducerListenerStub : public IRemoteStub<IProducerListener> {
public:
    ProducerListenerStub() = default;
    ~ProducerListenerStub() = default;
    int32_t OnRemoteRequest(uint32_t code, MessageParcel &arguments,
                            MessageParcel &reply, MessageOption &option) override
    {
        (void)(option);
        auto remoteDescriptor = arguments.ReadInterfaceToken();
        if (GetDescriptor() != remoteDescriptor) {
            return ERR_INVALID_STATE;
        }

        auto ret = ERR_NONE;
        switch (code) {
            case ON_BUFFER_RELEASED: {
                auto sret = OnBufferReleased();
                reply.WriteInt32(sret);
                break;
            }
            case ON_BUFFER_RELEASED_WITH_FENCE: {
                auto sret = OnBufferReleasedWithFenceRemote(arguments);
                reply.WriteInt32(sret);
                break;
            }
            case ON_PROPERTY_CHANGE: {
                auto sret = OnPropertyChangeRemote(arguments);
                reply.WriteInt32(sret);
                break;
            } 
            default: {
                ret = ERR_UNKNOWN_TRANSACTION;
                break;
            }
        }
        return ret;
    }
private:
    GSError OnBufferReleasedWithFenceRemote(MessageParcel& arguments)
    {
        sptr<SurfaceBuffer> buffer;
        sptr<SyncFence> fence = SyncFence::InvalidFence();
        uint32_t sequence = 0;
        GSError ret = ReadSurfaceBufferImpl(arguments, sequence, buffer);
        if (ret != GSERROR_OK) {
            BLOGE("ReadSurfaceBufferImpl failed, return %{public}d", ret);
            return OnBufferReleasedWithFence(buffer, fence);
        }
        if (arguments.ReadBool()) {
            fence = SyncFence::ReadFromMessageParcel(arguments);
        }
        ret = OnBufferReleasedWithFence(buffer, fence);
        return ret;
    }
    GSError OnPropertyChangeRemote(MessageParcel& arguments)
    {
        SurfaceProperty property;
        GSError ret = ReadSurfaceProperty(arguments, property);
        if(ret = GSERROR_OK){
            OnPropertyChange(property);
        }
        return ret;
    }
};

class BufferReleaseProducerListener : public ProducerListenerStub {
public:
    BufferReleaseProducerListener(OnReleaseFunc func = nullptr, OnReleaseFuncWithFence funcWithFence = nullptr)
        : func_(func), funcWithFence_(funcWithFence) {};
    ~BufferReleaseProducerListener() override {};
    GSError OnBufferReleased() override
    {
        OnReleaseFunc func = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (func_ != nullptr) {
                func = func_;
            }
        }
        if (func != nullptr) {
            sptr<OHOS::SurfaceBuffer> sBuffer = nullptr;
            return func(sBuffer);
        }
        return GSERROR_INTERNAL;
    };
    GSError OnBufferReleasedWithFence(const sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence) override
    {
        OnReleaseFuncWithFence funcWithFence = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (funcWithFence_ != nullptr) {
                funcWithFence = funcWithFence_;
            }
        }
        if (funcWithFence != nullptr) {
            return funcWithFence(buffer, fence);
        }
        return GSERROR_INTERNAL;
    }
    GSError OnPropertyChange(const SurfaceProperty& property) override {return GSERROR_NOT_SUPPORT; }
    void ResetReleaseFunc() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        func_ = nullptr;
        funcWithFence_ = nullptr;
    }
private:
    OnReleaseFunc func_;
    OnReleaseFuncWithFence funcWithFence_;
    std::mutex mutex_;
};

class PropertyChangeProducerListener : public ProducerListenerStub{
public:
    PropertyChangeProducerListener(OnPropertyChangeFunc func = nullptr)
        : func_(func){};
    ~PropertyChangeProducerListener() override {};
    GSError OnBufferReleased() override
    {
        return GSERROR_NOT_SUPPORT;
    }
    GSError OnBufferReleasedWithFence(const sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence) override
    {
        return GSERROR_NOT_SUPPORT;
    }
    GSError OnPropertyChange(const SurfaceProperty& property) override
    {
        OnPropertyChangeFunc func = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(func_ != nullptr) {
                func = func_;
            }
        }
        if(func != nullptr) {
            return func(property);
        }
        return GSERROR_OK;
    }

    void ResetReleaseFunc() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        func_ = nullptr;
    }

private:
    OnPropertyChangeFunc func_;
    std::mutex mutex_;

}
} // namespace OHOS

#endif // INTERFACES_INNERKITS_SURFACE_BUFFER_PRODUCER_LISTENER_H
