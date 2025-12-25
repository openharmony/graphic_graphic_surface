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
#ifndef PRODUCER_SURFACE_DELEGATOR_H
#define PRODUCER_SURFACE_DELEGATOR_H

#include <atomic>
#include <map>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <message_option.h>
#include <message_parcel.h>
#include <iremote_stub.h>

#include "surface.h"
#include "sync_fence.h"

namespace OHOS {
class ProducerSurfaceDelegator : public OHOS::RefBase {
public:
    static sptr<ProducerSurfaceDelegator> Create();
    ~ProducerSurfaceDelegator();
    GSError ReleaseBuffer(const sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence);
    void SetSurface(sptr<Surface> surface);
    bool SetClient(sptr<IRemoteObject> client);
private:
    uintptr_t mDelegator_ = 0;
    ProducerSurfaceDelegator();
};
} // namespace OHOS
#endif // PRODUCER_SURFACE_DELEGATOR_H
