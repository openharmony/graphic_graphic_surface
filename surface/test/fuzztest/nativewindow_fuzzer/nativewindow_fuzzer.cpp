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

using namespace g_fuzzCommon;
namespace OHOS {
    namespace {
        constexpr uint32_t MATRIX_SIZE = 16;
        constexpr size_t STR_LEN = 10;
        constexpr int DEFAULT_FENCE = 100;
        const uint8_t* g_data = nullptr;
        size_t g_size = 0;
        size_t g_pos;
    }

    void HandleOpt(OHNativeWindow *nativeWindow)
    {
        int code = SET_USAGE;
        uint64_t usage = GetData<uint64_t>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, usage);
        code = SET_BUFFER_GEOMETRY;
        int32_t height = GetData<int32_t>();
        int32_t width = GetData<int32_t>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, height, width);
        code = SET_FORMAT;
        int32_t format = GetData<int32_t>();
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
        int32_t strideSet = 0x8;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, strideSet);
        code = GET_STRIDE;
        int32_t strideGet = 0;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &strideGet);
        code = SET_TIMEOUT;
        int32_t timeoutSet = GetData<int32_t>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, timeoutSet);
        code = SET_COLOR_GAMUT;
        int32_t colorGamutSet = GetData<int32_t>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, colorGamutSet);
        code = SET_TRANSFORM;
        int32_t transform = GetData<int32_t>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, transform);
    }

    void HandleOpt1(OHNativeWindow *nativeWindow)
    {
        int code = SET_UI_TIMESTAMP;
        uint64_t uiTimestamp = GetData<uint64_t>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, uiTimestamp);
        code = SET_DESIRED_PRESENT_TIMESTAMP;
        uint64_t desiredPresentTimestamp = GetData<uint64_t>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, desiredPresentTimestamp);
        code = SET_SOURCE_TYPE;
        OHSurfaceSource typeSet = GetData<OHSurfaceSource>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, typeSet);
        code = GET_SOURCE_TYPE;
        OHSurfaceSource typeGet = OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &typeGet);
        code = SET_APP_FRAMEWORK_TYPE;
        const char* frameWorkTypeSet = GetData<const char*>();
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
        code = GetData<int>();
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code);
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
        NativeObjectReference(nwBuffer);
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
        OHNativeWindow *nativeWindowTmp;
        CreateNativeWindowFromSurfaceId(surfaceId, &nativeWindowTmp);
        OH_NativeBuffer_ColorSpace space = GetData<OH_NativeBuffer_ColorSpace>();
        OH_NativeWindow_SetColorSpace(nativeWindow, space);
        OH_NativeWindow_GetColorSpace(nativeWindow, &space);
        OH_NativeBuffer_MetadataKey metaKey = GetData<OH_NativeBuffer_MetadataKey>();
        int32_t metaSize = GetData<int32_t>();
        uint8_t len = GetData<uint8_t>();
        uint8_t buff[len];
        for (int i = 0; i < len; ++i) {
            buff[i] = GetData<uint8_t>();
        }
        OH_NativeWindow_SetMetadataValue(nativeWindow, metaKey, metaSize, buff);
        uint8_t *checkMetaData;
        OH_NativeWindow_GetMetadataValue(nativeWindow, metaKey, &metaSize, &checkMetaData);
        std::vector<OHHDRMetaData> metaDatas = {metaData};
        uint32_t sequence = GetData<uint32_t>();
        NativeWindowSetMetaData(nativeWindow, sequence, metaDatas.size(), metaDatas.data());
        NativeWindowSetMetaDataSet(nativeWindow, sequence, key, STR_LEN, metaData2);
        OHExtDataHandle *handle = reinterpret_cast<OHExtDataHandle *>(AllocExtDataHandle(reserveInts));
        NativeWindowSetTunnelHandle(nativeWindow, reinterpret_cast<OHExtDataHandle *>(handle));
        FreeExtDataHandle(reinterpret_cast<GraphicExtDataHandle *>(handle));
        NativeWindowDisconnect(nativeWindow);
        DestoryNativeWindow(nativeWindow);
        DestroyNativeWindowBuffer(nwBuffer);
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
        HandleOpt(nativeWindow);
        HandleOpt1(nativeWindow);
        NativeWindowFuzzTest(nativeWindow, nwBuffer);
        NativeWindowFuzzTest1(nativeWindow, nwBuffer);
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

