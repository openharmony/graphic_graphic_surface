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

#include <fcntl.h>
#include <securec.h>
#include <string>
#include <unistd.h>

#include "data_generate.h"
#include "native_fence.h"

using namespace g_fuzzCommon;

namespace OHOS {
    constexpr uint32_t TIMEOUT_MS = 1000;
    bool DoSomethingInterestingWithMyAPI001(const uint8_t* data, size_t size)
    {
        if (data == nullptr) {
            return false;
        }

        // initialize
        g_data = data;
        g_size = size;
        g_pos = 0;

        int32_t fenceFd = open("/dev/GPIO_TEST", O_RDONLY);
        if (fenceFd < 0) {
            return false;
        }
        OH_NativeFence_IsValid(fenceFd);
        OH_NativeFence_Wait(fenceFd, TIMEOUT_MS);
        OH_NativeFence_Close(fenceFd);

        return true;
    }

    bool DoSomethingInterestingWithMyAPI002(const uint8_t* data, size_t size)
    {
        if (data == nullptr) {
            return false;
        }

        // initialize
        g_data = data;
        g_size = size;

        int32_t fenceFd = GetData<int32_t>();
        OH_NativeFence_IsValid(fenceFd);
        OH_NativeFence_WaitForever(-1);

        return true;
    }
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::DoSomethingInterestingWithMyAPI001(data, size);
    OHOS::DoSomethingInterestingWithMyAPI002(data, size);
    return 0;
}

