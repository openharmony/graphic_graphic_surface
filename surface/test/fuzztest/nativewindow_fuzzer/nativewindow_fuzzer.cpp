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
#include <cstdint>
#include <vector>
#include <string>

#include "ipc_inner_object.h"
#include "ipc_cparcel.h"
#include "surface.h"
#include "surface_buffer.h"
#include "surface_buffer_impl.h"
#include "ibuffer_producer.h"
#include "iconsumer_surface.h"
#include "native_window.h"
#include "native_buffer.h"
#include "native_buffer_inner.h"

namespace OHOS {
namespace {

class FuzzBufferConsumerListener : public IBufferConsumerListener {
public:
    void OnBufferAvailable() override {}
};

constexpr uint32_t MATRIX_SIZE = (1U << 4);
constexpr size_t STR_LEN_MAX = (5U << 1);
constexpr int DEFAULT_FENCE = ((1 << 6) + (1 << 5) + (1 << 2));
constexpr uint32_t DEFAULT_BUFFER_SIZE = ((1U << 6) + (1U << 5) + (1U << 2));
constexpr int32_t DEFAULT_REGION_EDGE = 256;
constexpr int32_t DEFAULT_WIDTH_HEIGHT = 0x100;
constexpr int FENCE_FD_MAX = 32768;
constexpr int32_t STD_FD_MAX = 2;
constexpr int32_t BUFFER_GEOMETRY_MAX = 10000;
constexpr uint32_t RESERVE_INTS_MAX = 0x100000;
constexpr int32_t META_DATA_LEN_MAX = 256;
constexpr uint32_t STABILITY_LOOP_COUNT = 10;
constexpr uint32_t PREALLOC_BUFFER_MAX = 100;
constexpr int32_t SCALING_MODE_RANGE = 100;
constexpr int32_t SCALING_MODE_OFFSET = 50;
constexpr int32_t METADATA_TEST_SIZE_MAX = 100;
constexpr int32_t EXTRA_LARGE_METADATA_SIZE = 1000 * 1000;
constexpr int32_t UPSCALE_WIDTH_1920 = 1920;
constexpr int32_t UPSCALE_HEIGHT_1080 = 1080;
constexpr int32_t UPSCALE_WIDTH_3840 = 3840;
constexpr int32_t UPSCALE_HEIGHT_2160 = 2160;

} // namespace

void HandleOpt(OHNativeWindow *nativeWindow, FuzzedDataProvider& fdp)
{
    int code = SET_USAGE;
    uint64_t usage = fdp.ConsumeIntegral<uint64_t>();
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, usage);
    code = SET_BUFFER_GEOMETRY;
    int32_t height = fdp.ConsumeIntegralInRange<int32_t>(0, BUFFER_GEOMETRY_MAX);
    int32_t width = fdp.ConsumeIntegralInRange<int32_t>(0, BUFFER_GEOMETRY_MAX);
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, height, width);
    code = SET_FORMAT;
    int32_t format = fdp.ConsumeIntegral<int32_t>();
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, format);
    code = SET_STRIDE;
    int32_t stride = fdp.ConsumeIntegral<int32_t>();
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
    int32_t timeoutSet = fdp.ConsumeIntegral<int32_t>();
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, timeoutSet);
    code = SET_COLOR_GAMUT;
    int32_t colorGamutSet = fdp.ConsumeIntegral<int32_t>();
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, colorGamutSet);
    code = SET_TRANSFORM;
    int32_t transform = fdp.ConsumeIntegral<int32_t>();
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, transform);
}

void HandleOpt1(OHNativeWindow *nativeWindow, FuzzedDataProvider& fdp)
{
    int code = SET_UI_TIMESTAMP;
    uint64_t uiTimestamp = fdp.ConsumeIntegral<uint64_t>();
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, uiTimestamp);
    code = SET_DESIRED_PRESENT_TIMESTAMP;
    int64_t desiredPresentTimestamp = fdp.ConsumeIntegral<int64_t>();
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, desiredPresentTimestamp);
    code = SET_SOURCE_TYPE;
    OHSurfaceSource typeSet = static_cast<OHSurfaceSource>(fdp.ConsumeIntegral<int32_t>());
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, typeSet);
    code = GET_SOURCE_TYPE;
    OHSurfaceSource typeGet = OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &typeGet);
    code = SET_APP_FRAMEWORK_TYPE;
    std::string frameworkType = fdp.ConsumeRandomLengthString(STR_LEN_MAX);
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
    const char* frameWorkTypeGet = nullptr;
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &frameWorkTypeGet);
    code = SET_HDR_WHITE_POINT_BRIGHTNESS;
    float brightness = fdp.ConsumeFloatingPoint<float>();
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, brightness);
    code = SET_SDR_WHITE_POINT_BRIGHTNESS;
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, brightness);
}

void NativeWindowFuzzTest(OHNativeWindow *nativeWindow, OHNativeWindowBuffer *nwBuffer,
    FuzzedDataProvider& fdp)
{
    int fenceFd = fdp.ConsumeIntegralInRange<int>(0, FENCE_FD_MAX);
    if (fenceFd >= 0 && fenceFd <= STD_FD_MAX) {
        fenceFd = DEFAULT_FENCE;
    }

    Region::Rect rect;
    rect.x = fdp.ConsumeIntegral<int32_t>();
    rect.y = fdp.ConsumeIntegral<int32_t>();
    rect.w = fdp.ConsumeIntegral<int32_t>();
    rect.h = fdp.ConsumeIntegral<int32_t>();
    Region region = {.rects = &rect, .rectNumber = 1};
    OHScalingMode scalingMode = static_cast<OHScalingMode>(fdp.ConsumeIntegral<int32_t>());
    OHScalingModeV2 scalingModeV2 = static_cast<OHScalingModeV2>(fdp.ConsumeIntegral<int32_t>());
    NativeWindowRequestBuffer(nativeWindow, &nwBuffer, &fenceFd);
    if (nwBuffer != nullptr) {
        NativeWindowFlushBuffer(nativeWindow, nwBuffer, fenceFd, region);
        NativeWindowCancelBuffer(nativeWindow, nwBuffer);
        GetBufferHandleFromNative(nwBuffer);
    }
    float matrix[MATRIX_SIZE] = {0};
    for (size_t i = 0; i < MATRIX_SIZE; ++i) {
        matrix[i] = fdp.ConsumeFloatingPoint<float>();
    }
    GetLastFlushedBuffer(nativeWindow, &nwBuffer, &fenceFd, matrix);
    GetLastFlushedBufferV2(nativeWindow, &nwBuffer, &fenceFd, matrix);
    if (nwBuffer != nullptr) {
        NativeWindowAttachBuffer(nativeWindow, nwBuffer);
        NativeWindowDetachBuffer(nativeWindow, nwBuffer);
    }
    uint32_t sequence = fdp.ConsumeIntegral<uint32_t>();
    NativeWindowSetScalingMode(nativeWindow, sequence, scalingMode);
    NativeWindowSetScalingModeV2(nativeWindow, scalingModeV2);
    OH_NativeBuffer_TransformType transform =
        static_cast<OH_NativeBuffer_TransformType>(fdp.ConsumeIntegral<int32_t>());
    NativeWindowSetTransformHint(nativeWindow, transform);
    NativeWindowGetTransformHint(nativeWindow, &transform);
}

void TestSurfaceIdAndParcel(OHNativeWindow *nativeWindow, uint64_t surfaceId)
{
    OHIPCParcel *parcel = OH_IPCParcel_Create();
    if (parcel != nullptr) {
        NativeWindowWriteToParcel(nativeWindow, parcel);
        NativeWindowReadFromParcel(parcel, &nativeWindow);
        OH_IPCParcel_Destroy(parcel);
    }
    OHNativeWindow *nativeWindowTmp = nullptr;
    CreateNativeWindowFromSurfaceId(surfaceId, &nativeWindowTmp);
    DestoryNativeWindow(nativeWindowTmp);
}

void TestMetadataValue(OHNativeWindow *nativeWindow, OH_NativeBuffer_MetadataKey metaKey,
    FuzzedDataProvider& fdp)
{
    int32_t len = fdp.ConsumeIntegralInRange<int32_t>(0, META_DATA_LEN_MAX);
    if (len <= 0) {
        len = 1;
    }
    std::vector<uint8_t> buff(static_cast<size_t>(len));
    for (int32_t i = 0; i < len; ++i) {
        buff[static_cast<size_t>(i)] = fdp.ConsumeIntegral<uint8_t>();
    }
    OH_NativeWindow_SetMetadataValue(nativeWindow, metaKey, len, buff.data());
    uint8_t *checkMetaData = nullptr;
    OH_NativeWindow_GetMetadataValue(nativeWindow, metaKey, &len, &checkMetaData);
    if (checkMetaData != nullptr) {
        delete[] checkMetaData;
    }
}

void TestHDRMetadata(OHNativeWindow *nativeWindow, uint32_t sequence,
    OHHDRMetaData metaData, OHHDRMetadataKey key, uint8_t *metaData2)
{
    std::vector<OHHDRMetaData> metaDatas = {metaData};
    NativeWindowSetMetaData(nativeWindow, sequence, metaDatas.size(), metaDatas.data());
    NativeWindowSetMetaDataSet(nativeWindow, sequence, key, STR_LEN_MAX, metaData2);
}

void TestTunnelHandle(OHNativeWindow *nativeWindow, uint32_t reserveInts)
{
    OHExtDataHandle *handle = reinterpret_cast<OHExtDataHandle *>(AllocExtDataHandle(reserveInts));
    if (handle != nullptr) {
        NativeWindowSetTunnelHandle(nativeWindow, reinterpret_cast<OHExtDataHandle *>(handle));
        FreeExtDataHandle(reinterpret_cast<GraphicExtDataHandle *>(handle));
    }
}

void NativeWindowFuzzTest1(OHNativeWindow *nativeWindow, OHNativeWindowBuffer *nwBuffer,
    FuzzedDataProvider& fdp)
{
    uint64_t surfaceId = 0;
    GetSurfaceId(nativeWindow, &surfaceId);
    OHHDRMetaData metaData = {
        .key = static_cast<OHHDRMetadataKey>(fdp.ConsumeIntegral<int32_t>()),
        .value = fdp.ConsumeFloatingPoint<float>(),
    };
    OHHDRMetadataKey key = static_cast<OHHDRMetadataKey>(fdp.ConsumeIntegral<int32_t>());
    uint8_t metaData2[STR_LEN_MAX] = {0};
    for (size_t i = 0; i < STR_LEN_MAX; i++) {
        metaData2[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    uint32_t reserveInts = fdp.ConsumeIntegralInRange<uint32_t>(0, RESERVE_INTS_MAX);

    int32_t width = fdp.ConsumeIntegral<int32_t>();
    int32_t height = fdp.ConsumeIntegral<int32_t>();
    NativeWindowSetRequestWidthAndHeight(nativeWindow, width, height);
    NativeWindowGetDefaultWidthAndHeight(nativeWindow, &width, &height);
    NativeWindowSetBufferHold(nativeWindow);

    TestSurfaceIdAndParcel(nativeWindow, surfaceId);

    OH_NativeBuffer_ColorSpace space =
        static_cast<OH_NativeBuffer_ColorSpace>(fdp.ConsumeIntegral<int32_t>());
    OH_NativeWindow_SetColorSpace(nativeWindow, space);
    OH_NativeWindow_GetColorSpace(nativeWindow, &space);

    OH_NativeBuffer_MetadataKey metaKey =
        static_cast<OH_NativeBuffer_MetadataKey>(fdp.ConsumeIntegral<int32_t>());
    TestMetadataValue(nativeWindow, metaKey, fdp);

    uint32_t sequence = fdp.ConsumeIntegral<uint32_t>();
    TestHDRMetadata(nativeWindow, sequence, metaData, key, metaData2);
    TestTunnelHandle(nativeWindow, reserveInts);
    (void)nwBuffer;
}

void NativeWindowFuzzTest2(OHNativeWindow *nativeWindow)
{
    for (uint32_t i = 0; i < STABILITY_LOOP_COUNT; i++) {
        OHNativeWindowBuffer* buffer = nullptr;
        Region::Rect rect = {0};
        rect.x = DEFAULT_REGION_EDGE;
        rect.y = DEFAULT_REGION_EDGE;
        rect.w = DEFAULT_REGION_EDGE;
        rect.h = DEFAULT_REGION_EDGE;
        Region region = {.rects = &rect, .rectNumber = 1};
        NativeWindowLockBuffer(nativeWindow, region, &buffer);
        NativeWindowUnlockAndFlushBuffer(nativeWindow);
    }
}

void NativeWindowFuzzTest3(OHNativeWindow *nativeWindow, FuzzedDataProvider& fdp)
{
    NativeWindowCleanCache(nativeWindow);
    uint32_t allocBufferCnt = fdp.ConsumeIntegralInRange<uint32_t>(1, PREALLOC_BUFFER_MAX);
    NativeWindowPreAllocBuffers(nativeWindow, allocBufferCnt);
    NativeWindowPreAllocBuffers(nativeWindow, 0);
    NativeWindowPreAllocBuffers(nativeWindow, 1);
    allocBufferCnt = fdp.ConsumeIntegral<uint32_t>();
    if (allocBufferCnt == 0) {
        allocBufferCnt = 1;
    }
    NativeWindowPreAllocBuffers(nativeWindow, allocBufferCnt);
}

void TestEdgeCaseRequestBuffer(OHNativeWindow *nativeWindow, OHNativeWindowBuffer *nwBuffer,
    int fenceFd)
{
    NativeWindowRequestBuffer(nullptr, &nwBuffer, &fenceFd);
    NativeWindowRequestBuffer(nativeWindow, nullptr, &fenceFd);
}

void TestEdgeCaseFlushBuffer(OHNativeWindow *nativeWindow, OHNativeWindowBuffer *nwBuffer,
    int flushFence)
{
    NativeWindowFlushBuffer(nullptr, nwBuffer, flushFence, {.rects = nullptr, .rectNumber = 0});
    NativeWindowFlushBuffer(nativeWindow, nullptr, flushFence, {.rects = nullptr, .rectNumber = 0});
}

void TestEdgeCaseLockBuffer(OHNativeWindow *nativeWindow, OHNativeWindowBuffer *nwBuffer)
{
    OHNativeWindowBuffer* buffer = nullptr;
    Region region = {.rects = nullptr, .rectNumber = 0};
    NativeWindowLockBuffer(nullptr, region, &buffer);
    NativeWindowLockBuffer(nativeWindow, region, nullptr);
}

void TestEdgeCaseAttachDetachBuffer(OHNativeWindow *nativeWindow, OHNativeWindowBuffer *nwBuffer)
{
    NativeWindowUnlockAndFlushBuffer(nullptr);
    NativeWindowAttachBuffer(nullptr, nwBuffer);
    NativeWindowAttachBuffer(nativeWindow, nullptr);
    NativeWindowDetachBuffer(nullptr, nwBuffer);
    NativeWindowDetachBuffer(nativeWindow, nullptr);
}

void TestEdgeCaseGetLastFlushedBuffer(OHNativeWindow *nativeWindow, OHNativeWindowBuffer *nwBuffer,
    int edgeFenceFd, float *matrix)
{
    GetLastFlushedBuffer(nullptr, &nwBuffer, &edgeFenceFd, matrix);
    GetLastFlushedBuffer(nativeWindow, nullptr, &edgeFenceFd, matrix);
}

void NativeWindowFuzzTestEdgeCases(OHNativeWindow *nativeWindow, OHNativeWindowBuffer *nwBuffer,
    FuzzedDataProvider& fdp)
{
    int fenceFd = fdp.ConsumeIntegral<int>();
    TestEdgeCaseRequestBuffer(nativeWindow, nwBuffer, fenceFd);

    int flushFence = fdp.ConsumeIntegral<int>();
    TestEdgeCaseFlushBuffer(nativeWindow, nwBuffer, flushFence);

    NativeWindowCancelBuffer(nullptr, nwBuffer);
    NativeWindowCancelBuffer(nativeWindow, nullptr);
    NativeWindowCleanCache(nullptr);
    NativeWindowPreAllocBuffers(nullptr, fdp.ConsumeIntegral<uint32_t>());

    TestEdgeCaseLockBuffer(nativeWindow, nwBuffer);
    TestEdgeCaseAttachDetachBuffer(nativeWindow, nwBuffer);

    int edgeFenceFd = fdp.ConsumeIntegral<int>();
    float matrix[MATRIX_SIZE] = {0};
    TestEdgeCaseGetLastFlushedBuffer(nativeWindow, nwBuffer, edgeFenceFd, matrix);

    OHScalingMode scalingMode = static_cast<OHScalingMode>(
        fdp.ConsumeIntegral<int32_t>() % SCALING_MODE_RANGE - SCALING_MODE_OFFSET);
    NativeWindowSetScalingMode(nativeWindow, fdp.ConsumeIntegral<uint32_t>(), scalingMode);

    OHScalingModeV2 scalingModeV2 = static_cast<OHScalingModeV2>(
        fdp.ConsumeIntegral<int32_t>() % SCALING_MODE_RANGE - SCALING_MODE_OFFSET);
    NativeWindowSetScalingModeV2(nativeWindow, scalingModeV2);
}

void NativeWindowFuzzTestColorSpaceAndMetadata(OHNativeWindow *nativeWindow, FuzzedDataProvider& fdp)
{
    OH_NativeBuffer_ColorSpace colorSpace;
    int32_t colorSpaceInt = fdp.ConsumeIntegral<int32_t>();
    colorSpace = static_cast<OH_NativeBuffer_ColorSpace>(colorSpaceInt);
    OH_NativeWindow_SetColorSpace(nativeWindow, colorSpace);
    OH_NativeWindow_GetColorSpace(nativeWindow, &colorSpace);

    OH_NativeBuffer_MetadataKey metaKey = OH_HDR_DYNAMIC_METADATA;
    int32_t testSize = fdp.ConsumeIntegralInRange<int32_t>(0, METADATA_TEST_SIZE_MAX);
    uint8_t* testMetadata = nullptr;
    if (testSize > 0) {
        testMetadata = new uint8_t[testSize];
        for (int i = 0; i < testSize; i++) {
            testMetadata[i] = fdp.ConsumeIntegral<uint8_t>();
        }
    }
    OH_NativeWindow_SetMetadataValue(nativeWindow, metaKey, testSize, testMetadata);
    OH_NativeWindow_SetMetadataValue(nullptr, metaKey, testSize, testMetadata);
    OH_NativeWindow_SetMetadataValue(nativeWindow, metaKey, testSize, nullptr);
    OH_NativeWindow_SetMetadataValue(nativeWindow, metaKey, -1, testMetadata);
    OH_NativeWindow_SetMetadataValue(nativeWindow, metaKey, EXTRA_LARGE_METADATA_SIZE, testMetadata);

    int32_t getSize = 0;
    uint8_t* getMetadata = nullptr;
    OH_NativeWindow_GetMetadataValue(nativeWindow, metaKey, &getSize, &getMetadata);
    if (getMetadata != nullptr) {
        delete[] getMetadata;
    }
    OH_NativeWindow_GetMetadataValue(nullptr, metaKey, &getSize, &getMetadata);
    OH_NativeWindow_GetMetadataValue(nativeWindow, metaKey, nullptr, &getMetadata);
    OH_NativeWindow_GetMetadataValue(nativeWindow, metaKey, &getSize, nullptr);

    if (testMetadata != nullptr) {
        delete[] testMetadata;
    }
}

void NativeWindowFuzzTestCreateBufferFromNative(OHNativeWindow *nativeWindow)
{
    int fenceFd = 0;
    OHNativeWindowBuffer *nwBuffer = nullptr;
    NativeWindowRequestBuffer(nativeWindow, &nwBuffer, &fenceFd);
    if (nwBuffer != nullptr) {
        OH_NativeBuffer *nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(nwBuffer);
        if (nativeBuffer != nullptr) {
            OHNativeWindowBuffer *nwBufferFromNative =
                CreateNativeWindowBufferFromNativeBuffer(nativeBuffer);
            if (nwBufferFromNative != nullptr) {
                DestroyNativeWindowBuffer(nwBufferFromNative);
            }
            OH_NativeBuffer_Unreference(nativeBuffer);
        }
        NativeWindowCancelBuffer(nativeWindow, nwBuffer);
    }

    OHNativeWindowBuffer *nullBuffer = CreateNativeWindowBufferFromNativeBuffer(nullptr);
    if (nullBuffer != nullptr) {
        DestroyNativeWindowBuffer(nullBuffer);
    }

    OH_NativeBuffer_Config config;
    config.width = DEFAULT_BUFFER_SIZE;
    config.height = DEFAULT_BUFFER_SIZE;
    config.format = GRAPHIC_PIXEL_FMT_RGBA_8888;
    config.usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE;
    config.stride = 0;
    OH_NativeBuffer *allocatedBuffer = OH_NativeBuffer_Alloc(&config);
    if (allocatedBuffer != nullptr) {
        OHNativeWindowBuffer *nwBufferFromAlloc =
            CreateNativeWindowBufferFromNativeBuffer(allocatedBuffer);
        if (nwBufferFromAlloc != nullptr) {
            DestroyNativeWindowBuffer(nwBufferFromAlloc);
        }
        OH_NativeBuffer_Unreference(allocatedBuffer);
    }
}

void NativeWindowFuzzTestGameUpscaleProcessor(OHNativeWindow *nativeWindow)
{
    static void (*testProcessor)(int32_t *, int32_t *) = nullptr;
    NativeWindowSetGameUpscaleProcessor(nullptr, testProcessor);
    NativeWindowSetGameUpscaleProcessor(nativeWindow, nullptr);

    static auto processorCallback = [](int32_t *width, int32_t *height) -> void {
        if (width != nullptr) {
            *width = UPSCALE_WIDTH_1920;
        }
        if (height != nullptr) {
            *height = UPSCALE_HEIGHT_1080;
        }
    };
    NativeWindowSetGameUpscaleProcessor(nativeWindow, processorCallback);

    static auto processorCallback2 = [](int32_t *width, int32_t *height) -> void {
        if (width != nullptr) {
            *width = UPSCALE_WIDTH_3840;
        }
        if (height != nullptr) {
            *height = UPSCALE_HEIGHT_2160;
        }
    };
    NativeWindowSetGameUpscaleProcessor(nativeWindow, processorCallback2);
    NativeWindowSetGameUpscaleProcessor(nativeWindow, nullptr);
}

void NativeWindowFuzzTestObjectApis(OHNativeWindow *nativeWindow, OHNativeWindowBuffer *nwBuffer,
    FuzzedDataProvider& fdp)
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
    ConvertColorSpaceTypeToNativeBufferColorSpace(fdp.ConsumeIntegral<int32_t>(), &converted);
    ConvertColorSpaceTypeToNativeBufferColorSpace(fdp.ConsumeIntegral<int32_t>(), nullptr);
}

bool DoSomethingInterestingWithMyAPI(FuzzedDataProvider& fdp)
{
    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create();
    if (cSurface == nullptr) {
        return false;
    }
    sptr<IBufferConsumerListener> listener = new FuzzBufferConsumerListener();
    cSurface->RegisterConsumerListener(listener);
    sptr<OHOS::IBufferProducer> producer = cSurface->GetProducer();
    if (producer == nullptr) {
        return false;
    }
    sptr<OHOS::Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
    if (pSurface == nullptr) {
        return false;
    }
    cSurface->SetDefaultWidthAndHeight(DEFAULT_WIDTH_HEIGHT, DEFAULT_WIDTH_HEIGHT);
    OHNativeWindow* nativeWindow = CreateNativeWindowFromSurface(&pSurface);
    if (nativeWindow == nullptr) {
        return false;
    }
    uint32_t seqNum = fdp.ConsumeIntegral<uint32_t>();
    sptr<OHOS::SurfaceBuffer> sBuffer = new SurfaceBufferImpl(seqNum);
    OHNativeWindowBuffer* nwBuffer = CreateNativeWindowBufferFromSurfaceBuffer(&sBuffer);
    DestroyNativeWindowBuffer(nwBuffer);
    nwBuffer = nullptr;
    HandleOpt(nativeWindow, fdp);
    HandleOpt1(nativeWindow, fdp);
    NativeWindowFuzzTest(nativeWindow, nwBuffer, fdp);
    NativeWindowFuzzTest1(nativeWindow, nwBuffer, fdp);
    NativeWindowFuzzTest2(nativeWindow);
    NativeWindowFuzzTest3(nativeWindow, fdp);
    NativeWindowFuzzTestColorSpaceAndMetadata(nativeWindow, fdp);
    NativeWindowFuzzTestCreateBufferFromNative(nativeWindow);
    NativeWindowFuzzTestGameUpscaleProcessor(nativeWindow);
    NativeWindowFuzzTestObjectApis(nativeWindow, nwBuffer, fdp);
    NativeWindowDisconnect(nativeWindow);
    DestoryNativeWindow(nativeWindow);
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