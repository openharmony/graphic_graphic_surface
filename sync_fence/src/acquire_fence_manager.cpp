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

#include "acquire_fence_manager.h"
#include "hilog/log.h"
#include "rs_trace.h"

namespace OHOS {

SyncFenceTracker* AcquireFenceTracker::tracker_ = nullptr;

AcquireFenceTracker::~AcquireFenceTracker()
{
    if (tracker_ != nullptr) {
        delete tracker_;
        tracker_ = nullptr;
    }
}

void AcquireFenceTracker::TrackFence(const sptr<SyncFence>& fence, bool traceTag)
{
    if (tracker_ == nullptr) {
        tracker_ = new SyncFenceTracker("Acquire Fence");
        if (tracker_ == nullptr) {
        return;
    }
    }
    tracker_->TrackFence(fence, traceTag);
}

void AcquireFenceTracker::SetBlurSize(int32_t blurSize)
{
    if (tracker_ == nullptr) {
        return;
    }
    tracker_->SetBlurSize(blurSize);
}

void AcquireFenceTracker::SetContainerNodeNum(int containerNodeNum)
{
    if (tracker_ == nullptr) {
        return;
    }
    tracker_->SetContainerNodeNum(containerNodeNum);
}

} // namespace OHOS