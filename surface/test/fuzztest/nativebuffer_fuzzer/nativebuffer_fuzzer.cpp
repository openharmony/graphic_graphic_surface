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
#include <cstdint>
#include <string>
#include <climits>
#include <unistd.h>

#include "native_buffer.h"
#include "native_window.h"
#include "native_buffer_inner.h"
#include "ipc_cparcel.h"
#include "ipc_inner_object.h"

namespace OHOS {
namespace {

constexpr int32_t ROI_METADATA_CAPACITY = 256;
constexpr int32_t ROI_METADATA_FUZZ_TEST_RANGE = 600;
constexpr uint8_t ROI_METADATA_FUZZ_INVALID_SIZE = 10;
constexpr size_t STR_LEN_MAX = 32;
constexpr uint32_t METADATA_SIZE_MAX = 1000;
constexpr int32_t INVALID_FENCE_FD = -1;
constexpr int32_t INVALID_COLOR_SPACE_VALUE = 0x7FFFFFFF;
constexpr uint32_t EDGE_CASE_METADATA_SIZE = 10;

} // namespace

void NativeBufferFuzzTestEdgeCases(OH_NativeBuffer *buffer)
{
    OH_NativeBuffer_Config config;
    OH_NativeBuffer_GetConfig(nullptr, &config);
    OH_NativeBuffer_GetConfig(buffer, nullptr);

    OH_NativeBuffer_ColorSpace invalidColorSpace =
        static_cast<OH_NativeBuffer_ColorSpace>(INVALID_COLOR_SPACE_VALUE);
    OH_NativeBuffer_SetColorSpace(nullptr, OH_COLORSPACE_SRGB_FULL);
    OH_NativeBuffer_SetColorSpace(buffer, invalidColorSpace);

    OH_NativeBuffer_ColorSpace colorSpace;
    OH_NativeBuffer_GetColorSpace(nullptr, &colorSpace);
    OH_NativeBuffer_GetColorSpace(buffer, nullptr);

    uint8_t metadata[EDGE_CASE_METADATA_SIZE] = {0};
    OH_NativeBuffer_SetMetadataValue(nullptr, OH_HDR_DYNAMIC_METADATA,
        EDGE_CASE_METADATA_SIZE, metadata);
    OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_DYNAMIC_METADATA,
        EDGE_CASE_METADATA_SIZE, nullptr);
    OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_DYNAMIC_METADATA, 0, metadata);
    OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_DYNAMIC_METADATA,
        INVALID_FENCE_FD, metadata);

    int32_t getSize = 0;
    uint8_t *getMetadata = nullptr;
    OH_NativeBuffer_GetMetadataValue(nullptr, OH_HDR_DYNAMIC_METADATA, &getSize, &getMetadata);
    OH_NativeBuffer_GetMetadataValue(buffer, OH_HDR_DYNAMIC_METADATA, nullptr, &getMetadata);
    OH_NativeBuffer_GetMetadataValue(buffer, OH_HDR_DYNAMIC_METADATA, &getSize, nullptr);

    OH_NativeBuffer_IsSupported(config, nullptr);

    void *addr = nullptr;
    OH_NativeBuffer_MapWaitFence(nullptr, INVALID_FENCE_FD, &addr);
    OH_NativeBuffer_MapWaitFence(buffer, INVALID_FENCE_FD, &addr);

    OH_NativeBuffer *tmpBuffer = nullptr;
    OH_NativeBuffer_WriteToParcel(nullptr, nullptr);
    OH_NativeBuffer_ReadFromParcel(nullptr, &tmpBuffer);
}

void TestMetadataOperations(OH_NativeBuffer *buffer, FuzzedDataProvider& fdp)
{
    OH_NativeBuffer_MetadataKey metadataKey =
        static_cast<OH_NativeBuffer_MetadataKey>(fdp.ConsumeIntegral<int32_t>());
    uint32_t setSize = fdp.ConsumeIntegralInRange<uint32_t>(0, METADATA_SIZE_MAX);
    if (metadataKey == OH_REGION_OF_INTEREST_METADATA) {
        setSize = fdp.ConsumeIntegralInRange<uint32_t>(0, ROI_METADATA_FUZZ_TEST_RANGE);
    }
    uint8_t *metadata = static_cast<uint8_t *>(malloc(setSize * sizeof(uint8_t)));
    if (metadata != nullptr) {
        for (uint32_t i = 0; i < setSize; ++i) {
            metadata[i] = fdp.ConsumeIntegral<uint8_t>();
        }
    }
    OH_NativeBuffer_SetMetadataValue(buffer, metadataKey, static_cast<int32_t>(setSize), metadata);
    free(metadata);
    metadata = nullptr;

    if (metadataKey == OH_REGION_OF_INTEREST_METADATA || fdp.ConsumeBool()) {
        uint8_t dummyBuff[ROI_METADATA_FUZZ_INVALID_SIZE] = {0};
        OH_NativeBuffer_SetMetadataValue(nullptr, OH_REGION_OF_INTEREST_METADATA,
            ROI_METADATA_FUZZ_INVALID_SIZE, dummyBuff);
        OH_NativeBuffer_SetMetadataValue(buffer, OH_REGION_OF_INTEREST_METADATA, 0, dummyBuff);
        OH_NativeBuffer_SetMetadataValue(buffer, OH_REGION_OF_INTEREST_METADATA,
            INVALID_FENCE_FD, dummyBuff);
        OH_NativeBuffer_SetMetadataValue(buffer, OH_REGION_OF_INTEREST_METADATA,
            ROI_METADATA_CAPACITY, nullptr);
    }

    int32_t getSize = 0;
    uint8_t *getMetadata = nullptr;
    OH_NativeBuffer_GetMetadataValue(buffer, metadataKey, &getSize, &getMetadata);
    if (getMetadata != nullptr) {
        free(getMetadata);
        getMetadata = nullptr;
    }

    if (metadataKey == OH_REGION_OF_INTEREST_METADATA || fdp.ConsumeBool()) {
        OH_NativeBuffer_GetMetadataValue(nullptr, OH_REGION_OF_INTEREST_METADATA,
            &getSize, &getMetadata);
        OH_NativeBuffer_GetMetadataValue(buffer, OH_REGION_OF_INTEREST_METADATA,
            nullptr, &getMetadata);
        OH_NativeBuffer_GetMetadataValue(buffer, OH_REGION_OF_INTEREST_METADATA,
            &getSize, nullptr);
    }
}

void TestBufferBasicOperations(OH_NativeBuffer *buffer, OH_NativeBuffer_Config &checkConfig, void *virAddr)
{
    OH_NativeBuffer_GetConfig(buffer, &checkConfig);
    OH_NativeBuffer_Reference(buffer);
    OH_NativeBuffer_Unreference(buffer);
    OH_NativeBuffer_Map(buffer, &virAddr);
    OH_NativeBuffer_Unmap(buffer);
    OH_NativeBuffer_MapWaitFence(buffer, INVALID_FENCE_FD, &virAddr);
    int32_t nullFd = open("/dev/null", O_RDONLY);
    if (nullFd >= 0) {
        OH_NativeBuffer_MapWaitFence(buffer, nullFd, &virAddr);
        close(nullFd);
    }
    OH_NativeBuffer_Unreference(buffer);
}

void TestWindowBufferOperations(OH_NativeBuffer *buffer, OH_NativeBuffer_Config &config,
    FuzzedDataProvider& fdp)
{
    OHNativeWindowBuffer *nativeWindowBuffer = CreateNativeWindowBufferFromNativeBuffer(buffer);
    if (nativeWindowBuffer == nullptr) {
        return;
    }
    OH_NativeBufferFromNativeWindowBuffer(nativeWindowBuffer);
    OH_NativeBuffer *nativeBuffer = nullptr;
    OH_NativeBuffer_FromNativeWindowBuffer(nativeWindowBuffer, &nativeBuffer);

    OHIPCParcel *parcel = OH_IPCParcel_Create();
    if (parcel != nullptr) {
        OH_NativeBuffer_WriteToParcel(nativeBuffer, parcel);
        OH_NativeBuffer *outBuffer = nullptr;
        OH_NativeBuffer_ReadFromParcel(parcel, &outBuffer);
        OH_IPCParcel_Destroy(parcel);
        if (outBuffer != nullptr) {
            OH_NativeBuffer_Unreference(outBuffer);
        }
    }

    bool isSupported = false;
    OH_NativeBuffer_IsSupported(config, &isSupported);
    void *virAddr1 = nullptr;
    OH_NativeBuffer_Config testConfig = {};
    OH_NativeBuffer_MapAndGetConfig(nativeBuffer, &virAddr1, &testConfig);

    OH_NativeBuffer_ColorSpace getColorSpace;
    OH_NativeBuffer_GetColorSpace(buffer, &getColorSpace);

    std::string virAddrStr = fdp.ConsumeRandomLengthString(STR_LEN_MAX);
    void *virAddr = static_cast<void*>(virAddrStr.data());
    TestMetadataOperations(buffer, fdp);
    TestBufferBasicOperations(buffer, config, virAddr);

    DestroyNativeWindowBuffer(nativeWindowBuffer);
    OH_NativeBuffer_Unreference(buffer);
    NativeBufferFuzzTestEdgeCases(buffer);
}

void TestFallbackOperations(OH_NativeBuffer *buffer, OH_NativeBuffer_ColorSpace colorSpace,
    OH_NativeBuffer_Config &checkConfig, FuzzedDataProvider& fdp)
{
    OH_NativeBuffer_SetColorSpace(buffer, colorSpace);
    void *getVirAddr = nullptr;
    OH_NativeBuffer_Planes outPlanes;
    OH_NativeBuffer_MapPlanes(buffer, &getVirAddr, &outPlanes);
    OH_NativeBuffer_ColorSpace getColorSpace;
    OH_NativeBuffer_GetColorSpace(buffer, &getColorSpace);

    OH_NativeBuffer_MetadataKey metadataKey =
        static_cast<OH_NativeBuffer_MetadataKey>(fdp.ConsumeIntegral<int32_t>());
    uint32_t setSize = fdp.ConsumeIntegralInRange<uint32_t>(0, METADATA_SIZE_MAX);
    uint8_t *metadata = static_cast<uint8_t *>(malloc(setSize * sizeof(uint8_t)));
    if (metadata != nullptr) {
        OH_NativeBuffer_SetMetadataValue(buffer, metadataKey,
            static_cast<int32_t>(setSize), metadata);
        free(metadata);
    }

    int32_t getSize = 0;
    uint8_t *getMetadata = nullptr;
    OH_NativeBuffer_GetMetadataValue(buffer, metadataKey, &getSize, &getMetadata);

    std::string virAddrStr = fdp.ConsumeRandomLengthString(STR_LEN_MAX);
    void *virAddr = static_cast<void*>(virAddrStr.data());
    TestBufferBasicOperations(buffer, checkConfig, virAddr);
}

bool DoSomethingInterestingWithMyAPI(FuzzedDataProvider& fdp)
{
    OH_NativeBuffer_Config config;
    config.width = fdp.ConsumeIntegral<int32_t>();
    config.height = fdp.ConsumeIntegral<int32_t>();
    config.format = fdp.ConsumeIntegral<int32_t>();
    config.usage = fdp.ConsumeIntegral<uint64_t>();

    OH_NativeBuffer_Config checkConfig;
    checkConfig.width = fdp.ConsumeIntegral<int32_t>();
    checkConfig.height = fdp.ConsumeIntegral<int32_t>();
    checkConfig.format = fdp.ConsumeIntegral<int32_t>();
    checkConfig.usage = fdp.ConsumeIntegral<uint64_t>();

    OH_NativeBuffer_ColorSpace colorSpace =
        static_cast<OH_NativeBuffer_ColorSpace>(fdp.ConsumeIntegral<int32_t>());

    OH_NativeBuffer* buffer = OH_NativeBuffer_Alloc(&config);
    if (buffer == nullptr) {
        return false;
    }

    OH_NativeBuffer_GetSeqNum(buffer);
    OH_NativeBuffer_GetBufferHandle(buffer);
    OH_NativeBuffer_GetNativeBufferConfig(buffer, &checkConfig);

    OHNativeWindowBuffer *nativeWindowBuffer = CreateNativeWindowBufferFromNativeBuffer(buffer);
    if (nativeWindowBuffer != nullptr) {
        TestWindowBufferOperations(buffer, config, fdp);
        return true;
    }

    TestFallbackOperations(buffer, colorSpace, checkConfig, fdp);
    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    FuzzedDataProvider fdp(data, size);
    OHOS::DoSomethingInterestingWithMyAPI(fdp);
    return 0;
}