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

#include "framereport_fuzzer.h"

#include <securec.h>

#include "frame_report.h"

namespace OHOS {
namespace Rosen {
namespace {
const uint8_t* DATA = nullptr;
size_t g_size = 0;
size_t g_pos = 0;
} // namespace

template<class T>
T GetData()
{
    T object {};
    size_t objectSize = sizeof(object);
    if (DATA == nullptr || objectSize > g_size - g_pos) {
        return object;
    }
    errno_t ret = memcpy_s(&object, objectSize, DATA + g_pos, objectSize);
    if (ret != EOK) {
        return {};
    }
    g_pos += objectSize;
    return object;
}

bool DoSetGameScene(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    // initialize
    DATA = data;
    g_size = size;
    g_pos = 0;

    std::string layerName = "LayerName";
    // get data
    int32_t pid = GetData<int32_t>();
    int32_t state = GetData<int32_t>();
    Rosen::FrameReport::GetInstance().SetGameScene(pid, state);

    uint64_t uniqueId = GetData<uint64_t>();
    Rosen::FrameReport::GetInstance().IsActiveGameWithUniqueId(uniqueId);

    int64_t lastSwapBufferTime = GetData<int64_t>();
    Rosen::FrameReport::GetInstance().SetLastSwapBufferTime(lastSwapBufferTime);

    int64_t dequeueBufferTime = GetData<int64_t>();
    Rosen::FrameReport::GetInstance().SetDequeueBufferTime(layerName, dequeueBufferTime);

    int64_t queueBufferTime = GetData<int64_t>();
    Rosen::FrameReport::GetInstance().SetQueueBufferTime(uniqueId, layerName, queueBufferTime);

    int64_t pendingBufferNum = GetData<int64_t>();
    Rosen::FrameReport::GetInstance().SetPendingBufferNum(layerName, pendingBufferNum);

    Rosen::FrameReport::GetInstance().Report(layerName);

    return true;
}
} // namespace Rosen
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::Rosen::DoSetGameScene(data, size);
    return 0;
}

