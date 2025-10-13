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
#include <gtest/gtest.h>
#include "iconsumer_surface.h"
#include <iservice_registry.h>
#include <ctime>
#include "native_buffer.h"
#include "native_buffer_inner.h"
#include "native_window.h"
#include "surface_type.h"
#include "graphic_common_c.h"

using namespace std;
using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class BufferConsumerListener : public IBufferConsumerListener {
public:
    void OnBufferAvailable() override
    {
    }
};

class NativeBufferTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

    static inline OH_NativeBuffer_Config config = {
        .width = 0x100,
        .height = 0x100,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
    };
    static inline OH_NativeBuffer_Config checkConfig = {};
    static inline OH_NativeBuffer* buffer = nullptr;
    static inline int pipeWriteFd_;
};

void NativeBufferTest::SetUpTestCase()
{
    buffer = nullptr;
}

void NativeBufferTest::TearDownTestCase()
{
    buffer = nullptr;
}

/*
* Function: OH_NativeBufferFromSurfaceBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBufferFromSurfaceBuffer by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OH_NativeBufferFromSurfaceBuffer001, TestSize.Level0)
{
    sptr<OHOS::SurfaceBuffer> surfaceBuffer = OHOS::SurfaceBuffer::Create();
    NativeWindowBuffer* nativeWindowBuffer = OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer(&surfaceBuffer);
    ASSERT_NE(nativeWindowBuffer, nullptr);
    nativeWindowBuffer->sfbuffer = nullptr;
    OH_NativeBuffer* nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(nativeWindowBuffer);
    ASSERT_EQ(nativeBuffer, nullptr);
}

/*
* Function: OH_NativeBuffer_Alloc
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_Alloc by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferAlloc001, TestSize.Level0)
{
    buffer = OH_NativeBuffer_Alloc(nullptr);
    ASSERT_EQ(buffer, nullptr);
}

/*
* Function: OH_NativeBuffer_Alloc
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_Alloc
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferAlloc002, TestSize.Level0)
{
    buffer = OH_NativeBuffer_Alloc(&config);
    ASSERT_NE(buffer, nullptr);
}

/*
* Function: OH_NativeBuffer_GetSeqNum
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_GetSeqNum by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferGetSeqNum001, TestSize.Level0)
{
    uint32_t id = OH_NativeBuffer_GetSeqNum(nullptr);
    ASSERT_EQ(id, UINT_MAX);
}

/*
* Function: OH_NativeBuffer_GetSeqNum
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_GetSeqNum
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferGetSeqNum002, TestSize.Level0)
{
    uint32_t id = OH_NativeBuffer_GetSeqNum(buffer);
    ASSERT_NE(id, GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeBuffer_GetConfig
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_GetConfig
*                  2. check ret
 */
HWTEST_F(NativeBufferTest, OHNativeBufferGetConfig001, TestSize.Level0)
{
    OH_NativeBuffer_GetConfig(buffer, &checkConfig);
    ASSERT_NE(&checkConfig, nullptr);
}

/*
* Function: OH_NativeBuffer_GetConfig
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_GetConfig by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferGetConfig002, TestSize.Level0)
{
    checkConfig.width = 0x0;
    checkConfig.height = 0x0;
    checkConfig.format = 0x0;
    checkConfig.usage = 0x0;
    OH_NativeBuffer_GetConfig(nullptr, &checkConfig);
    ASSERT_EQ(checkConfig.width, 0x0);
    ASSERT_EQ(checkConfig.height, 0x0);
    ASSERT_EQ(checkConfig.format, 0x0);
    ASSERT_EQ(checkConfig.usage, 0x0);
}

/*
* Function: OH_NativeBuffer_Reference
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_Reference by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferReference001, TestSize.Level0)
{
    int32_t ret = OH_NativeBuffer_Reference(nullptr);
    ASSERT_NE(ret, GSERROR_OK);
}

/*
* Function: OH_NativeBuffer_Reference
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_Reference
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferReference002, TestSize.Level0)
{
    int32_t ret = OH_NativeBuffer_Reference(buffer);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: OH_NativeBuffer_Unreference
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_Unreference by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferUnreference001, TestSize.Level0)
{
    int32_t ret = OH_NativeBuffer_Unreference(nullptr);
    ASSERT_NE(ret, GSERROR_OK);
}

/*
* Function: OH_NativeBuffer_Unreference
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_Unreference
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferUnreference002, TestSize.Level0)
{
    int32_t ret = OH_NativeBuffer_Unreference(buffer);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: OH_NativeBuffer_GetSeqNum and OH_NativeBuffer_Unreference
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_GetSeqNum
*                  2. call OH_NativeBuffer_Unreference
*                  3. OH_NativeBuffer_Alloc again
*                  4. check OH_NativeBuffer_GetSeqNum = oldSeq + 1
*/
HWTEST_F(NativeBufferTest, OHNativeBufferGetSeqNum003, TestSize.Level0)
{
    uint32_t oldSeq = OH_NativeBuffer_GetSeqNum(buffer);
    int32_t ret = OH_NativeBuffer_Unreference(buffer);
    ASSERT_EQ(ret, GSERROR_OK);
    buffer = OH_NativeBuffer_Alloc(&config);
    ASSERT_EQ(oldSeq + 1, OH_NativeBuffer_GetSeqNum(buffer));
}

/*
* Function: OH_NativeBuffer_GetNativeBufferConfig
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_GetNativeBufferConfig
*                  2. check result
*/
HWTEST_F(NativeBufferTest, OHNativeBufferGetNativeBufferConfig001, TestSize.Level0)
{
    OH_NativeBuffer_Config testConfig = {};
    OH_NativeBuffer_GetNativeBufferConfig(buffer, &testConfig);
    ASSERT_EQ(testConfig.width, config.width);
    ASSERT_EQ(testConfig.height, config.height);
    ASSERT_EQ(testConfig.format, config.format);
    ASSERT_EQ(testConfig.usage, config.usage);
}

/*
* Function: OH_NativeBuffer_SetColorSpace
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_SetColorSpace by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferSetColorSpace001, TestSize.Level0)
{
    int32_t ret = OH_NativeBuffer_SetColorSpace(nullptr, OH_COLORSPACE_DISPLAY_BT2020_PQ);
    ASSERT_NE(ret, GSERROR_OK);
}

/*
* Function: OH_NativeBuffer_SetColorSpace
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_SetColorSpace
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferSetColorSpace002, TestSize.Level0)
{
    if (buffer == nullptr) {
        buffer = OH_NativeBuffer_Alloc(&config);
        ASSERT_NE(buffer, nullptr);
    }

    int32_t ret = OH_NativeBuffer_SetColorSpace(buffer, OH_COLORSPACE_BT709_LIMIT);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
}

/*
* Function: OH_NativeBuffer_GetColorSpace
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_GetColorSpace by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferGetColorSpace001, TestSize.Level0)
{
    OH_NativeBuffer_ColorSpace *colorSpace = nullptr;
    int32_t ret = OH_NativeBuffer_GetColorSpace(nullptr, colorSpace);
    ASSERT_NE(ret, GSERROR_OK);
}

/*
* Function: OH_NativeBuffer_GetColorSpace
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_GetColorSpace
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferGetColorSpace002, TestSize.Level0)
{
    if (buffer == nullptr) {
        buffer = OH_NativeBuffer_Alloc(&config);
        ASSERT_NE(buffer, nullptr);
    }
    OH_NativeBuffer_ColorSpace colorSpace = OH_COLORSPACE_NONE;
    int32_t ret = OH_NativeBuffer_SetColorSpace(buffer, OH_COLORSPACE_BT709_LIMIT);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeBuffer_GetColorSpace(buffer, &colorSpace);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(colorSpace, OH_COLORSPACE_BT709_LIMIT);
        ASSERT_EQ(ret, GSERROR_OK);
    }
}

/*
* Function: OH_NativeBuffer_GetColorSpace
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_GetColorSpace
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferGetColorSpace003, TestSize.Level0)
{
    if (buffer == nullptr) {
        buffer = OH_NativeBuffer_Alloc(&config);
        ASSERT_NE(buffer, nullptr);
    }
    OH_NativeBuffer_ColorSpace colorSpace = OH_COLORSPACE_NONE;
    int32_t ret = OH_NativeBuffer_GetColorSpace(buffer, &colorSpace);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeBuffer_SetColorSpace(buffer, OH_COLORSPACE_BT709_LIMIT);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(colorSpace, OH_COLORSPACE_BT709_LIMIT);
        ASSERT_EQ(ret, GSERROR_OK);
    }
}

/*
* Function: OH_NativeBuffer_SetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_SetMetadataValue by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OH_NativeBuffer_SetMetadataValue001, TestSize.Level0)
{
    int32_t size = 1024;
    uint8_t buff[size];
    int32_t ret = OH_NativeBuffer_SetMetadataValue(nullptr, OH_HDR_STATIC_METADATA, size, buff);
    ASSERT_NE(ret, GSERROR_OK);
}

/*
* Function: OH_NativeBuffer_SetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_SetMetadataValue
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OH_NativeBuffer_SetMetadataValue002, TestSize.Level0)
{
    if (buffer == nullptr) {
        buffer = OH_NativeBuffer_Alloc(&config);
        ASSERT_NE(buffer, nullptr);
    }
    int32_t len = 60;
    uint8_t outbuff[len];
    for (int i = 0; i < 60; ++i) {
        outbuff[i] = static_cast<uint8_t>(i);
    }
    int32_t ret = OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_STATIC_METADATA, len, outbuff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_DYNAMIC_METADATA, len, outbuff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    uint8_t type = OH_NativeBuffer_MetadataType::OH_VIDEO_HDR_HLG;
    ret = OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_METADATA_TYPE, sizeof(OH_NativeBuffer_MetadataType), &type);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    type = OH_NativeBuffer_MetadataType::OH_VIDEO_NONE;
    ret = OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_METADATA_TYPE, sizeof(OH_NativeBuffer_MetadataType), &type);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_INVALID_ARGUMENTS);
    }
    ret = OH_NativeBuffer_SetMetadataValue(buffer, static_cast<OH_NativeBuffer_MetadataKey>(-1), len, outbuff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_INTERNAL);
    }
    type = 4;
    ret = OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_METADATA_TYPE, sizeof(OH_NativeBuffer_MetadataType), &type);
    if (ret != GSERROR_NOT_SUPPORT) {
        ASSERT_EQ(ret, GSERROR_OK);
    }
}

/*
* Function: OH_NativeBuffer_SetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_SetMetadataValue by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OH_NativeBuffer_SetMetadataValue003, TestSize.Level0)
{
    int32_t max_size = -1;
    int32_t size = 60;
    uint8_t buff[size];
    int32_t ret = OH_NativeBuffer_SetMetadataValue(nullptr, OH_HDR_STATIC_METADATA, max_size, buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_NE(ret, GSERROR_OK);
    }
}

/*
* Function: OH_NativeBuffer_SetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_SetMetadataValue
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OH_NativeBuffer_SetMetadataValue004, TestSize.Level0)
{
    if (buffer == nullptr) {
        buffer = OH_NativeBuffer_Alloc(&config);
        ASSERT_NE(buffer, nullptr);
    }
    int32_t len = 60;
    uint8_t outbuff[len];
    for (int i = 0; i < 60; ++i) {
        outbuff[i] = static_cast<uint8_t>(i);
    }
    int32_t ret = OH_NativeBuffer_SetMetadataValue(buffer, OH_REGION_OF_INTEREST_METADATA, len, outbuff);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: OH_NativeBuffer_SetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_SetMetadataValue
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OH_NativeBuffer_SetMetadataValue004, TestSize.Level0)
{
    if (buffer == nullptr) {
        buffer = OH_NativeBuffer_Alloc(&config);
        ASSERT_NE(buffer, nullptr);
    }
    int32_t len = 60;
    uint8_t outbuff[len];
    for (int i = 0; i < 60; ++i) {
        outbuff[i] = static_cast<uint8_t>(i);
    }
    uint8_t type = OH_NativeBuffer_MetadataType::OH_IMAGE_HDR_VIVID_DUAL;
    int32_t ret = OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_METADATA_TYPE,
                                                   sizeof(OH_NativeBuffer_MetadataType), &type);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    type = OH_NativeBuffer_MetadataType::OH_IMAGE_HDR_VIVID_SINGLE;
    ret = OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_METADATA_TYPE, sizeof(OH_NativeBuffer_MetadataType), &type);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    type = OH_NativeBuffer_MetadataType::OH_VIDEO_NONE;
    ret = OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_METADATA_TYPE, sizeof(OH_NativeBuffer_MetadataType), &type);
    ASSERT_EQ(ret, SURFACE_ERROR_INVALID_PARAM);
}

/*
* Function: OH_NativeBuffer_GetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_GetMetadataValue by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OH_NativeBuffer_GetMetadataValue001, TestSize.Level0)
{
    int32_t size = 1024;
    uint8_t *buff;
    int32_t ret = OH_NativeBuffer_GetMetadataValue(nullptr, OH_HDR_STATIC_METADATA, &size, &buff);
    if (buff != nullptr) {
        delete[] buff;
        buff = nullptr;
    }
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_NE(ret, GSERROR_OK);
    }
}

/*
* Function: OH_NativeBuffer_GetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_GetMetadataValue
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OH_NativeBuffer_GetMetadataValue002, TestSize.Level0)
{
    if (buffer == nullptr) {
        buffer = OH_NativeBuffer_Alloc(&config);
        ASSERT_NE(buffer, nullptr);
    }
    int32_t len = 60;
    uint8_t outbuff[len];
    for (int i = 0; i < 60; ++i) {
        outbuff[i] = static_cast<uint8_t>(i);
    }
    int32_t ret = OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_STATIC_METADATA, len, outbuff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    int32_t buffSize = 0;
    uint8_t *buff;
    ret = OH_NativeBuffer_GetMetadataValue(buffer, OH_HDR_STATIC_METADATA, &buffSize, &buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        if (buff != nullptr) {
            ASSERT_EQ(memcmp(outbuff, buff, 60), 0);
            delete[] buff;
            buff = nullptr;
        }
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_DYNAMIC_METADATA, len, outbuff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeBuffer_GetMetadataValue(buffer, OH_HDR_DYNAMIC_METADATA, &buffSize, &buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        if (buff != nullptr) {
            ASSERT_EQ(memcmp(outbuff, buff, 60), 0);
            delete[] buff;
            buff = nullptr;
        }
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeBuffer_GetMetadataValue(buffer, static_cast<OH_NativeBuffer_MetadataKey>(-1), &buffSize, &buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_INTERNAL);
    }
}

/*
* Function: OH_NativeBuffer_GetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_GetMetadataValue
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OH_NativeBuffer_GetMetadataValue003, TestSize.Level0)
{
    if (buffer == nullptr) {
        buffer = OH_NativeBuffer_Alloc(&config);
        ASSERT_NE(buffer, nullptr);
    }
    uint8_t *buff;
    int32_t ret = OH_NativeBuffer_GetMetadataValue(buffer, OH_HDR_STATIC_METADATA, nullptr, &buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_NE(ret, GSERROR_OK);
    }
    uint8_t type = OH_NativeBuffer_MetadataType::OH_VIDEO_HDR_HLG;
    uint8_t *resType = new uint8_t;
    int32_t typeSize = sizeof(OH_NativeBuffer_MetadataType);
    ret = OH_NativeBuffer_SetMetadataValue(buffer, OH_HDR_METADATA_TYPE, typeSize, &type);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeBuffer_GetMetadataValue(buffer, OH_HDR_METADATA_TYPE, &typeSize, &resType);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(type, *resType);
        ASSERT_EQ(ret, GSERROR_OK);
    }
    delete resType;
    resType = nullptr;
}

/*
* Function: OH_NativeBuffer_Map
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_Map by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferMap001, TestSize.Level0)
{
    void *virAddr = nullptr;
    int32_t ret = OH_NativeBuffer_Map(nullptr, &virAddr);
    ASSERT_NE(ret, GSERROR_OK);
}

/*
* Function: OH_NativeBuffer_Map
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_Map
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferMap002, TestSize.Level0)
{
    void *virAddr = nullptr;
    int32_t ret = OH_NativeBuffer_Map(buffer, &virAddr);
    ASSERT_EQ(ret, GSERROR_OK);
    ASSERT_NE(virAddr, nullptr);
}

/*
 * Function: OHNativeBufferMapWaitFence001
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call OH_NativeBuffer_MapWaitFence by abnormal virAddr input
 *                  2. check ret
 */
HWTEST_F(NativeBufferTest, OHNativeBufferMapWaitFence001, TestSize.Level0)
{
    OH_NativeBuffer* nativeBuffer = OH_NativeBuffer_Alloc(&config);
    ASSERT_NE(nativeBuffer, nullptr);

    int32_t fenceFd = 1;
    int32_t ret = OH_NativeBuffer_MapWaitFence(nativeBuffer, fenceFd, nullptr);
    ASSERT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);
    EXPECT_EQ(OH_NativeBuffer_Unreference(nativeBuffer), OHOS::GSERROR_OK);
}

/*
 * Function: OHNativeBufferMapWaitFence002
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call OH_NativeBuffer_MapWaitFence by abnormal buffer input
 *                  2. check ret
 */
HWTEST_F(NativeBufferTest, OHNativeBufferMapWaitFence002, TestSize.Level0)
{
    void *virAddr = nullptr;
    int32_t fenceFd = -1;
    int32_t ret = OH_NativeBuffer_MapWaitFence(nullptr, fenceFd, &virAddr);
    ASSERT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(virAddr, nullptr);
}

/*
 * Function: OHNativeBufferMapWaitFence003
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call OH_NativeBuffer_MapWaitFence by valid buffer input
 *                  2. wait for pipe fence with delayed signaling
 *                  3. check ret and virAddr, ret is ok and virAddr is non null pointer
 */
TEST_F(NativeBufferTest, OHNativeBufferMapWaitFence003)
{
    OH_NativeBuffer* nativeBuffer = OH_NativeBuffer_Alloc(&config);
    ASSERT_NE(nativeBuffer, nullptr);

    int pipefds[2];
    ASSERT_NE(pipe(pipefds), -1);
        
    // Return read end, keep write end for signaling
    int fenceFd = pipefds[0];
    ASSERT_GE(fenceFd, 0);

    pipeWriteFd_ = pipefds[1];
    int writeFdCopy = pipeWriteFd_;
    // Start a thread to signal the fence after a delay
    std::thread signalThread([writeFdCopy]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // delay for 100ms
        char signal = '1';
        write(writeFdCopy, &signal, 1);
        close(writeFdCopy);
    });

    void *virAddr = nullptr;
    auto start = std::chrono::steady_clock::now();
    int32_t result = OH_NativeBuffer_MapWaitFence(nativeBuffer, fenceFd, &virAddr);
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_EQ(result, OHOS::SURFACE_ERROR_OK);
    EXPECT_GE(duration.count(), 100); // Should have waited at least 100ms
    EXPECT_LE(duration.count(), 200); // Should not wait longer than 200ms
    EXPECT_NE(virAddr, nullptr);
    
    signalThread.join();
    close(fenceFd);
    EXPECT_EQ(OH_NativeBuffer_Unmap(nativeBuffer), OHOS::GSERROR_OK);
    EXPECT_EQ(OH_NativeBuffer_Unreference(nativeBuffer), OHOS::GSERROR_OK);
}

/*
 * Function: OHNativeBufferMapWaitFence004
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call OH_NativeBuffer_MapWaitFence by abnormal buffer input
 *                  2. check ret
 */
HWTEST_F(NativeBufferTest, OHNativeBufferMapWaitFence004, TestSize.Level0)
{
    sptr<OHOS::SurfaceBuffer> sBuffer = SurfaceBuffer::Create();
    OH_NativeBuffer* nativeBuffer = sBuffer->SurfaceBufferToNativeBuffer();
    void *virAddr = nullptr;
    int32_t fenceFd = 1;
    int32_t ret = OH_NativeBuffer_MapWaitFence(nativeBuffer, fenceFd, &virAddr);
    ASSERT_EQ(ret, OHOS::SURFACE_ERROR_UNKOWN);
    ASSERT_EQ(virAddr, nullptr);
    delete sBuffer;
    sBuffer = nullptr;
}

/*
 * Function: OHNativeBufferMapWaitFence005
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call OH_NativeBuffer_MapWaitFence by abnormal fenceFd input
 *                  2. check ret
 */
HWTEST_F(NativeBufferTest, OHNativeBufferMapWaitFence005, TestSize.Level0)
{
    OH_NativeBuffer* nativeBuffer = OH_NativeBuffer_Alloc(&config);
    ASSERT_NE(nativeBuffer, nullptr);

    int32_t fenceFd = -1;
    void *virAddr = nullptr;
    int32_t ret = OH_NativeBuffer_MapWaitFence(nativeBuffer, fenceFd, &virAddr);
    ASSERT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);
    EXPECT_EQ(OH_NativeBuffer_Unreference(nativeBuffer), OHOS::GSERROR_OK);
}

/*
* Function: OH_NativeBuffer_Unmap
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_Unmap by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferUnmap001, TestSize.Level0)
{
    int32_t ret = OH_NativeBuffer_Unmap(nullptr);
    ASSERT_NE(ret, GSERROR_OK);
}

/*
* Function: OH_NativeBuffer_Unmap and OH_NativeBuffer_Unreference
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_Unmap
*                  2. check ret
*                  3. call OH_NativeBuffer_Unreference
*/
HWTEST_F(NativeBufferTest, OHNativeBufferUnmap002, TestSize.Level0)
{
    int32_t ret = OH_NativeBuffer_Unmap(buffer);
    ASSERT_EQ(ret, GSERROR_OK);
    ret = OH_NativeBuffer_Unreference(buffer);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
* Function: OH_NativeBufferFromNativeWindowBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBufferFromNativeWindowBuffer by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, NativeBufferFromNativeWindowBuffer001, TestSize.Level0)
{
    OH_NativeBuffer* nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(nullptr);
    ASSERT_EQ(nativeBuffer, nullptr);
}

/*
* Function: OH_NativeBufferFromNativeWindowBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBufferFromNativeWindowBuffer
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, NativeBufferFromNativeWindowBuffer002, TestSize.Level0)
{
    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    cSurface->RegisterConsumerListener(listener);
    sptr<OHOS::IBufferProducer> producer = cSurface->GetProducer();
    sptr<OHOS::Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
    int32_t fence;
    sptr<OHOS::SurfaceBuffer> sBuffer = nullptr;
    BufferRequestConfig requestConfig = {
        .width = 0x100,  // small
        .height = 0x100, // small
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    pSurface->RequestBuffer(sBuffer, fence, requestConfig);
    NativeWindow* nativeWindow = OH_NativeWindow_CreateNativeWindow(&pSurface);
    ASSERT_NE(nativeWindow, nullptr);
    NativeWindowBuffer* nativeWindowBuffer = OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer(&sBuffer);
    ASSERT_NE(nativeWindowBuffer, nullptr);
    OH_NativeBuffer* nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(nativeWindowBuffer);
    ASSERT_NE(nativeBuffer, nullptr);

    int32_t ret = OH_NativeBuffer_FromNativeWindowBuffer(nativeWindowBuffer, nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    OH_NativeBuffer* nativeBufferTmp = nullptr;
    ret = OH_NativeBuffer_FromNativeWindowBuffer(nativeWindowBuffer, &nativeBufferTmp);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(nativeBuffer, nativeBufferTmp);

    void *virAddr = nullptr;
    OH_NativeBuffer_Planes outPlanes;
    ret = OH_NativeBuffer_MapPlanes(nativeBuffer, &virAddr, &outPlanes);
    if (ret != GSERROR_HDI_ERROR) {
        ASSERT_EQ(ret, OHOS::GSERROR_OK);
    }

    sBuffer = nullptr;
    cSurface = nullptr;
    producer = nullptr;
    pSurface = nullptr;
    nativeWindow = nullptr;
    nativeWindowBuffer = nullptr;
}

/*
* Function: OH_NativeBuffer_FromNativeWindowBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_FromNativeWindowBuffer by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, NativeBufferFromNativeWindowBuffer003, TestSize.Level0)
{
    int32_t ret = OH_NativeBuffer_FromNativeWindowBuffer(nullptr, nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);

    NativeWindowBuffer nativeWindowBuffer;
    ret = OH_NativeBuffer_FromNativeWindowBuffer(&nativeWindowBuffer, nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);

    OH_NativeBuffer_GetNativeBufferConfig(nullptr, nullptr);
    OH_NativeBuffer_GetNativeBufferConfig(buffer, nullptr);
    ASSERT_EQ(OH_NativeBuffer_GetBufferHandle(nullptr), nullptr);
}

/*
* Function: OH_NativeBuffer_MapPlanes
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_MapPlanes by abnormal input
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferMapPlanes001, TestSize.Level0)
{
    int32_t ret = OH_NativeBuffer_MapPlanes(nullptr, nullptr, nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);

    OH_NativeBuffer *buffer = (OH_NativeBuffer *)0xFFFFFFFF;
    ret = OH_NativeBuffer_MapPlanes(buffer, nullptr, nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);

    void *virAddr = nullptr;
    ret = OH_NativeBuffer_MapPlanes(buffer, &virAddr, nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeBuffer_MapPlanes
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_MapPlanes
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferMapPlanes002, TestSize.Level0)
{
    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    cSurface->RegisterConsumerListener(listener);
    sptr<OHOS::IBufferProducer> producer = cSurface->GetProducer();
    sptr<OHOS::Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
    int32_t fence;
    sptr<OHOS::SurfaceBuffer> sBuffer = nullptr;
    BufferRequestConfig requestConfig = {.width = 0x100, .height = 0x100, .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_YCBCR_420_SP,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA, .timeout = 0};
    pSurface->SetQueueSize(4);
    int32_t formatType[] = {GRAPHIC_PIXEL_FMT_YCBCR_420_SP, GRAPHIC_PIXEL_FMT_YCRCB_420_SP,
        GRAPHIC_PIXEL_FMT_YCBCR_420_P, GRAPHIC_PIXEL_FMT_YCRCB_420_P};
    NativeWindow* nativeWindow;
    NativeWindowBuffer* nativeWindowBuffer;
    for (int32_t i = 0; i < sizeof(formatType) / sizeof(int32_t); i++) {
        requestConfig.format = formatType[i];
        pSurface->RequestBuffer(sBuffer, fence, requestConfig);
        nativeWindow = OH_NativeWindow_CreateNativeWindow(&pSurface);
        ASSERT_NE(nativeWindow, nullptr);
        nativeWindowBuffer = OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer(&sBuffer);
        ASSERT_NE(nativeWindowBuffer, nullptr);
        OH_NativeBuffer* nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(nativeWindowBuffer);
        ASSERT_NE(nativeBuffer, nullptr);

        void *virAddr = nullptr;
        OH_NativeBuffer_Planes outPlanes;
        int32_t ret = OH_NativeBuffer_MapPlanes(nativeBuffer, &virAddr, &outPlanes);
        if (ret != GSERROR_HDI_ERROR) {
            ASSERT_EQ(ret, OHOS::GSERROR_OK);
            ASSERT_NE(virAddr, nullptr);
        }
    }

    sBuffer = nullptr;
    cSurface = nullptr;
    producer = nullptr;
    pSurface = nullptr;
    nativeWindow = nullptr;
    nativeWindowBuffer = nullptr;
}

/*
* Function: OH_NativeBuffer_MapPlanes
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeBuffer_MapPlanes
*                  2. check ret
*/
HWTEST_F(NativeBufferTest, OHNativeBufferMapPlanes003, TestSize.Level0)
{
    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    cSurface->RegisterConsumerListener(listener);
    sptr<OHOS::IBufferProducer> producer = cSurface->GetProducer();
    sptr<OHOS::Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
    int32_t fence;
    sptr<OHOS::SurfaceBuffer> sBuffer = nullptr;
    BufferRequestConfig requestConfig = {.width = 0x100, .height = 0x100, .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_YCBCR_420_SP,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA, .timeout = 0};
    pSurface->SetQueueSize(4);
    int32_t formatType = GRAPHIC_PIXEL_FMT_YCBCR_420_SP;
    NativeWindow* nativeWindow;
    NativeWindowBuffer* nativeWindowBuffer;
    requestConfig.format = formatType;
    pSurface->RequestBuffer(sBuffer, fence, requestConfig);
    nativeWindow = OH_NativeWindow_CreateNativeWindow(&pSurface);
    ASSERT_NE(nativeWindow, nullptr);
    nativeWindowBuffer = OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer(&sBuffer);
    ASSERT_NE(nativeWindowBuffer, nullptr);
    OH_NativeBuffer* nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(nativeWindowBuffer);
    ASSERT_NE(nativeBuffer, nullptr);
    OH_NativeBuffer* nativeBufferTmp = nullptr;
    for (int32_t i = 0; i < 1000; i++) {
        int32_t ret = OH_NativeBuffer_FromNativeWindowBuffer(nativeWindowBuffer, &nativeBufferTmp);
        ASSERT_EQ(ret, OHOS::GSERROR_OK);
        ASSERT_EQ(nativeBuffer, nativeBufferTmp);
    }
    void *virAddr = nullptr;
    OH_NativeBuffer_Planes outPlanes;
    clock_t startTime, endTime;
    startTime = clock();
    for (int32_t i = 0; i < 1000; i++) {
        int32_t ret = OH_NativeBuffer_MapPlanes(nativeBuffer, &virAddr, &outPlanes);
        if (ret != GSERROR_HDI_ERROR) {
            ASSERT_EQ(ret, OHOS::GSERROR_OK);
            ASSERT_NE(virAddr, nullptr);
        }
    }
    endTime = clock();
    cout << "OH_NativeBuffer_MapPlanes 1000 times cost time: " << (endTime - startTime) << "ms" << endl;
    sBuffer = nullptr;
    cSurface = nullptr;
    producer = nullptr;
    pSurface = nullptr;
    nativeWindow = nullptr;
    nativeWindowBuffer = nullptr;
}
}