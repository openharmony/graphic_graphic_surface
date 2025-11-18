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

#include "nativebuffer_fuzzer.h"

#include <securec.h>
#include <string>

#include "data_generate.h"
#include "native_buffer.h"
#include "native_window.h"
#include "native_buffer_inner.h"

using namespace g_fuzzCommon;

namespace OHOS {
    bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
    {
        if (data == nullptr) {
            return false;
        }

        // initialize
        g_data = data;
        g_size = size;
        g_pos = 0;

        // get data
        OH_NativeBuffer_Config config;
        config.width = 100; // 100 pixels
        config.height = 100; // 100 pixels
        config.format = GRAPHIC_PIXEL_FMT_RGBA_8888;
        config.usage = BUFFER_USAGE_CPU_READ;
        OH_NativeBuffer_Config checkConfig = GetData<OH_NativeBuffer_Config>();
        void *virAddr = static_cast<void*>(GetStringFromData(STR_LEN).data());
        OH_NativeBuffer_ColorSpace colorSpace = GetData<OH_NativeBuffer_ColorSpace>();

        // test
        OH_NativeBuffer* buffer = OH_NativeBuffer_Alloc(&config);
        OH_NativeBuffer_GetSeqNum(buffer);
        OH_NativeBuffer_GetBufferHandle(buffer);
        OH_NativeBuffer_GetNativeBufferConfig(buffer, &checkConfig);
        OHNativeWindowBuffer *nativeWindowBuffer = CreateNativeWindowBufferFromNativeBuffer(buffer);
        OH_NativeBufferFromNativeWindowBuffer(nativeWindowBuffer);
        OH_NativeBuffer_SetColorSpace(buffer, colorSpace);
        void *getVirAddr;
        OH_NativeBuffer_Planes outPlanes;
        OH_NativeBuffer_MapPlanes(buffer, &getVirAddr, &outPlanes);
        OH_NativeBuffer *nativeBuffer;
        OH_NativeBuffer_FromNativeWindowBuffer(nativeWindowBuffer, &nativeBuffer);
        OH_NativeBuffer_ColorSpace getColorSpace;
        OH_NativeBuffer_GetColorSpace(buffer, &getColorSpace);
        OH_NativeBuffer_MetadataKey metadataKey = GetData<OH_NativeBuffer_MetadataKey>();
        uint32_t setSize = GetData<uint32_t>() % 1000;
        if (metadataKey == OH_REGION_OF_INTEREST_METADATA) {
            setSize = 256; // 256：内部为固定size
        }
        uint8_t *metadata = (uint8_t *)malloc(setSize * sizeof(uint8_t));
        OH_NativeBuffer_SetMetadataValue(buffer, metadataKey, static_cast<int32_t>(setSize), metadata);
        free(metadata);
        int32_t getSize;
        uint8_t *getMetadata;
        OH_NativeBuffer_GetMetadataValue(buffer, metadataKey, &getSize, &getMetadata);
        OH_NativeBuffer_GetConfig(buffer, &checkConfig);
        OH_NativeBuffer_Reference(buffer);
        OH_NativeBuffer_Unreference(buffer);
        OH_NativeBuffer_Map(buffer, &virAddr);
        OH_NativeBuffer_Unmap(buffer);
        DestroyNativeWindowBuffer(nativeWindowBuffer);
        OH_NativeBuffer_Unreference(buffer);

        return true;
    }
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}

