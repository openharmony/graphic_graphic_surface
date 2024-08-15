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
#include <chrono>
#include <thread>
#include <unistd.h>
#include <gtest/gtest.h>
#include <iservice_registry.h>
#include <securec.h>
#include <native_window.h>
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "external_window.h"
#include "iconsumer_surface.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class NativeWindowTest : public testing::Test,  public IBufferConsumerListenerClazz {
public:
    static void SetUpTestCase();
    void OnBufferAvailable() override;
    void SetData(NativeWindowBuffer *nativeWindowBuffer, NativeWindow *nativeWindow);
    bool GetData(sptr<SurfaceBuffer> &buffer);

    // OH_NativeWindow_CreateNativeWindow001
    pid_t ChildNativeWindowProcess001();
    int32_t CreateNativeWindowAndRequestBuffer001(NativeWindow **nativeWindow);

    // OH_NativeWindow_CreateNativeWindow002
    pid_t ChildNativeWindowProcess002();
    int32_t CreateNativeWindowAndRequestBuffer002(NativeWindow **nativeWindow);

    static inline sptr<OHOS::IConsumerSurface> cSurface = nullptr;
    static inline sptr<Surface> pSurface = nullptr;
    static inline int32_t pipeFd[2] = {};
};

void NativeWindowTest::SetUpTestCase() {}

void NativeWindowTest::OnBufferAvailable()
{

}

int32_t NativeWindowTest::CreateNativeWindowAndRequestBuffer001(NativeWindow **nativeWindow)
{
    *nativeWindow = OH_NativeWindow_CreateNativeWindow(&pSurface);
    struct NativeWindowBuffer *nativeWindowBuffer = nullptr;

    int32_t code = SET_BUFFER_GEOMETRY;
    int32_t height = 0x100;
    int32_t width = 0x100;
    OH_NativeWindow_NativeWindowHandleOpt(*nativeWindow, code, height, width);

    int32_t fenceFd = -1;
    auto ret = OH_NativeWindow_NativeWindowRequestBuffer(*nativeWindow, &nativeWindowBuffer, &fenceFd);
    if (ret != OHOS::GSERROR_OK) {
        return ret;
    }

    struct Region *region = new Region();
    struct Region::Rect *rect = new Region::Rect();
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;
    region->rectNumber = 1;
    ret = OH_NativeWindow_NativeWindowFlushBuffer(*nativeWindow, nativeWindowBuffer, -1, *region);
    if (ret != OHOS::GSERROR_OK) {
        delete rect;
        delete region;
        return ret;
    }
    delete rect;
    delete region;
    return OHOS::GSERROR_OK;
}

pid_t NativeWindowTest::ChildNativeWindowProcess001()
{
    pipe(pipeFd);
    pid_t pid = fork();
    if (pid != 0) {
        return pid;
    }

    int64_t data;
    read(pipeFd[0], &data, sizeof(data));

    NativeWindow *nativeWindow = nullptr;
    int32_t ret = CreateNativeWindowAndRequestBuffer001(&nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        exit(0);
        return -1;
    }

    data = ret;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    close(pipeFd[0]);
    close(pipeFd[1]);
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);
    exit(0);
    return 0;
}

/*
* Function: OH_NativeWindow_CreateNativeWindow001
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. produce surface by nativewindow interface
*                  2. consume surface and check buffer
* @tc.require: issueI5GMZN issueI5IWHW
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_CreateNativeWindow001, Function | MediumTest | Level2)
{
    auto pid = ChildNativeWindowProcess001();
    ASSERT_GE(pid, 0);

    cSurface = IConsumerSurface::Create("OH_NativeWindow_CreateNativeWindow001");
    cSurface->RegisterConsumerListener(this);
    auto producer = cSurface->GetProducer();
    pSurface = Surface::CreateSurfaceAsProducer(producer);

    int64_t data = 0;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    EXPECT_EQ(data, OHOS::GSERROR_OK);

    OHOS::sptr<SurfaceBuffer> buffer = nullptr;
    int32_t fence = -1;
    int64_t timestamp;
    Rect damage;
    auto ret = cSurface->AcquireBuffer(buffer, fence, timestamp, damage);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);
    EXPECT_NE(buffer, nullptr);

    ret = cSurface->ReleaseBuffer(buffer, -1);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    write(pipeFd[1], &data, sizeof(data));
    close(pipeFd[0]);
    close(pipeFd[1]);
    waitpid(pid, nullptr, 0);
    pSurface = nullptr;
    cSurface = nullptr;
}

void NativeWindowTest::SetData(NativeWindowBuffer *nativeWindowBuffer, NativeWindow *nativeWindow)
{
    nativeWindowBuffer->sfbuffer->GetExtraData()->ExtraSet("123", 0x123);
    nativeWindowBuffer->sfbuffer->GetExtraData()->ExtraSet("345", (int64_t)0x345);
    nativeWindowBuffer->sfbuffer->GetExtraData()->ExtraSet("567", "567");
}

bool NativeWindowTest::GetData(sptr<SurfaceBuffer> &buffer)
{
    int32_t int32;
    int64_t int64;
    std::string str;
    buffer->GetExtraData()->ExtraGet("123", int32);
    buffer->GetExtraData()->ExtraGet("345", int64);
    buffer->GetExtraData()->ExtraGet("567", str);
    if ((int32 != 0x123) || (int64 != 0x345) || (str != "567")) {
        return false;
    }

    return true;
}

int32_t NativeWindowTest::CreateNativeWindowAndRequestBuffer002(NativeWindow **nativeWindow)
{
    *nativeWindow = OH_NativeWindow_CreateNativeWindow(&pSurface);
    struct NativeWindowBuffer *nativeWindowBuffer = nullptr;

    int32_t code = SET_BUFFER_GEOMETRY;
    int32_t height = 0x100;
    int32_t width = 0x100;
    OH_NativeWindow_NativeWindowHandleOpt(*nativeWindow, code, height, width);

    code = SET_FORMAT;
    int32_t format = GRAPHIC_PIXEL_FMT_RGBA_8888;
    OH_NativeWindow_NativeWindowHandleOpt(*nativeWindow, code, format);

    int32_t fenceFd = -1;
    auto ret = OH_NativeWindow_NativeWindowRequestBuffer(*nativeWindow, &nativeWindowBuffer, &fenceFd);
    if (ret != OHOS::GSERROR_OK) {
        return ret;
    }
    SetData(nativeWindowBuffer, *nativeWindow);

    struct Region *region = new Region();
    struct Region::Rect *rect = new Region::Rect();
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;
    region->rectNumber = 1;
    ret = OH_NativeWindow_NativeWindowFlushBuffer(*nativeWindow, nativeWindowBuffer, -1, *region);
    if (ret != OHOS::GSERROR_OK) {
        delete rect;
        delete region;
        return ret;
    }
    delete rect;
    delete region;
    return OHOS::GSERROR_OK;
}

pid_t NativeWindowTest::ChildNativeWindowProcess002()
{
    pipe(pipeFd);
    pid_t pid = fork();
    if (pid != 0) {
        return pid;
    }

    int64_t data;
    read(pipeFd[0], &data, sizeof(data));

    NativeWindow *nativeWindow = nullptr;
    int32_t ret = CreateNativeWindowAndRequestBuffer002(&nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        exit(0);
        return -1;
    }

    data = ret;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    close(pipeFd[0]);
    close(pipeFd[1]);
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);
    exit(0);
    return 0;
}

/*
* Function: OH_NativeWindow_CreateNativeWindow002
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. produce surface by nativewindow interface
*                  2. consume surface and check buffer
* @tc.require: issueI5GMZN issueI5IWHW
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_CreateNativeWindow002, Function | MediumTest | Level2)
{
    auto pid = ChildNativeWindowProcess002();
    ASSERT_GE(pid, 0);

    cSurface = IConsumerSurface::Create("OH_NativeWindow_CreateNativeWindow002");
    cSurface->RegisterConsumerListener(this);
    auto producer = cSurface->GetProducer();
    pSurface = Surface::CreateSurfaceAsProducer(producer);

    int64_t data = 0;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    EXPECT_EQ(data, OHOS::GSERROR_OK);

    OHOS::sptr<SurfaceBuffer> buffer = nullptr;
    int32_t fence = -1;
    int64_t timestamp;
    Rect damage;
    auto ret = cSurface->AcquireBuffer(buffer, fence, timestamp, damage);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);
    EXPECT_NE(buffer, nullptr);
    EXPECT_EQ(GetData(buffer), true);

    ret = cSurface->ReleaseBuffer(buffer, -1);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    write(pipeFd[1], &data, sizeof(data));
    close(pipeFd[0]);
    close(pipeFd[1]);
    waitpid(pid, nullptr, 0);
    pSurface = nullptr;
    cSurface = nullptr;
}
}