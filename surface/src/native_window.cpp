/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "native_window.h"

#include <cstdint>
#include <map>
#include <cinttypes>
#include <securec.h>
#include "buffer_log.h"
#include "window.h"
#include "surface_type.h"
#include "surface_utils.h"
#include "sync_fence.h"
#include "ipc_inner_object.h"
#include "external_window.h"
#include "metadata_helper.h"

#ifndef WEAK_ALIAS
    #define WEAK_ALIAS(old, new) \
        extern __typeof(old) new __attribute__((__weak__, __alias__(#old)))
#endif

using namespace OHOS;
using namespace HDI::Display::Graphic::Common::V1_0;
static std::unordered_map<OH_NativeBuffer_ColorSpace, CM_ColorSpaceType> NATIVE_COLORSPACE_TO_HDI_MAP = {
    {OH_COLORSPACE_NONE, CM_COLORSPACE_NONE},
    {OH_COLORSPACE_BT601_EBU_FULL, CM_BT601_EBU_FULL},
    {OH_COLORSPACE_BT601_SMPTE_C_FULL, CM_BT601_SMPTE_C_FULL},
    {OH_COLORSPACE_BT709_FULL, CM_BT709_FULL},
    {OH_COLORSPACE_BT2020_HLG_FULL, CM_BT2020_HLG_FULL},
    {OH_COLORSPACE_BT2020_PQ_FULL, CM_BT2020_PQ_FULL},
    {OH_COLORSPACE_BT601_EBU_LIMIT, CM_BT601_EBU_LIMIT},
    {OH_COLORSPACE_BT601_SMPTE_C_LIMIT, CM_BT601_SMPTE_C_LIMIT},
    {OH_COLORSPACE_BT709_LIMIT, CM_BT709_LIMIT},
    {OH_COLORSPACE_BT2020_HLG_LIMIT, CM_BT2020_HLG_LIMIT},
    {OH_COLORSPACE_BT2020_PQ_LIMIT, CM_BT2020_PQ_LIMIT},
    {OH_COLORSPACE_SRGB_FULL, CM_SRGB_FULL},
    {OH_COLORSPACE_P3_FULL, CM_P3_FULL},
    {OH_COLORSPACE_P3_HLG_FULL, CM_P3_HLG_FULL},
    {OH_COLORSPACE_P3_PQ_FULL, CM_P3_PQ_FULL},
    {OH_COLORSPACE_ADOBERGB_FULL, CM_ADOBERGB_FULL},
    {OH_COLORSPACE_SRGB_LIMIT, CM_SRGB_LIMIT},
    {OH_COLORSPACE_P3_LIMIT, CM_P3_LIMIT},
    {OH_COLORSPACE_P3_HLG_LIMIT, CM_P3_HLG_LIMIT},
    {OH_COLORSPACE_P3_PQ_LIMIT, CM_P3_PQ_LIMIT},
    {OH_COLORSPACE_ADOBERGB_LIMIT, CM_ADOBERGB_LIMIT},
    {OH_COLORSPACE_LINEAR_SRGB, CM_LINEAR_SRGB},
    {OH_COLORSPACE_LINEAR_BT709, CM_LINEAR_BT709},
    {OH_COLORSPACE_LINEAR_P3, CM_LINEAR_P3},
    {OH_COLORSPACE_LINEAR_BT2020, CM_LINEAR_BT2020},
    {OH_COLORSPACE_DISPLAY_SRGB, CM_DISPLAY_SRGB},
    {OH_COLORSPACE_DISPLAY_P3_SRGB, CM_DISPLAY_P3_SRGB},
    {OH_COLORSPACE_DISPLAY_P3_HLG, CM_DISPLAY_P3_HLG},
    {OH_COLORSPACE_DISPLAY_P3_PQ, CM_DISPLAY_P3_PQ},
    {OH_COLORSPACE_DISPLAY_BT2020_SRGB, CM_DISPLAY_BT2020_SRGB},
    {OH_COLORSPACE_DISPLAY_BT2020_HLG, CM_DISPLAY_BT2020_HLG},
    {OH_COLORSPACE_DISPLAY_BT2020_PQ, CM_DISPLAY_BT2020_PQ}
};

static std::unordered_map<OH_NativeBuffer_MetadataType, CM_HDR_Metadata_Type> NATIVE_METADATATYPE_TO_HDI_MAP = {
    {OH_VIDEO_HDR_HLG, CM_VIDEO_HLG},
    {OH_VIDEO_HDR_HDR10, CM_VIDEO_HDR10},
    {OH_VIDEO_HDR_VIVID, CM_VIDEO_HDR_VIVID},
};

OHNativeWindow* CreateNativeWindowFromSurface(void* pSurface)
{
    if (pSurface == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return nullptr;
    }

    OHNativeWindow* nativeWindow = new OHNativeWindow();
    nativeWindow->surface =
                *reinterpret_cast<OHOS::sptr<OHOS::Surface> *>(pSurface);
    if (nativeWindow->surface == nullptr) {
        BLOGE("window surface is null");
        delete nativeWindow;
        return nullptr;
    }
    OHOS::BufferRequestConfig *windowConfig = nativeWindow->surface->GetWindowConfig();
    if (windowConfig == nullptr) {
        BLOGE("windowConfig is null");
        delete nativeWindow;
        return nullptr;
    }
    windowConfig->width = nativeWindow->surface->GetDefaultWidth();
    windowConfig->height = nativeWindow->surface->GetDefaultHeight();
    windowConfig->usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_MEM_DMA;
    windowConfig->format = GRAPHIC_PIXEL_FMT_RGBA_8888;
    windowConfig->strideAlignment = 8;   // default stride is 8
    windowConfig->timeout = 3000;        // default timeout is 3000 ms
    windowConfig->colorGamut = GraphicColorGamut::GRAPHIC_COLOR_GAMUT_SRGB;
    windowConfig->transform = GraphicTransformType::GRAPHIC_ROTATE_NONE;

    NativeObjectReference(nativeWindow);
    auto utils = SurfaceUtils::GetInstance();
    utils->AddNativeWindow(nativeWindow->surface->GetUniqueId(), nativeWindow);
    nativeWindow->surface->SetWptrNativeWindowToPSurface(nativeWindow);
    return nativeWindow;
}

void DestoryNativeWindow(OHNativeWindow *window)
{
    if (window == nullptr) {
        BLOGD("parameter error, please check input parameter");
        return;
    }
    // unreference nativewindow object
    NativeObjectUnreference(window);
}

OHNativeWindowBuffer* CreateNativeWindowBufferFromSurfaceBuffer(void* pSurfaceBuffer)
{
    if (pSurfaceBuffer == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return nullptr;
    }
    OHNativeWindowBuffer *nwBuffer = new OHNativeWindowBuffer();
    nwBuffer->sfbuffer = *reinterpret_cast<OHOS::sptr<OHOS::SurfaceBuffer> *>(pSurfaceBuffer);
    NativeObjectReference(nwBuffer);
    return nwBuffer;
}

OHNativeWindowBuffer* CreateNativeWindowBufferFromNativeBuffer(OH_NativeBuffer* nativeBuffer)
{
    if (nativeBuffer == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return nullptr;
    }
    OHNativeWindowBuffer *nwBuffer = new OHNativeWindowBuffer();
    OHOS::sptr<OHOS::SurfaceBuffer> surfaceBuffer(reinterpret_cast<OHOS::SurfaceBuffer *>(nativeBuffer));
    nwBuffer->sfbuffer = surfaceBuffer;

    NativeObjectReference(nwBuffer);
    return nwBuffer;
}

void DestroyNativeWindowBuffer(OHNativeWindowBuffer* buffer)
{
    if (buffer == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return;
    }
    NativeObjectUnreference(buffer);
}

int32_t NativeWindowRequestBuffer(OHNativeWindow *window,
    OHNativeWindowBuffer **buffer, int *fenceFd)
{
    if (window == nullptr || buffer == nullptr || fenceFd == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    OHOS::sptr<OHOS::SurfaceBuffer> sfbuffer;
    OHOS::sptr<OHOS::SyncFence> releaseFence = OHOS::SyncFence::INVALID_FENCE;
    BLOGE_CHECK_AND_RETURN_RET(window->surface != nullptr, SURFACE_ERROR_ERROR, "window surface is null");
    int32_t ret;
    int32_t requestWidth = window->surface->GetRequestWidth();
    int32_t requestHeight = window->surface->GetRequestHeight();
    OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
    if (requestWidth != 0 && requestHeight != 0) {
        OHOS::BufferRequestConfig config;
        if (memcpy_s(&config, sizeof(OHOS::BufferRequestConfig), windowConfig,
            sizeof(OHOS::BufferRequestConfig)) != EOK) {
            BLOGE("memcpy_s failed");
            return OHOS::SURFACE_ERROR_UNKOWN;
        }
        config.width = requestWidth;
        config.height = requestHeight;
        ret = window->surface->RequestBuffer(sfbuffer, releaseFence, config);
    } else {
        ret = window->surface->RequestBuffer(sfbuffer, releaseFence, *windowConfig);
    }
    if (ret != OHOS::GSError::SURFACE_ERROR_OK || sfbuffer == nullptr) {
        BLOGE("API failed, please check RequestBuffer function ret:%{public}d, Queue Id:%{public}" PRIu64,
                ret, window->surface->GetUniqueId());
        return ret;
    }
    uint32_t seqNum = sfbuffer->GetSeqNum();
    if (window->bufferCache_.find(seqNum) == window->bufferCache_.end()) {
        OHNativeWindowBuffer *nwBuffer = new OHNativeWindowBuffer();
        nwBuffer->sfbuffer = sfbuffer;
        nwBuffer->uiTimestamp = window->uiTimestamp;
        *buffer = nwBuffer;
        // Add to cache
        NativeObjectReference(nwBuffer);
        window->bufferCache_[seqNum] = nwBuffer;
    } else {
        *buffer = window->bufferCache_[seqNum];
        (*buffer)->uiTimestamp = window->uiTimestamp;
    }
    *fenceFd = releaseFence->Dup();
    return OHOS::SURFACE_ERROR_OK;
}

int32_t NativeWindowFlushBuffer(OHNativeWindow *window, OHNativeWindowBuffer *buffer,
    int fenceFd, struct Region region)
{
    if (window == nullptr || buffer == nullptr || window->surface == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }

    OHOS::BufferFlushConfigWithDamages config;
    if ((region.rectNumber != 0) && (region.rects != nullptr)) {
        config.damages.reserve(region.rectNumber);
        for (int32_t i = 0; i < region.rectNumber; i++) {
            OHOS::Rect damage = {
                .x = region.rects[i].x,
                .y = region.rects[i].y,
                .w = static_cast<int32_t>(region.rects[i].w),
                .h = static_cast<int32_t>(region.rects[i].h),
            };
            config.damages.emplace_back(damage);
        }
        config.timestamp = buffer->uiTimestamp;
    } else {
        OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
        config.damages.reserve(1);
        OHOS::Rect damage = {
            .x = 0,
            .y = 0,
            .w = windowConfig->width,
            .h = windowConfig->height,
        };
        config.damages.emplace_back(damage);
        config.timestamp = buffer->uiTimestamp;
    }
    OHOS::sptr<OHOS::SyncFence> acquireFence = new OHOS::SyncFence(fenceFd);
    int32_t ret = window->surface->FlushBuffer(buffer->sfbuffer, acquireFence, config);
    if (ret != OHOS::GSError::SURFACE_ERROR_OK) {
        BLOGE("FlushBuffer buffer failed, ret:%{public}d, Queue Id:%{public}" PRIu64,
            ret, window->surface->GetUniqueId());
        return ret;
    }

    for (auto &[seqNum, buf] : window->bufferCache_) {
        if (buf == buffer) {
            window->lastBufferSeqNum = seqNum;
            break;
        }
    }

    return OHOS::SURFACE_ERROR_OK;
}

int32_t GetLastFlushedBuffer(OHNativeWindow *window, OHNativeWindowBuffer **buffer, int *fenceFd, float matrix[16])
{
    if (window == nullptr || buffer == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    OHNativeWindowBuffer *nwBuffer = new OHNativeWindowBuffer();
    OHOS::sptr<OHOS::SyncFence> acquireFence = OHOS::SyncFence::INVALID_FENCE;
    int32_t ret = window->surface->GetLastFlushedBuffer(nwBuffer->sfbuffer, acquireFence, matrix, false);
    if (ret != OHOS::GSError::SURFACE_ERROR_OK || nwBuffer->sfbuffer == nullptr) {
        BLOGE("GetLastFlushedBuffer fail");
        delete nwBuffer;
        nwBuffer = nullptr;
        return ret;
    }
    *buffer = nwBuffer;
    NativeObjectReference(nwBuffer);
    *fenceFd = acquireFence->Dup();
    return OHOS::SURFACE_ERROR_OK;
}

int32_t NativeWindowAttachBuffer(OHNativeWindow *window, OHNativeWindowBuffer *buffer)
{
    if (window == nullptr || buffer == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    BLOGE_CHECK_AND_RETURN_RET(window->surface != nullptr, SURFACE_ERROR_INVALID_PARAM, "window surface is null");
    return window->surface->AttachBufferToQueue(buffer->sfbuffer);
}

int32_t NativeWindowDetachBuffer(OHNativeWindow *window, OHNativeWindowBuffer *buffer)
{
    if (window == nullptr || buffer == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    BLOGE_CHECK_AND_RETURN_RET(window->surface != nullptr, SURFACE_ERROR_INVALID_PARAM, "window surface is null");
    return window->surface->DetachBufferFromQueue(buffer->sfbuffer);
}

int32_t NativeWindowCancelBuffer(OHNativeWindow *window, OHNativeWindowBuffer *buffer)
{
    if (window == nullptr || buffer == nullptr) {
        BLOGD("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    BLOGE_CHECK_AND_RETURN_RET(window->surface != nullptr, SURFACE_ERROR_INVALID_PARAM, "window surface is null");
    window->surface->CancelBuffer(buffer->sfbuffer);
    return OHOS::GSERROR_OK;
}

static void HandleNativeWindowSetUsage(OHNativeWindow *window, va_list args)
{
    uint64_t usage = va_arg(args, uint64_t);
    OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
    windowConfig->usage = usage;
}

static void HandleNativeWindowSetBufferGeometry(OHNativeWindow *window, va_list args)
{
    int32_t width = va_arg(args, int32_t);
    int32_t height = va_arg(args, int32_t);
    OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
    windowConfig->height = height;
    windowConfig->width = width;
}

static void HandleNativeWindowSetFormat(OHNativeWindow *window, va_list args)
{
    int32_t format = va_arg(args, int32_t);
    OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
    windowConfig->format = format;
}

static void HandleNativeWindowSetStride(OHNativeWindow *window, va_list args)
{
    int32_t stride = va_arg(args, int32_t);
    OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
    windowConfig->strideAlignment = stride;
}

static void HandleNativeWindowSetTimeout(OHNativeWindow *window, va_list args)
{
    int32_t timeout = va_arg(args, int32_t);
    OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
    windowConfig->timeout = timeout;
}

static void HandleNativeWindowSetColorGamut(OHNativeWindow *window, va_list args)
{
    int32_t colorGamut = va_arg(args, int32_t);
    OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
    windowConfig->colorGamut = static_cast<GraphicColorGamut>(colorGamut);
}

static void HandleNativeWindowSetTransform(OHNativeWindow *window, va_list args)
{
    int32_t transform = va_arg(args, int32_t);
    window->surface->SetTransform(static_cast<GraphicTransformType>(transform));
    OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
    windowConfig->transform = static_cast<GraphicTransformType>(transform);
}

static void HandleNativeWindowSetUiTimestamp(OHNativeWindow *window, va_list args)
{
    uint64_t uiTimestamp = va_arg(args, uint64_t);
    window->uiTimestamp = static_cast<int64_t>(uiTimestamp);
}

static void HandleNativeWindowSetSurfaceSourceType(OHNativeWindow *window, va_list args)
{
    OHSurfaceSource sourceType = va_arg(args, OHSurfaceSource);
    window->surface->SetSurfaceSourceType(sourceType);
}

static void HandleNativeWindowSetSurfaceAppFrameworkType(OHNativeWindow *window, va_list args)
{
    char* appFrameworkType = va_arg(args, char*);
    std::string typeStr(appFrameworkType);
    window->surface->SetSurfaceAppFrameworkType(typeStr);
}

static void HandleNativeWindowGetUsage(OHNativeWindow *window, va_list args)
{
    uint64_t *value = va_arg(args, uint64_t*);
    OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
    uint64_t usage = windowConfig->usage;
    *value = usage;
}

static void HandleNativeWindowGetBufferGeometry(OHNativeWindow *window, va_list args)
{
    int32_t *height = va_arg(args, int32_t*);
    int32_t *width = va_arg(args, int32_t*);
    OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
    *height = windowConfig->height;
    *width = windowConfig->width;
}

static void HandleNativeWindowGetFormat(OHNativeWindow *window, va_list args)
{
    int32_t *format = va_arg(args, int32_t*);
    OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
    *format = windowConfig->format;
}

static void HandleNativeWindowGetStride(OHNativeWindow *window, va_list args)
{
    int32_t *stride = va_arg(args, int32_t*);
    OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
    *stride = windowConfig->strideAlignment;
}

static void HandleNativeWindowGetTimeout(OHNativeWindow *window, va_list args)
{
    int32_t *timeout = va_arg(args, int32_t*);
    OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
    *timeout = windowConfig->timeout;
}

static void HandleNativeWindowGetColorGamut(OHNativeWindow *window, va_list args)
{
    int32_t *colorGamut = va_arg(args, int32_t*);
    OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
    *colorGamut = static_cast<int32_t>(windowConfig->colorGamut);
}

static void HandleNativeWindowGetTransform(OHNativeWindow *window, va_list args)
{
    int32_t *transform = va_arg(args, int32_t*);
    *transform = static_cast<int32_t>(window->surface->GetTransform());
}

static void HandleNativeWindowGetBufferQueueSize(OHNativeWindow *window, va_list args)
{
    int32_t *bufferQueueSize = va_arg(args, int32_t*);
    *bufferQueueSize = static_cast<int32_t>(window->surface->GetQueueSize());
}

static void HandleNativeWindowGetSurfaceSourceType(OHNativeWindow *window, va_list args)
{
    OHSurfaceSource *sourceType = va_arg(args, OHSurfaceSource*);
    *sourceType = window->surface->GetSurfaceSourceType();
}

static void HandleNativeWindowGetSurfaceAppFrameworkType(OHNativeWindow *window, va_list args)
{
    const char **appFrameworkType = va_arg(args, const char**);
    std::string typeStr = window->surface->GetSurfaceAppFrameworkType();
    *appFrameworkType = typeStr.c_str();
}

static void HandleNativeWindowSetHdrWhitePointBrightness(OHNativeWindow *window, va_list args)
{
    float hdrWhitePointBrightness = static_cast<float>(va_arg(args, double));
    window->surface->SetHdrWhitePointBrightness(hdrWhitePointBrightness);
}

static void HandleNativeWindowSetSdrWhitePointBrightness(OHNativeWindow *window, va_list args)
{
    float sdrWhitePointBrightness = static_cast<float>(va_arg(args, double));
    window->surface->SetSdrWhitePointBrightness(sdrWhitePointBrightness);
}

static std::map<int, std::function<void(OHNativeWindow*, va_list)>> operationMap = {
    {SET_USAGE, HandleNativeWindowSetUsage},
    {SET_BUFFER_GEOMETRY, HandleNativeWindowSetBufferGeometry},
    {SET_FORMAT, HandleNativeWindowSetFormat},
    {SET_STRIDE, HandleNativeWindowSetStride},
    {SET_TIMEOUT, HandleNativeWindowSetTimeout},
    {SET_COLOR_GAMUT, HandleNativeWindowSetColorGamut},
    {SET_TRANSFORM, HandleNativeWindowSetTransform},
    {SET_UI_TIMESTAMP, HandleNativeWindowSetUiTimestamp},
    {SET_SOURCE_TYPE, HandleNativeWindowSetSurfaceSourceType},
    {SET_APP_FRAMEWORK_TYPE, HandleNativeWindowSetSurfaceAppFrameworkType},
    {GET_USAGE, HandleNativeWindowGetUsage},
    {GET_BUFFER_GEOMETRY, HandleNativeWindowGetBufferGeometry},
    {GET_FORMAT, HandleNativeWindowGetFormat},
    {GET_STRIDE, HandleNativeWindowGetStride},
    {GET_TIMEOUT, HandleNativeWindowGetTimeout},
    {GET_COLOR_GAMUT, HandleNativeWindowGetColorGamut},
    {GET_TRANSFORM, HandleNativeWindowGetTransform},
    {GET_BUFFERQUEUE_SIZE, HandleNativeWindowGetBufferQueueSize},
    {GET_SOURCE_TYPE, HandleNativeWindowGetSurfaceSourceType},
    {GET_APP_FRAMEWORK_TYPE, HandleNativeWindowGetSurfaceAppFrameworkType},
    {SET_HDR_WHITE_POINT_BRIGHTNESS, HandleNativeWindowSetHdrWhitePointBrightness},
    {SET_SDR_WHITE_POINT_BRIGHTNESS, HandleNativeWindowSetSdrWhitePointBrightness},
};

static int32_t InternalHandleNativeWindowOpt(OHNativeWindow *window, int code, va_list args)
{
    auto it = operationMap.find(code);
    if (it != operationMap.end()) {
        it->second(window, args);
    }
    return OHOS::GSERROR_OK;
}

int32_t NativeWindowHandleOpt(OHNativeWindow *window, int code, ...)
{
    if (window == nullptr || window->surface == nullptr) {
        BLOGD("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    va_list args;
    va_start(args, code);
    InternalHandleNativeWindowOpt(window, code, args);
    va_end(args);
    return OHOS::GSERROR_OK;
}

BufferHandle *GetBufferHandleFromNative(OHNativeWindowBuffer *buffer)
{
    if (buffer == nullptr || buffer->sfbuffer == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return nullptr;
    }
    return buffer->sfbuffer->GetBufferHandle();
}

int32_t GetNativeObjectMagic(void *obj)
{
    if (obj == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return -1;
    }
    NativeWindowMagic* nativeWindowMagic = reinterpret_cast<NativeWindowMagic *>(obj);
    return nativeWindowMagic->magic;
}

int32_t NativeObjectReference(void *obj)
{
    if (obj == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    switch (GetNativeObjectMagic(obj)) {
        case NATIVE_OBJECT_MAGIC_WINDOW:
        case NATIVE_OBJECT_MAGIC_WINDOW_BUFFER:
            break;
        default:
            BLOGE("parameter error, magic illegal");
            return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    OHOS::RefBase *ref = reinterpret_cast<OHOS::RefBase *>(obj);
    ref->IncStrongRef(ref);
    return OHOS::GSERROR_OK;
}

int32_t NativeObjectUnreference(void *obj)
{
    if (obj == nullptr) {
        BLOGD("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    switch (GetNativeObjectMagic(obj)) {
        case NATIVE_OBJECT_MAGIC_WINDOW:
        case NATIVE_OBJECT_MAGIC_WINDOW_BUFFER:
            break;
        default:
            BLOGE("parameter error, magic illegal");
            return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    OHOS::RefBase *ref = reinterpret_cast<OHOS::RefBase *>(obj);
    ref->DecStrongRef(ref);
    return OHOS::GSERROR_OK;
}

int32_t NativeWindowSetScalingMode(OHNativeWindow *window, uint32_t sequence, OHScalingMode scalingMode)
{
    if (window == nullptr || window->surface == nullptr ||
        scalingMode < OHScalingMode::OH_SCALING_MODE_FREEZE ||
        scalingMode > OHScalingMode::OH_SCALING_MODE_NO_SCALE_CROP) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    return window->surface->SetScalingMode(sequence, static_cast<ScalingMode>(scalingMode));
}

int32_t NativeWindowSetScalingModeV2(OHNativeWindow *window, OHScalingModeV2 scalingMode)
{
    if (window == nullptr || window->surface == nullptr ||
        scalingMode < OHScalingModeV2::OH_SCALING_MODE_FREEZE_V2 ||
        scalingMode > OHScalingModeV2::OH_SCALING_MODE_SCALE_FIT_V2) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::GSERROR_INVALID_ARGUMENTS;
    }
    return window->surface->SetScalingMode(static_cast<ScalingMode>(scalingMode));
}

int32_t NativeWindowSetMetaData(OHNativeWindow *window, uint32_t sequence, int32_t size,
                                const OHHDRMetaData *metaData)
{
    if (window == nullptr || window->surface == nullptr || size <= 0 || metaData == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }

    std::vector<GraphicHDRMetaData> data(reinterpret_cast<const GraphicHDRMetaData *>(metaData),
                                         reinterpret_cast<const GraphicHDRMetaData *>(metaData) + size);
    return window->surface->SetMetaData(sequence, data);
}

int32_t NativeWindowSetMetaDataSet(OHNativeWindow *window, uint32_t sequence, OHHDRMetadataKey key,
                                   int32_t size, const uint8_t *metaData)
{
    if (window == nullptr || window->surface == nullptr ||
        key < OHHDRMetadataKey::OH_METAKEY_RED_PRIMARY_X || key > OHHDRMetadataKey::OH_METAKEY_HDR_VIVID ||
        size <= 0 || metaData == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    std::vector<uint8_t> data(metaData, metaData + size);
    return window->surface->SetMetaDataSet(sequence, static_cast<GraphicHDRMetadataKey>(key), data);
}

int32_t NativeWindowSetTunnelHandle(OHNativeWindow *window, const OHExtDataHandle *handle)
{
    if (window == nullptr || window->surface == nullptr || handle == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    return window->surface->SetTunnelHandle(reinterpret_cast<const OHOS::GraphicExtDataHandle*>(handle));
}

int32_t GetSurfaceId(OHNativeWindow *window, uint64_t *surfaceId)
{
    if (window == nullptr || surfaceId == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }

    *surfaceId = window->surface->GetUniqueId();
    return OHOS::GSERROR_OK;
}

int32_t CreateNativeWindowFromSurfaceId(uint64_t surfaceId, OHNativeWindow **window)
{
    if (window == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }

    auto utils = SurfaceUtils::GetInstance();
    *window = reinterpret_cast<OHNativeWindow*>(utils->GetNativeWindow(surfaceId));
    if (*window != nullptr) {
        NativeObjectReference(*window);
        BLOGD("get nativeWindow from cache.");
        return OHOS::GSERROR_OK;
    }

    OHNativeWindow *nativeWindow = new OHNativeWindow();
    nativeWindow->surface = utils->GetSurface(surfaceId);
    if (nativeWindow->surface == nullptr) {
        BLOGE("window surface is null");
        delete nativeWindow;
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }

    NativeObjectReference(nativeWindow);
    utils->AddNativeWindow(nativeWindow->surface->GetUniqueId(), nativeWindow);
    nativeWindow->surface->SetWptrNativeWindowToPSurface(nativeWindow);
    *window = nativeWindow;
    return OHOS::GSERROR_OK;
}

int32_t NativeWindowGetTransformHint(OHNativeWindow *window, OH_NativeBuffer_TransformType *transform)
{
    if (window == nullptr || window->surface == nullptr || transform == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    *transform = static_cast<OH_NativeBuffer_TransformType>(window->surface->GetTransformHint());
    return OHOS::GSERROR_OK;
}

int32_t NativeWindowSetTransformHint(OHNativeWindow *window, OH_NativeBuffer_TransformType transform)
{
    if (window == nullptr || window->surface == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    return window->surface->SetTransformHint(static_cast<OHOS::GraphicTransformType>(transform));
}

int32_t NativeWindowGetDefaultWidthAndHeight(OHNativeWindow *window, int32_t *width, int32_t *height)
{
    if (window == nullptr || window->surface == nullptr || width == nullptr || height == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    OHOS::BufferRequestConfig *windowConfig = window->surface->GetWindowConfig();
    if (windowConfig->width != 0 && windowConfig->height != 0) {
        *width = windowConfig->width;
        *height = windowConfig->height;
    } else {
        *width = window->surface->GetDefaultWidth();
        *height = window->surface->GetDefaultHeight();
    }
    return OHOS::GSERROR_OK;
}

int32_t NativeWindowSetRequestWidthAndHeight(OHNativeWindow *window, int32_t width, int32_t height)
{
    if (window == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    window->surface->SetRequestWidthAndHeight(width, height);
    return OHOS::GSERROR_OK;
}

void NativeWindowSetBufferHold(OHNativeWindow *window)
{
    if (window == nullptr || window->surface == nullptr) {
        BLOGE("parameter error, please check input parameter(window or surface is nullptr)");
        return;
    }
    window->surface->SetBufferHold(true);
}

int32_t NativeWindowWriteToParcel(OHNativeWindow *window, OHIPCParcel *parcel)
{
    if (window == nullptr) {
        BLOGE("parameter error, please check input window");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    if (parcel == nullptr || parcel->msgParcel == nullptr) {
        BLOGE("parcel error, please check input parcel");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    sptr<OHOS::Surface> windowSurface = window->surface;
    if (windowSurface == nullptr) {
        BLOGE("parameter error, please check input window");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    auto producer = windowSurface->GetProducer();
    (parcel->msgParcel)->WriteRemoteObject(producer->AsObject());
    return OHOS::GSERROR_OK;
}

int32_t NativeWindowReadFromParcel(OHIPCParcel *parcel, OHNativeWindow **window)
{
    if (parcel == nullptr || parcel->msgParcel == nullptr || window == nullptr) {
        BLOGE("parcel error, please check input parcel");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    sptr<OHOS::IRemoteObject> surfaceObject = (parcel->msgParcel)->ReadRemoteObject();
    if (surfaceObject == nullptr) {
        BLOGE("read object error, please check input parcel");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    sptr<OHOS::IBufferProducer> bp = iface_cast<IBufferProducer>(surfaceObject);
    sptr <OHOS::Surface> windowSurface = OHOS::Surface::CreateSurfaceAsProducer(bp);
    if (windowSurface == nullptr) {
        BLOGE("create surface error, please check input parcel");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    auto utils = SurfaceUtils::GetInstance();
    *window = reinterpret_cast<OHNativeWindow*>(utils->GetNativeWindow(windowSurface->GetUniqueId()));
    if (*window == nullptr) {
        *window = CreateNativeWindowFromSurface(&windowSurface);
    }
    return OHOS::GSERROR_OK;
}

int32_t GetLastFlushedBufferV2(OHNativeWindow *window, OHNativeWindowBuffer **buffer, int *fenceFd, float matrix[16])
{
    if (window == nullptr || buffer == nullptr || fenceFd == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    OHNativeWindowBuffer *nwBuffer = new OHNativeWindowBuffer();
    OHOS::sptr<OHOS::SyncFence> acquireFence = OHOS::SyncFence::INVALID_FENCE;
    int32_t ret = window->surface->GetLastFlushedBuffer(nwBuffer->sfbuffer, acquireFence, matrix, true);
    if (ret != OHOS::GSError::SURFACE_ERROR_OK || nwBuffer->sfbuffer == nullptr) {
        BLOGE("GetLastFlushedBufferV2 fail");
        delete nwBuffer;
        nwBuffer = nullptr;
        return ret;
    }
    *buffer = nwBuffer;
    NativeObjectReference(nwBuffer);
    *fenceFd = acquireFence->Dup();
    return OHOS::SURFACE_ERROR_OK;
}

int32_t NativeWindowDisconnect(OHNativeWindow *window)
{
    if (window == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    sptr<OHOS::Surface> windowSurface = window->surface;
    if (windowSurface == nullptr) {
        BLOGE("parameter error, please check input window");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    return windowSurface->Disconnect();
}

int32_t OH_NativeWindow_SetColorSpace(OHNativeWindow *window, OH_NativeBuffer_ColorSpace colorSpace)
{
    if (window == nullptr || NATIVE_COLORSPACE_TO_HDI_MAP.find(colorSpace) == NATIVE_COLORSPACE_TO_HDI_MAP.end()) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    OHNativeWindowBuffer *lastFlushedBuffer;
    int lastFlushFenceFd;
    float matrix[16];
    int32_t status = GetLastFlushedBuffer(window, &lastFlushedBuffer, &lastFlushFenceFd, matrix);
    if (status != OHOS::SURFACE_ERROR_OK) {
        BLOGE("GetLastFlushedBuffer fail");
        return status;
    }
    GSError ret = MetadataHelper::SetColorSpaceType(lastFlushedBuffer->sfbuffer,
        NATIVE_COLORSPACE_TO_HDI_MAP[colorSpace]);
    if (ret != OHOS::SURFACE_ERROR_OK) {
        BLOGE("SetColorSpaceType failed!, retVal:%d", ret);
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    return OHOS::SURFACE_ERROR_OK;
}

int32_t OH_NativeWindow_GetColorSpace(OHNativeWindow *window, OH_NativeBuffer_ColorSpace *colorSpace)
{
    if (window == nullptr || colorSpace == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType colorSpaceType;
    OHNativeWindowBuffer *lastFlushedBuffer;
    int lastFlushFenceFd;
    float matrix[16];
    int32_t status = GetLastFlushedBuffer(window, &lastFlushedBuffer, &lastFlushFenceFd, matrix);
    if (status != OHOS::SURFACE_ERROR_OK) {
        BLOGE("GetLastFlushedBuffer fail");
        return status;
    }
    GSError ret = MetadataHelper::GetColorSpaceType(lastFlushedBuffer->sfbuffer, colorSpaceType);
    if (ret != OHOS::SURFACE_ERROR_OK) {
        BLOGE("GetColorSpaceType failed!, retVal:%d", ret);
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    for (auto type : NATIVE_COLORSPACE_TO_HDI_MAP) {
        if (type.second == colorSpaceType) {
            *colorSpace = type.first;
            return OHOS::SURFACE_ERROR_OK;
        }
    }
    BLOGE("the colorSpace does not support it.");
    return OHOS::SURFACE_ERROR_UNKOWN;
}

int32_t OH_NativeWindow_SetMetadataValue(OHNativeWindow *window, OH_NativeBuffer_MetadataKey metadataKey,
    int32_t size, uint8_t *metadata)
{
    if (window == nullptr || metadata == nullptr || size <= 0) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    OHNativeWindowBuffer *lastFlushedBuffer;
    int lastFlushFenceFd;
    float matrix[16];
    int32_t status = GetLastFlushedBuffer(window, &lastFlushedBuffer, &lastFlushFenceFd, matrix);
    if (status != OHOS::SURFACE_ERROR_OK) {
        BLOGE("GetLastFlushedBuffer fail");
        return status;
    }
    GSError ret = GSERROR_OK;
    std::vector<uint8_t> mD(metadata, metadata + size);
    if (metadataKey == OH_HDR_DYNAMIC_METADATA) {
        ret = MetadataHelper::SetHDRDynamicMetadata(lastFlushedBuffer->sfbuffer, mD);
    } else if (metadataKey == OH_HDR_STATIC_METADATA) {
        ret = MetadataHelper::SetHDRStaticMetadata(lastFlushedBuffer->sfbuffer, mD);
    } else if (metadataKey == OH_HDR_METADATA_TYPE) {
        OH_NativeBuffer_MetadataType hdrMetadataType = static_cast<OH_NativeBuffer_MetadataType>(*metadata);
        ret = MetadataHelper::SetHDRMetadataType(lastFlushedBuffer->sfbuffer,
            NATIVE_METADATATYPE_TO_HDI_MAP[hdrMetadataType]);
    } else {
        BLOGE("the metadataKey does not support it.");
        return OHOS::SURFACE_ERROR_NOT_SUPPORT;
    }
    if (ret != OHOS::SURFACE_ERROR_OK) {
        BLOGE("SetHDRMetadata failed!, retVal:%d", ret);
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    return OHOS::SURFACE_ERROR_OK;
}

GSError OH_NativeWindow_GetMatedataValueType(sptr<SurfaceBuffer> sfbuffer, int32_t *size, uint8_t **metadata)
{
    CM_HDR_Metadata_Type hdrMetadataType = CM_METADATA_NONE;
    GSError ret = MetadataHelper::GetHDRMetadataType(sfbuffer, hdrMetadataType);
    if (ret != OHOS::SURFACE_ERROR_OK) {
        BLOGE("GetHDRMetadataType failed!, retVal:%d", ret);
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    for (auto type : NATIVE_METADATATYPE_TO_HDI_MAP) {
        if (type.second == hdrMetadataType) {
            *size = sizeof(OH_NativeBuffer_MetadataType);
            *metadata = new uint8_t[*size];
            errno_t err = memcpy_s(*metadata, *size, &(type.first), *size);
            if (err != 0) {
                delete[] *metadata;
                BLOGE("memcpy_s failed! , retVal:%d", err);
                return OHOS::SURFACE_ERROR_UNKOWN;
            }
            return OHOS::SURFACE_ERROR_OK;
        }
    }
    BLOGE("the hdrMetadataType does not support it.");
    return OHOS::SURFACE_ERROR_NOT_SUPPORT;
}

int32_t OH_NativeWindow_GetMetadataValue(OHNativeWindow *window, OH_NativeBuffer_MetadataKey metadataKey,
    int32_t *size, uint8_t **metadata)
{
    if (window == nullptr || metadata == nullptr || size == nullptr) {
        BLOGE("parameter error, please check input parameter");
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    OHNativeWindowBuffer *lastFlushedBuffer;
    int lastFlushFenceFd;
    float matrix[16];
    int32_t status = GetLastFlushedBuffer(window, &lastFlushedBuffer, &lastFlushFenceFd, matrix);
    if (status != OHOS::SURFACE_ERROR_OK) {
        BLOGE("GetLastFlushedBuffer fail");
        return status;
    }
    GSError ret = GSERROR_OK;
    std::vector<uint8_t> mD;
    if (metadataKey == OH_HDR_DYNAMIC_METADATA) {
        ret = MetadataHelper::GetHDRDynamicMetadata(lastFlushedBuffer->sfbuffer, mD);
    } else if (metadataKey == OH_HDR_STATIC_METADATA) {
        ret = MetadataHelper::GetHDRStaticMetadata(lastFlushedBuffer->sfbuffer, mD);
    } else if (metadataKey == OH_HDR_METADATA_TYPE) {
        ret = OH_NativeWindow_GetMatedataValueType(lastFlushedBuffer->sfbuffer, size, metadata);
        return ret;
    } else {
        BLOGE("the metadataKey does not support it.");
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    if (ret != OHOS::SURFACE_ERROR_OK) {
        BLOGE("SetHDRSMetadata failed! , retVal:%d", ret);
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    *size = mD.size();
    *metadata = new uint8_t[mD.size()];
    if (!mD.empty()) {
        errno_t err = memcpy_s(*metadata, mD.size(), &mD[0], mD.size());
        if (err != 0) {
            delete[] *metadata;
            BLOGE("memcpy_s failed! , retVal:%d", err);
            return OHOS::SURFACE_ERROR_UNKOWN;
        }
    } else {
        delete[] *metadata;
        BLOGE("new metadata failed! ");
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    return OHOS::SURFACE_ERROR_OK;
}

NativeWindow::NativeWindow() : NativeWindowMagic(NATIVE_OBJECT_MAGIC_WINDOW), surface(nullptr)
{
}

NativeWindow::~NativeWindow()
{
    if (surface != nullptr) {
        auto utils = SurfaceUtils::GetInstance();
        utils->RemoveNativeWindow(surface->GetUniqueId());
    }

    for (auto &[seqNum, buffer] : bufferCache_) {
        NativeObjectUnreference(buffer);
    }
    surface = nullptr;
    bufferCache_.clear();
}

NativeWindowBuffer::~NativeWindowBuffer()
{
    sfbuffer = nullptr;
}

NativeWindowBuffer::NativeWindowBuffer() : NativeWindowMagic(NATIVE_OBJECT_MAGIC_WINDOW_BUFFER), sfbuffer(nullptr)
{
}

WEAK_ALIAS(CreateNativeWindowFromSurface, OH_NativeWindow_CreateNativeWindow);
WEAK_ALIAS(DestoryNativeWindow, OH_NativeWindow_DestroyNativeWindow);
WEAK_ALIAS(CreateNativeWindowBufferFromSurfaceBuffer, OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer);
WEAK_ALIAS(CreateNativeWindowBufferFromNativeBuffer, OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer);
WEAK_ALIAS(DestroyNativeWindowBuffer, OH_NativeWindow_DestroyNativeWindowBuffer);
WEAK_ALIAS(NativeWindowRequestBuffer, OH_NativeWindow_NativeWindowRequestBuffer);
WEAK_ALIAS(NativeWindowFlushBuffer, OH_NativeWindow_NativeWindowFlushBuffer);
WEAK_ALIAS(GetLastFlushedBuffer, OH_NativeWindow_GetLastFlushedBuffer);
WEAK_ALIAS(NativeWindowAttachBuffer, OH_NativeWindow_NativeWindowAttachBuffer);
WEAK_ALIAS(NativeWindowDetachBuffer, OH_NativeWindow_NativeWindowDetachBuffer);
WEAK_ALIAS(NativeWindowCancelBuffer, OH_NativeWindow_NativeWindowAbortBuffer);
WEAK_ALIAS(NativeWindowHandleOpt, OH_NativeWindow_NativeWindowHandleOpt);
WEAK_ALIAS(GetBufferHandleFromNative, OH_NativeWindow_GetBufferHandleFromNative);
WEAK_ALIAS(NativeObjectReference, OH_NativeWindow_NativeObjectReference);
WEAK_ALIAS(NativeObjectUnreference, OH_NativeWindow_NativeObjectUnreference);
WEAK_ALIAS(GetNativeObjectMagic, OH_NativeWindow_GetNativeObjectMagic);
WEAK_ALIAS(NativeWindowSetScalingMode, OH_NativeWindow_NativeWindowSetScalingMode);
WEAK_ALIAS(NativeWindowSetScalingModeV2, OH_NativeWindow_NativeWindowSetScalingModeV2);
WEAK_ALIAS(NativeWindowSetMetaData, OH_NativeWindow_NativeWindowSetMetaData);
WEAK_ALIAS(NativeWindowSetMetaDataSet, OH_NativeWindow_NativeWindowSetMetaDataSet);
WEAK_ALIAS(NativeWindowSetTunnelHandle, OH_NativeWindow_NativeWindowSetTunnelHandle);
WEAK_ALIAS(GetSurfaceId, OH_NativeWindow_GetSurfaceId);
WEAK_ALIAS(CreateNativeWindowFromSurfaceId, OH_NativeWindow_CreateNativeWindowFromSurfaceId);
WEAK_ALIAS(NativeWindowSetBufferHold, OH_NativeWindow_SetBufferHold);
WEAK_ALIAS(NativeWindowWriteToParcel, OH_NativeWindow_WriteToParcel);
WEAK_ALIAS(NativeWindowReadFromParcel, OH_NativeWindow_ReadFromParcel);
WEAK_ALIAS(GetLastFlushedBufferV2, OH_NativeWindow_GetLastFlushedBufferV2);

