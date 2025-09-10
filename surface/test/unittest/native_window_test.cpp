/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include <native_window.h>
#include <securec.h>
#include <ctime>
#include "buffer_log.h"
#include "external_window.h"
#include "surface_utils.h"
#include "sync_fence.h"
#include "ipc_inner_object.h"
#include "ipc_cparcel.h"
#include "isurface_aps_plugin.h"

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

class ApsPluginMock : public ISurfaceApsPlugin {
private:
    uint32_t queueSize = 0;

public:
    uint32_t InitSurface(const std::string &surfaceName) override
    {
        return queueSize;
    }
};

static inline GSError OnBufferRelease(sptr<SurfaceBuffer> &buffer)
{
    return GSERROR_OK;
}

static OHExtDataHandle *AllocOHExtDataHandle(uint32_t reserveInts)
{
    size_t handleSize = sizeof(OHExtDataHandle) + (sizeof(int32_t) * reserveInts);
    OHExtDataHandle *handle = static_cast<OHExtDataHandle *>(malloc(handleSize));
    if (handle == nullptr) {
        BLOGE("AllocOHExtDataHandle malloc %zu failed", handleSize);
        return nullptr;
    }
    auto ret = memset_s(handle, handleSize, 0, handleSize);
    if (ret != EOK) {
        BLOGE("AllocOHExtDataHandle memset_s failed");
        free(handle);
        return nullptr;
    }
    handle->fd = -1;
    handle->reserveInts = reserveInts;
    for (uint32_t i = 0; i < reserveInts; i++) {
        handle->reserve[i] = -1;
    }
    return handle;
}

static void FreeOHExtDataHandle(OHExtDataHandle *handle)
{
    if (handle == nullptr) {
        BLOGW("FreeOHExtDataHandle with nullptr handle");
        return ;
    }
    if (handle->fd >= 0) {
        close(handle->fd);
        handle->fd = -1;
    }
    free(handle);
}

class NativeWindowTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

    static inline BufferRequestConfig requestConfig = {};
    static inline BufferFlushConfig flushConfig = {};
    static inline sptr<OHOS::IConsumerSurface> cSurface = nullptr;
    static inline sptr<OHOS::IBufferProducer> producer = nullptr;
    static inline sptr<OHOS::Surface> pSurface = nullptr;
    static inline sptr<OHOS::SurfaceBuffer> sBuffer = nullptr;
    static inline NativeWindow* nativeWindow = nullptr;
    static inline NativeWindowBuffer* nativeWindowBuffer = nullptr;
    static inline uint32_t firstSeqnum = 0;
};

void NativeWindowTest::SetUpTestCase()
{
    requestConfig = {
        .width = 0x100,  // small
        .height = 0x100, // small
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };

    cSurface = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    cSurface->RegisterConsumerListener(listener);
    producer = cSurface->GetProducer();
    pSurface = Surface::CreateSurfaceAsProducer(producer);
    int32_t fence;
    pSurface->RequestBuffer(sBuffer, fence, requestConfig);
    firstSeqnum = sBuffer->GetSeqNum();
}

void NativeWindowTest::TearDownTestCase()
{
    flushConfig = { .damage = {
        .w = 0x100,
        .h = 0x100,
    } };
    pSurface->FlushBuffer(sBuffer, -1, flushConfig);
    sBuffer = nullptr;
    cSurface = nullptr;
    producer = nullptr;
    pSurface = nullptr;
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);
    nativeWindow = nullptr;
    nativeWindowBuffer = nullptr;
}

/*
* Function: OH_NativeWindow_CreateNativeWindow
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_CreateNativeWindow by abnormal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, CreateNativeWindow001, TestSize.Level0)
{
    ASSERT_EQ(OH_NativeWindow_CreateNativeWindow(nullptr), nullptr);
}

/*
* Function: OH_NativeWindow_CreateNativeWindow
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_CreateNativeWindow
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, CreateNativeWindow002, TestSize.Level0)
{
    nativeWindow = OH_NativeWindow_CreateNativeWindow(&pSurface);
    ASSERT_NE(nativeWindow, nullptr);
}

/*
* Function: OH_NativeWindow_CreateNativeWindow
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_CreateNativeWindow
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, CreateNativeWindow003, TestSize.Level0)
{
    uint64_t surfaceId = 0;
    int32_t ret = OH_NativeWindow_GetSurfaceId(nativeWindow, &surfaceId);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(surfaceId, pSurface->GetUniqueId());
}

/*
* Function: OH_NativeWindow_CreateNativeWindow
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_CreateNativeWindow
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, CreateNativeWindow004, TestSize.Level0)
{
    sptr<OHOS::Surface> surfaceTmp = nullptr;
    auto nativeWindowTmp = OH_NativeWindow_CreateNativeWindow(&surfaceTmp);
    ASSERT_EQ(nativeWindowTmp, nullptr);
}

/*
* Function: OH_NativeWindow_CreateNativeWindowFromSurfaceId
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_CreateNativeWindowFromSurfaceId
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, CreateNativeWindowFromSurfaceId001, TestSize.Level0)
{
    uint64_t surfaceId = static_cast<uint64_t>(pSurface->GetUniqueId());
    OHNativeWindow *window = nullptr;
    int32_t ret = OH_NativeWindow_CreateNativeWindowFromSurfaceId(surfaceId, &window);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    surfaceId = 0;
    ret = OH_NativeWindow_GetSurfaceId(window, &surfaceId);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(surfaceId, pSurface->GetUniqueId());
    OH_NativeWindow_DestroyNativeWindow(window);
}

/*
* Function: OH_NativeWindow_CreateNativeWindowFromSurfaceId
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_CreateNativeWindowFromSurfaceId
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, CreateNativeWindowFromSurfaceId002, TestSize.Level0)
{
    int32_t ret = OH_NativeWindow_CreateNativeWindowFromSurfaceId(0, nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ret = OH_NativeWindow_GetSurfaceId(nullptr, nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_CreateNativeWindowFromSurfaceId
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_CreateNativeWindowFromSurfaceId
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, CreateNativeWindowFromSurfaceId003, TestSize.Level0)
{
    sptr<OHOS::IConsumerSurface> cSurfaceTmp = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    cSurfaceTmp->RegisterConsumerListener(listener);
    sptr<OHOS::IBufferProducer> producerTmp = cSurfaceTmp->GetProducer();
    sptr<OHOS::Surface> pSurfaceTmp = Surface::CreateSurfaceAsProducer(producerTmp);

    uint64_t surfaceId = static_cast<uint64_t>(pSurfaceTmp->GetUniqueId());
    auto utils = SurfaceUtils::GetInstance();
    utils->Add(surfaceId, pSurfaceTmp);
    OHNativeWindow *nativeWindowTmp = nullptr;
    int32_t ret = OH_NativeWindow_CreateNativeWindowFromSurfaceId(0xFFFFFFFF, &nativeWindowTmp);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
    ret = OH_NativeWindow_CreateNativeWindowFromSurfaceId(surfaceId, &nativeWindowTmp);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    surfaceId = 0;
    ret = OH_NativeWindow_GetSurfaceId(nativeWindowTmp, &surfaceId);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(surfaceId, pSurfaceTmp->GetUniqueId());

    cSurfaceTmp = nullptr;
    producerTmp = nullptr;
    pSurfaceTmp = nullptr;
    OH_NativeWindow_DestroyNativeWindow(nativeWindowTmp);
}

/*
* Function: OH_NativeWindow_NativeWindowHandleOpt
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowHandleOpt by abnormal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, HandleOpt001, TestSize.Level0)
{
    int code = SET_USAGE;
    uint64_t usage = BUFFER_USAGE_CPU_READ;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nullptr, code, usage), OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowHandleOpt
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowHandleOpt by different param
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, HandleOpt002, TestSize.Level0)
{
    int code = SET_USAGE;
    uint64_t usageSet = BUFFER_USAGE_CPU_READ;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, usageSet), OHOS::GSERROR_OK);

    code = GET_USAGE;
    uint64_t usageGet = BUFFER_USAGE_CPU_WRITE;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &usageGet), OHOS::GSERROR_OK);
    ASSERT_EQ(usageSet, usageGet);
}

/*
* Function: OH_NativeWindow_NativeWindowHandleOpt
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowHandleOpt by different param
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, HandleOpt003, TestSize.Level0)
{
    int code = SET_BUFFER_GEOMETRY;
    int32_t heightSet = 0x100;
    int32_t widthSet = 0x100;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, heightSet, widthSet), OHOS::GSERROR_OK);

    code = GET_BUFFER_GEOMETRY;
    int32_t heightGet = 0;
    int32_t widthGet = 0;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &heightGet, &widthGet), OHOS::GSERROR_OK);
    ASSERT_EQ(heightSet, heightGet);
    ASSERT_EQ(widthSet, widthGet);
}

/*
* Function: OH_NativeWindow_NativeWindowHandleOpt
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowHandleOpt by different param
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, HandleOpt004, TestSize.Level0)
{
    int code = SET_FORMAT;
    int32_t formatSet = GRAPHIC_PIXEL_FMT_RGBA_8888;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, formatSet), OHOS::GSERROR_OK);

    code = GET_FORMAT;
    int32_t formatGet = GRAPHIC_PIXEL_FMT_CLUT8;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &formatGet), OHOS::GSERROR_OK);
    ASSERT_EQ(formatSet, formatGet);
}

/*
* Function: OH_NativeWindow_NativeWindowHandleOpt
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowHandleOpt by different param
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, HandleOpt005, TestSize.Level0)
{
    int code = SET_STRIDE;
    int32_t strideSet = 0x8;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, strideSet), OHOS::GSERROR_OK);

    code = GET_STRIDE;
    int32_t strideGet = 0;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &strideGet), OHOS::GSERROR_OK);
    ASSERT_EQ(strideSet, strideGet);
}

/*
* Function: OH_NativeWindow_NativeWindowHandleOpt
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowHandleOpt by different param
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, HandleOpt006, TestSize.Level0)
{
    int code = SET_COLOR_GAMUT;
    int32_t colorGamutSet = static_cast<int32_t>(GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DCI_P3);
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, colorGamutSet), OHOS::GSERROR_OK);

    code = GET_COLOR_GAMUT;
    int32_t colorGamutGet = 0;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &colorGamutGet), OHOS::GSERROR_OK);
    ASSERT_EQ(colorGamutSet, colorGamutGet);
}

/*
* Function: OH_NativeWindow_NativeWindowHandleOpt
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowHandleOpt by different param
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, HandleOpt007, TestSize.Level0)
{
    int code = SET_TIMEOUT;
    int32_t timeoutSet = 10;  // 10: for test
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, timeoutSet), OHOS::GSERROR_OK);

    code = GET_TIMEOUT;
    int32_t timeoutGet = 0;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &timeoutGet), OHOS::GSERROR_OK);
    ASSERT_EQ(timeoutSet, timeoutGet);
}

/*
* Function: OH_NativeWindow_NativeWindowHandleOpt
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowHandleOpt by different param
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, HandleOpt008, TestSize.Level0)
{
    int code = GET_TRANSFORM;
    int32_t transform = 0;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &transform), OHOS::GSERROR_OK);
    transform = GraphicTransformType::GRAPHIC_ROTATE_90;
    code = SET_TRANSFORM;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, transform), OHOS::GSERROR_OK);
    int32_t transformTmp = 0;
    code = GET_TRANSFORM;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &transformTmp), OHOS::GSERROR_OK);
    ASSERT_EQ(transformTmp, GraphicTransformType::GRAPHIC_ROTATE_90);
    nativeWindow->surface->SetTransform(GraphicTransformType::GRAPHIC_ROTATE_180);
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &transformTmp), OHOS::GSERROR_OK);
    ASSERT_EQ(transformTmp, GraphicTransformType::GRAPHIC_ROTATE_180);
}

/*
* Function: OH_NativeWindow_NativeWindowHandleOpt
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowHandleOpt by different param
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, HandleOpt009, TestSize.Level0)
{
    int code = GET_BUFFERQUEUE_SIZE;
    int32_t queueSize = 0;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &queueSize), OHOS::GSERROR_OK);
    ASSERT_EQ(queueSize, 3);
    nativeWindow->surface->SetQueueSize(5);
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &queueSize), OHOS::GSERROR_OK);
    ASSERT_EQ(queueSize, 5);
}

/*
* Function: OH_NativeWindow_NativeWindowHandleOpt
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowHandleOpt by different param
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, HandleOpt010, TestSize.Level0)
{
    int code = SET_USAGE;
    uint64_t usageSet = NATIVEBUFFER_USAGE_HW_RENDER | NATIVEBUFFER_USAGE_HW_TEXTURE |
    NATIVEBUFFER_USAGE_CPU_READ_OFTEN | NATIVEBUFFER_USAGE_ALIGNMENT_512;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, usageSet), OHOS::GSERROR_OK);

    code = GET_USAGE;
    uint64_t usageGet = usageSet;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &usageGet), OHOS::GSERROR_OK);
    ASSERT_EQ(usageSet, usageGet);

    code = SET_FORMAT;
    int32_t formatSet = NATIVEBUFFER_PIXEL_FMT_YCBCR_P010;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, formatSet), OHOS::GSERROR_OK);

    code = GET_FORMAT;
    int32_t formatGet = NATIVEBUFFER_PIXEL_FMT_YCBCR_P010;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &formatGet), OHOS::GSERROR_OK);
    ASSERT_EQ(formatSet, formatGet);
}

/*
* Function: OH_NativeWindow_NativeWindowHandleOpt
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowHandleOpt by different param
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, HandleOpt011, TestSize.Level0)
{
    int code = SET_SOURCE_TYPE;
    OHSurfaceSource typeSet = OHSurfaceSource::OH_SURFACE_SOURCE_GAME;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, typeSet), OHOS::GSERROR_OK);

    code = GET_SOURCE_TYPE;
    OHSurfaceSource typeGet = OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &typeGet), OHOS::GSERROR_OK);
    ASSERT_EQ(typeSet, typeGet);
}

/*
* Function: OH_NativeWindow_NativeWindowHandleOpt
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowHandleOpt by different param
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, HandleOpt012, TestSize.Level0)
{
    int code = SET_APP_FRAMEWORK_TYPE;
    const char* typeSet = "test";
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, typeSet), OHOS::GSERROR_OK);

    code = GET_APP_FRAMEWORK_TYPE;
    const char* typeGet;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &typeGet), OHOS::GSERROR_OK);
    ASSERT_EQ(0, strcmp(typeSet, typeGet));
}

/*
* Function: OH_NativeWindow_NativeWindowHandleOpt
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowHandleOpt by different param
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, HandleOpt013, TestSize.Level0)
{
    int code = SET_HDR_WHITE_POINT_BRIGHTNESS;
    float brightness = 0.8;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, brightness), OHOS::GSERROR_OK);

    code = SET_SDR_WHITE_POINT_BRIGHTNESS;
    brightness = 0.5;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, brightness), OHOS::GSERROR_OK);

    ASSERT_EQ(fabs(cSurface->GetHdrWhitePointBrightness() - 0.8) < 1e-6, true);
    ASSERT_EQ(fabs(cSurface->GetSdrWhitePointBrightness() - 0.5) < 1e-6, true);

    code = SET_HDR_WHITE_POINT_BRIGHTNESS;
    brightness = 1.8;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, brightness), OHOS::GSERROR_OK);
    ASSERT_EQ(fabs(cSurface->GetHdrWhitePointBrightness() - 0.8) < 1e-6, true);
    brightness = -0.5;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, brightness), OHOS::GSERROR_OK);
    ASSERT_EQ(fabs(cSurface->GetHdrWhitePointBrightness() - 0.8) < 1e-6, true);
    brightness = 0.5;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, brightness), OHOS::GSERROR_OK);
    ASSERT_EQ(fabs(cSurface->GetHdrWhitePointBrightness() - 0.5) < 1e-6, true);

    code = SET_SDR_WHITE_POINT_BRIGHTNESS;
    brightness = 1.5;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, brightness), OHOS::GSERROR_OK);
    ASSERT_EQ(fabs(cSurface->GetSdrWhitePointBrightness() - 0.5) < 1e-6, true);
    brightness = -0.1;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, brightness), OHOS::GSERROR_OK);
    ASSERT_EQ(fabs(cSurface->GetSdrWhitePointBrightness() - 0.5) < 1e-6, true);
    brightness = 0.8;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, brightness), OHOS::GSERROR_OK);
    ASSERT_EQ(fabs(cSurface->GetSdrWhitePointBrightness() - 0.8) < 1e-6, true);
}

/*
* Function: OH_NativeWindow_NativeWindowHandleOpt
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowHandleOpt by different param
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, HandleOpt014, TestSize.Level0)
{
    int code = SET_APP_FRAMEWORK_TYPE;
    const char* typeSet = "testtesttesttesttesttesttesttesttesttesttesttesttesttesttesttest";
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, typeSet), OHOS::GSERROR_OK);

    code = GET_APP_FRAMEWORK_TYPE;
    const char* typeGet;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &typeGet), OHOS::GSERROR_OK);
    ASSERT_EQ(0, strcmp(typeSet, typeGet));
}

/*
* Function: OH_NativeWindow_NativeWindowAttachBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowAttachBuffer by abnormal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, NativeWindowAttachBuffer001, TestSize.Level0)
{
    ASSERT_EQ(OH_NativeWindow_NativeWindowAttachBuffer(nullptr, nullptr), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(OH_NativeWindow_NativeWindowDetachBuffer(nullptr, nullptr), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(OH_NativeWindow_NativeWindowAttachBuffer(nativeWindow, nullptr), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(OH_NativeWindow_NativeWindowDetachBuffer(nativeWindow, nullptr), OHOS::GSERROR_INVALID_ARGUMENTS);
}

void SetNativeWindowConfig(NativeWindow *nativeWindow)
{
    int code = SET_USAGE;
    uint64_t usageSet = BUFFER_USAGE_CPU_READ;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, usageSet), OHOS::GSERROR_OK);

    code = SET_BUFFER_GEOMETRY;
    int32_t heightSet = 0x100;
    int32_t widthSet = 0x100;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, heightSet, widthSet), OHOS::GSERROR_OK);

    code = SET_FORMAT;
    int32_t formatSet = GRAPHIC_PIXEL_FMT_RGBA_8888;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, formatSet), OHOS::GSERROR_OK);

    code = SET_STRIDE;
    int32_t strideSet = 0x8;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, strideSet), OHOS::GSERROR_OK);

    code = SET_COLOR_GAMUT;
    int32_t colorGamutSet = static_cast<int32_t>(GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DCI_P3);
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, colorGamutSet), OHOS::GSERROR_OK);

    code = SET_TIMEOUT;
    int32_t timeoutSet = 10;  // 10: for test
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, timeoutSet), OHOS::GSERROR_OK);
}

/*
* Function: OH_NativeWindow_NativeWindowAttachBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowAttachBuffer by normal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, NativeWindowAttachBuffer002, TestSize.Level0)
{
    sptr<OHOS::IConsumerSurface> cSurfaceTmp = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    cSurfaceTmp->RegisterConsumerListener(listener);
    sptr<OHOS::IBufferProducer> producerTmp = cSurfaceTmp->GetProducer();
    sptr<OHOS::Surface> pSurfaceTmp = Surface::CreateSurfaceAsProducer(producerTmp);

    NativeWindow *nativeWindowTmp = OH_NativeWindow_CreateNativeWindow(&pSurfaceTmp);
    ASSERT_NE(nativeWindowTmp, nullptr);
    SetNativeWindowConfig(nativeWindowTmp);

    NativeWindowBuffer *nativeWindowBuffer = nullptr;
    int fenceFd = -1;
    int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowTmp, &nativeWindowBuffer, &fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);

    int code = GET_BUFFERQUEUE_SIZE;
    int32_t queueSize = 0;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindowTmp, code, &queueSize), OHOS::GSERROR_OK);
    ASSERT_EQ(queueSize, 3);

    ASSERT_EQ(OH_NativeWindow_NativeWindowDetachBuffer(nativeWindowTmp, nativeWindowBuffer), OHOS::GSERROR_OK);

    ASSERT_EQ(OH_NativeWindow_NativeWindowAttachBuffer(nativeWindow, nativeWindowBuffer), OHOS::GSERROR_OK);

    ASSERT_EQ(OH_NativeWindow_NativeWindowDetachBuffer(nativeWindow, nativeWindowBuffer), OHOS::GSERROR_OK);

    ASSERT_EQ(OH_NativeWindow_NativeWindowAttachBuffer(nativeWindowTmp, nativeWindowBuffer), OHOS::GSERROR_OK);
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindowTmp, code, &queueSize), OHOS::GSERROR_OK);
    ASSERT_EQ(queueSize, 3);
    ASSERT_EQ(OH_NativeWindow_NativeWindowAttachBuffer(nativeWindowTmp, nativeWindowBuffer),
        OHOS::GSERROR_BUFFER_IS_INCACHE);

    struct Region *region = new Region();
    struct Region::Rect *rect = new Region::Rect();
    rect->x = 0x100;
    rect->y = 0x100;
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;
    ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindowTmp, nativeWindowBuffer, fenceFd, *region);
    ASSERT_EQ(ret, GSERROR_OK);

    OH_NativeWindow_DestroyNativeWindow(nativeWindowTmp);
}

void NativeWindowAttachBuffer003Test(NativeWindow *nativeWindowTmp, NativeWindow *nativeWindowTmp1)
{
    NativeWindowBuffer *nativeWindowBuffer1 = nullptr;
    int fenceFd = -1;
    int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowTmp, &nativeWindowBuffer1, &fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);

    NativeWindowBuffer *nativeWindowBuffer2 = nullptr;
    fenceFd = -1;
    ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowTmp, &nativeWindowBuffer2, &fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);

    NativeWindowBuffer *nativeWindowBuffer3 = nullptr;
    fenceFd = -1;
    ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowTmp, &nativeWindowBuffer3, &fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);

    int code = GET_BUFFERQUEUE_SIZE;
    int32_t queueSize = 0;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindowTmp, code, &queueSize), OHOS::GSERROR_OK);
    ASSERT_EQ(queueSize, 3);

    ASSERT_EQ(OH_NativeWindow_NativeWindowDetachBuffer(nativeWindowTmp, nativeWindowBuffer1), OHOS::GSERROR_OK);
    ASSERT_EQ(OH_NativeWindow_NativeWindowDetachBuffer(nativeWindowTmp, nativeWindowBuffer2), OHOS::GSERROR_OK);
    ASSERT_EQ(OH_NativeWindow_NativeWindowDetachBuffer(nativeWindowTmp, nativeWindowBuffer3), OHOS::GSERROR_OK);

    NativeWindowBuffer *nativeWindowBuffer4 = nullptr;
    fenceFd = -1;
    ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowTmp, &nativeWindowBuffer4, &fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);

    NativeWindowBuffer *nativeWindowBuffer10 = nullptr;
    fenceFd = -1;
    ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowTmp1, &nativeWindowBuffer10, &fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);

    NativeWindowBuffer *nativeWindowBuffer11 = nullptr;
    fenceFd = -1;
    ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowTmp1, &nativeWindowBuffer11, &fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);

    NativeWindowBuffer *nativeWindowBuffer12 = nullptr;
    fenceFd = -1;
    ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowTmp1, &nativeWindowBuffer12, &fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);

    ASSERT_EQ(OH_NativeWindow_NativeWindowAttachBuffer(nativeWindowTmp1, nativeWindowBuffer1),
        OHOS::SURFACE_ERROR_BUFFER_QUEUE_FULL);
}

/*
* Function: OH_NativeWindow_NativeWindowAttachBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowAttachBuffer by normal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, NativeWindowAttachBuffer003, TestSize.Level0)
{
    sptr<OHOS::IConsumerSurface> cSurfaceTmp = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    cSurfaceTmp->RegisterConsumerListener(listener);
    sptr<OHOS::IBufferProducer> producerTmp = cSurfaceTmp->GetProducer();
    sptr<OHOS::Surface> pSurfaceTmp = Surface::CreateSurfaceAsProducer(producerTmp);

    NativeWindow *nativeWindowTmp = OH_NativeWindow_CreateNativeWindow(&pSurfaceTmp);
    ASSERT_NE(nativeWindowTmp, nullptr);
    SetNativeWindowConfig(nativeWindowTmp);

    sptr<OHOS::IConsumerSurface> cSurfaceTmp1 = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener1 = new BufferConsumerListener();
    cSurfaceTmp1->RegisterConsumerListener(listener1);
    sptr<OHOS::IBufferProducer> producerTmp1 = cSurfaceTmp1->GetProducer();
    sptr<OHOS::Surface> pSurfaceTmp1 = Surface::CreateSurfaceAsProducer(producerTmp1);

    NativeWindow *nativeWindowTmp1 = OH_NativeWindow_CreateNativeWindow(&pSurfaceTmp1);
    ASSERT_NE(nativeWindowTmp1, nullptr);
    SetNativeWindowConfig(nativeWindowTmp1);

    NativeWindowAttachBuffer003Test(nativeWindowTmp, nativeWindowTmp1);

    OH_NativeWindow_DestroyNativeWindow(nativeWindowTmp);
    OH_NativeWindow_DestroyNativeWindow(nativeWindowTmp1);
}

/*
* Function: OH_NativeWindow_NativeWindowAttachBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowAttachBuffer by normal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, NativeWindowAttachBuffer004, TestSize.Level0)
{
    sptr<OHOS::IConsumerSurface> cSurfaceTmp = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    cSurfaceTmp->RegisterConsumerListener(listener);
    sptr<OHOS::IBufferProducer> producerTmp = cSurfaceTmp->GetProducer();
    sptr<OHOS::Surface> pSurfaceTmp = Surface::CreateSurfaceAsProducer(producerTmp);

    NativeWindow *nativeWindowTmp = OH_NativeWindow_CreateNativeWindow(&pSurfaceTmp);
    ASSERT_NE(nativeWindowTmp, nullptr);
    SetNativeWindowConfig(nativeWindowTmp);

    NativeWindowBuffer *nativeWindowBuffer = nullptr;
    int fenceFd = -1;
    int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowTmp, &nativeWindowBuffer, &fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);

    struct Region *region = new Region();
    struct Region::Rect *rect = new Region::Rect();
    rect->x = 0x100;
    rect->y = 0x100;
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;
    ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindowTmp, nativeWindowBuffer, fenceFd, *region);
    ASSERT_EQ(ret, GSERROR_OK);

    ASSERT_EQ(OH_NativeWindow_NativeWindowDetachBuffer(nativeWindowTmp, nativeWindowBuffer),
        OHOS::SURFACE_ERROR_BUFFER_STATE_INVALID);

    ASSERT_EQ(OH_NativeWindow_NativeWindowDetachBuffer(nativeWindow, nativeWindowBuffer),
        OHOS::SURFACE_ERROR_BUFFER_NOT_INCACHE);

    OH_NativeWindow_DestroyNativeWindow(nativeWindowTmp);
}

/*
* Function: OH_NativeWindow_NativeWindowAttachBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowAttachBuffer by normal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, NativeWindowAttachBuffer005, TestSize.Level0)
{
    sptr<OHOS::IConsumerSurface> cSurfaceTmp = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    cSurfaceTmp->RegisterConsumerListener(listener);
    sptr<OHOS::IBufferProducer> producerTmp = cSurfaceTmp->GetProducer();
    sptr<OHOS::Surface> pSurfaceTmp = Surface::CreateSurfaceAsProducer(producerTmp);

    NativeWindow *nativeWindowTmp = OH_NativeWindow_CreateNativeWindow(&pSurfaceTmp);
    ASSERT_NE(nativeWindowTmp, nullptr);
    SetNativeWindowConfig(nativeWindowTmp);

    NativeWindowBuffer *nativeWindowBuffer = nullptr;
    int fenceFd = -1;
    int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowTmp, &nativeWindowBuffer, &fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);

    ASSERT_EQ(cSurface->AttachBufferToQueue(nativeWindowBuffer->sfbuffer), GSERROR_OK);

    ASSERT_EQ(cSurface->DetachBufferFromQueue(nativeWindowBuffer->sfbuffer), GSERROR_OK);

    ASSERT_EQ(cSurface->AttachBufferToQueue(nativeWindowBuffer->sfbuffer), GSERROR_OK);

    sptr<SyncFence> fence = SyncFence::INVALID_FENCE;
    ASSERT_EQ(cSurface->ReleaseBuffer(nativeWindowBuffer->sfbuffer, fence), GSERROR_OK);

    OH_NativeWindow_DestroyNativeWindow(nativeWindowTmp);
}

/*
* Function: OH_NativeWindow_NativeWindowAttachBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowAttachBuffer by normal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, NativeWindowAttachBuffer006, TestSize.Level0)
{
    sptr<OHOS::IConsumerSurface> cSurfaceTmp = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    cSurfaceTmp->RegisterConsumerListener(listener);
    sptr<OHOS::IBufferProducer> producerTmp = cSurfaceTmp->GetProducer();
    sptr<OHOS::Surface> pSurfaceTmp = Surface::CreateSurfaceAsProducer(producerTmp);

    NativeWindow *nativeWindowTmp = OH_NativeWindow_CreateNativeWindow(&pSurfaceTmp);
    ASSERT_NE(nativeWindowTmp, nullptr);
    SetNativeWindowConfig(nativeWindowTmp);

    NativeWindowBuffer *nativeWindowBuffer1 = nullptr;
    int fenceFd = -1;
    int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowTmp, &nativeWindowBuffer1, &fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);

    int code = GET_BUFFERQUEUE_SIZE;
    int32_t queueSize = 0;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindowTmp, code, &queueSize), OHOS::GSERROR_OK);
    ASSERT_EQ(queueSize, 3);
    clock_t startTime, endTime;
    startTime = clock();
    for (int32_t i = 0; i < 1000; i++) {
        ASSERT_EQ(OH_NativeWindow_NativeWindowDetachBuffer(nativeWindowTmp, nativeWindowBuffer1), OHOS::GSERROR_OK);
        ASSERT_EQ(OH_NativeWindow_NativeWindowAttachBuffer(nativeWindowTmp, nativeWindowBuffer1), OHOS::GSERROR_OK);
    }
    endTime = clock();
    cout << "DetachBuffer and AttachBuffer 1000 times cost time: " << (endTime - startTime) << "ms" << endl;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindowTmp, code, &queueSize), OHOS::GSERROR_OK);
    ASSERT_EQ(queueSize, 3);
    OH_NativeWindow_DestroyNativeWindow(nativeWindowTmp);
}

/*
* Function: OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer by abnormal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, CreateNativeWindowBuffer001, TestSize.Level0)
{
    ASSERT_EQ(OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer(nullptr), nullptr);
}

/*
* Function: OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, CreateNativeWindowBuffer002, TestSize.Level0)
{
    nativeWindowBuffer = OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer(&sBuffer);
    ASSERT_NE(nativeWindowBuffer, nullptr);
}

/*
* Function: OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer
*                  2. check ret
*/
HWTEST_F(NativeWindowTest, CreateNativeWindowBuffer003, TestSize.Level0)
{
    OH_NativeBuffer* nativeBuffer = sBuffer->SurfaceBufferToNativeBuffer();
    ASSERT_NE(nativeBuffer, nullptr);
    NativeWindowBuffer* nwBuffer = OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(nativeBuffer);
    ASSERT_NE(nwBuffer, nullptr);
    OH_NativeWindow_DestroyNativeWindowBuffer(nwBuffer);
}

/*
* Function: OH_NativeWindow_NativeWindowRequestBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowRequestBuffer by abnormal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, RequestBuffer001, TestSize.Level0)
{
    ASSERT_EQ(OH_NativeWindow_NativeWindowRequestBuffer(nullptr, &nativeWindowBuffer, nullptr),
              OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowRequestBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowRequestBuffer by abnormal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, RequestBuffer002, TestSize.Level0)
{
    ASSERT_EQ(OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, nullptr, nullptr),
              OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_GetBufferHandleFromNative
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_GetBufferHandleFromNative by abnormal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, GetBufferHandle001, TestSize.Level0)
{
    ASSERT_EQ(OH_NativeWindow_GetBufferHandleFromNative(nullptr), nullptr);
}

/*
* Function: OH_NativeWindow_GetBufferHandleFromNative
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_GetBufferHandleFromNative
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, GetBufferHandle002, TestSize.Level0)
{
    struct NativeWindowBuffer *buffer = new NativeWindowBuffer();
    buffer->sfbuffer = sBuffer;
    ASSERT_NE(OH_NativeWindow_GetBufferHandleFromNative(nativeWindowBuffer), nullptr);
    delete buffer;
}

/*
* Function: OH_NativeWindow_NativeWindowFlushBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowFlushBuffer by abnormal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, FlushBuffer001, TestSize.Level0)
{
    int fenceFd = -1;
    struct Region *region = new Region();
    struct Region::Rect * rect = new Region::Rect();
    rect->x = 0x100;
    rect->y = 0x100;
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;

    ASSERT_EQ(OH_NativeWindow_NativeWindowFlushBuffer(nullptr, nullptr, fenceFd, *region),
              OHOS::GSERROR_INVALID_ARGUMENTS);
    delete region;
}

/*
* Function: OH_NativeWindow_NativeWindowFlushBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowFlushBuffer by abnormal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, FlushBuffer002, TestSize.Level0)
{
    int fenceFd = -1;
    struct Region *region = new Region();
    struct Region::Rect * rect = new Region::Rect();
    rect->x = 0x100;
    rect->y = 0x100;
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;

    ASSERT_EQ(OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, nullptr, fenceFd, *region),
              OHOS::GSERROR_INVALID_ARGUMENTS);
    delete region;
}

/*
* Function: OH_NativeWindow_NativeWindowFlushBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowFlushBuffer
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, FlushBuffer003, TestSize.Level0)
{
    int fenceFd = -1;
    struct Region *region = new Region();
    region->rectNumber = 0;
    region->rects = nullptr;
    ASSERT_EQ(OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, nativeWindowBuffer, fenceFd, *region),
              OHOS::GSERROR_OK);

    region->rectNumber = 1;
    struct Region::Rect * rect = new Region::Rect();
    rect->x = 0x100;
    rect->y = 0x100;
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;
    ASSERT_EQ(OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, nativeWindowBuffer, fenceFd, *region),
              OHOS::SURFACE_ERROR_BUFFER_STATE_INVALID);
    delete rect;
    delete region;
}
constexpr int32_t MATRIX_SIZE = 16;
bool CheckMatricIsSame(float matrixOld[MATRIX_SIZE], float matrixNew[MATRIX_SIZE])
{
    for (int32_t i = 0; i < MATRIX_SIZE; i++) {
        if (fabs(matrixOld[i] - matrixNew[i]) > 1e-6) {
            return false;
        }
    }
    return true;
}

/*
* Function: OH_NativeWindow_GetLastFlushedBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowRequestBuffer
*                  2. call OH_NativeWindow_NativeWindowFlushBuffer
*                  3. call OH_NativeWindow_GetLastFlushedBuffer
*                  4. check ret
 */
HWTEST_F(NativeWindowTest, GetLastFlushedBuffer001, TestSize.Level0)
{
    int code = SET_TRANSFORM;
    int32_t transform = GraphicTransformType::GRAPHIC_ROTATE_90;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, transform), OHOS::GSERROR_OK);

    code = SET_FORMAT;
    int32_t format = GRAPHIC_PIXEL_FMT_RGBA_8888;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, format), OHOS::GSERROR_OK);

    NativeWindowBuffer *nativeWindowBuffer = nullptr;
    int fenceFd = -1;
    int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &nativeWindowBuffer, &fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);

    struct Region *region = new Region();
    struct Region::Rect *rect = new Region::Rect();
    rect->x = 0x100;
    rect->y = 0x100;
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;
    BufferHandle *bufferHanlde = OH_NativeWindow_GetBufferHandleFromNative(nativeWindowBuffer);
    ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, nativeWindowBuffer, fenceFd, *region);
    ASSERT_EQ(ret, GSERROR_OK);
    NativeWindowBuffer *lastFlushedBuffer;
    int lastFlushedFenceFd;
    float matrix[16];
    ASSERT_EQ(OH_NativeWindow_GetLastFlushedBuffer(nativeWindow, &lastFlushedBuffer, nullptr, matrix),
        SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(OH_NativeWindow_GetLastFlushedBuffer(nativeWindow, &lastFlushedBuffer, &lastFlushedFenceFd, matrix),
        OHOS::GSERROR_OK);
    BufferHandle *lastFlushedHanlde = OH_NativeWindow_GetBufferHandleFromNative(lastFlushedBuffer);
    ASSERT_EQ(bufferHanlde->virAddr, lastFlushedHanlde->virAddr);

    ASSERT_EQ(OH_NativeWindow_GetLastFlushedBufferV2(nativeWindow, &lastFlushedBuffer, &lastFlushedFenceFd, matrix),
        OHOS::GSERROR_OK);
    float matrix90[16] = {0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    bool bRet = CheckMatricIsSame(matrix90, matrix);
    ASSERT_EQ(bRet, true);
}

/*
* Function: OH_NativeWindow_GetLastFlushedBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call NativeWindowHandleOpt set BUFFER_USAGE_PROTECTED
*                  2. call OH_NativeWindow_NativeWindowRequestBuffer
*                  3. call OH_NativeWindow_NativeWindowFlushBuffer
*                  4. call OH_NativeWindow_GetLastFlushedBuffer
*                  5. check ret
 */
HWTEST_F(NativeWindowTest, GetLastFlushedBuffer002, TestSize.Level0)
{
    int code = SET_USAGE;
    uint64_t usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_PROTECTED;
    ASSERT_EQ(NativeWindowHandleOpt(nativeWindow, code, usage), OHOS::GSERROR_OK);

    NativeWindowBuffer* nativeWindowBuffer = nullptr;
    int fenceFd = -1;
    int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &nativeWindowBuffer, &fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);

    struct Region *region = new Region();
    struct Region::Rect *rect = new Region::Rect();
    rect->x = 0x100;
    rect->y = 0x100;
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;
    ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, nativeWindowBuffer, fenceFd, *region);
    ASSERT_EQ(ret, GSERROR_OK);
    NativeWindowBuffer* lastFlushedBuffer;
    int lastFlushedFenceFd;
    float matrix[16];
    ASSERT_EQ(OH_NativeWindow_GetLastFlushedBuffer(nativeWindow, &lastFlushedBuffer, &lastFlushedFenceFd, matrix),
        OHOS::SURFACE_ERROR_NOT_SUPPORT);
}

/*
* Function: OH_NativeWindow_SetColorSpace
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_SetColorSpace
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_SetColorSpace001, TestSize.Level0)
{
    OH_NativeBuffer_ColorSpace colorSpace = OH_COLORSPACE_BT709_LIMIT;
    auto ret = OH_NativeWindow_GetColorSpace(nullptr, &colorSpace);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_NE(ret, GSERROR_INTERNAL);
    }
}

/*
* Function: OH_NativeWindow_SetColorSpace
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_SetColorSpace
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_SetColorSpace002, TestSize.Level0)
{
    OH_NativeBuffer_ColorSpace colorSpace = OH_COLORSPACE_BT709_LIMIT;
    auto ret = OH_NativeWindow_SetColorSpace(nativeWindow, colorSpace);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
}

/*
* Function: OH_NativeWindow_GetColorSpace
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_GetColorSpace
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_GetColorSpace001, TestSize.Level0)
{
    OH_NativeBuffer_ColorSpace colorSpace = OH_COLORSPACE_NONE;
    auto ret = OH_NativeWindow_GetColorSpace(nativeWindow, &colorSpace);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
}

/*
* Function: OH_NativeWindow_GetColorSpace
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_GetColorSpace
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_GetColorSpace002, TestSize.Level0)
{
    OH_NativeBuffer_ColorSpace colorSpace = OH_COLORSPACE_NONE;
    OH_NativeBuffer_ColorSpace colorSpaceSet = OH_COLORSPACE_BT709_FULL;
    auto ret = OH_NativeWindow_SetColorSpace(nativeWindow, colorSpaceSet);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeWindow_GetColorSpace(nativeWindow, &colorSpace);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
        ASSERT_EQ(colorSpace, colorSpaceSet);
    }
}

/*
* Function: OH_NativeWindow_SetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_SetMetadataValue
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_SetMetadataValue001, TestSize.Level0)
{
    int len = 60;
    uint8_t buff[len];
    for (int i = 0; i < 60; ++i) {
        buff[i] = static_cast<uint8_t>(i);
    }
    int32_t buffSize;
    uint8_t *checkMetaData;
    auto ret = OH_NativeWindow_GetMetadataValue(nativeWindow, OH_HDR_STATIC_METADATA, &buffSize, &checkMetaData);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_NE(ret, GSERROR_OK);
    }
    ret = OH_NativeWindow_SetMetadataValue(nullptr, OH_HDR_STATIC_METADATA, (int32_t)len, buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_NE(ret, GSERROR_OK);
    }
}

/*
* Function: OH_NativeWindow_SetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_SetMetadataValue
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_SetMetadataValue002, TestSize.Level0)
{
    int len = 60;
    uint8_t buff[len];
    for (int i = 0; i < 60; ++i) {
        buff[i] = static_cast<uint8_t>(i);
    }
    int32_t max_size = -1;
    auto ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_STATIC_METADATA, (int32_t)max_size, buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_NE(ret, GSERROR_OK);
    }
}

/*
* Function: OH_NativeWindow_SetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_SetMetadataValue
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_SetMetadataValue003, TestSize.Level0)
{
    int len = 60;
    uint8_t buff[len];
    for (int i = 0; i < 60; ++i) {
        buff[i] = static_cast<uint8_t>(i);
    }
    auto ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_STATIC_METADATA, (int32_t)len, buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_DYNAMIC_METADATA, (int32_t)len, buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    OH_NativeBuffer_MetadataType type = OH_VIDEO_HDR_HLG;
    ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_METADATA_TYPE, sizeof(type),
                                           reinterpret_cast<uint8_t *>(&type));
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
}

/*
* Function: OH_NativeWindow_SetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_SetMetadataValue
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_SetMetadataValue004, TestSize.Level0)
{
    int len = 60;
    uint8_t buff[len];
    for (int i = 0; i < 60; ++i) {
        buff[i] = static_cast<uint8_t>(i);
    }
    auto ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_STATIC_METADATA, (int32_t)len, buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_STATIC_METADATA, (int32_t)len, buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_DYNAMIC_METADATA, (int32_t)len, buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_DYNAMIC_METADATA, (int32_t)len, buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    OH_NativeBuffer_MetadataType type = OH_VIDEO_HDR_HLG;
    ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_METADATA_TYPE, sizeof(type),
                                           reinterpret_cast<uint8_t *>(&type));
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_METADATA_TYPE, sizeof(type),
                                           reinterpret_cast<uint8_t *>(&type));
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
}

/*
* Function: OH_NativeWindow_SetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_SetMetadataValue
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_SetMetadataValue005, TestSize.Level0)
{
    int len = 60;
    uint8_t buff[len];
    for (int i = 0; i < 60; ++i) {
        buff[i] = static_cast<uint8_t>(i);
    }
    NativeWindowBuffer *nativeWindowbuffer1 = nullptr;
    int fenceFd = -1;
    auto ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &nativeWindowbuffer1, &fenceFd);
    if (ret != GSERROR_HDI_ERROR) {
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_STATIC_METADATA, (int32_t)len, buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_DYNAMIC_METADATA, (int32_t)len, buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    OH_NativeBuffer_MetadataType type = OH_VIDEO_HDR_HLG;
    ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_METADATA_TYPE, sizeof(type),
                                           reinterpret_cast<uint8_t *>(&type));
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
}

/*
* Function: OH_NativeWindow_GetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_SetMetadataValue
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_GetMetadataValue001, TestSize.Level0)
{
    int32_t buffSize;
    uint8_t *checkMetaData;
    auto ret = OH_NativeWindow_GetMetadataValue(nullptr, OH_HDR_STATIC_METADATA, &buffSize, &checkMetaData);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_NE(ret, GSERROR_OK);
    }
}

/*
* Function: OH_NativeWindow_GetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_GetMetadataValue
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_GetMetadataValue002, TestSize.Level0)
{
    uint8_t *checkMetaData;
    auto ret = OH_NativeWindow_GetMetadataValue(nativeWindow, OH_HDR_STATIC_METADATA, nullptr, &checkMetaData);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_NE(ret, GSERROR_OK);
    }
}

/*
* Function: OH_NativeWindow_SetMetadataValue
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_SetMetadataValue
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_GetMetadataValue003, TestSize.Level0)
{
    int len = 60;
    uint8_t buff[len];
    for (int i = 0; i < 60; ++i) {
        buff[i] = static_cast<uint8_t>(60 - i);
    }
    int32_t buffSize;
    uint8_t *checkMetaData;
    auto ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_STATIC_METADATA, (int32_t)len, buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set metadataValue
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeWindow_GetMetadataValue(nativeWindow, OH_HDR_STATIC_METADATA, &buffSize, &checkMetaData);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set metadataValue
        ASSERT_EQ(memcmp(checkMetaData, buff, 60), 0);
        delete[] checkMetaData;
        checkMetaData = nullptr;
        ASSERT_EQ(ret, GSERROR_OK);
    }
    for (int i = 0; i < 60; i++) {
        buff[i] = static_cast<uint8_t>(70 - i);
    }
    ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_DYNAMIC_METADATA, (int32_t)len, buff);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set metadataValue
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeWindow_GetMetadataValue(nativeWindow, OH_HDR_DYNAMIC_METADATA, &buffSize, &checkMetaData);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set metadataValue
        ASSERT_EQ(memcmp(checkMetaData, buff, 60), 0);
        delete[] checkMetaData;
        checkMetaData = nullptr;
        ASSERT_EQ(ret, GSERROR_OK);
    }
    OH_NativeBuffer_MetadataType type = OH_VIDEO_HDR_HDR10;
    int32_t typeSize = sizeof(type);
    uint8_t pa = static_cast<uint8_t>(type);
    ret = OH_NativeWindow_SetMetadataValue(nativeWindow, OH_HDR_METADATA_TYPE, sizeof(type), &pa);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set colorspace
        ASSERT_EQ(ret, GSERROR_OK);
    }
    ret = OH_NativeWindow_GetMetadataValue(nativeWindow, OH_HDR_METADATA_TYPE, &typeSize, &checkMetaData);
    if (ret != GSERROR_NOT_SUPPORT) { // some device not support set metadataValue
        ASSERT_EQ(static_cast<uint8_t>(type), checkMetaData[0]);
        delete[] checkMetaData;
        checkMetaData = nullptr;
        ASSERT_EQ(ret, GSERROR_OK);
    }
}
/*
* Function: OH_NativeWindow_NativeWindowAbortBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowAbortBuffer by abnormal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, CancelBuffer001, TestSize.Level0)
{
    ASSERT_EQ(OH_NativeWindow_NativeWindowAbortBuffer(nullptr, nullptr), OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowAbortBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowAbortBuffer by abnormal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, CancelBuffer002, TestSize.Level0)
{
    ASSERT_EQ(OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, nullptr), OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowAbortBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowAbortBuffer
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, CancelBuffer003, TestSize.Level0)
{
    ASSERT_EQ(OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, nativeWindowBuffer), OHOS::GSERROR_OK);
}

/*
* Function: OH_NativeWindow_NativeObjectReference
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeObjectReference
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, Reference001, TestSize.Level0)
{
    struct NativeWindowBuffer *buffer = new NativeWindowBuffer();
    buffer->sfbuffer = sBuffer;
    ASSERT_EQ(OH_NativeWindow_NativeObjectReference(reinterpret_cast<void *>(buffer)), OHOS::GSERROR_OK);
    delete buffer;
}

/*
* Function: OH_NativeWindow_NativeObjectUnreference
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeObjectUnreference
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, Unreference001, TestSize.Level0)
{
    struct NativeWindowBuffer *buffer = new NativeWindowBuffer();
    buffer->sfbuffer = sBuffer;
    ASSERT_EQ(OH_NativeWindow_NativeObjectUnreference(reinterpret_cast<void *>(buffer)), OHOS::GSERROR_OK);
    delete buffer;
}

/*
* Function: OH_NativeWindow_DestroyNativeWindow
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_DestroyNativeWindow by abnormal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, DestroyNativeWindow001, TestSize.Level0)
{
    OHNativeWindow* window = nullptr;
    ASSERT_EQ(window, nullptr);
    OH_NativeWindow_DestroyNativeWindow(window);
}

/*
* Function: OH_NativeWindow_DestroyNativeWindowBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_DestroyNativeWindowBuffer by abnormal input
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_DestroyNativeWindowBuffer001, TestSize.Level0)
{
    OHNativeWindowBuffer* buffer = nullptr;
    ASSERT_EQ(buffer, nullptr);
    OH_NativeWindow_DestroyNativeWindowBuffer(buffer);
}

/*
* Function: OH_NativeWindow_NativeWindowSetScalingMode
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetScalingMode with abnormal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetScalingMode001, TestSize.Level0)
{
    OHScalingMode scalingMode = OHScalingMode::OH_SCALING_MODE_SCALE_TO_WINDOW;
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetScalingMode(nullptr, -1, scalingMode), OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowSetScalingMode
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetScalingMode with abnormal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetScalingMode002, TestSize.Level0)
{
    OHScalingMode scalingMode = OHScalingMode::OH_SCALING_MODE_SCALE_TO_WINDOW;
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetScalingMode(nativeWindow, -1, scalingMode), OHOS::GSERROR_NO_ENTRY);
}

/*
* Function: OH_NativeWindow_NativeWindowSetScalingMode
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetScalingMode with abnormal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetScalingMode003, TestSize.Level0)
{
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetScalingMode(nativeWindow, firstSeqnum,
                                         static_cast<OHScalingMode>(OHScalingMode::OH_SCALING_MODE_NO_SCALE_CROP + 1)),
                                         OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowSetScalingMode
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetScalingMode with abnormal parameters and check ret
*                  2. call OH_NativeWindow_NativeWindowSetScalingMode with normal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetScalingMode004, TestSize.Level0)
{
    OHScalingMode scalingMode = OHScalingMode::OH_SCALING_MODE_SCALE_TO_WINDOW;
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetScalingMode(nativeWindow, firstSeqnum, scalingMode), OHOS::GSERROR_OK);
}

/*
* Function: OH_NativeWindow_NativeWindowSetScalingModeV2
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetScalingModeV2 with abnormal parameters and check ret
*                  2. call OH_NativeWindow_NativeWindowSetScalingModeV2 with normal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetScalingMode005, TestSize.Level0)
{
    OHScalingModeV2 scalingMode = OHScalingModeV2::OH_SCALING_MODE_SCALE_TO_WINDOW_V2;
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetScalingModeV2(nativeWindow, scalingMode), OHOS::GSERROR_OK);
}


/*
* Function: OH_NativeWindow_NativeWindowSetMetaData
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetMetaData with abnormal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetMetaData001, TestSize.Level0)
{
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetMetaData(nullptr, -1, 0, nullptr), OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowSetMetaData
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetMetaData with abnormal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetMetaData002, TestSize.Level0)
{
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetMetaData(nativeWindow, -1, 0, nullptr), OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowSetMetaData
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetMetaData with abnormal parameters and check ret
*                  2. call OH_NativeWindow_NativeWindowSetMetaData with normal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetMetaData003, TestSize.Level0)
{
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetMetaData(nativeWindow, firstSeqnum, 0, nullptr),
              OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowSetMetaData
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetMetaData with abnormal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetMetaData004, TestSize.Level0)
{
    int32_t size = 1;
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetMetaData(nativeWindow, firstSeqnum, size, nullptr),
              OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowSetMetaData
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetMetaData with abnormal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetMetaData005, TestSize.Level0)
{
    int32_t size = 1;
    const OHHDRMetaData metaData[] = {{OH_METAKEY_RED_PRIMARY_X, 0}};
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetMetaData(nativeWindow, -1, size, metaData), OHOS::GSERROR_NO_ENTRY);
}

/*
* Function: OH_NativeWindow_NativeWindowSetMetaData
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetMetaData with normal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetMetaData006, TestSize.Level0)
{
    int32_t size = 1;
    const OHHDRMetaData metaData[] = {{OH_METAKEY_RED_PRIMARY_X, 0}};
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetMetaData(nativeWindow, firstSeqnum, size, metaData), OHOS::GSERROR_OK);
}

/*
* Function: OH_NativeWindow_NativeWindowSetMetaDataSet
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetMetaDataSet with abnormal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetMetaDataSet001, TestSize.Level0)
{
    OHHDRMetadataKey key = OHHDRMetadataKey::OH_METAKEY_HDR10_PLUS;
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetMetaDataSet(nullptr, -1, key, 0, nullptr),
              OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowSetMetaDataSet
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetMetaDataSet with abnormal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetMetaDataSet002, TestSize.Level0)
{
    OHHDRMetadataKey key = OHHDRMetadataKey::OH_METAKEY_HDR10_PLUS;
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetMetaDataSet(nativeWindow, -1, key, 0, nullptr),
              OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowSetMetaDataSet
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetMetaDataSet with abnormal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetMetaDataSet003, TestSize.Level0)
{
    OHHDRMetadataKey key = OHHDRMetadataKey::OH_METAKEY_HDR10_PLUS;
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetMetaDataSet(nativeWindow, firstSeqnum, key, 0, nullptr),
              OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowSetMetaDataSet
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetMetaDataSet with abnormal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetMetaDataSet004, TestSize.Level0)
{
    int32_t size = 1;
    OHHDRMetadataKey key = OHHDRMetadataKey::OH_METAKEY_HDR10_PLUS;
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetMetaDataSet(nativeWindow, firstSeqnum, key, size, nullptr),
              OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowSetMetaDataSet
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetMetaDataSet with abnormal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetMetaDataSet005, TestSize.Level0)
{
    int32_t size = 1;
    OHHDRMetadataKey key = OHHDRMetadataKey::OH_METAKEY_HDR10_PLUS;
    const uint8_t metaData[] = {0};
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetMetaDataSet(nativeWindow, -1, key, size, metaData),
              OHOS::GSERROR_NO_ENTRY);
}

/*
* Function: OH_NativeWindow_NativeWindowSetMetaDataSet
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetMetaDataSet with normal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetMetaDataSet006, TestSize.Level0)
{
    int32_t size = 1;
    OHHDRMetadataKey key = OHHDRMetadataKey::OH_METAKEY_HDR10_PLUS;
    const uint8_t metaData[] = {0};
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetMetaDataSet(nativeWindow, firstSeqnum, key, size, metaData),
              OHOS::GSERROR_OK);
}

/*
* Function: OH_NativeWindow_NativeWindowSetTunnelHandle
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetTunnelHandle with abnormal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetTunnelHandle001, TestSize.Level0)
{
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetTunnelHandle(nullptr, nullptr), OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowSetTunnelHandle
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetTunnelHandle with abnormal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetTunnelHandle002, TestSize.Level0)
{
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetTunnelHandle(nativeWindow, nullptr), OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: OH_NativeWindow_NativeWindowSetTunnelHandle
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetTunnelHandle with normal parameters and check ret
 */
HWTEST_F(NativeWindowTest, SetTunnelHandle003, TestSize.Level0)
{
    uint32_t reserveInts = 1;
    OHExtDataHandle *handle = AllocOHExtDataHandle(reserveInts);
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetTunnelHandle(nativeWindow, handle), OHOS::GSERROR_OK);
    FreeOHExtDataHandle(handle);
}

/*
* Function: OH_NativeWindow_NativeWindowSetTunnelHandle
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_NativeWindowSetTunnelHandle with normal parameters and check ret
* @tc.require: issueI5GMZN issueI5IWHW
 */
HWTEST_F(NativeWindowTest, SetTunnelHandle004, TestSize.Level0)
{
    uint32_t reserveInts = 2;
    OHExtDataHandle *handle = AllocOHExtDataHandle(reserveInts);
    nativeWindow = OH_NativeWindow_CreateNativeWindow(&pSurface);
    ASSERT_NE(nativeWindow, nullptr);
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetTunnelHandle(nativeWindow, handle), OHOS::GSERROR_OK);
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetTunnelHandle(nativeWindow, handle), OHOS::GSERROR_NO_ENTRY);
    FreeOHExtDataHandle(handle);
}

/*
* Function: NativeWindowGetTransformHint
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call NativeWindowGetTransformHint with normal parameters and check ret
* @tc.require: issueI5GMZN issueI5IWHW
 */
HWTEST_F(NativeWindowTest, NativeWindowGetTransformHint001, TestSize.Level0)
{
    OH_NativeBuffer_TransformType transform = OH_NativeBuffer_TransformType::NATIVEBUFFER_ROTATE_180;
    ASSERT_EQ(NativeWindowGetTransformHint(nullptr, &transform), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(NativeWindowSetTransformHint(nullptr, transform), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(NativeWindowSetTransformHint(nativeWindow, transform), OHOS::GSERROR_OK);
    transform = OH_NativeBuffer_TransformType::NATIVEBUFFER_ROTATE_NONE;
    ASSERT_EQ(NativeWindowGetTransformHint(nativeWindow, &transform), OHOS::GSERROR_OK);
    ASSERT_EQ(transform, OH_NativeBuffer_TransformType::NATIVEBUFFER_ROTATE_180);
}

/*
* Function: NativeWindowGetDefaultWidthAndHeight
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call NativeWindowGetDefaultWidthAndHeight with normal parameters and check ret
* @tc.require: issueI5GMZN issueI5IWHW
 */
HWTEST_F(NativeWindowTest, NativeWindowGetDefaultWidthAndHeight001, TestSize.Level0)
{
    ASSERT_EQ(NativeWindowGetDefaultWidthAndHeight(nullptr, nullptr, nullptr), OHOS::GSERROR_INVALID_ARGUMENTS);
    cSurface->SetDefaultWidthAndHeight(300, 400);
    int32_t width;
    int32_t height;
    ASSERT_EQ(NativeWindowGetDefaultWidthAndHeight(nativeWindow, &width, &height), OHOS::GSERROR_OK);
    ASSERT_EQ(width, 300);
    ASSERT_EQ(height, 400);
}

/*
* Function: NativeWindowSetBufferHold
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call NativeWindowSetBufferHold and no ret
* @tc.require: issueI5GMZN issueI5IWHW
 */
HWTEST_F(NativeWindowTest, NativeWindowSetBufferHold001, TestSize.Level0)
{
    NativeWindowSetBufferHold(nullptr);
    NativeWindowSetBufferHold(nativeWindow);
    int fenceFd = -1;
    struct Region *region = new Region();
    region->rectNumber = 0;
    region->rects = nullptr;
    ASSERT_EQ(OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, nativeWindowBuffer, fenceFd, *region),
              OHOS::GSERROR_BUFFER_STATE_INVALID);
    region->rectNumber = 1;
    struct Region::Rect * rect = new Region::Rect();
    rect->x = 0x100;
    rect->y = 0x100;
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;
    ASSERT_EQ(OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, nativeWindowBuffer, fenceFd, *region),
              OHOS::GSERROR_BUFFER_STATE_INVALID);
    cSurface->SetBufferHold(false);
    delete rect;
    delete region;
}

/*
* Function: NativeWindow_ReadWriteWindow
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_WriteToParcel and OH_NativeWindow_ReadFromParcel
* @tc.require: issueI5GMZN issueI5IWHW
 */
HWTEST_F(NativeWindowTest, NativeWindowReadWriteWindow001, TestSize.Level0)
{
    using namespace OHOS;
    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    cSurface->RegisterConsumerListener(listener);
    sptr<OHOS::IBufferProducer> producer = cSurface->GetProducer();
    sptr<OHOS::Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
    OHNativeWindow* nativeWindow = CreateNativeWindowFromSurface(&pSurface);
    auto uniqueId = nativeWindow->surface->GetUniqueId();
    ASSERT_NE(nativeWindow, nullptr);
    OHIPCParcel *parcel1 = OH_IPCParcel_Create();
    OHIPCParcel *parcel2 = OH_IPCParcel_Create();
    ASSERT_NE(parcel1, nullptr);
    ASSERT_NE(parcel2, nullptr);
    ASSERT_EQ(OH_NativeWindow_WriteToParcel(nullptr, parcel1), SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(OH_NativeWindow_WriteToParcel(nativeWindow, nullptr), SURFACE_ERROR_INVALID_PARAM);
    auto innerParcel = parcel1->msgParcel;
    parcel1->msgParcel = nullptr;
    ASSERT_EQ(OH_NativeWindow_WriteToParcel(nativeWindow, parcel1), SURFACE_ERROR_INVALID_PARAM);
    parcel1->msgParcel = innerParcel;
    ASSERT_EQ(OH_NativeWindow_WriteToParcel(nativeWindow, parcel1), GSERROR_OK);
    ASSERT_EQ(OH_NativeWindow_WriteToParcel(nativeWindow, parcel2), GSERROR_OK);
    // test read
    OHNativeWindow *readWindow = nullptr;
    ASSERT_EQ(OH_NativeWindow_ReadFromParcel(nullptr, &readWindow), SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(OH_NativeWindow_ReadFromParcel(parcel1, &readWindow), GSERROR_OK);
    ASSERT_NE(readWindow, nullptr);
    // test read twice
    OHNativeWindow *tempWindow = nullptr;
    ASSERT_EQ(OH_NativeWindow_ReadFromParcel(parcel1, &tempWindow), SURFACE_ERROR_INVALID_PARAM);
    cout << "test read write window, write window is " << nativeWindow << ", read windows is " << readWindow << endl;
    auto readId = readWindow->surface->GetUniqueId();
    ASSERT_EQ(uniqueId, readId);
    OHNativeWindow *readWindow1 = nullptr;
    SurfaceUtils::GetInstance()->RemoveNativeWindow(uniqueId);
    ASSERT_EQ(OH_NativeWindow_ReadFromParcel(parcel2, &readWindow1), GSERROR_OK);
    ASSERT_NE(readWindow1, nativeWindow);
    auto readId1 = readWindow1->surface->GetUniqueId();
    ASSERT_EQ(uniqueId, readId1);
    cout << "write uniqueId is " << uniqueId << ", parcel1 read id is " << readId <<
        ", parcel2 read id is " << readId1 << endl;
    OH_NativeWindow_DestroyNativeWindow(readWindow1);
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);
    OH_IPCParcel_Destroy(parcel1);
    OH_IPCParcel_Destroy(parcel2);
}

/*
 * Function: CreateNativeWindowFromSurface
 * Type: Function
 * Rank: Important(1)
 * EnvConditions: N/A
 * CaseDescription: CreateNativeWindowFromSurface and SetQueueSize apsplugin is nullptr
 * @tc.require: issueI5GMZN issueI5IWHW
 */
HWTEST_F(NativeWindowTest, CreateNativeWindowFromSurface_SetQueueSize001, TestSize.Level0)
{
    using namespace OHOS;
    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create();
    sptr<OHOS::IBufferProducer> producer = cSurface->GetProducer();
    sptr<OHOS::Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
    ISurfaceApsPlugin::LoadPlugin();
    ISurfaceApsPlugin::instance_ = nullptr;
    OHNativeWindow* nativeWindow = CreateNativeWindowFromSurface(&pSurface);
    ASSERT_EQ(ISurfaceApsPlugin::LoadApsFunc(), nullptr);
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);
}

/*
 * Function: CreateNativeWindowFromSurface
 * Type: Function
 * Rank: Important(1)
 * EnvConditions: N/A
 * CaseDescription: CreateNativeWindowFromSurface and SetQueueSize queueSize = 0
 * @tc.require: issueI5GMZN issueI5IWHW
 */
HWTEST_F(NativeWindowTest, CreateNativeWindowFromSurface_SetQueueSize002, TestSize.Level0)
{
    using namespace OHOS;
    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create();
    sptr<OHOS::IBufferProducer> producer = cSurface->GetProducer();
    sptr<OHOS::Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
    sptr<ApsPluginMock> mockPlugin = new ApsPluginMock();
    ISurfaceApsPlugin::LoadPlugin();
    ISurfaceApsPlugin::instance_ = mockPlugin;
    mockPlugin->queueSize = 0;
    OHNativeWindow* nativeWindow = CreateNativeWindowFromSurface(&pSurface);
    ASSERT_NE(ISurfaceApsPlugin::LoadApsFunc(), nullptr);
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);
}

/*
 * Function: CreateNativeWindowFromSurface
 * Type: Function
 * Rank: Important(1)
 * EnvConditions: N/A
 * CaseDescription: CreateNativeWindowFromSurface and SetQueueSize queueSize = 6
 * @tc.require: issueI5GMZN issueI5IWHW
 */
HWTEST_F(NativeWindowTest, CreateNativeWindowFromSurface_SetQueueSize003, TestSize.Level0)
{
    using namespace OHOS;
    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create();
    sptr<OHOS::IBufferProducer> producer = cSurface->GetProducer();
    sptr<OHOS::Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
    sptr<ApsPluginMock> mockPlugin = new ApsPluginMock();
    ISurfaceApsPlugin::LoadPlugin();
    ISurfaceApsPlugin::instance_ = mockPlugin;
    mockPlugin->queueSize = 6;
    OHNativeWindow* nativeWindow = CreateNativeWindowFromSurface(&pSurface);
    ASSERT_NE(ISurfaceApsPlugin::LoadApsFunc(), nullptr);
    ASSERT_EQ(cSurface->GetQueueSize(), mockPlugin->queueSize);
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);
}

/*
* Function: NativeWindow_ReadWriteWindow
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_WriteToParcel and OH_NativeWindow_ReadFromParcel
* @tc.require: issueI5GMZN issueI5IWHW
 */
HWTEST_F(NativeWindowTest, NativeWindowReadWriteWindow002, TestSize.Level0)
{
    using namespace OHOS;
    // test for no surface->GetUniqueId
    OHNativeWindow* nativeWindow1 = new OHNativeWindow();
    ASSERT_NE(nativeWindow1, nullptr);
    OHIPCParcel *parcel1 = OH_IPCParcel_Create();
    ASSERT_NE(parcel1, nullptr);
    ASSERT_EQ(OH_NativeWindow_WriteToParcel(nativeWindow1, parcel1), SURFACE_ERROR_INVALID_PARAM);
    OHNativeWindow *readWindow = nullptr;
    ASSERT_EQ(OH_NativeWindow_ReadFromParcel(parcel1, nullptr), SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(OH_NativeWindow_ReadFromParcel(parcel1, &readWindow), SURFACE_ERROR_INVALID_PARAM);
    OH_IPCParcel_Destroy(parcel1);
    delete nativeWindow1;
}

/*
* Function: SurfaceErrorInvalidParameter
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call functions with invalid parameters and check ret
*/
HWTEST_F(NativeWindowTest, SurfaceErrorInvalidParameter001, TestSize.Level0)
{
    int fence = -1;
    ASSERT_EQ(OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(nullptr), nullptr);
    ASSERT_EQ(OH_NativeWindow_GetLastFlushedBuffer(nullptr, nullptr, &fence, nullptr), SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(OH_NativeWindow_GetLastFlushedBuffer(nativeWindow, nullptr, &fence, nullptr),
        SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(GetNativeObjectMagic(nullptr), -1);
    ASSERT_EQ(GetSurfaceId(nativeWindow, nullptr), SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(NativeWindowGetTransformHint(nativeWindow, nullptr), SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(NativeWindowGetDefaultWidthAndHeight(nativeWindow, nullptr, nullptr), SURFACE_ERROR_INVALID_PARAM);
    int32_t width;
    ASSERT_EQ(NativeWindowGetDefaultWidthAndHeight(nativeWindow, &width, nullptr), SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(OH_NativeWindow_GetLastFlushedBufferV2(nullptr, nullptr, &fence, nullptr), SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(OH_NativeWindow_GetLastFlushedBufferV2(nativeWindow, nullptr, &fence, nullptr),
        SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(OH_NativeWindow_GetLastFlushedBufferV2(nativeWindow, nullptr, nullptr, nullptr),
        SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(NativeWindowDisconnect(nullptr), SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(OH_NativeWindow_SetColorSpace(nullptr, OH_COLORSPACE_NONE), SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(OH_NativeWindow_GetColorSpace(nullptr, nullptr), SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(OH_NativeWindow_GetColorSpace(nativeWindow, nullptr), SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(OH_NativeWindow_GetMetadataValue(nullptr, OH_HDR_METADATA_TYPE, nullptr, nullptr),
        SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(OH_NativeWindow_GetMetadataValue(nativeWindow, OH_HDR_METADATA_TYPE, nullptr, nullptr),
        SURFACE_ERROR_INVALID_PARAM);
}

/*
* Function: SurfaceErrorInvalidParameter
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call functions with invalid parameters and check ret
*/
HWTEST_F(NativeWindowTest, SurfaceErrorInvalidParameter002, TestSize.Level0)
{
    OHNativeWindow *nativeWindowTemp = new OHNativeWindow();
    NativeWindowBuffer *nativeWindowBuffer1;
    Region region;
    int32_t height;
    int32_t width;
    int fence = -1;
    ASSERT_EQ(OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &nativeWindowBuffer1, nullptr),
        SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(NativeWindowFlushBuffer(nativeWindowTemp, nativeWindowBuffer, fence, region),
        SURFACE_ERROR_INVALID_PARAM);
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindowTemp, 0), OHOS::GSERROR_INVALID_ARGUMENTS);
    OHScalingMode scalingMode1 = OHScalingMode::OH_SCALING_MODE_SCALE_TO_WINDOW;
    OHScalingModeV2 scalingMode2 = OHScalingModeV2::OH_SCALING_MODE_SCALE_TO_WINDOW_V2;
    ASSERT_EQ(OH_NativeWindow_NativeWindowSetScalingMode(nativeWindowTemp, firstSeqnum, scalingMode1),
        OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(NativeWindowSetScalingModeV2(nativeWindowTemp, scalingMode2), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(NativeWindowSetScalingModeV2(nullptr, scalingMode2), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(NativeWindowSetMetaData(nativeWindowTemp, 0, 0, nullptr), OHOS::GSERROR_INVALID_ARGUMENTS);
    OHHDRMetadataKey key = OHHDRMetadataKey::OH_METAKEY_HDR10_PLUS;
    ASSERT_EQ(NativeWindowSetMetaDataSet(nativeWindowTemp, 0, key, 0, nullptr), OHOS::GSERROR_INVALID_ARGUMENTS);
    OHExtDataHandle *handle = AllocOHExtDataHandle(1);
    ASSERT_EQ(NativeWindowSetTunnelHandle(nativeWindowTemp, handle), OHOS::GSERROR_INVALID_ARGUMENTS);
    OH_NativeBuffer_TransformType transform = OH_NativeBuffer_TransformType::NATIVEBUFFER_ROTATE_180;
    ASSERT_EQ(NativeWindowGetTransformHint(nativeWindowTemp, &transform), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(NativeWindowSetTransformHint(nativeWindowTemp, transform), OHOS::GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(NativeWindowGetDefaultWidthAndHeight(nativeWindowTemp, &width, &height), OHOS::GSERROR_INVALID_ARGUMENTS);
    NativeWindowSetBufferHold(nativeWindowTemp);
}

/*
* Function: NativeWindowSetRequestWidthAndHeight
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call NativeWindowSetRequestWidthAndHeight with invalid parameters and check ret
*                  2. call NativeWindowSetRequestWidthAndHeight with normal parameters and check ret
*                  3. call NativeWindowSetRequestWidthAndHeight with zore width and check ret
*                  3. call NativeWindowSetRequestWidthAndHeight with zore height and check ret
 */
HWTEST_F(NativeWindowTest, NativeWindowSetRequestWidthAndHeight001, TestSize.Level0)
{
    int fence = -1;
    ASSERT_EQ(NativeWindowSetRequestWidthAndHeight(nullptr, 0, 0), SURFACE_ERROR_INVALID_PARAM);
    cSurface->SetDefaultWidthAndHeight(300, 400);
    //分支1：走使用requestWidth/Height新建config分支
    ASSERT_EQ(NativeWindowSetRequestWidthAndHeight(nativeWindow, 100, 200), OHOS::GSERROR_OK);
    NativeWindowBuffer *nativeWindowBuffer1 = nullptr;
    auto ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &nativeWindowBuffer1, &fence);
    if (ret != GSERROR_HDI_ERROR) {
        ASSERT_EQ(ret, GSERROR_OK);
        ASSERT_EQ(nativeWindowBuffer1->sfbuffer->GetWidth(), 100);
        ASSERT_EQ(nativeWindowBuffer1->sfbuffer->GetHeight(), 200);
        ASSERT_EQ(NativeWindowCancelBuffer(nativeWindow, nativeWindowBuffer1), GSERROR_OK);
    }
    //分支2：使用surface成员变量windowConfig_(未初始化)
    ASSERT_EQ(NativeWindowSetRequestWidthAndHeight(nativeWindow, 0, 200), OHOS::GSERROR_OK);
    ASSERT_NE(OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &nativeWindowBuffer1, &fence),
        OHOS::GSERROR_OK);
    ASSERT_EQ(NativeWindowSetRequestWidthAndHeight(nativeWindow, 100, 0), OHOS::GSERROR_OK);
    ASSERT_NE(OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &nativeWindowBuffer1, &fence),
        OHOS::GSERROR_OK);
}

/*
* Function: OH_NativeWindow_DestroyNativeWindowBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call OH_NativeWindow_DestroyNativeWindowBuffer again
*                  2. check ret
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_DestroyNativeWindowBuffer002, TestSize.Level0)
{
    ASSERT_NE(nativeWindowBuffer, nullptr);
    OH_NativeWindow_DestroyNativeWindowBuffer(nativeWindowBuffer);
}

/*
 * Function: NativeWindowSetUsageAndFormat
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSet: call SET_USAGE with NATIVEBUFFER_USAGE_MEM_MMZ_CACHE and SET_FORMAT with
 *                             NATIVEBUFFER_PIXEL_FMT_Y8 and NATIVEBUFFER_PIXEL_FMT_Y16.
 *                  2. operation: request buffer and alloc buffer
 *                  3. result: request buffer and alloc buffer success
 */
HWTEST_F(NativeWindowTest, NativeWindowSetUsageAndFormat, Function | MediumTest | Level1)
{
    sptr<OHOS::IConsumerSurface> cSurfaceTmp = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    cSurfaceTmp->RegisterConsumerListener(listener);
    sptr<OHOS::IBufferProducer> producerTmp = cSurfaceTmp->GetProducer();
    sptr<OHOS::Surface> pSurfaceTmp = Surface::CreateSurfaceAsProducer(producerTmp);

    NativeWindow *nativeWindowTmp = OH_NativeWindow_CreateNativeWindow(&pSurfaceTmp);
    ASSERT_NE(nativeWindowTmp, nullptr);
    SetNativeWindowConfig(nativeWindowTmp);
    
    int code = SET_USAGE;
    uint64_t usageSet = NATIVEBUFFER_USAGE_MEM_MMZ_CACHE;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, usageSet), OHOS::GSERROR_OK);
    ASSERT_EQ(usageSet, BUFFER_USAGE_MEM_MMZ_CACHE);

    code = SET_FORMAT;
    int32_t formatSet = NATIVEBUFFER_PIXEL_FMT_Y8;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, formatSet), OHOS::GSERROR_OK);
    ASSERT_EQ(formatSet, GRAPHIC_PIXEL_FMT_Y8);
    
    NativeWindowBuffer *nativeWindowBuffer1 = nullptr;
    int fenceFd = -1;
    int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowTmp, &nativeWindowBuffer1, &fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);
    ASSERT_NE(nativeWindowBuffer1, nullptr);

    code = SET_FORMAT;
    formatSet = NATIVEBUFFER_PIXEL_FMT_Y16;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, formatSet), OHOS::GSERROR_OK);
    ASSERT_EQ(formatSet, GRAPHIC_PIXEL_FMT_Y16);

    NativeWindowBuffer *nativeWindowBuffer2 = nullptr;
    ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowTmp, &nativeWindowBuffer2, &fenceFd);
    ASSERT_EQ(ret, GSERROR_OK);
    ASSERT_NE(nativeWindowBuffer2, nullptr);
}

/*
 * Function: GetNativeObjectMagic
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call Func With Invalid NativeWindow
 *                  2. check ret
 */
HWTEST_F(NativeWindowTest, InvalidNativewindow001, TestSize.Level0)
{
    sptr<OHOS::IConsumerSurface> cSurfaceTestInvalid = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    cSurfaceTestInvalid->RegisterConsumerListener(listener);
    sptr<OHOS::IBufferProducer> producerTestInvalid = cSurfaceTestInvalid->GetProducer();
    sptr<OHOS::Surface> pSurfaceTestInvalid = Surface::CreateSurfaceAsProducer(producerTestInvalid);
    NativeWindow* InvalidNativeWindow = OH_NativeWindow_CreateNativeWindow(&pSurfaceTestInvalid);
    const char src[sizeof(NativeWindow)] = "TestInvalidNativeWindow";
    memcpy_s((void*)InvalidNativeWindow, sizeof(NativeWindow), src, strlen(src) + 1);

    NativeWindowBuffer *nativeWindowBuffer = nullptr;
    int fenceFd = -1;
    int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(InvalidNativeWindow, &nativeWindowBuffer, &fenceFd);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);

    struct Region *region = new Region();
    struct NativeWindowBuffer *buffer = new NativeWindowBuffer();
    ret = OH_NativeWindow_NativeWindowFlushBuffer(InvalidNativeWindow, buffer, fenceFd, *region);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);

    float matrix[16];
    ret = OH_NativeWindow_GetLastFlushedBuffer(InvalidNativeWindow, &nativeWindowBuffer, &fenceFd, matrix);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);

    ret = OH_NativeWindow_NativeWindowAttachBuffer(InvalidNativeWindow, buffer);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);

    ret = OH_NativeWindow_NativeWindowDetachBuffer(InvalidNativeWindow, buffer);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);

    ret = OH_NativeWindow_NativeWindowAbortBuffer(InvalidNativeWindow, buffer);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);

    int code = SET_USAGE;
    uint64_t usageSet = BUFFER_USAGE_CPU_READ;
    ret = OH_NativeWindow_NativeWindowHandleOpt(InvalidNativeWindow, code, usageSet);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);

    OHScalingMode scalingMode = OHScalingMode::OH_SCALING_MODE_SCALE_TO_WINDOW;
    ret = OH_NativeWindow_NativeWindowSetScalingMode(InvalidNativeWindow, -1, scalingMode);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);

    OHScalingModeV2 scalingModev2 = OHScalingModeV2::OH_SCALING_MODE_SCALE_TO_WINDOW_V2;
    ret = OH_NativeWindow_NativeWindowSetScalingModeV2(InvalidNativeWindow, scalingModev2);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);
}

/*
 * Function: GetNativeObjectMagic
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call Func With Invalid NativeWindow
 *                  2. check ret
 */
HWTEST_F(NativeWindowTest, InvalidNativewindow002, TestSize.Level0)
{
    sptr<OHOS::IConsumerSurface> cSurfaceTestInvalid = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    cSurfaceTestInvalid->RegisterConsumerListener(listener);
    sptr<OHOS::IBufferProducer> producerTestInvalid = cSurfaceTestInvalid->GetProducer();
    sptr<OHOS::Surface> pSurfaceTestInvalid = Surface::CreateSurfaceAsProducer(producerTestInvalid);
    NativeWindow* InvalidNativeWindow = OH_NativeWindow_CreateNativeWindow(&pSurfaceTestInvalid);
    const char src[sizeof(NativeWindow)] = "TestInvalidNativeWindow";
    memcpy_s((void*)InvalidNativeWindow, sizeof(NativeWindow), src, strlen(src) + 1);

    
    int32_t size = 1;
    const OHHDRMetaData metaData[] = {{OH_METAKEY_RED_PRIMARY_X, 0}};
    int32_t ret = OH_NativeWindow_NativeWindowSetMetaData(InvalidNativeWindow, firstSeqnum, size, metaData);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);

    OHHDRMetadataKey key = OHHDRMetadataKey::OH_METAKEY_HDR10_PLUS;
    const uint8_t metaDataSet[] = {0};
    ret = OH_NativeWindow_NativeWindowSetMetaDataSet(InvalidNativeWindow, firstSeqnum, key, size, metaDataSet);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);

    OHExtDataHandle *handle = AllocOHExtDataHandle(1);
    ret = NativeWindowSetTunnelHandle(InvalidNativeWindow, handle);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);

    uint64_t surfaceId = 0;
    ret = OH_NativeWindow_GetSurfaceId(InvalidNativeWindow, &surfaceId);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);

    NativeWindowSetBufferHold(InvalidNativeWindow);
    OHIPCParcel *parcel = OH_IPCParcel_Create();
    ret = NativeWindowWriteToParcel(InvalidNativeWindow, parcel);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);

    float matrix[16];
    NativeWindowBuffer* lastFlushedBuffer;
    int lastFlushedFenceFd;
    ret = OH_NativeWindow_GetLastFlushedBufferV2(InvalidNativeWindow, &lastFlushedBuffer, &lastFlushedFenceFd, matrix);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);

    ret = OH_NativeWindow_CleanCache(InvalidNativeWindow);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_INVALID_PARAM);
}

/*
 * Function: OH_NativeWindow_NativeWindowRequestBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call OH_NativeWindow_NativeWindowRequestBuffer by abnormal input
 *                  2. check ret
 */
HWTEST_F(NativeWindowTest, DisconnectStrictly001, TestSize.Level0)
{
    int32_t sRet = 0;
    uint32_t queueSize = 32;

    sptr<OHOS::IConsumerSurface> pConsumerSurface = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> pListener = new BufferConsumerListener();
    pConsumerSurface->RegisterConsumerListener(pListener);
    sptr<OHOS::IBufferProducer> pBufferProducer = pConsumerSurface->GetProducer();
    sptr<OHOS::Surface> pSurface1 = Surface::CreateSurfaceAsProducer(pBufferProducer);
    sptr<OHOS::Surface> pSurface2 = Surface::CreateSurfaceAsProducer(pBufferProducer);

    sptr<OHOS::IConsumerSurface> tConsumerSurface = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> tListener = new BufferConsumerListener();
    tConsumerSurface->RegisterConsumerListener(tListener);
    sptr<OHOS::IBufferProducer> tBufferProducer = tConsumerSurface->GetProducer();
    sptr<OHOS::Surface> tSurface = Surface::CreateSurfaceAsProducer(tBufferProducer);

    pSurface1->RegisterReleaseListener(OnBufferRelease);
    pSurface2->RegisterReleaseListener(OnBufferRelease);
    tSurface->RegisterReleaseListener(OnBufferRelease);
    pSurface1->SetQueueSize(queueSize);
    pSurface2->SetQueueSize(queueSize);
    tSurface->SetQueueSize(queueSize);

    OHNativeWindow* nativeWindow1 = OH_NativeWindow_CreateNativeWindow(&pSurface1);
    OHNativeWindow* nativeWindow2 = OH_NativeWindow_CreateNativeWindow(&pSurface2);
    OHNativeWindow* nativeWindow3 = OH_NativeWindow_CreateNativeWindow(&tSurface);

    SetNativeWindowConfig(nativeWindow1);
    SetNativeWindowConfig(nativeWindow2);
    SetNativeWindowConfig(nativeWindow3);

    int fenceFd = -1;
    struct Region *region = new Region();
    struct Region::Rect *rect = new Region::Rect();
    rect->x = 0x100;
    rect->y = 0x100;
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;

    OHNativeWindowBuffer* nativeWindowBuffer1 = nullptr;
    OHNativeWindowBuffer* nativeWindowBuffer2 = nullptr;
   
    // 不同的surface(指不同消费端)创建不同的nativewindow
    pSurface1->ConnectStrictly();
    tSurface->DisconnectStrictly();
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow1, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow1, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow3, &nativeWindowBuffer2, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    tSurface->ConnectStrictly();
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow3, &nativeWindowBuffer2, &fenceFd);
    tSurface->DisconnectStrictly();
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow3, nativeWindowBuffer2, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);

    pSurface1->DisconnectStrictly();
    tSurface->ConnectStrictly();
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow1, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow1, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow3, &nativeWindowBuffer2, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow3, nativeWindowBuffer2, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);

    // 同一个surface(指相同消费端)调用不同nativewindow
    pSurface1->ConnectStrictly();
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow1, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow1, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow2, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow2, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);


    pSurface2->DisconnectStrictly();
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow1, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow1, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow2, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow2, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);

    // 同一个surface调用相同nativewindow
    pSurface1->ConnectStrictly();
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow1, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow1, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow1, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow1, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);

    pSurface1->DisconnectStrictly();
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow1, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow1, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow1, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow1, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);

    // 调用1000次ConnectStrictly和DisconnectStrictly
    uint32_t testCount = 1000;
    for (int i = 0; i < testCount; i++) {
        pSurface1->ConnectStrictly();
    }
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow1, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow1, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    for (int i = 0; i < testCount; i++) {
        pSurface1->DisconnectStrictly();
    }
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow1, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow1, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);

    // 调用ConnectStrictly->DisconnectStrictly，再调用不同的nativewindow
    pSurface1->ConnectStrictly();
    pSurface1->DisconnectStrictly();
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow1, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow1, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow2, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow2, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);

    // 调用DisconnectStrictly->ConnectStrictly，再调用不同的nativewindow
    pSurface1->DisconnectStrictly();
    pSurface1->ConnectStrictly();
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow1, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow1, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow2, &nativeWindowBuffer1, &fenceFd);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow2, nativeWindowBuffer1, fenceFd, *region);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);

	// 验证OH_NativeWindow_CleanCache在消费端断连后的错误码
	pSurface1->Connect();
	sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow1, &nativeWindowBuffer1, &fenceFd);
	pSurface1->Disconnect();
	sRet = OH_NativeWindow_CleanCache(nativeWindow1);
	EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    rect = nullptr;
    region = nullptr;
    delete rect;
    delete region;
}

/*
 * Function: NativeWindowLockBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSet: call NativeWindowLockBuffer with null surface or null window
 *                  2. operation: request native buffer with lock
 *                  3. result: request native buffer with lock buffer fail
 */
HWTEST_F(NativeWindowTest, NativeWindowLockBuffer001, TestSize.Level0)
{
    OHNativeWindow* window = new OHNativeWindow();
    ASSERT_NE(window, nullptr);

    OHNativeWindowBuffer* buffer = nullptr;
    Region::Rect rect = {0};
    rect.x = 0x100;
    rect.y = 0x100;
    rect.w = 0x100;
    rect.h = 0x100;
    Region region = {.rects = &rect, .rectNumber = 1};
    int32_t ret = NativeWindowLockBuffer(nullptr, region, &buffer);
    ASSERT_EQ(ret, SURFACE_ERROR_INVALID_PARAM);
    ret = NativeWindowLockBuffer(window, region, nullptr);
    ASSERT_EQ(ret, SURFACE_ERROR_INVALID_PARAM);
    ret = NativeWindowLockBuffer(window, region, &buffer);
    ASSERT_EQ(ret, SURFACE_ERROR_ERROR);
    delete window;
}

/*
 * Function: NativeWindowLockBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSet: native window is unlocked, call NativeWindowLockBuffer with null surface or null window
 *                  2. operation: request native buffer with lock
 *                  3. result: request native buffer with lock buffer fail
 */
HWTEST_F(NativeWindowTest, NativeWindowLockBuffer002, TestSize.Level0)
{
    sptr<OHOS::IConsumerSurface> cSurfaceTmp = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listenerTmp = new BufferConsumerListener();
    cSurfaceTmp->RegisterConsumerListener(listenerTmp);
    sptr<OHOS::IBufferProducer> producerTmp = cSurface->GetProducer();
    sptr<OHOS::Surface> pSurfaceTmp = Surface::CreateSurfaceAsProducer(producerTmp);

    OHNativeWindow* window = OH_NativeWindow_CreateNativeWindow(&pSurfaceTmp);
    ASSERT_NE(window, nullptr);
    
    OHNativeWindowBuffer* buffer = nullptr;
    Region::Rect rect = {0};
    rect.x = 0x100;
    rect.y = 0x100;
    rect.w = 0x100;
    rect.h = 0x100;
    Region region = {.rects = &rect, .rectNumber = 1};
    int32_t ret = NativeWindowLockBuffer(window, region, &buffer);
    ASSERT_EQ(ret, GSERROR_OK);
    ASSERT_NE(buffer, nullptr);
    
    ret = NativeWindowLockBuffer(window, region, &buffer);
    ASSERT_EQ(ret, GSERROR_INVALID_OPERATING);
    ASSERT_EQ(buffer, nullptr);
    
    ret = NativeWindowUnlockAndFlushBuffer(window);
    ASSERT_EQ(ret, GSERROR_OK);
    ASSERT_EQ(buffer, nullptr);
    OH_NativeWindow_DestroyNativeWindow(window);
}

/*
 * Function: NativeWindowUnlockAndFlushBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSet: call NativeWindowUnlockAndFlushBuffer with null surface or null window
 *                  2. operation: unlock native buffer
 *                  3. result: unlock native buffer fail
 */
HWTEST_F(NativeWindowTest, NativeWindowUnlockAndFlushBuffer001, TestSize.Level0)
{
    OHNativeWindow* window = new OHNativeWindow();
    ASSERT_NE(window, nullptr);

    int32_t ret = NativeWindowUnlockAndFlushBuffer(nullptr);
    ASSERT_EQ(ret, SURFACE_ERROR_INVALID_PARAM);

    ret = NativeWindowUnlockAndFlushBuffer(window);
    ASSERT_EQ(ret, SURFACE_ERROR_ERROR);
    delete window;
}

/*
 * Function: NativeWindowUnlockAndFlushBuffer
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSet: call NativeWindowUnlockAndFlushBuffer with valid window.
 *                  2. operation: unlock native buffer
 *                  3. result: unlock native buffer success
 */
HWTEST_F(NativeWindowTest, NativeWindowUnlockAndFlushBuffer002, TestSize.Level0)
{
    sptr<OHOS::IConsumerSurface> cSurfaceTmp = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listenerTmp = new BufferConsumerListener();
    cSurfaceTmp->RegisterConsumerListener(listenerTmp);
    sptr<OHOS::IBufferProducer> producerTmp = cSurface->GetProducer();
    sptr<OHOS::Surface> pSurfaceTmp = Surface::CreateSurfaceAsProducer(producerTmp);

    OHNativeWindow* window = OH_NativeWindow_CreateNativeWindow(&pSurfaceTmp);
    ASSERT_NE(window, nullptr);
    
    OHNativeWindowBuffer* buffer = nullptr;
    Region::Rect rect = {0};
    rect.x = 0x100;
    rect.y = 0x100;
    rect.w = 0x100;
    rect.h = 0x100;
    Region region = {.rects = &rect, .rectNumber = 1};
    int32_t ret = NativeWindowLockBuffer(window, region, &buffer);
    ASSERT_EQ(ret, GSERROR_OK);
    ASSERT_NE(buffer, nullptr);
    
    ret = NativeWindowUnlockAndFlushBuffer(window);
    ASSERT_EQ(ret, GSERROR_OK);

    ret = NativeWindowUnlockAndFlushBuffer(window);
    ASSERT_EQ(ret, GSERROR_INVALID_OPERATING);
    OH_NativeWindow_DestroyNativeWindow(window);
}
}
