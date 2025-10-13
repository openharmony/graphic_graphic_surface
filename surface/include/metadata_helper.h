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

#ifndef FRAMEWORKS_SURFACE_INCLUDE_METADATA_MANAGER_H
#define FRAMEWORKS_SURFACE_INCLUDE_METADATA_MANAGER_H

#include <vector>
#include <securec.h>

#include "graphic_common.h"
#include "surface_buffer.h"
#include "v1_0/cm_color_space.h"
#include "v1_0/hdr_static_metadata.h"
#include "v1_0/buffer_handle_meta_key_type.h"
#ifdef RS_ENABLE_TV_PQ_METADATA
#include <functional>
#include "tv_pq_metadata.h"
#endif

namespace OHOS {
class MetadataHelper {
public:
    // GSERROR_OK for success, GSERROR_API_FAILED for fail
    template <typename T>
    static GSError ConvertMetadataToVec(const T& metadata, std::vector<uint8_t>& data)
    {
        data.resize(sizeof(T));
        if (memcpy_s(data.data(), data.size(), &metadata, sizeof(T)) != EOK) {
            return GSERROR_API_FAILED;
        }
        return GSERROR_OK;
    }

    template <typename T>
    static GSError ConvertVecToMetadata(const std::vector<uint8_t>& data, T& metadata)
    {
        if (data.size() != sizeof(T)) {
            return GSERROR_NOT_SUPPORT;
        }

        if (memcpy_s(&metadata, sizeof(T), data.data(), data.size()) != EOK) {
            return GSERROR_API_FAILED;
        }
        return GSERROR_OK;
    }

    static GSError ConvertColorSpaceTypeToInfo(
        const HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType& colorSpaceType,
        HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceInfo& colorSpaceInfo);
    static GSError ConvertColorSpaceInfoToType(
        const HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceInfo& colorSpaceInfo,
        HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType& colorSpaceType);

    static GSError SetColorSpaceInfo(sptr<SurfaceBuffer>& buffer,
        const HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceInfo& colorSpaceInfo);
    static GSError GetColorSpaceInfo(const sptr<SurfaceBuffer>& buffer,
        HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceInfo& colorSpaceInfo);

    static GSError SetColorSpaceType(sptr<SurfaceBuffer>& buffer,
        const HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType& colorSpaceType);
    static GSError GetColorSpaceType(const sptr<SurfaceBuffer>& buffer,
        HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType& colorSpaceType);

    static GSError SetHDRMetadataType(sptr<SurfaceBuffer>& buffer,
        const HDI::Display::Graphic::Common::V1_0::CM_HDR_Metadata_Type& hdrMetadataType);
    static GSError GetHDRMetadataType(const sptr<SurfaceBuffer>& buffer,
        HDI::Display::Graphic::Common::V1_0::CM_HDR_Metadata_Type& hdrMetadataType);

    static GSError SetHDRStaticMetadata(sptr<SurfaceBuffer>& buffer,
        const HDI::Display::Graphic::Common::V1_0::HdrStaticMetadata& hdrStaticMetadata);
    static GSError GetHDRStaticMetadata(const sptr<SurfaceBuffer>& buffer,
        HDI::Display::Graphic::Common::V1_0::HdrStaticMetadata& hdrStaticMetadata);

    static GSError SetHDRDynamicMetadata(sptr<SurfaceBuffer>& buffer, const std::vector<uint8_t>& hdrDynamicMetadata);
    static GSError GetHDRDynamicMetadata(const sptr<SurfaceBuffer>& buffer, std::vector<uint8_t>& hdrDynamicMetadata);

    static GSError SetHDRStaticMetadata(sptr<SurfaceBuffer>& buffer, const std::vector<uint8_t>& hdrStaticMetadata);
    static GSError GetHDRStaticMetadata(const sptr<SurfaceBuffer>& buffer, std::vector<uint8_t>& hdrStaticMetadata);

    static GSError GetCropRectMetadata(const sptr<SurfaceBuffer>& buffer,
        HDI::Display::Graphic::Common::V1_0::BufferHandleMetaRegion& crop);

    static GSError SetROIMetadata(sptr<SurfaceBuffer>& buffer, const std::vector<uint8_t>& roiMetadata);

#ifdef RS_ENABLE_TV_PQ_METADATA
    using OnSetFieldsFunc = std::function<void(TvPQMetadata &tvPQMetadata)>;
    static GSError SetVideoTVMetadata(sptr<SurfaceBuffer>& buffer, const TvPQMetadata& tvMetadata);
    static GSError GetVideoTVMetadata(const sptr<SurfaceBuffer>& buffer, TvPQMetadata& tvMetadata);

    static GSError SetSceneTag(sptr<SurfaceBuffer>& buffer, unsigned char value);
    static GSError SetUIFrameCount(sptr<SurfaceBuffer>& buffer, unsigned char value);
    static GSError SetVideoFrameCount(sptr<SurfaceBuffer>& buffer, unsigned char value);
    static GSError SetVideoFrameRate(sptr<SurfaceBuffer>& buffer, unsigned char value);
    static GSError SetVideoDecoderHigh(sptr<SurfaceBuffer>& buffer, unsigned short vidVdhWidth,
        unsigned short vidVdhHeight);
    static GSError SetVideoTVScaleMode(sptr<SurfaceBuffer>& buffer, unsigned char value);
    static GSError SetVideoTVDpPixelFormat(sptr<SurfaceBuffer>& buffer, unsigned int value);
    static GSError SetVideoColorimetryHdr(sptr<SurfaceBuffer>& buffer, unsigned char hdr, unsigned char colorimetry);
    static GSError SetVideoTVInfo(sptr<SurfaceBuffer>& buffer, const TvVideoWindow& tvVideoWindow);
    static GSError EraseVideoTVInfoKey(sptr<SurfaceBuffer>& buffer);
#endif
    static GSError SetAdaptiveFOVMetadata(sptr<SurfaceBuffer>& buffer,
        const std::vector<uint8_t>& adaptiveFOVMetadata);
    static GSError GetAdaptiveFOVMetadata(const sptr<SurfaceBuffer>& buffer,
        std::vector<uint8_t>& adaptiveFOVMetadata);

    static GSError GetSDRDynamicMetadata(const sptr<SurfaceBuffer>& buffer,
        std::vector<uint8_t>& sdrDynamicMetadata);
private:
#ifdef RS_ENABLE_TV_PQ_METADATA
    static GSError UpdateTVMetadataField(sptr<SurfaceBuffer>& buffer, OnSetFieldsFunc func);
#endif
    static constexpr uint32_t PRIMARIES_MASK =
        static_cast<uint32_t>(HDI::Display::Graphic::Common::V1_0::CM_PRIMARIES_MASK);
    static constexpr uint32_t TRANSFUNC_MASK =
        static_cast<uint32_t>(HDI::Display::Graphic::Common::V1_0::CM_TRANSFUNC_MASK);
    static constexpr uint32_t MATRIX_MASK =
        static_cast<uint32_t>(HDI::Display::Graphic::Common::V1_0::CM_MATRIX_MASK);
    static constexpr uint32_t RANGE_MASK =
        static_cast<uint32_t>(HDI::Display::Graphic::Common::V1_0::CM_RANGE_MASK);
    static constexpr uint32_t TRANSFUNC_OFFSET = 8;
    static constexpr uint32_t MATRIX_OFFSET = 16;
    static constexpr uint32_t RANGE_OFFSET = 21;
};
} // namespace OHOS

#endif // FRAMEWORKS_SURFACE_INCLUDE_METADATA_MANAGER_H