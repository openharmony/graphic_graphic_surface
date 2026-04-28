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

#include <fuzzer/FuzzedDataProvider.h>

#include <fcntl.h>
#include <securec.h>
#include <string>
#include <climits>  // For INT32_MAX
#include <unistd.h>

#include "data_generate.h"
#include "native_buffer.h"
#include "native_window.h"
#include "native_buffer_inner.h"
#include "ipc_cparcel.h"
#include "ipc_inner_object.h"

using namespace g_fuzzCommon;

namespace OHOS {
namespace {
    constexpr int32_t ROI_METADATA_CAPACITY = 256;
    constexpr int32_t ROI_METADATA_FUZZ_TEST_RANGE = 600;
    constexpr uint8_t ROI_METADATA_FUZZ_INVALID_SIZE = 10;
}

void NativeBufferFuzzTestEdgeCases(OH_NativeBuffer *buffer)
    {
        OH_NativeBuffer_Config config;
        // Test GetConfig null pointer
        OH_NativeBuffer_GetConfig(nullptr, &config);                     // buffer = nullptr
        OH_NativeBuffer_GetConfig(buffer, nullptr);                      // config = nullptr

        // Test SetColorSpace null pointer and invalid parameters
        OH_NativeBuffer_ColorSpace invalidColorSpace = (OH_NativeBuffer_ColorSpace)0x7FFFFFFF;
        OH_NativeBuffer_SetColorSpace(nullptr, OH_COLORSPACE_SRGB_FULL); // buffer = nullptr
        OH_NativeBuffer_SetColorSpace(buffer, invalidColorSpace);        // invalid color space

        // Test GetColorSpace null pointer
        OH_NativeBuffer_ColorSpace colorSpace;
        OH_NativeBuffer_GetColorSpace(nullptr, &colorSpace);            // buffer = nullptr
        OH_NativeBuffer_GetColorSpace(buffer, nullptr);                 // colorSpace = nullptr

        // Test SetMetadataValue null pointer and invalid parameters
        const uint32_t metadataSize = 10;
        uint8_t metadata[metadataSize] = {0};
        OH_NativeBuffer_SetMetadataValue(nullptr, OH_HDR_DYNAMIC_METADATA, metadataSize, metadata); // buffer = nullptr
        OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_DYNAMIC_METADATA, metadataSize, nullptr); // metadata = nullptr
        OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_DYNAMIC_METADATA, 0, metadata);             // size = 0
        OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_DYNAMIC_METADATA, -1, metadata);            // size < 0

        // Test GetMetadataValue null pointer
        int32_t getSize = 0;
        uint8_t *getMetadata = nullptr;
        OH_NativeBuffer_GetMetadataValue(nullptr, OH_HDR_DYNAMIC_METADATA, &getSize, &getMetadata);
        OH_NativeBuffer_GetMetadataValue(buffer, OH_HDR_DYNAMIC_METADATA, nullptr, &getMetadata);
        OH_NativeBuffer_GetMetadataValue(buffer, OH_HDR_DYNAMIC_METADATA, &getSize, nullptr);

        // Test IsSupported null pointer and invalid parameters
        OH_NativeBuffer_IsSupported(config, nullptr);                  // isSupported = nullptr

        // Test MapWaitFence invalid parameters
        void *addr = nullptr;
        OH_NativeBuffer_MapWaitFence(nullptr, -1, &addr);
        OH_NativeBuffer_MapWaitFence(buffer, -1, &addr);

        // Test parcel interfaces invalid parameters
        OH_NativeBuffer *tmpBuffer = nullptr;
        OH_NativeBuffer_WriteToParcel(nullptr, nullptr);
        OH_NativeBuffer_ReadFromParcel(nullptr, &tmpBuffer);
    }

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
        OHIPCParcel *parcel = OH_IPCParcel_Create();
        OH_NativeBuffer_WriteToParcel(nativeBuffer, parcel);
        OH_NativeBuffer *outBuffer = nullptr;
        OH_NativeBuffer_ReadFromParcel(parcel, &outBuffer);
        OH_IPCParcel_Destroy(parcel);
        if (outBuffer != nullptr) {
            OH_NativeBuffer_Unreference(outBuffer);
        }
        bool isSupported = false;
        OH_NativeBuffer_IsSupported(config, &isSupported);
        void *virAddr1 = nullptr;
        OH_NativeBuffer_Config testConfig = {};
        OH_NativeBuffer_MapAndGetConfig(nativeBuffer, &virAddr1, &testConfig);
        OH_NativeBuffer_ColorSpace getColorSpace;
        OH_NativeBuffer_GetColorSpace(buffer, &getColorSpace);
        OH_NativeBuffer_MetadataKey metadataKey = GetData<OH_NativeBuffer_MetadataKey>();
        uint32_t setSize = GetData<uint32_t>() % 1000;
        if (metadataKey == OH_REGION_OF_INTEREST_METADATA) {
            setSize = GetData<uint32_t>() % ROI_METADATA_FUZZ_TEST_RANGE; // test sizes including truncation > 256
        }
        uint8_t *metadata = (uint8_t *)malloc(setSize * sizeof(uint8_t));
        if (metadata != nullptr) {
            for (uint32_t i = 0; i < setSize; ++i) {
                metadata[i] = GetData<uint8_t>();
            }
        }
        OH_NativeBuffer_SetMetadataValue(buffer, metadataKey, static_cast<int32_t>(setSize), metadata);
        free(metadata);
        metadata = nullptr;
        // test invalid params for ROI metadata
        if (metadataKey == OH_REGION_OF_INTEREST_METADATA || GetData<bool>()) {
            uint8_t dummyBuff[ROI_METADATA_FUZZ_INVALID_SIZE] = {0};
            OH_NativeBuffer_SetMetadataValue(nullptr, OH_REGION_OF_INTEREST_METADATA,
                                             ROI_METADATA_FUZZ_INVALID_SIZE, dummyBuff);
            OH_NativeBuffer_SetMetadataValue(buffer, OH_REGION_OF_INTEREST_METADATA, 0, dummyBuff);
            OH_NativeBuffer_SetMetadataValue(buffer, OH_REGION_OF_INTEREST_METADATA, -1, dummyBuff);
            OH_NativeBuffer_SetMetadataValue(buffer, OH_REGION_OF_INTEREST_METADATA, ROI_METADATA_CAPACITY, nullptr);
        }
        int32_t getSize;
        uint8_t *getMetadata = nullptr;
        OH_NativeBuffer_GetMetadataValue(buffer, metadataKey, &getSize, &getMetadata);
        if (getMetadata != nullptr) {
            free(getMetadata);
            getMetadata = nullptr;
        }
        // test invalid params for Get ROI metadata
        if (metadataKey == OH_REGION_OF_INTEREST_METADATA || GetData<bool>()) {
            OH_NativeBuffer_GetMetadataValue(nullptr, OH_REGION_OF_INTEREST_METADATA, &getSize, &getMetadata);
            OH_NativeBuffer_GetMetadataValue(buffer, OH_REGION_OF_INTEREST_METADATA, nullptr, &getMetadata);
            OH_NativeBuffer_GetMetadataValue(buffer, OH_REGION_OF_INTEREST_METADATA, &getSize, nullptr);
        }
        OH_NativeBuffer_GetConfig(buffer, &checkConfig);
        OH_NativeBuffer_Reference(buffer);
        OH_NativeBuffer_Unreference(buffer);
        OH_NativeBuffer_Map(buffer, &virAddr);
        OH_NativeBuffer_Unmap(buffer);
        int32_t invalidFence = -1;
        OH_NativeBuffer_MapWaitFence(buffer, invalidFence, &virAddr);
        int32_t nullFd = open("/dev/null", O_RDONLY);
        if (nullFd >= 0) {
            OH_NativeBuffer_MapWaitFence(buffer, nullFd, &virAddr);
            close(nullFd);
        }
        DestroyNativeWindowBuffer(nativeWindowBuffer);
        OH_NativeBuffer_Unreference(buffer);

        // Run boundary value and exception tests
        NativeBufferFuzzTestEdgeCases(buffer);

        return true;
    }
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    FuzzedDataProvider provider(data, size);
    (void)provider.ConsumeIntegral<uint8_t>();
    /* Run your code on data */
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}

