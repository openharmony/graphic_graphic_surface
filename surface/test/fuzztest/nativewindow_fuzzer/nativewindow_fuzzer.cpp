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

#include "nativewindow_fuzzer.h"

#include <fuzzer/FuzzedDataProvider.h>

#include <securec.h>
#include <vector>
#include <string>

#include "ipc_inner_object.h"
#include "ipc_cparcel.h"
#include "data_generate.h"
#include "surface.h"
#include "surface_buffer.h"
#include "surface_buffer_impl.h"
#include "ibuffer_producer.h"
#include "iconsumer_surface.h"
#include "native_window.h"
#include "native_buffer.h"
#include "native_buffer_inner.h"

using namespace g_fuzzCommon;
namespace OHOS {
    namespace {
        constexpr uint32_t MATRIX_SIZE = (1U << 4);
        constexpr size_t STR_LEN = (5U << 1);
        constexpr int DEFAULT_FENCE = ((1 << 6) + (1 << 5) + (1 << 2));
        constexpr uint32_t DEFAULT_BUFFER_SIZE = ((1U << 6) + (1U << 5) + (1U << 2));
        const uint8_t* g_data = nullptr;
        size_t g_size = 0;
        size_t g_pos = 0;
    }

    void HandleOpt(OHNativeWindow *nativeWindow)
    {
        int code = SET_USAGE;
        uint64_t usage = GetData<OH_NativeBuffer_Usage>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, usage);
        code = SET_BUFFER_GEOMETRY;
        int32_t height = GetData<int32_t>() % 10000;
        int32_t width = GetData<int32_t>() % 10000;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, height, width);
        code = SET_FORMAT;
        int32_t format = GetData<OH_NativeBuffer_Format>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, format);
        code = SET_STRIDE;
        int32_t stride = GetData<int32_t>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, stride);
        code = GET_USAGE;
        uint64_t usageGet = BUFFER_USAGE_CPU_WRITE;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &usageGet);
        code = GET_BUFFER_GEOMETRY;
        int32_t heightGet = 0;
        int32_t widthGet = 0;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &heightGet, &widthGet);
        code = GET_FORMAT;
        int32_t formatGet = GRAPHIC_PIXEL_FMT_CLUT8;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &formatGet);
        code = SET_STRIDE;
        int32_t strideSet = (1 << 3);
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, strideSet);
        code = GET_STRIDE;
        int32_t strideGet = 0;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &strideGet);
        code = SET_TIMEOUT;
        int32_t timeoutSet = GetData<int32_t>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, timeoutSet);
        code = SET_COLOR_GAMUT;
        int32_t colorGamutSet = GetData<OH_NativeBuffer_ColorGamut>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, colorGamutSet);
        code = SET_TRANSFORM;
        int32_t transform = GetData<OH_NativeBuffer_TransformType>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, transform);
    }

    void HandleOpt1(OHNativeWindow *nativeWindow)
    {
        int code = SET_UI_TIMESTAMP;
        uint64_t uiTimestamp = GetData<uint64_t>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, uiTimestamp);
        code = SET_DESIRED_PRESENT_TIMESTAMP;
        int64_t desiredPresentTimestamp = GetData<int64_t>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, desiredPresentTimestamp);
        code = SET_SOURCE_TYPE;
        OHSurfaceSource typeSet = GetData<OHSurfaceSource>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, typeSet);
        code = GET_SOURCE_TYPE;
        OHSurfaceSource typeGet = OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &typeGet);
        code = SET_APP_FRAMEWORK_TYPE;
        std::string frameworkType = GetStringFromData(STR_LEN);
        const char* frameWorkTypeSet = frameworkType.c_str();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, frameWorkTypeSet);
        code = GET_TIMEOUT;
        int32_t timeoutGet = 0;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &timeoutGet);
        code = GET_COLOR_GAMUT;
        int32_t colorGamutGet = 0;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &colorGamutGet);
        code = GET_TRANSFORM;
        int32_t transformTmp = 0;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &transformTmp);
        code = GET_BUFFERQUEUE_SIZE;
        int32_t queueSize = 0;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &queueSize);
        code = GET_APP_FRAMEWORK_TYPE;
        const char* frameWorkTypeGet;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &frameWorkTypeGet);
        code = SET_HDR_WHITE_POINT_BRIGHTNESS;
        float brightness = GetData<float>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, brightness);
        code = SET_SDR_WHITE_POINT_BRIGHTNESS;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, brightness);
    }

    void NativeWindowFuzzTest(OHNativeWindow *nativeWindow, OHNativeWindowBuffer *nwBuffer)
    {
        // get data
        int fenceFd = GetData<int>() % 32768; // maximum fd of linux is 32768
        // fd 0,1,2 represent stdin, stdout and stderr respectively, they should not be closed.
        if (fenceFd >= 0 && fenceFd <= 2) {
            fenceFd = DEFAULT_FENCE;
        }

        Region::Rect rect = GetData<Region::Rect>();
        Region region = {.rects = &rect, .rectNumber = 1};
        OHScalingMode scalingMode = GetData<OHScalingMode>();
        OHScalingModeV2 scalingModeV2 = GetData<OHScalingModeV2>();
        NativeWindowRequestBuffer(nativeWindow, &nwBuffer, &fenceFd);
        NativeWindowFlushBuffer(nativeWindow, nwBuffer, fenceFd, region);
        NativeWindowCancelBuffer(nativeWindow, nwBuffer);
        GetBufferHandleFromNative(nwBuffer);
        float matrix[MATRIX_SIZE] = {0};
        for (size_t i = 0; i < MATRIX_SIZE; ++i) {
            matrix[i] = GetData<float>();
        }
        GetLastFlushedBuffer(nativeWindow, &nwBuffer, &fenceFd, matrix);
        GetLastFlushedBufferV2(nativeWindow, &nwBuffer, &fenceFd, matrix);
        NativeWindowAttachBuffer(nativeWindow, nwBuffer);
        NativeWindowDetachBuffer(nativeWindow, nwBuffer);
        uint32_t sequence = GetData<uint32_t>();
        NativeWindowSetScalingMode(nativeWindow, sequence, scalingMode);
        NativeWindowSetScalingModeV2(nativeWindow, scalingModeV2);
        OH_NativeBuffer_TransformType transform = GetData<OH_NativeBuffer_TransformType>();
        NativeWindowSetTransformHint(nativeWindow, transform);
        NativeWindowGetTransformHint(nativeWindow, &transform);
    }

    void NativeWindowFuzzTest1(OHNativeWindow *nativeWindow, OHNativeWindowBuffer *nwBuffer)
    {
        uint64_t surfaceId = 0;
        GetSurfaceId(nativeWindow, &surfaceId);
        OHHDRMetaData metaData = GetData<OHHDRMetaData>();
        OHHDRMetadataKey key = GetData<OHHDRMetadataKey>();
        uint8_t metaData2[STR_LEN];
        for (uint64_t i = 0; i < STR_LEN; i++) {
            metaData2[i] = GetData<uint8_t>();
        }
        uint32_t reserveInts = GetData<uint32_t>() % 0x100000; // no more than 0x100000
        int32_t width = GetData<int32_t>();
        int32_t height = GetData<int32_t>();
        NativeWindowSetRequestWidthAndHeight(nativeWindow, width, height);
        NativeWindowGetDefaultWidthAndHeight(nativeWindow, &width, &height);
        NativeWindowSetBufferHold(nativeWindow);
        OHIPCParcel *parcel = OH_IPCParcel_Create();
        NativeWindowWriteToParcel(nativeWindow, parcel);
        NativeWindowReadFromParcel(parcel, &nativeWindow);
        OH_IPCParcel_Destroy(parcel);
        OHNativeWindow *nativeWindowTmp;
        CreateNativeWindowFromSurfaceId(surfaceId, &nativeWindowTmp);
        DestoryNativeWindow(nativeWindowTmp);
        OH_NativeBuffer_ColorSpace space = GetData<OH_NativeBuffer_ColorSpace>();
        OH_NativeWindow_SetColorSpace(nativeWindow, space);
        OH_NativeWindow_GetColorSpace(nativeWindow, &space);
        OH_NativeBuffer_MetadataKey metaKey = GetData<OH_NativeBuffer_MetadataKey>();
        int32_t len = GetData<int32_t>() % 256;
        if (len <= 0) {
            len = 1;
        }
        std::vector<uint8_t> buff(static_cast<size_t>(len));
        for (int32_t i = 0; i < len; ++i) {
            buff[static_cast<size_t>(i)] = GetData<uint8_t>();
        }
        OH_NativeWindow_SetMetadataValue(nativeWindow, metaKey, len, buff.data());
        uint8_t *checkMetaData = nullptr;
        OH_NativeWindow_GetMetadataValue(nativeWindow, metaKey, &len, &checkMetaData);
        delete[] checkMetaData;
        std::vector<OHHDRMetaData> metaDatas = {metaData};
        uint32_t sequence = GetData<uint32_t>();
        NativeWindowSetMetaData(nativeWindow, sequence, metaDatas.size(), metaDatas.data());
        NativeWindowSetMetaDataSet(nativeWindow, sequence, key, STR_LEN, metaData2);
        OHExtDataHandle *handle = reinterpret_cast<OHExtDataHandle *>(AllocExtDataHandle(reserveInts));
        NativeWindowSetTunnelHandle(nativeWindow, reinterpret_cast<OHExtDataHandle *>(handle));
        FreeExtDataHandle(reinterpret_cast<GraphicExtDataHandle *>(handle));
        NativeWindowDisconnect(nativeWindow);
        DestroyNativeWindowBuffer(nwBuffer);
    }

    void NativeWindowFuzzTestObjectApis(OHNativeWindow *nativeWindow, OHNativeWindowBuffer *nwBuffer)
    {
        GetNativeObjectMagic(nullptr);
        GetNativeObjectMagic(nativeWindow);
        NativeObjectReference(nullptr);
        NativeObjectUnreference(nullptr);
        NativeObjectReference(nativeWindow);
        NativeObjectUnreference(nativeWindow);
        if (nwBuffer != nullptr) {
            GetNativeObjectMagic(nwBuffer);
            NativeObjectReference(nwBuffer);
            NativeObjectUnreference(nwBuffer);
        }

        OH_NativeBuffer_ColorSpace converted = OH_COLORSPACE_NONE;
        ConvertColorSpaceTypeToNativeBufferColorSpace(GetData<int32_t>(), &converted);
        ConvertColorSpaceTypeToNativeBufferColorSpace(GetData<int32_t>(), nullptr);
    }

    void NativeWindowFuzzTest2(OHNativeWindow *nativeWindow)
    {
        // Stability test, repeat lock->unlock 10 times
        for (uint32_t i = 0; i < 10; i++) {
            OHNativeWindowBuffer* buffer = nullptr;
            Region::Rect rect = {0};
            rect.x = (1 << 8);
            rect.y = (1 << 8);
            rect.w = (1 << 8);
            rect.h = (1 << 8);
            Region region = {.rects = &rect, .rectNumber = 1};
            NativeWindowLockBuffer(nativeWindow, region, &buffer);
            NativeWindowUnlockAndFlushBuffer(nativeWindow);
        }
    }

    // Test CleanCache and PreAllocBuffers interfaces
    void NativeWindowFuzzTest3(OHNativeWindow *nativeWindow)
    {
        // Test CleanCache interface
        NativeWindowCleanCache(nativeWindow);

        // Test PreAllocBuffers interface - use fuzz data to generate buffer count
        uint32_t allocBufferCnt = GetData<uint32_t>() % 100 + 1;  // Limit to 1-100 range
        NativeWindowPreAllocBuffers(nativeWindow, allocBufferCnt);

        // Test boundary case: allocBufferCnt = 0 (should return error)
        NativeWindowPreAllocBuffers(nativeWindow, 0);

        // Test boundary case: allocBufferCnt = 1 (minimum valid value)
        NativeWindowPreAllocBuffers(nativeWindow, 1);

        // Test boundary case: allocBufferCnt = max value (test overflow handling)
        allocBufferCnt = GetData<uint32_t>();
        if (allocBufferCnt == 0) {
            allocBufferCnt = 1;
        }
        NativeWindowPreAllocBuffers(nativeWindow, allocBufferCnt);
    }

    // Test null pointer and exception scenarios
    void NativeWindowFuzzTestEdgeCases(OHNativeWindow *nativeWindow, OHNativeWindowBuffer *nwBuffer)
    {
        // Test null pointer scenarios
        int fenceFd = GetData<int>();
        NativeWindowRequestBuffer(nullptr, &nwBuffer, &fenceFd);
        NativeWindowRequestBuffer(nativeWindow, nullptr, &fenceFd);
        NativeWindowFlushBuffer(nullptr, nwBuffer, GetData<int>(), {.rects = nullptr, .rectNumber = 0});
        NativeWindowFlushBuffer(nativeWindow, nullptr, GetData<int>(), {.rects = nullptr, .rectNumber = 0});
        NativeWindowCancelBuffer(nullptr, nwBuffer);
        NativeWindowCancelBuffer(nativeWindow, nullptr);

        // Test CleanCache null pointer
        NativeWindowCleanCache(nullptr);

        // Test PreAllocBuffers null pointer
        NativeWindowPreAllocBuffers(nullptr, GetData<uint32_t>());

        // Test LockBuffer null pointer
        OHNativeWindowBuffer* buffer = nullptr;
        Region region = {.rects = nullptr, .rectNumber = 0};
        NativeWindowLockBuffer(nullptr, region, &buffer);
        NativeWindowLockBuffer(nativeWindow, region, nullptr);

        // Test UnlockAndFlushBuffer null pointer
        NativeWindowUnlockAndFlushBuffer(nullptr);

        // Test AttachBuffer/DetachBuffer null pointer
        NativeWindowAttachBuffer(nullptr, nwBuffer);
        NativeWindowAttachBuffer(nativeWindow, nullptr);
        NativeWindowDetachBuffer(nullptr, nwBuffer);
        NativeWindowDetachBuffer(nativeWindow, nullptr);

        // Test GetLastFlushedBuffer null pointer
        int edgeFenceFd = GetData<int>();
        float matrix[MATRIX_SIZE] = {0};
        GetLastFlushedBuffer(nullptr, &nwBuffer, &edgeFenceFd, matrix);
        GetLastFlushedBuffer(nativeWindow, nullptr, &edgeFenceFd, matrix);

        // Test SetScalingMode boundary values
        OHScalingMode scalingMode = (OHScalingMode)(GetData<int32_t>() % 100 - 50);  // Including invalid values
        NativeWindowSetScalingMode(nativeWindow, GetData<uint32_t>(), scalingMode);

        OHScalingModeV2 scalingModeV2 = (OHScalingModeV2)(GetData<int32_t>() % 100 - 50);  // Including invalid values
        NativeWindowSetScalingModeV2(nativeWindow, scalingModeV2);
    }

    // Test color space and metadata interface boundary scenarios
    void NativeWindowFuzzTestColorSpaceAndMetadata(OHNativeWindow *nativeWindow)
    {
        // Test all color space enum values
        OH_NativeBuffer_ColorSpace colorSpace;
        int32_t colorSpaceInt = GetData<int32_t>();
        colorSpace = (OH_NativeBuffer_ColorSpace)colorSpaceInt;
        OH_NativeWindow_SetColorSpace(nativeWindow, colorSpace);
        OH_NativeWindow_GetColorSpace(nativeWindow, &colorSpace);

        // Test metadata interface
        OH_NativeBuffer_MetadataKey metaKey = OH_HDR_DYNAMIC_METADATA;
        int32_t testSize = GetData<int32_t>() % 100;  // Limit test size
        if (testSize < 0) {
            testSize = 0;
        }
        uint8_t* testMetadata = nullptr;
        if (testSize > 0) {
            testMetadata = new uint8_t[testSize];
            for (int i = 0; i < testSize; i++) {
                testMetadata[i] = GetData<uint8_t>();
            }
        }
        OH_NativeWindow_SetMetadataValue(nativeWindow, metaKey, testSize, testMetadata);

        // Test null pointer scenarios
        OH_NativeWindow_SetMetadataValue(nullptr, metaKey, testSize, testMetadata);
        OH_NativeWindow_SetMetadataValue(nativeWindow, metaKey, testSize, nullptr);
        OH_NativeWindow_SetMetadataValue(nativeWindow, metaKey, -1, testMetadata);  // Invalid size
        const int extraLargeSize = 1000 * 1000;
        OH_NativeWindow_SetMetadataValue(nativeWindow, metaKey, extraLargeSize, testMetadata);  // Extra large size

        int32_t getSize = 0;
        uint8_t* getMetadata = nullptr;
        OH_NativeWindow_GetMetadataValue(nativeWindow, metaKey, &getSize, &getMetadata);
        delete[] getMetadata;

        // Test null pointer scenarios
        OH_NativeWindow_GetMetadataValue(nullptr, metaKey, &getSize, &getMetadata);
        OH_NativeWindow_GetMetadataValue(nativeWindow, metaKey, nullptr, &getMetadata);
        OH_NativeWindow_GetMetadataValue(nativeWindow, metaKey, &getSize, nullptr);

        delete[] testMetadata;
    }

    // Test CreateNativeWindowBufferFromNativeBuffer interface
    void NativeWindowFuzzTestCreateBufferFromNative(OHNativeWindow *nativeWindow)
    {
        // Test with valid OH_NativeBuffer created from SurfaceBuffer
        int fenceFd = 0;
        OHNativeWindowBuffer *nwBuffer = nullptr;
        NativeWindowRequestBuffer(nativeWindow, &nwBuffer, &fenceFd);
        if (nwBuffer != nullptr) {
            // Get OH_NativeBuffer from nativeWindowBuffer
            OH_NativeBuffer *nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(nwBuffer);
            if (nativeBuffer != nullptr) {
                // Test CreateNativeWindowBufferFromNativeBuffer with valid buffer
                OHNativeWindowBuffer *nwBufferFromNative = CreateNativeWindowBufferFromNativeBuffer(nativeBuffer);
                if (nwBufferFromNative != nullptr) {
                    DestroyNativeWindowBuffer(nwBufferFromNative);
                }
                OH_NativeBuffer_Unreference(nativeBuffer);
            }
            DestroyNativeWindowBuffer(nwBuffer);
        }

        // Test with nullptr - should return nullptr
        OHNativeWindowBuffer *nullBuffer = CreateNativeWindowBufferFromNativeBuffer(nullptr);
        if (nullBuffer != nullptr) {
            DestroyNativeWindowBuffer(nullBuffer);
        }

        // Test with allocated OH_NativeBuffer
        OH_NativeBuffer_Config config;
        config.width = DEFAULT_BUFFER_SIZE;
        config.height = DEFAULT_BUFFER_SIZE;
        config.format = GRAPHIC_PIXEL_FMT_RGBA_8888;
        config.usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE;
        config.stride = 0;
        OH_NativeBuffer *allocatedBuffer = OH_NativeBuffer_Alloc(&config);
        if (allocatedBuffer != nullptr) {
            OHNativeWindowBuffer *nwBufferFromAlloc = CreateNativeWindowBufferFromNativeBuffer(allocatedBuffer);
            if (nwBufferFromAlloc != nullptr) {
                DestroyNativeWindowBuffer(nwBufferFromAlloc);
            }
            OH_NativeBuffer_Unreference(allocatedBuffer);
        }
    }

    // Test NativeWindowSetGameUpscaleProcessor interface
    void NativeWindowFuzzTestGameUpscaleProcessor(OHNativeWindow *nativeWindow)
    {
        // Define a test processor callback
        static void (*testProcessor)(int32_t *, int32_t *) = nullptr;

        // Test with nullptr window
        NativeWindowSetGameUpscaleProcessor(nullptr, testProcessor);

        // Test with nullptr processor (should clear the processor)
        NativeWindowSetGameUpscaleProcessor(nativeWindow, nullptr);

        // Test with valid processor callback
        static auto processorCallback = [](int32_t *width, int32_t *height) -> void {
            if (width != nullptr) {
                *width = 192 * 10;
            }
            if (height != nullptr) {
                *height = 108 * 10;
            }
        };
        NativeWindowSetGameUpscaleProcessor(nativeWindow, processorCallback);

        // Test with another processor to verify replacement
        static auto processorCallback2 = [](int32_t *width, int32_t *height) -> void {
            if (width != nullptr) {
                *width = 384 * 10;
            }
            if (height != nullptr) {
                *height = 216 * 10;
            }
        };
        NativeWindowSetGameUpscaleProcessor(nativeWindow, processorCallback2);

        // Test clearing the processor
        NativeWindowSetGameUpscaleProcessor(nativeWindow, nullptr);
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

        // test
        sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create();
        sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
        cSurface->RegisterConsumerListener(listener);
        sptr<OHOS::IBufferProducer> producer = cSurface->GetProducer();
        sptr<OHOS::Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
        cSurface->SetDefaultWidthAndHeight(0x100, 0x100); // width and height is 0x100
        OHNativeWindow* nativeWindow = CreateNativeWindowFromSurface(&pSurface);
        uint32_t seqNum = GetData<uint32_t>();
        sptr<OHOS::SurfaceBuffer> sBuffer = new SurfaceBufferImpl(seqNum);
        OHNativeWindowBuffer* nwBuffer = CreateNativeWindowBufferFromSurfaceBuffer(&sBuffer);
        DestroyNativeWindowBuffer(nwBuffer);
        nwBuffer = nullptr;
        HandleOpt(nativeWindow);
        HandleOpt1(nativeWindow);
        NativeWindowFuzzTest(nativeWindow, nwBuffer);
        NativeWindowFuzzTest1(nativeWindow, nwBuffer);
        NativeWindowFuzzTest2(nativeWindow);
        NativeWindowFuzzTest3(nativeWindow);
        NativeWindowFuzzTestEdgeCases(nativeWindow, nwBuffer);
        NativeWindowFuzzTestColorSpaceAndMetadata(nativeWindow);
        NativeWindowFuzzTestCreateBufferFromNative(nativeWindow);
        NativeWindowFuzzTestGameUpscaleProcessor(nativeWindow);
        NativeWindowFuzzTestObjectApis(nativeWindow, nwBuffer);
        DestoryNativeWindow(nativeWindow);
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

