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

#ifndef CONSUMER_SURFACE_DELEGATOR_H
#define CONSUMER_SURFACE_DELEGATOR_H

#include <iremote_stub.h>
#include <message_option.h>
#include <message_parcel.h>

#include "buffer_log.h"
#include "buffer_extra_data.h"
#include "delegator_adapter.h"
#include "ibuffer_producer.h"
#include "sync_fence.h"
#include "surface.h"
#include "surface_type.h"

namespace OHOS {
class ConsumerSurfaceDelegator : public OHOS::RefBase {
public:
    static sptr<ConsumerSurfaceDelegator> Create();
    ~ConsumerSurfaceDelegator();
    GSError DequeueBuffer(const BufferRequestConfig& config, sptr<BufferExtraData>& bedata,
                          struct IBufferProducer::RequestBufferReturnValue& retval);
    GSError QueueBuffer(sptr<SurfaceBuffer>& buffer, int32_t fenceFd);
    void SetSurface(sptr<Surface> surface);
    bool SetClient(sptr<IRemoteObject> client);

private:
    ConsumerSurfaceDelegator();
    uintptr_t mDelegator_ = 0;
};
} // namespace OHOS
#endif // CONSUMER_SURFACE_DELEGATOR_H
