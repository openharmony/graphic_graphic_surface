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
HWTEST_F(NativeWindowTest, CreateNativeWindow001, Function | MediumTest | Level2)
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
HWTEST_F(NativeWindowTest, CreateNativeWindow002, Function | MediumTest | Level2)
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
HWTEST_F(NativeWindowTest, CreateNativeWindow003, Function | MediumTest | Level2)
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
HWTEST_F(NativeWindowTest, CreateNativeWindow004, Function | MediumTest | Level2)
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
HWTEST_F(NativeWindowTest, CreateNativeWindowFromSurfaceId001, Function | MediumTest | Level2)
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
HWTEST_F(NativeWindowTest, CreateNativeWindowFromSurfaceId002, Function | MediumTest | Level2)
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
HWTEST_F(NativeWindowTest, CreateNativeWindowFromSurfaceId003, Function | MediumTest | Level2)
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
HWTEST_F(NativeWindowTest, HandleOpt001, Function | MediumTest | Level2)
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
HWTEST_F(NativeWindowTest, HandleOpt002, Function | MediumTest | Level2)
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
HWTEST_F(NativeWindowTest, HandleOpt003, Function | MediumTest | Level2)
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
HWTEST_F(NativeWindowTest, HandleOpt004, Function | MediumTest | Level2)
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
HWTEST_F(NativeWindowTest, HandleOpt005, Function | MediumTest | Level2)
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
HWTEST_F(NativeWindowTest, HandleOpt006, Function | MediumTest | Level2)
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
HWTEST_F(NativeWindowTest, HandleOpt007, Function | MediumTest | Level2)
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
HWTEST_F(NativeWindowTest, HandleOpt008, Function | MediumTest | Level1)
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
HWTEST_F(NativeWindowTest, HandleOpt009, Function | MediumTest | Level1)
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
HWTEST_F(NativeWindowTest, HandleOpt010, Function | MediumTest | Level2)
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
HWTEST_F(NativeWindowTest, HandleOpt011, Function | MediumTest | Level1)
{
    int code = SET_SOURCE_TYPE;
    OHSurfaceSource typeSet = OHSurfaceSource::OH_SURFACE_SOURCE_GAME;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, typeSet), OHOS::GSERROR_OK);

    code = GET_SOURCE_TYPE;
    OHSurfaceSource typeGet = OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
    ASSERT_EQ(OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &typeGet), OHOS::GSERROR_OK);
    ASSERT_EQ(typeSet, typeGet);
}
