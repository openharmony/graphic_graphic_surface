/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "metadata_helper.h"
#include "buffer_log.h"

#include "v2_1/buffer_handle_meta_key_type.h"

using namespace OHOS::HDI::Display::Graphic::Common::V1_0;

namespace OHOS {
GSError MetadataHelper::ConvertColorSpaceTypeToInfo(const CM_ColorSpaceType& colorSpaceType,
    CM_ColorSpaceInfo& colorSpaceInfo)
{
    uint32_t colorSpace = static_cast<uint32_t>(colorSpaceType);
    colorSpaceInfo.primaries = static_cast<CM_ColorPrimaries>(colorSpace & PRIMARIES_MASK);
    colorSpaceInfo.transfunc = static_cast<CM_TransFunc>((colorSpace & TRANSFUNC_MASK) >> TRANSFUNC_OFFSET);
    colorSpaceInfo.matrix = static_cast<CM_Matrix>((colorSpace & MATRIX_MASK) >> MATRIX_OFFSET);
    colorSpaceInfo.range = static_cast<CM_Range>((colorSpace & RANGE_MASK) >> RANGE_OFFSET);
    return GSERROR_OK;
}

GSError MetadataHelper::ConvertColorSpaceInfoToType(const CM_ColorSpaceInfo& colorSpaceInfo,
    CM_ColorSpaceType& colorSpaceType)
{
    uint32_t primaries = static_cast<uint32_t>(colorSpaceInfo.primaries);
    uint32_t transfunc = static_cast<uint32_t>(colorSpaceInfo.transfunc);
    uint32_t matrix = static_cast<uint32_t>(colorSpaceInfo.matrix);
    uint32_t range = static_cast<uint32_t>(colorSpaceInfo.range);
    colorSpaceType = static_cast<CM_ColorSpaceType>(primaries | (transfunc << TRANSFUNC_OFFSET) |
        (matrix << MATRIX_OFFSET) | (range << RANGE_OFFSET));

    return GSERROR_OK;
}

GSError MetadataHelper::SetColorSpaceInfo(sptr<SurfaceBuffer>& buffer, const CM_ColorSpaceInfo& colorSpaceInfo)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    std::vector<uint8_t> colorSpaceInfoVec;
    auto ret = ConvertMetadataToVec(colorSpaceInfo, colorSpaceInfoVec);
    if (ret != GSERROR_OK) {
        return ret;
    }
    return buffer->SetMetadata(ATTRKEY_COLORSPACE_INFO, colorSpaceInfoVec);
}

GSError MetadataHelper::GetColorSpaceInfo(const sptr<SurfaceBuffer>& buffer, CM_ColorSpaceInfo& colorSpaceInfo)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    std::vector<uint8_t> colorSpaceInfoVec;
    auto ret = buffer->GetMetadata(ATTRKEY_COLORSPACE_INFO, colorSpaceInfoVec);
    if (ret != GSERROR_OK) {
        return ret;
    }
    return ConvertVecToMetadata(colorSpaceInfoVec, colorSpaceInfo);
}

GSError MetadataHelper::SetColorSpaceType(sptr<SurfaceBuffer>& buffer, const CM_ColorSpaceType& colorSpaceType)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    CM_ColorSpaceInfo colorSpaceInfo;
    auto ret = ConvertColorSpaceTypeToInfo(colorSpaceType, colorSpaceInfo);
    if (ret != GSERROR_OK) {
        return ret;
    }
    return SetColorSpaceInfo(buffer, colorSpaceInfo);
}

GSError MetadataHelper::GetColorSpaceType(const sptr<SurfaceBuffer>& buffer, CM_ColorSpaceType& colorSpaceType)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    CM_ColorSpaceInfo colorSpaceInfo;
    auto ret = GetColorSpaceInfo(buffer, colorSpaceInfo);
    if (ret != GSERROR_OK) {
        return ret;
    }
    return ConvertColorSpaceInfoToType(colorSpaceInfo, colorSpaceType);
}

GSError MetadataHelper::SetHDRMetadataType(sptr<SurfaceBuffer>& buffer, const CM_HDR_Metadata_Type& hdrMetadataType)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    std::vector<uint8_t> hdrMetadataTypeVec;
    auto ret = ConvertMetadataToVec(hdrMetadataType, hdrMetadataTypeVec);
    if (ret != GSERROR_OK) {
        return ret;
    }
    return buffer->SetMetadata(ATTRKEY_HDR_METADATA_TYPE, hdrMetadataTypeVec);
}

GSError MetadataHelper::GetHDRMetadataType(const sptr<SurfaceBuffer>& buffer, CM_HDR_Metadata_Type& hdrMetadataType)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    std::vector<uint8_t> hdrMetadataTypeVec;
    auto ret = buffer->GetMetadata(ATTRKEY_HDR_METADATA_TYPE, hdrMetadataTypeVec);
    if (ret != GSERROR_OK) {
        return ret;
    }
    return ConvertVecToMetadata(hdrMetadataTypeVec, hdrMetadataType);
}

GSError MetadataHelper::SetHDRStaticMetadata(sptr<SurfaceBuffer>& buffer,
    const HdrStaticMetadata& hdrStaticMetadata)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    std::vector<uint8_t> hdrStaticMetadataVec;
    auto ret = ConvertMetadataToVec(hdrStaticMetadata, hdrStaticMetadataVec);
    if (ret != GSERROR_OK) {
        return ret;
    }
    return buffer->SetMetadata(ATTRKEY_HDR_STATIC_METADATA, hdrStaticMetadataVec);
}

GSError MetadataHelper::GetHDRStaticMetadata(const sptr<SurfaceBuffer>& buffer,
    HdrStaticMetadata& hdrStaticMetadata)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    std::vector<uint8_t> hdrStaticMetadataVec;
    auto ret = buffer->GetMetadata(ATTRKEY_HDR_STATIC_METADATA, hdrStaticMetadataVec);
    if (ret != GSERROR_OK) {
        return ret;
    }
    return ConvertVecToMetadata(hdrStaticMetadataVec, hdrStaticMetadata);
}

GSError MetadataHelper::SetHDRDynamicMetadata(sptr<SurfaceBuffer>& buffer,
    const std::vector<uint8_t>& hdrDynamicMetadata)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    return buffer->SetMetadata(ATTRKEY_HDR_DYNAMIC_METADATA, hdrDynamicMetadata);
}

GSError MetadataHelper::GetHDRDynamicMetadata(const sptr<SurfaceBuffer>& buffer,
    std::vector<uint8_t>& hdrDynamicMetadata)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    return buffer->GetMetadata(ATTRKEY_HDR_DYNAMIC_METADATA, hdrDynamicMetadata);
}

GSError MetadataHelper::SetHDRStaticMetadata(sptr<SurfaceBuffer>& buffer,
    const std::vector<uint8_t>& hdrStaticMetadata)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    return buffer->SetMetadata(ATTRKEY_HDR_STATIC_METADATA, hdrStaticMetadata);
}

GSError MetadataHelper::GetHDRStaticMetadata(const sptr<SurfaceBuffer>& buffer,
    std::vector<uint8_t>& hdrStaticMetadata)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    return buffer->GetMetadata(ATTRKEY_HDR_STATIC_METADATA, hdrStaticMetadata);
}

GSError MetadataHelper::GetCropRectMetadata(const sptr<SurfaceBuffer>& buffer, BufferHandleMetaRegion& crop)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    std::vector<uint8_t> cropRect;
    auto ret = buffer->GetMetadata(ATTRKEY_CROP_REGION, cropRect);
    if (ret != GSERROR_OK) {
        return ret;
    }
    return ConvertVecToMetadata(cropRect, crop);
}

GSError MetadataHelper::SetImageHDRMetadataType(sptr<SurfaceBuffer>& buffer, uint8_t* metadata)
{
    uint8_t metaDataType = static_cast<uint8_t>(*metadata);
    GSError ret = MetadataHelper::SetHDRMetadataType(buffer, static_cast<CM_HDR_Metadata_Type>(metaDataType + 1));
    return ret == OHOS::GSERROR_HDI_ERROR ? OHOS::SURFACE_ERROR_NOT_SUPPORT :
        ret == OHOS::SURFACE_ERROR_OK ? OHOS::SURFACE_ERROR_OK : OHOS::SURFACE_ERROR_UNKOWN;
}

bool MetadataHelper::IsImageMetadataType(uint8_t* metadata)
{
    uint8_t metaDataType = static_cast<uint8_t>(*metadata);
    return (CM_IMAGE_HDR_VIVID_SINGLE == metaDataType + 1);
}

GSError MetadataHelper::SetAdaptiveFOVMetadata(sptr<SurfaceBuffer>& buffer,
    const std::vector<uint8_t>& adaptiveFOVMetadata)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    return buffer->SetMetadata(OHOS::HDI::Display::Graphic::Common::V2_1::ATTRKEY_ADAPTIVE_FOV_METADATA,
        adaptiveFOVMetadata);
}

GSError MetadataHelper::GetAdaptiveFOVMetadata(const sptr<SurfaceBuffer>& buffer,
    std::vector<uint8_t>& adaptiveFOVMetadata)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    return buffer->GetMetadata(OHOS::HDI::Display::Graphic::Common::V2_1::ATTRKEY_ADAPTIVE_FOV_METADATA,
        adaptiveFOVMetadata);
}

GSError MetadataHelper::GetSDRDynamicMetadata(const sptr<SurfaceBuffer>& buffer,
    std::vector<uint8_t>& sdrDynamicMetadata)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }

    return buffer->GetMetadata(OHOS::HDI::Display::Graphic::Common::V2_0::ATTRKEY_EXTERNAL_METADATA_002,
        sdrDynamicMetadata);
}

#ifdef RS_ENABLE_TV_PQ_METADATA
using namespace OHOS::HDI::Display::Graphic::Common::V2_1;

GSError MetadataHelper::UpdateTVMetadataField(sptr<SurfaceBuffer>& buffer, OnSetFieldsFunc func)
{
    if (buffer == nullptr) {
        BLOGE("invalid buffer!");
        return GSERROR_NO_BUFFER;
    }
    TvPQMetadata tvMetadata;
    if (GetVideoTVMetadata(buffer, tvMetadata) != GSERROR_OK) {
        BLOGD("tvMetadata not exist, reset data");
        (void)memset_s(&tvMetadata, sizeof(tvMetadata), 0, sizeof(tvMetadata));
    }
    func(tvMetadata);
    return SetVideoTVMetadata(buffer, tvMetadata);
}

GSError MetadataHelper::SetVideoTVMetadata(sptr<SurfaceBuffer>& buffer, const TvPQMetadata& tvMetadata)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }
    std::vector<uint8_t> tvMetadataVec;
    auto ret = ConvertMetadataToVec(tvMetadata, tvMetadataVec);
    if (ret != GSERROR_OK) {
        BLOGE("tvMetadata ConvertMetadataToVec failed");
        return ret;
    }
    buffer->SetBufferHandle(buffer->GetBufferHandle());
    auto ret1 = buffer->SetMetadata(ATTRKEY_VIDEO_TV_PQ, tvMetadataVec);
    if (ret1 != GSERROR_OK) {
        BLOGE("tvMetadata SetMetadata failed %{public}d! id = %{public}d", ret1, buffer->GetSeqNum());
        return ret1;
    }
    return ret1;
}

GSError MetadataHelper::GetVideoTVMetadata(const sptr<SurfaceBuffer>& buffer, TvPQMetadata& tvMetadata)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }
    std::vector<uint8_t> tvMetadataVec;
    buffer->SetBufferHandle(buffer->GetBufferHandle());
    auto ret = buffer->GetMetadata(ATTRKEY_VIDEO_TV_PQ, tvMetadataVec);
    if (ret != GSERROR_OK) {
        return ret;
    }
    return ConvertVecToMetadata(tvMetadataVec, tvMetadata);
}

GSError MetadataHelper::SetSceneTag(sptr<SurfaceBuffer>& buffer, unsigned char value)
{
    return UpdateTVMetadataField(buffer, [=](TvPQMetadata &tvPQMetadata) {
        tvPQMetadata.sceneTag = value;
        BLOGD("tvMetadata sceneTag = %{public}u", tvPQMetadata.sceneTag);
    });
}

GSError MetadataHelper::SetUIFrameCount(sptr<SurfaceBuffer>& buffer, unsigned char value)
{
    return UpdateTVMetadataField(buffer, [=](TvPQMetadata &tvPQMetadata) {
        tvPQMetadata.uiFrameCnt = value;
        BLOGD("tvMetadata uiFrameCnt = %{public}u", tvPQMetadata.uiFrameCnt);
    });
}

GSError MetadataHelper::SetVideoFrameCount(sptr<SurfaceBuffer>& buffer, unsigned char value)
{
    return UpdateTVMetadataField(buffer, [=](TvPQMetadata &tvPQMetadata) {
        tvPQMetadata.vidFrameCnt = value;
        BLOGD("tvMetadata vidFrameCnt = %{public}u", tvPQMetadata.vidFrameCnt);
    });
}

GSError MetadataHelper::SetVideoFrameRate(sptr<SurfaceBuffer>& buffer, unsigned char value)
{
    return UpdateTVMetadataField(buffer, [=](TvPQMetadata &tvPQMetadata) {
        tvPQMetadata.vidFps = value;
        BLOGD("tvMetadata vidFps = %{public}u", tvPQMetadata.vidFps);
    });
}

GSError MetadataHelper::SetVideoTVInfo(sptr<SurfaceBuffer>& buffer, const TvVideoWindow& tvVideoWindow)
{
    return UpdateTVMetadataField(buffer, [=](TvPQMetadata &tvPQMetadata) {
        tvPQMetadata.vidWinX = tvVideoWindow.x;
        tvPQMetadata.vidWinY = tvVideoWindow.y;
        tvPQMetadata.vidWinWidth = tvVideoWindow.width;
        tvPQMetadata.vidWinHeight = tvVideoWindow.height;
        tvPQMetadata.vidWinSize = tvVideoWindow.size;
        BLOGD("tvMetadata vid_win = (%{public}u, %{public}u, %{public}u, %{public}u), size = %{public}u",
            tvPQMetadata.vidWinX, tvPQMetadata.vidWinY, tvPQMetadata.vidWinWidth, tvPQMetadata.vidWinHeight,
            tvPQMetadata.vidWinSize);
    });
}

GSError MetadataHelper::SetVideoDecoderHigh(sptr<SurfaceBuffer>& buffer, unsigned short vidVdhWidth,
    unsigned short vidVdhHeight)
{
    return UpdateTVMetadataField(buffer, [=](TvPQMetadata &tvPQMetadata) {
        tvPQMetadata.vidVdhWidth = vidVdhWidth;
        tvPQMetadata.vidVdhHeight = vidVdhHeight;
        BLOGD("tvPQMetadata vidVdhWidth = %{public}u, tvPQMetadata vidVdhHeight = %{public}u",
            tvPQMetadata.vidVdhWidth, tvPQMetadata.vidVdhHeight);
    });
}

GSError MetadataHelper::SetVideoTVScaleMode(sptr<SurfaceBuffer>& buffer, unsigned char value)
{
    return UpdateTVMetadataField(buffer, [=](TvPQMetadata &tvPQMetadata) {
        tvPQMetadata.scaleMode = value;
        BLOGD("tvMetadata scaleMode = %{public}u", tvPQMetadata.scaleMode);
    });
}

GSError MetadataHelper::SetVideoTVDpPixelFormat(sptr<SurfaceBuffer>& buffer, unsigned int value)
{
    return UpdateTVMetadataField(buffer, [=](TvPQMetadata &tvPQMetadata) {
        tvPQMetadata.dpPixFmt = value;
        BLOGD("tvMetadata dpPixFmt = %{public}u", tvPQMetadata.dpPixFmt);
    });
}

GSError MetadataHelper::SetVideoColorimetryHdr(sptr<SurfaceBuffer>& buffer, unsigned char hdr,
    unsigned char colorimetry)
{
    return UpdateTVMetadataField(buffer, [=](TvPQMetadata &tvPQMetadata) {
        tvPQMetadata.colorimetry = colorimetry;
        tvPQMetadata.hdr = hdr;
        BLOGD("Colorimetry = %{public}u, HDR = %{public}u", tvPQMetadata.colorimetry, tvPQMetadata.hdr);
    });
}

GSError MetadataHelper::EraseVideoTVInfoKey(sptr<SurfaceBuffer>& buffer)
{
    if (buffer == nullptr) {
        return GSERROR_NO_BUFFER;
    }
    auto ret = buffer->EraseMetadataKey(ATTRKEY_VIDEO_TV_PQ);
    if (ret != GSERROR_OK) {
        BLOGE("tvMetadata EraseVideoTVInfoKey ret = %{public}d", ret);
        return ret;
    }
    return GSERROR_OK;
}
#endif
} // namespace OHOS