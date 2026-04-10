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

#include "nativefence_fuzzer.h"

#include <fuzzer/FuzzedDataProvider.h>

#include <fcntl.h>
#include <securec.h>
#include <unistd.h>

#include "native_fence.h"

namespace OHOS {
    namespace {
        constexpr uint32_t MAX_TIMEOUT = 100;
    }

    void NativeFenceFuzzTestWithValidFd(FuzzedDataProvider& provider)
    {
        int32_t devNullFd = open("/dev/null", O_RDONLY);
        if (devNullFd < 0) {
            return;
        }

        uint32_t timeout = provider.ConsumeIntegral<uint32_t>() % MAX_TIMEOUT;
        if (timeout == 0) {
            timeout = 1;
        }

        OH_NativeFence_IsValid(devNullFd);
        OH_NativeFence_Wait(devNullFd, timeout);
        OH_NativeFence_WaitForever(devNullFd);
        OH_NativeFence_Close(devNullFd);
    }

    void NativeFenceFuzzTestWithRandomFd(FuzzedDataProvider& provider)
    {
        int32_t fenceFd = provider.ConsumeIntegral<int32_t>();
        uint32_t timeout = provider.ConsumeIntegral<uint32_t>() % MAX_TIMEOUT;
        OH_NativeFence_IsValid(fenceFd);
        OH_NativeFence_Wait(fenceFd, timeout);
        OH_NativeFence_WaitForever(fenceFd);

        // Only close safe fd values from fuzz data to avoid closing stdio descriptors.
        if (fenceFd > STDERR_FILENO) {
            OH_NativeFence_Close(fenceFd);
        }
    }
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    FuzzedDataProvider provider(data, size);
    OHOS::NativeFenceFuzzTestWithValidFd(provider);
    OHOS::NativeFenceFuzzTestWithRandomFd(provider);
    return 0;
}

