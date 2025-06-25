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

#include "native_fence.h"
#include "sync_fence.h"

using namespace OHOS;

bool OH_NativeFence_Wait(int fenceFd, uint32_t timeout)
{
    if (fenceFd < 0 || timeout == 0) {
        return false;
    }
    int dupFd = dup(fenceFd);
    if (dupFd < 0) {
        return false;
    }
    sptr<SyncFence> fence = new SyncFence(dupFd);
    if (fence == nullptr) {
        close(dupFd);
        return false;
    }

    bool result = fence->Wait(timeout) == 0 ? true : false;
    return result;
}

bool OH_NativeFence_WaitForever(int fenceFd)
{
    if (fenceFd < 0) {
        return false;
    }
    int dupFd = dup(fenceFd);
    if (dupFd < 0) {
        return false;
    }
    sptr<SyncFence> fence = new SyncFence(dupFd);
    if (fence == nullptr) {
        close(dupFd);
        return false;
    }

    bool result = fence->Wait(-1) == 0 ? true : false;
    return result;
}

bool OH_NativeFence_IsValid(int fenceFd)
{
    if (fenceFd < 0) {
        return false;
    }

    return true;
}

void OH_NativeFence_Close(int fenceFd)
{
    if (fenceFd < 0) {
        return;
    }

    close(fenceFd);
}