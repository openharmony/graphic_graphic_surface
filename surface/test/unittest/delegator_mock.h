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
#ifndef DELEGATOR_MOCK_H
#define DELEGATOR_MOCK_H

#include <dlfcn.h>
#include <cstdio>
#include <cstring>

#include "delegator_adapter.h"
#include "surface_type.h"
#include "sync_fence.h"
#include "graphic_common_c.h"

namespace OHOS {
namespace Rosen {

extern bool g_dlopenNull;
extern bool g_producerSurfaceDelegatorCreateNull;
extern bool g_producerSurfaceDelegatorDestroyNull;
extern bool g_producerSurfaceDelegatorSetSurfaceNull;
extern bool g_producerSurfaceDelegatorSetClientNull;
extern bool g_producerSurfaceDelegatorReleaseBufferNull;
extern bool g_consumerSurfaceDelegatorCreateNull;
extern bool g_consumerSurfaceDelegatorSetClientNull;
extern bool g_consumerSurfaceDelegatorSetSurfaceNull;
extern bool g_consumerSurfaceDelegatorDequeueBufferNull;
extern bool g_consumerSurfaceDelegatorQueueBufferNull;
extern bool g_consumerSurfaceDelegatorDestroyNull;

uintptr_t MockProducerSurfaceDelegatorCreate();
void MockProducerSurfaceDelegatorDestroy(uintptr_t delegator);
void MockProducerSurfaceDelegatorSetSurface(uintptr_t delegator, sptr<Surface> surface);
bool MockProducerSurfaceDelegatorSetClient(uintptr_t delegator, sptr<IRemoteObject> client);
GSError MockProducerSurfaceDelegatorReleaseBuffer(uintptr_t delegator,
    const sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence);
uintptr_t MockConsumerSurfaceDelegatorCreate();
bool MockConsumerSurfaceDelegatorSetClient(uintptr_t delegator, sptr<IRemoteObject> client);
void MockConsumerSurfaceDelegatorSetSurface(uintptr_t delegator, sptr<Surface> surface);
GSError MockConsumerSurfaceDelegatorDequeueBuffer(uintptr_t delegator,
    const BufferRequestConfig& config, sptr<BufferExtraData>& bedata,
    struct IBufferProducer::RequestBufferReturnValue& retval);
GSError MockConsumerSurfaceDelegatorQueueBuffer(uintptr_t delegator,
    sptr<SurfaceBuffer>& buffer, int32_t fenceFd);
void MockConsumerSurfaceDelegatorDestroy(uintptr_t delegator);

}  // namespace Rosen
}  // namespace OHOS

#endif // DELEGATOR_MOCK_H