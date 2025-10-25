/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

/**
 * @addtogroup OH_NativeBuffer
 * @{
 *
 * @brief Provides the common types for native buffer.
 *
 * @since 12
 * @version 1.0
 */

/**
 * @file buffer_common.h
 *
 * @brief Defines the common types for native buffer.
 *
 * @kit ArkGraphics2D
 * @library libnative_buffer.so
 * @syscap SystemCapability.Graphic.Graphic2D.NativeBuffer
 * @since 12
 * @version 1.0
 */

#ifndef NDK_INCLUDE_BUFFER_COMMON_H_
#define NDK_INCLUDE_BUFFER_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Indicates the color space of a native buffer.
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeBuffer
 * @since 11
 * @version 1.0
 */
/**
 * @brief Indicates the color space of a native buffer.
 * Move from native_buffer.h to native_common.h
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeBuffer
 * @since 12
 * @version 1.0
 */
typedef enum OH_NativeBuffer_ColorSpace {
    /** None color space */
    OH_COLORSPACE_NONE,
    /** COLORPRIMARIES_BT601_P | (TRANSFUNC_BT709 << 8) | (MATRIX_BT601_P << 16) | (RANGE_FULL << 21) */
    OH_COLORSPACE_BT601_EBU_FULL,
    /** COLORPRIMARIES_BT601_N | (TRANSFUNC_BT709 << 8) | (MATRIX_BT601_N << 16) | (RANGE_FULL << 21)*/
    OH_COLORSPACE_BT601_SMPTE_C_FULL,
    /** COLORPRIMARIES_BT709 | (TRANSFUNC_BT709 << 8) | (MATRIX_BT709 << 16) | (RANGE_FULL << 21) */
    OH_COLORSPACE_BT709_FULL,
    /** COLORPRIMARIES_BT2020 | (TRANSFUNC_HLG << 8) | (MATRIX_BT2020 << 16) | (RANGE_FULL << 21) */
    OH_COLORSPACE_BT2020_HLG_FULL,
    /** COLORPRIMARIES_BT2020 | (TRANSFUNC_PQ << 8) | (MATRIX_BT2020 << 16) | (RANGE_FULL << 21) */
    OH_COLORSPACE_BT2020_PQ_FULL,
    /** COLORPRIMARIES_BT601_P | (TRANSFUNC_BT709 << 8) | (MATRIX_BT601_P << 16) | (RANGE_LIMITED << 21) */
    OH_COLORSPACE_BT601_EBU_LIMIT,
    /** COLORPRIMARIES_BT601_N | (TRANSFUNC_BT709 << 8) | (MATRIX_BT601_N << 16) | (RANGE_LIMITED << 21) */
    OH_COLORSPACE_BT601_SMPTE_C_LIMIT,
    /** COLORPRIMARIES_BT709 | (TRANSFUNC_BT709 << 8) | (MATRIX_BT709 << 16) | (RANGE_LIMITED << 21) */
    OH_COLORSPACE_BT709_LIMIT,
    /** COLORPRIMARIES_BT2020 | (TRANSFUNC_HLG << 8) | (MATRIX_BT2020 << 16) | (RANGE_LIMITED << 21) */
    OH_COLORSPACE_BT2020_HLG_LIMIT,
    /** COLORPRIMARIES_BT2020 | (TRANSFUNC_PQ << 8) | (MATRIX_BT2020 << 16) | (RANGE_LIMITED << 21) */
    OH_COLORSPACE_BT2020_PQ_LIMIT,
    /** COLORPRIMARIES_SRGB | (TRANSFUNC_SRGB << 8) | (MATRIX_BT601_N << 16) | (RANGE_FULL << 21) */
    OH_COLORSPACE_SRGB_FULL,
    /** COLORPRIMARIES_P3_D65 | (TRANSFUNC_SRGB << 8) | (MATRIX_P3 << 16) | (RANGE_FULL << 21) */
    OH_COLORSPACE_P3_FULL,
    /** COLORPRIMARIES_P3_D65 | (TRANSFUNC_HLG << 8) | (MATRIX_P3 << 16) | (RANGE_FULL << 21) */
    OH_COLORSPACE_P3_HLG_FULL,
    /** COLORPRIMARIES_P3_D65 | (TRANSFUNC_PQ << 8) | (MATRIX_P3 << 16) | (RANGE_FULL << 21) */
    OH_COLORSPACE_P3_PQ_FULL,
    /** COLORPRIMARIES_ADOBERGB | (TRANSFUNC_ADOBERGB << 8) | (MATRIX_ADOBERGB << 16) | (RANGE_FULL << 21) */
    OH_COLORSPACE_ADOBERGB_FULL,
    /** COLORPRIMARIES_SRGB | (TRANSFUNC_SRGB << 8) | (MATRIX_BT601_N << 16) | (RANGE_LIMITED << 21) */
    OH_COLORSPACE_SRGB_LIMIT,
    /** COLORPRIMARIES_P3_D65 | (TRANSFUNC_SRGB << 8) | (MATRIX_P3 << 16) | (RANGE_LIMITED << 21) */
    OH_COLORSPACE_P3_LIMIT,
    /** COLORPRIMARIES_P3_D65 | (TRANSFUNC_HLG << 8) | (MATRIX_P3 << 16) | (RANGE_LIMITED << 21) */
    OH_COLORSPACE_P3_HLG_LIMIT,
    /** COLORPRIMARIES_P3_D65 | (TRANSFUNC_PQ << 8) | (MATRIX_P3 << 16) | (RANGE_LIMITED << 21) */
    OH_COLORSPACE_P3_PQ_LIMIT,
    /** COLORPRIMARIES_ADOBERGB | (TRANSFUNC_ADOBERGB << 8) | (MATRIX_ADOBERGB << 16) | (RANGE_LIMITED << 21) */
    OH_COLORSPACE_ADOBERGB_LIMIT,
    /** COLORPRIMARIES_SRGB | (TRANSFUNC_LINEAR << 8) */
    OH_COLORSPACE_LINEAR_SRGB,
    /** equal to OH_COLORSPACE_LINEAR_SRGB */
    OH_COLORSPACE_LINEAR_BT709,
    /** COLORPRIMARIES_P3_D65 | (TRANSFUNC_LINEAR << 8) */
    OH_COLORSPACE_LINEAR_P3,
    /** COLORPRIMARIES_BT2020 | (TRANSFUNC_LINEAR << 8) */
    OH_COLORSPACE_LINEAR_BT2020,
    /** equal to OH_COLORSPACE_SRGB_FULL */
    OH_COLORSPACE_DISPLAY_SRGB,
    /** equal to OH_COLORSPACE_P3_FULL */
    OH_COLORSPACE_DISPLAY_P3_SRGB,
    /** equal to OH_COLORSPACE_P3_HLG_FULL */
    OH_COLORSPACE_DISPLAY_P3_HLG,
    /** equal to OH_COLORSPACE_P3_PQ_FULL */
    OH_COLORSPACE_DISPLAY_P3_PQ,
    /** COLORPRIMARIES_BT2020 | (TRANSFUNC_SRGB << 8) | (MATRIX_BT2020 << 16) | (RANGE_FULL << 21) */
    OH_COLORSPACE_DISPLAY_BT2020_SRGB,
    /** equal to OH_COLORSPACE_BT2020_HLG_FULL */
    OH_COLORSPACE_DISPLAY_BT2020_HLG,
    /** equal to OH_COLORSPACE_BT2020_PQ_FULL */
    OH_COLORSPACE_DISPLAY_BT2020_PQ,
} OH_NativeBuffer_ColorSpace;

/**
 * @brief Indicates the HDR metadata type of a native buffer.
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeBuffer
 * @since 12
 * @version 1.0
 */
typedef enum OH_NativeBuffer_MetadataType {
    /** HLG */
    OH_VIDEO_HDR_HLG,
    /** HDR10 */
    OH_VIDEO_HDR_HDR10,
    /** HDR VIVID */
    OH_VIDEO_HDR_VIVID,
    /**
     * HDR IMAGE DUAL.
     * @since 22
     */
    OH_IMAGE_HDR_VIVID_DUAL,
    /**
     * HDR IMAGE SINGLE.
     * @since 22
     */
    OH_IMAGE_HDR_VIVID_SINGLE,
    /**
     * NONE Metadata
     * @since 13
     */
    OH_VIDEO_NONE = -1
} OH_NativeBuffer_MetadataType;

/**
 * @brief Indicates the color x and y.
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeBuffer
 * @since 12
 * @version 1.0
 */
typedef struct OH_NativeBuffer_ColorXY {
    /** color X */
    float x;
    /** color Y */
    float y;
} OH_NativeBuffer_ColorXY;

/**
 * @brief Indicates the smpte2086 metadata.
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeBuffer
 * @since 12
 * @version 1.0
 */
typedef struct OH_NativeBuffer_Smpte2086 {
    /** primary red */
    OH_NativeBuffer_ColorXY displayPrimaryRed;
    /** primary green */
    OH_NativeBuffer_ColorXY displayPrimaryGreen;
    /** primary blue */
    OH_NativeBuffer_ColorXY displayPrimaryBlue;
    /** white point */
    OH_NativeBuffer_ColorXY whitePoint;
    /** max luminance */
    float maxLuminance;
    /** min luminance */
    float minLuminance;
} OH_NativeBuffer_Smpte2086;

/**
 * @brief Indicates the cta861.3 metadata.
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeBuffer
 * @since 12
 * @version 1.0
 */
typedef struct OH_NativeBuffer_Cta861 {
    /** max content lightLevel */
    float maxContentLightLevel;
    /** max frame average light level */
    float maxFrameAverageLightLevel;
} OH_NativeBuffer_Cta861;

/**
 * @brief Indicates the HDR static metadata.
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeBuffer
 * @since 12
 * @version 1.0
 */
typedef struct OH_NativeBuffer_StaticMetadata {
    /** smpte 2086 metadata*/
    OH_NativeBuffer_Smpte2086 smpte2086;
    /** CTA-861.3 metadata*/
    OH_NativeBuffer_Cta861 cta861;
} OH_NativeBuffer_StaticMetadata;

/**
 * @brief Indicates the descriptive information of a native buffer,
 *  such as HDR metadata, ROI metadata, etc.
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeBuffer
 * @since 12
 * @version 1.0
 */
typedef enum OH_NativeBuffer_MetadataKey {
    /** value: OH_NativeBuffer_MetadataType*/
    OH_HDR_METADATA_TYPE,
    /** value: OH_NativeBuffer_StaticMetadata*/
    OH_HDR_STATIC_METADATA,
    /** byte stream of SEI in video stream*/
    OH_HDR_DYNAMIC_METADATA,
    /**
     * Region of interest(ROI) metadata is used to conifgure ROI feature in video encoding. Value type is string
     * in the format "Top1,Left1-Bottom1,Right1=QpOffset1;Top2,Left2-Bottom2,Right2=QpOffset2;".
     * Each "Top,Left-Bottom,Right=QpOffset" represents the coordinate information and quantization parameter
     * offset of one ROI. Each "=QpOffset" in the string can be omitted,
     * like "Top1,Left1-Bottom1,Right1;Top2,Left2-Bottom2,Right2=QpOffset2;", the encoder will use the default
     * quantization parameter offset to perform the ROI encoding on the first ROI and use QpOffset2 on the second ROI.
     * Note that the number of ROIs that can be applied simultaneously does not exceed six, and the total area must
     * not exceed one-fifth of the total image area.
     *
     * @since 22
     */
    OH_REGION_OF_INTEREST_METADATA
} OH_NativeBuffer_MetadataKey;

/**
 * @brief Indicates the format of a native buffer.
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeBuffer
 * @since 10
 * @version 1.0
 */
typedef enum OH_NativeBuffer_Format {
    /**
     * CLUT8 format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_CLUT8 = 0,
    /**
     * CLUT1 format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_CLUT1,
    /**
     * CLUT4 format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_CLUT4,
    NATIVEBUFFER_PIXEL_FMT_RGB_565 = 3,               /// < RGB565 format */
    NATIVEBUFFER_PIXEL_FMT_RGBA_5658,                 /// < RGBA5658 format */
    NATIVEBUFFER_PIXEL_FMT_RGBX_4444,                 /// < RGBX4444 format */
    NATIVEBUFFER_PIXEL_FMT_RGBA_4444,                 /// < RGBA4444 format */
    NATIVEBUFFER_PIXEL_FMT_RGB_444,                   /// < RGB444 format */
    NATIVEBUFFER_PIXEL_FMT_RGBX_5551,                 /// < RGBX5551 format */
    NATIVEBUFFER_PIXEL_FMT_RGBA_5551,                 /// < RGBA5551 format */
    NATIVEBUFFER_PIXEL_FMT_RGB_555,                   /// < RGB555 format */
    NATIVEBUFFER_PIXEL_FMT_RGBX_8888,                 /// < RGBX8888 format */
    NATIVEBUFFER_PIXEL_FMT_RGBA_8888,                 /// < RGBA8888 format */
    NATIVEBUFFER_PIXEL_FMT_RGB_888,                   /// < RGB888 format */
    NATIVEBUFFER_PIXEL_FMT_BGR_565,                   /// < BGR565 format */
    NATIVEBUFFER_PIXEL_FMT_BGRX_4444,                 /// < BGRX4444 format */
    NATIVEBUFFER_PIXEL_FMT_BGRA_4444,                 /// < BGRA4444 format */
    NATIVEBUFFER_PIXEL_FMT_BGRX_5551,                 /// < BGRX5551 format */
    NATIVEBUFFER_PIXEL_FMT_BGRA_5551,                 /// < BGRA5551 format */
    NATIVEBUFFER_PIXEL_FMT_BGRX_8888,                 /// < BGRX8888 format */
    NATIVEBUFFER_PIXEL_FMT_BGRA_8888,                 /// < BGRA8888 format */
    /**
     * YUV422 interleaved format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_YUV_422_I,
    /**
     * YCBCR422 semi-plannar format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_YCBCR_422_SP,
    /**
     * YCRCB422 semi-plannar format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_YCRCB_422_SP,
    /**
     * YCBCR420 semi-plannar format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP,
    /**
     * YCRCB420 semi-plannar format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP,
    /**
     * YCBCR422 plannar format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_YCBCR_422_P,
    /**
     * YCRCB422 plannar format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_YCRCB_422_P,
    /**
     * YCBCR420 plannar format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_YCBCR_420_P,
    /**
     * YCRCB420 plannar format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_YCRCB_420_P,
    /**
     * YUYV422 packed format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_YUYV_422_PKG,
    /**
     * UYVY422 packed format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_UYVY_422_PKG,
    /**
     * YVYU422 packed format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_YVYU_422_PKG,
    /**
     * VYUY422 packed format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_VYUY_422_PKG,
    /**
     * RGBA_1010102 packed format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_RGBA_1010102,
    /**
     * YCBCR420 semi-planar 10bit packed format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_YCBCR_P010,
    /**
     * YCRCB420 semi-planar 10bit packed format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_YCRCB_P010,
    /**
     * Raw 10bit packed format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_RAW10,
    /**
     * BLOB format
     * @since 15
     */
    NATIVEBUFFER_PIXEL_FMT_BLOB,
    /**
     * RGBA16 float format
     * @since 15
     */
    NATIVEBUFFER_PIXEL_FMT_RGBA16_FLOAT,
    /**
     * Y8 format
     * @since 20
     */
    NATIVEBUFFER_PIXEL_FMT_Y8 = 40,
    /**
     * Y16 format
     * @since 20
     */
    NATIVEBUFFER_PIXEL_FMT_Y16 = 41,
    /**
     * vender mask format
     * @since 12
     */
    NATIVEBUFFER_PIXEL_FMT_VENDER_MASK = 0X7FFF0000,
    NATIVEBUFFER_PIXEL_FMT_BUTT = 0X7FFFFFFF          /// < Invalid pixel format */
} OH_NativeBuffer_Format;

/**
 * @brief Indicates the transform type of a native buffer.
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeBuffer
 * @since 12
 * @version 1.0
 */
typedef enum OH_NativeBuffer_TransformType {
    NATIVEBUFFER_ROTATE_NONE = 0,         /**< No rotation */
    NATIVEBUFFER_ROTATE_90,               /**< Rotation by 90 degrees */
    NATIVEBUFFER_ROTATE_180,              /**< Rotation by 180 degrees */
    NATIVEBUFFER_ROTATE_270,              /**< Rotation by 270 degrees */
    NATIVEBUFFER_FLIP_H,                  /**< Flip horizontally */
    NATIVEBUFFER_FLIP_V,                  /**< Flip vertically */
    NATIVEBUFFER_FLIP_H_ROT90,            /**< Flip horizontally and rotate 90 degrees */
    NATIVEBUFFER_FLIP_V_ROT90,            /**< Flip vertically and rotate 90 degrees */
    NATIVEBUFFER_FLIP_H_ROT180,           /**< Flip horizontally and rotate 180 degrees */
    NATIVEBUFFER_FLIP_V_ROT180,           /**< Flip vertically and rotate 180 degrees */
    NATIVEBUFFER_FLIP_H_ROT270,           /**< Flip horizontally and rotate 270 degrees */
    NATIVEBUFFER_FLIP_V_ROT270,           /**< Flip vertically and rotate 270 degrees */
} OH_NativeBuffer_TransformType;

#ifdef __cplusplus
}
#endif

/** @} */
#endif