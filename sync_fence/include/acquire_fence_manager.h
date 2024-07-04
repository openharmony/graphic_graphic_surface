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

#ifndef UTILS_INCLUDE_ACQUIRE_FENCE_TRACKER_H
#define UTILS_INCLUDE_ACQUIRE_FENCE_TRACKER_H

#include <atomic>
#include <event_handler.h>
#include "sync_fence_tracker.h"
#include "sync_fence.h"

namespace OHOS {
class AcquireFenceTracker {
public:
    AcquireFenceTracker() = default;
    ~AcquireFenceTracker();
    static void TrackFence(const sptr<SyncFence>& fence, bool traceTag);
    static void SetBlurSize(int32_t blurSize);
    static void SetContainerNodeNum(int containerNodeNum);
private:
    static SyncFenceTracker* tracker_;
};
}
#endif // UTILS_INCLUDE_SYNC_FENCE_TRACKER_H