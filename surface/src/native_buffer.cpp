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

#include "native_buffer_inner.h"

#include <linux/dma-buf.h>
#include <sys/ioctl.h>
#include <cinttypes>
#include "surface_type.h"
#include "buffer_log.h"
#include "native_window.h"
#include "surface_buffer_impl.h"
#include "metadata_helper.h"

#define DMA_BUF_SET_TYPE _IOW(DMA_BUF_BASE, 2, const char *)

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

static OH_NativeBuffer* OH_NativeBufferFromSurfaceBuffer(SurfaceBuffer* buffer)
{
    if (buffer == nullptr) {
        return nullptr;
    }
    return buffer->SurfaceBufferToNativeBuffer();
}

static SurfaceBuffer* OH_NativeBufferToSurfaceBuffer(OH_NativeBuffer *buffer)
{
    return SurfaceBuffer::NativeBufferToSurfaceBuffer(buffer);
}

static const SurfaceBuffer* OH_NativeBufferToSurfaceBuffer(const OH_NativeBuffer *buffer)
{
    return SurfaceBuffer::NativeBufferToSurfaceBuffer(buffer);
}

OH_NativeBuffer* OH_NativeBuffer_Alloc(const OH_NativeBuffer_Config* config)
{
    if (config == nullptr) {
        return nullptr;
    }
    BufferRequestConfig bfConfig = {};
    bfConfig.width = config->width;
    bfConfig.height = config->height;
    bfConfig.strideAlignment = 0x8; // set 0x8 as default value to alloc SurfaceBufferImpl
    bfConfig.format = config->format; // PixelFormat
    bfConfig.usage = config->usage;
    bfConfig.timeout = 0;
    bfConfig.colorGamut = GraphicColorGamut::GRAPHIC_COLOR_GAMUT_SRGB;
    bfConfig.transform = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    sptr<SurfaceBuffer> bufferImpl = new SurfaceBufferImpl();
    GSError ret = bufferImpl->Alloc(bfConfig);
    if (ret != OHOS::SURFACE_ERROR_OK) {
        BLOGE("Alloc failed ret: %{public}d, config info: width[%{public}d, height[%{public}d,"
            "format[%{public}d], usage[%{public}d]", ret, config->width, config->height,
            config->format, config->usage);
        return nullptr;
    }

    OH_NativeBuffer* buffer = OH_NativeBufferFromSurfaceBuffer(bufferImpl);
    int32_t err = OH_NativeBuffer_Reference(buffer);
    if (err != OHOS::SURFACE_ERROR_OK) {
        BLOGE("NativeBufferReference failed, err: %{public}d.", err);
        return nullptr;
    }
    if (bufferImpl->GetBufferHandle() != nullptr && bufferImpl->GetBufferHandle()->fd > 0) {
        ioctl(bufferImpl->GetBufferHandle()->fd, DMA_BUF_SET_TYPE, "external");
    }
    return buffer;
}

int32_t OH_NativeBuffer_Reference(OH_NativeBuffer *buffer)
{
    if (buffer == nullptr) {
        return OHOS::GSERROR_INVALID_ARGUMENTS;
    }
    OHOS::RefBase *ref = reinterpret_cast<OHOS::RefBase *>(buffer);
    ref->IncStrongRef(ref);
    return OHOS::SURFACE_ERROR_OK;
}

int32_t OH_NativeBuffer_Unreference(OH_NativeBuffer *buffer)
{
    if (buffer == nullptr) {
        return OHOS::GSERROR_INVALID_ARGUMENTS;
    }
    OHOS::RefBase *ref = reinterpret_cast<OHOS::RefBase *>(buffer);
    ref->DecStrongRef(ref);
    return OHOS::SURFACE_ERROR_OK;
}

void OH_NativeBuffer_GetConfig(OH_NativeBuffer *buffer, OH_NativeBuffer_Config* config)
{
    if (buffer == nullptr || config == nullptr) {
        return;
    }
    const SurfaceBuffer* sbuffer = OH_NativeBufferToSurfaceBuffer(buffer);
    config->width = sbuffer->GetWidth();
    config->height = sbuffer->GetHeight();
    config->format = sbuffer->GetFormat();
    config->usage = sbuffer->GetUsage();
    config->stride = sbuffer->GetStride();
}

int32_t OH_NativeBuffer_Map(OH_NativeBuffer *buffer, void **virAddr)
{
    if (buffer == nullptr || virAddr == nullptr) {
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    SurfaceBuffer* sbuffer = OH_NativeBufferToSurfaceBuffer(buffer);
    int32_t ret = sbuffer->Map();
    if (ret == OHOS::SURFACE_ERROR_OK) {
        *virAddr = sbuffer->GetVirAddr();
    } else {
        BLOGE("Map failed, ret:%{public}d", ret);
        ret = OHOS::SURFACE_ERROR_UNKOWN;
    }
    return ret;
}

int32_t OH_NativeBuffer_Unmap(OH_NativeBuffer *buffer)
{
    if (buffer == nullptr) {
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    SurfaceBuffer* sbuffer = OH_NativeBufferToSurfaceBuffer(buffer);
    int32_t ret = sbuffer->Unmap();
    if (ret != OHOS::SURFACE_ERROR_OK) {
        BLOGE("Unmap failed, ret:%{public}d", ret);
        ret = OHOS::SURFACE_ERROR_UNKOWN;
    }
    return ret;
}

uint32_t OH_NativeBuffer_GetSeqNum(OH_NativeBuffer *buffer)
{
    if (buffer == nullptr) {
        return UINT_MAX;
    }
    const SurfaceBuffer* sbuffer = OH_NativeBufferToSurfaceBuffer(buffer);
    return sbuffer->GetSeqNum();
}

const BufferHandle* OH_NativeBuffer_GetBufferHandle(const OH_NativeBuffer *buffer)
{
    if (buffer == nullptr) {
        return nullptr;
    }
    const SurfaceBuffer* sbuffer = OH_NativeBufferToSurfaceBuffer(buffer);
    return sbuffer->GetBufferHandle();
}

void OH_NativeBuffer_GetNativeBufferConfig(const OH_NativeBuffer *buffer, OH_NativeBuffer_Config* config)
{
    if (buffer == nullptr || config == nullptr) {
        return;
    }
    const SurfaceBuffer* sbuffer = OH_NativeBufferToSurfaceBuffer(buffer);
    config->width = sbuffer->GetWidth();
    config->height = sbuffer->GetHeight();
    config->format = sbuffer->GetFormat();
    config->usage = sbuffer->GetUsage();
    config->stride = sbuffer->GetStride();
}

OH_NativeBuffer* OH_NativeBufferFromNativeWindowBuffer(OHNativeWindowBuffer* nativeWindowBuffer)
{
    if (nativeWindowBuffer == nullptr) {
        return nullptr;
    }
    OH_NativeBuffer* buffer = OH_NativeBufferFromSurfaceBuffer(nativeWindowBuffer->sfbuffer);
    return buffer;
}

int32_t OH_NativeBuffer_SetColorSpace(OH_NativeBuffer *buffer, OH_NativeBuffer_ColorSpace colorSpace)
{
    if (buffer == nullptr || NATIVE_COLORSPACE_TO_HDI_MAP.find(colorSpace) == NATIVE_COLORSPACE_TO_HDI_MAP.end()) {
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    sptr<SurfaceBuffer> sbuffer = OH_NativeBufferToSurfaceBuffer(buffer);
    GSError ret = MetadataHelper::SetColorSpaceType(sbuffer, NATIVE_COLORSPACE_TO_HDI_MAP[colorSpace]);
    if (ret == OHOS::GSERROR_HDI_ERROR) {
        return OHOS::SURFACE_ERROR_NOT_SUPPORT;
    } else if (ret != OHOS::SURFACE_ERROR_OK) {
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    return OHOS::SURFACE_ERROR_OK;
}

int32_t OH_NativeBuffer_MapPlanes(OH_NativeBuffer *buffer, void **virAddr, OH_NativeBuffer_Planes *outPlanes)
{
    if (buffer == nullptr || virAddr == nullptr || outPlanes == nullptr) {
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    sptr<SurfaceBuffer> sbuffer = OH_NativeBufferToSurfaceBuffer(buffer);
    int32_t ret = sbuffer->Map();
    if (ret == OHOS::SURFACE_ERROR_OK) {
        *virAddr = sbuffer->GetVirAddr();
    } else {
        BLOGE("Map failed, %{public}d", ret);
        return ret;
    }
    OH_NativeBuffer_Planes *planes = nullptr;
    GSError retVal = sbuffer->GetPlanesInfo(reinterpret_cast<void**>(&planes));
    if (retVal != OHOS::SURFACE_ERROR_OK) {
        BLOGE("GetPlanesInfo failed, retVal:%{public}d", retVal);
        return retVal;
    }
    outPlanes->planeCount = planes->planeCount;
    for (uint32_t i = 0; i < planes->planeCount && i < 4; i++) { // 4: max plane count
        outPlanes->planes[i].offset = planes->planes[i].offset;
        outPlanes->planes[i].rowStride = planes->planes[i].rowStride;
        outPlanes->planes[i].columnStride = planes->planes[i].columnStride;
    }
    return OHOS::SURFACE_ERROR_OK;
}

int32_t OH_NativeBuffer_FromNativeWindowBuffer(OHNativeWindowBuffer *nativeWindowBuffer, OH_NativeBuffer **buffer)
{
    if (nativeWindowBuffer == nullptr || buffer == nullptr) {
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    *buffer = OH_NativeBufferFromSurfaceBuffer(nativeWindowBuffer->sfbuffer);
    if (*buffer == nullptr) {
        BLOGE("get sfbuffer is nullptr");
        return OHOS::GSERROR_INVALID_OPERATING;
    }
    return OHOS::SURFACE_ERROR_OK;
}

int32_t OH_NativeBuffer_GetColorSpace(OH_NativeBuffer *buffer, OH_NativeBuffer_ColorSpace *colorSpace)
{
    if (buffer == nullptr || colorSpace == nullptr) {
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    sptr<SurfaceBuffer> sbuffer = OH_NativeBufferToSurfaceBuffer(buffer);
    OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType colorSpaceType;
    GSError ret = MetadataHelper::GetColorSpaceType(sbuffer, colorSpaceType);
    if (ret == OHOS::GSERROR_HDI_ERROR) {
        return OHOS::SURFACE_ERROR_NOT_SUPPORT;
    } else if (ret != OHOS::SURFACE_ERROR_OK) {
        BLOGE("GetColorSpaceType failed!, retVal:%{public}d", ret);
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    auto it = std::find_if(NATIVE_COLORSPACE_TO_HDI_MAP.begin(), NATIVE_COLORSPACE_TO_HDI_MAP.end(),
        [colorSpaceType](const std::pair<OH_NativeBuffer_ColorSpace, CM_ColorSpaceType>& element) {
            return element.second == colorSpaceType;
        });
    if (it != NATIVE_COLORSPACE_TO_HDI_MAP.end()) {
        *colorSpace = it->first;
        return OHOS::SURFACE_ERROR_OK;
    }
    BLOGE("the colorSpace does not support it.");
    return OHOS::SURFACE_ERROR_UNKOWN;
}

int32_t OH_NativeBuffer_SetMetadataValue(OH_NativeBuffer *buffer, OH_NativeBuffer_MetadataKey metadataKey,
    int32_t size, uint8_t *metadata)
{
    if (buffer == nullptr || metadata == nullptr || size <= 0) {
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    sptr<SurfaceBuffer> sbuffer = OH_NativeBufferToSurfaceBuffer(buffer);
    GSError ret = GSERROR_OK;
    std::vector<uint8_t> mD(metadata, metadata + size);
    if (metadataKey == OH_HDR_DYNAMIC_METADATA) {
        ret = MetadataHelper::SetHDRDynamicMetadata(sbuffer, mD);
    } else if (metadataKey == OH_HDR_STATIC_METADATA) {
        ret = MetadataHelper::SetHDRStaticMetadata(sbuffer, mD);
    } else if (metadataKey == OH_HDR_METADATA_TYPE) {
        OH_NativeBuffer_MetadataType hdrMetadataType = static_cast<OH_NativeBuffer_MetadataType>(*metadata);
        if (NATIVE_METADATATYPE_TO_HDI_MAP.find(hdrMetadataType) == NATIVE_METADATATYPE_TO_HDI_MAP.end()) {
            BLOGE("the metadataType is not defined.");
            return OHOS::SURFACE_ERROR_INVALID_PARAM;
        }
        ret = MetadataHelper::SetHDRMetadataType(sbuffer, NATIVE_METADATATYPE_TO_HDI_MAP[hdrMetadataType]);
    } else {
        BLOGE("the metadataKey does not support it.");
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    if (ret == OHOS::GSERROR_HDI_ERROR) {
        return OHOS::SURFACE_ERROR_NOT_SUPPORT;
    } else if (ret != OHOS::SURFACE_ERROR_OK) {
        BLOGE("SetHDRMetadata failed!, retVal:%{public}d", ret);
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    return OHOS::SURFACE_ERROR_OK;
}

static GSError OH_NativeBuffer_GetMatedataValueType(sptr<SurfaceBuffer> sbuffer, int32_t *size, uint8_t **metadata)
{
    CM_HDR_Metadata_Type hdrMetadataType = CM_METADATA_NONE;
    GSError ret = MetadataHelper::GetHDRMetadataType(sbuffer, hdrMetadataType);
    if (ret == OHOS::GSERROR_HDI_ERROR) {
        return OHOS::SURFACE_ERROR_NOT_SUPPORT;
    } else if (ret != OHOS::SURFACE_ERROR_OK) {
        BLOGE("GetHDRMetadataType failed!, ret: %{public}d", ret);
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    auto it = std::find_if(NATIVE_METADATATYPE_TO_HDI_MAP.begin(), NATIVE_METADATATYPE_TO_HDI_MAP.end(),
    [hdrMetadataType](const std::pair<OH_NativeBuffer_MetadataType, CM_HDR_Metadata_Type>& element) {
        return element.second == hdrMetadataType;
    });
    if (it != NATIVE_METADATATYPE_TO_HDI_MAP.end()) {
        *size = sizeof(OH_NativeBuffer_MetadataType);
        *metadata = new uint8_t[*size];
        errno_t err = memcpy_s(*metadata, *size, &(it->first), *size);
        if (err != 0) {
            delete[] *metadata;
            *metadata = nullptr;
            BLOGE("memcpy_s failed!, ret: %{public}d", err);
            return OHOS::SURFACE_ERROR_UNKOWN;
        }
        return OHOS::SURFACE_ERROR_OK;
    }
    BLOGE("the hdrMetadataType does not support it.");
    return OHOS::SURFACE_ERROR_NOT_SUPPORT;
}

int32_t OH_NativeBuffer_GetMetadataValue(OH_NativeBuffer *buffer, OH_NativeBuffer_MetadataKey metadataKey,
    int32_t *size, uint8_t **metadata)
{
    if (buffer == nullptr || metadata == nullptr || size == nullptr) {
        return OHOS::SURFACE_ERROR_INVALID_PARAM;
    }
    sptr<SurfaceBuffer> sbuffer = OH_NativeBufferToSurfaceBuffer(buffer);
    GSError ret = GSERROR_OK;
    std::vector<uint8_t> mD;
    if (metadataKey == OH_HDR_DYNAMIC_METADATA) {
        ret = MetadataHelper::GetHDRDynamicMetadata(sbuffer, mD);
    } else if (metadataKey == OH_HDR_STATIC_METADATA) {
        ret = MetadataHelper::GetHDRStaticMetadata(sbuffer, mD);
    } else if (metadataKey == OH_HDR_METADATA_TYPE) {
        ret = OH_NativeBuffer_GetMatedataValueType(sbuffer, size, metadata);
        return ret;
    } else {
        BLOGE("the metadataKey does not support it.");
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    if (ret == OHOS::GSERROR_HDI_ERROR) {
        return OHOS::SURFACE_ERROR_NOT_SUPPORT;
    } else if (ret != OHOS::SURFACE_ERROR_OK) {
        BLOGE("SetHDRSMetadata failed!, ret: %{public}d", ret);
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    *size = mD.size();
    if (mD.empty()) {
        BLOGE("Metadata is empty!");
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    *metadata = new uint8_t[mD.size()];
    errno_t err = memcpy_s(*metadata, mD.size(), &mD[0], mD.size());
    if (err != 0) {
        delete[] *metadata;
        *metadata = nullptr;
        BLOGE("memcpy_s failed!, ret: %{public}d", err);
        return OHOS::SURFACE_ERROR_UNKOWN;
    }
    return OHOS::SURFACE_ERROR_OK;
}