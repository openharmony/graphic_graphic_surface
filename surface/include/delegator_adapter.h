/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef DISPLAY_COMPOSER_SERVER_H
#define DISPLAY_COMPOSER_SERVER_H

#include <any>
#include <mutex>
#include <typeindex>

#include "buffer_log.h"
#include "consumer_surface_delegator.h"
#include "producer_surface_delegator.h"
#include "singleton.h"

namespace OHOS {

using CreateFunc = void* (*)();
using ConvertHandle = void*;
using SetProducerClientFunc = bool (*)(uintptr_t, sptr<IRemoteObject>);
using SetProducerSurfaceFunc = void (*)(uintptr_t, sptr<Surface>);
using ReleaseBufferFunc = GSError (*)(uintptr_t, const sptr<SurfaceBuffer>&, const sptr<SyncFence>&);
using DestroyFunc = void (*)(uintptr_t);

using ConsumerDelegatorFunc = void* (*)();
using SetConsumerClientFunc = bool (*)(uintptr_t, sptr<IRemoteObject>);
using SetConsumerSurfaceFunc = void (*)(uintptr_t, sptr<Surface>);
using ConsumerDeQueueBuffer = GSError (*)(uintptr_t, const BufferRequestConfig&, sptr<BufferExtraData>&,
    struct IBufferProducer::RequestBufferReturnValue&);
using ConsumerQueueBuffer = GSError (*)(uintptr_t, sptr<SurfaceBuffer>&, int32_t);
using ConsumerDestroyFunc = void (*)(uintptr_t);

enum FunctionFlags {
    PRODUCER_CREATE_FUNC,
    PRODUCER_DESTROY_FUNC,
    SET_PRODUCER_SURFACE_FUNC,
    SET_PRODUCER_CLIENT_FUNC,
    PRODUCER_RELEASE_BUFFER_FUNC,
    CONSUMER_CREATE_FUNC,
    SET_CONSUMER_CLIENT_FUNC,
    SET_CONSUMER_SURFACE_FUNC,
    CONSUMER_DEQUEUE_BUFFER_FUNC,
    CONSUMER_QUEUE_BUFFER_FUNC,
    CONSUMER_DESTROY_FUNC,
};

struct CallbackConfig {
    FunctionFlags type;
    std::string name;
};

const std::vector<CallbackConfig> CALLBACK_FUNC_CONFIGS = {
    { PRODUCER_CREATE_FUNC, "ProducerSurfaceDelegatorCreate" },
    { PRODUCER_DESTROY_FUNC, "ProducerSurfaceDelegatorDestroy" },
    { SET_PRODUCER_SURFACE_FUNC, "ProducerSurfaceDelegatorSetSurface" },
    { SET_PRODUCER_CLIENT_FUNC, "ProducerSurfaceDelegatorSetClient" },
    { PRODUCER_RELEASE_BUFFER_FUNC, "ProducerSurfaceDelegatorReleaseBuffer" },
    { CONSUMER_CREATE_FUNC, "ConsumerSurfaceDelegatorCreate" },
    { SET_CONSUMER_CLIENT_FUNC, "ConsumerSurfaceDelegatorSetClient" },
    { SET_CONSUMER_SURFACE_FUNC, "ConsumerSurfaceDelegatorSetSurface" },
    { CONSUMER_DEQUEUE_BUFFER_FUNC, "ConsumerSurfaceDelegatorDequeueBuffer" },
    { CONSUMER_QUEUE_BUFFER_FUNC, "ConsumerSurfaceDelegatorQueueBuffer" },
    { CONSUMER_DESTROY_FUNC, "ConsumerSurfaceDelegatorDestroy" },
};

class DelegatorAdapter {
public:
    static DelegatorAdapter& GetInstance();
    template<typename T>
    T GetFunc(FunctionFlags type)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (funcMap_.find(type) == funcMap_.end() || funcMap_[type] == nullptr) {
            return nullptr;
        }
        return reinterpret_cast<T>(funcMap_[type]);
    }
    void RegisterFunction();
    DelegatorAdapter(const DelegatorAdapter&) = delete;
    DelegatorAdapter& operator=(const DelegatorAdapter&) = delete;
private:
    void Init();
    DelegatorAdapter();
    ~DelegatorAdapter();
    std::mutex mutex_;
    std::map<FunctionFlags, void*> funcMap_;
    ConvertHandle handle_ { nullptr };
};
}  // namespace OHOS
#endif