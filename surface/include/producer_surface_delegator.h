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

#include <map>
#include <vector>
#include <mutex>
#include <unordered_set>

#include "transact_surface_delegator_stub.h"

namespace OHOS {
class ProducerSurfaceDelegator : public TransactSurfaceDelegatorStub {
public:
    ~ProducerSurfaceDelegator();
    static sptr<ProducerSurfaceDelegator> Create()
    {
        return sptr<ProducerSurfaceDelegator>(new ProducerSurfaceDelegator());
    }
    GSError DequeueBuffer(int32_t slot, sptr<SurfaceBuffer> buffer);
    GSError QueueBuffer(int32_t slot, int32_t acquireFence);
    GSError ReleaseBuffer(const sptr<SurfaceBuffer> &buffer, const sptr<SyncFence> &fence);
    GSError ClearBufferSlot(int32_t slot);
    GSError ClearAllBuffers();
    GSError CancelBuffer(int32_t slot, int32_t fenceFd);
    GSError DetachBuffer(int32_t slot);
    int OnSetDataspace(MessageParcel& data, MessageParcel& reply);
    int OnDequeueBuffer(MessageParcel &data, MessageParcel &reply);
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    std::map<int32_t, std::vector<sptr<SurfaceBuffer>>> map_;
    std::vector<sptr<SurfaceBuffer>> pendingReleaseBuffer_;
    std::unordered_set<int> dequeueFailedSet_;
    std::mutex dequeueFailedSetMutex_;
    std::mutex mapMutex_;
    std::mutex mstate_;
    uint32_t mAncoDataspace = 0;

    void AddBufferLocked(const sptr<SurfaceBuffer>& buffer, int32_t slot);
    sptr<SurfaceBuffer> GetBufferLocked(int32_t slot);
    int32_t GetSlotLocked(const sptr<SurfaceBuffer>& buffer);
    GSError RetryFlushBuffer(sptr<SurfaceBuffer>& buffer, int32_t fence, BufferFlushConfig& config);
    bool HasSlotInSet(int32_t slot);
    void InsertSlotIntoSet(int32_t slot);
    void EraseSlotFromSet(int32_t slot);
};
} // namespace OHOS
#endif // PRODUCER_SURFACE_DELEGATOR_H
