/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef UTILS_INCLUDE_SYNC_FENCE_TRACKER_H
#define UTILS_INCLUDE_SYNC_FENCE_TRACKER_H

#include <atomic>
#include <event_handler.h>
#include <queue>
#include "sync_fence.h"

namespace OHOS {
class SyncFenceTracker {
public:
    explicit SyncFenceTracker(const std::string threadName);

    SyncFenceTracker() = delete;
    ~SyncFenceTracker() = default;

    void TrackFence(const sptr<SyncFence>& fence, bool traceTag = true);

private:
    const uint32_t SYNC_TIME_OUT = 3000;
    const int32_t GPU_SUBHEALTH_EVENT_LIMIT = 200;
    const int32_t GPU_SUBHEALTH_EVENT_THRESHOLD = 12;
    const uint32_t FRAME_QUEUE_SIZE_LIMIT = 4;
    const int32_t FRAME_PERIOD = 1000;
    const std::string threadName_;
    bool isGpuFence_ = false;
    bool isGpuEnable_ = false;
    bool isGpuFreq_ = false;
    std::shared_ptr<OHOS::AppExecFwk::EventRunner> runner_ = nullptr;
    std::shared_ptr<OHOS::AppExecFwk::EventHandler> handler_ = nullptr;
    std::atomic<uint32_t> fencesQueued_;
    std::atomic<uint32_t> fencesSignaled_;
    int32_t gpuSubhealthEventNum_ = 0;
    int32_t gpuSubhealthEventDay_ = 0;
    std::queue<int64_t> *frameStartTimes_ = new std::queue<int64_t>;
    void Loop(const sptr<SyncFence>& fence, bool traceTag);
    int32_t WaitFence(const sptr<SyncFence>& fence);
    bool CheckGpuSubhealthEventLimit();
    void ReportEventGpuSubhealth(int64_t duration);
    inline void UpdateFrameQueue(int64_t startTime);
    int64_t GetFrameRate();
};

class SyncFenceTrackerManager {
public:
    static std::shared_ptr<SyncFenceTracker> GetSyncFenceTracker(const std::string& name, uint32_t screenId)
    {
        auto it = trackers_.find(screenId);
        if (it == trackers_.end()) {
            return CreateSyncFenceTracker(name, screenId);
        } else {
            return it->second;
        }
    }
private:
    static std::shared_ptr<SyncFenceTracker> CreateSyncFenceTracker(const std::string& name, uint32_t screenId)
    {
        auto tracker = std::make_shared<SyncFenceTracker>(name);
        trackers_[screenId] = tracker;
        return tracker;
    }
    static inline std::unordered_map<uint32_t, std::shared_ptr<SyncFenceTracker>> trackers_;
};
}
#endif // UTILS_INCLUDE_SYNC_FENCE_TRACKER_H