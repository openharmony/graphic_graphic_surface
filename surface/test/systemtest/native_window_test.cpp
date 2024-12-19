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
#include <securec.h>
#include <native_window.h>
#include "external_window.h"
#include "iconsumer_surface.h"
#include "event_handler.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
constexpr uint32_t QUEUE_SIZE = 5;
class NativeWindowTest : public testing::Test, public IBufferConsumerListenerClazz {
public:
    static void SetUpTestCase();
    void OnBufferAvailable() override;
    void SetData(NativeWindowBuffer *nativeWindowBuffer, NativeWindow *nativeWindow);
    bool GetData(sptr<SurfaceBuffer> &buffer);

    // OH_NativeWindow_CreateNativeWindow001
    int32_t ThreadNativeWindowProcess001(int32_t *pipeFd, sptr<IBufferProducer> producer);
    int32_t CreateNativeWindowAndRequestBuffer001(sptr<IBufferProducer> producer, NativeWindow **nativeWindow);

    // OH_NativeWindow_CreateNativeWindow002
    int32_t ThreadNativeWindowProcess002(int32_t *pipeFd, sptr<IBufferProducer> producer);
    int32_t CreateNativeWindowAndRequestBuffer002(sptr<IBufferProducer> producer, NativeWindow **nativeWindow);

    // OH_NativeWindow_CreateNativeWindowFromSurfaceId001
    int32_t ThreadNativeWindowProcess003(int32_t *pipeFd, uint64_t uniqueId);
    int32_t CreateNativeWindowAndRequestBuffer003(uint64_t uniqueId, NativeWindow **nativeWindow);

    // OH_NativeWindow_CreateNativeWindowFromSurfaceId002
    int32_t ThreadNativeWindowProcess004(int32_t *pipeFd, uint64_t uniqueId);
    int32_t CreateNativeWindowAndRequestBuffer004(uint64_t uniqueId, NativeWindow **nativeWindow);
    int32_t RequestBuffer001(NativeWindow *nativeWindow);

    // OH_NativeWindow_GetLastFlushedBufferV2001
    int32_t ThreadNativeWindowProcess005(int32_t *pipeFd, uint64_t uniqueId);
    int32_t CreateNativeWindowAndRequestBuffer005(uint64_t uniqueId, NativeWindow **nativeWindow);

    // OH_NativeWindow_NativeObjectReference001
    int32_t ThreadNativeWindowProcess006(int32_t *pipeFd, uint64_t uniqueId);

    // OH_NativeWindow_GetSurfaceId001
    int32_t ThreadNativeWindowProcess007(int32_t *pipeFd, sptr<IBufferProducer> producer, uint64_t *uniqueId);
    int32_t CreateNativeWindowAndRequestBuffer007(sptr<IBufferProducer> producer, NativeWindow **nativeWindow);

    // OH_NativeWindow_NativeWindowAttachBuffer001
    int32_t ThreadNativeWindowProcess008(int32_t *pipeFd, uint64_t uniqueId);
    int32_t CreateNativeWindowAndRequestBuffer008(uint64_t uniqueId, NativeWindow **nativeWindow);
    int32_t CreateNativeWindowAndAttachBuffer001(NativeWindow *nativeWindow);

    int32_t g_onBufferAvailable_ = 0;
};

class OnBufferAvailableTest : public IBufferConsumerListenerClazz, public RefBase {
public:
    void OnBufferAvailable() override;
};

void OnBufferAvailableTest::OnBufferAvailable() {};

void NativeWindowTest::SetUpTestCase() {}

void NativeWindowTest::OnBufferAvailable()
{
    g_onBufferAvailable_++;
}

int32_t NativeWindowTest::CreateNativeWindowAndRequestBuffer001(sptr<IBufferProducer> producer,
    NativeWindow **nativeWindow)
{
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
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

int32_t NativeWindowTest::ThreadNativeWindowProcess001(int32_t *pipeFd, sptr<IBufferProducer> producer)
{
    int64_t data;
    NativeWindow *nativeWindow = nullptr;
    int32_t ret = CreateNativeWindowAndRequestBuffer001(producer, &nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        return -1;
    }

    data = ret;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);
    return 0;
}

HWTEST_F(NativeWindowTest, OH_NativeWindow_CreateNativeWindow001, Function | MediumTest | Level2)
{
    int32_t pipeFd[2] = {};
    pipe(pipeFd);

    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create("OH_NativeWindow_CreateNativeWindow001");
    cSurface->RegisterConsumerListener(this);
    auto producer = cSurface->GetProducer();

    std::thread thread([this, pipeFd, producer]() {
        int32_t ret = this->ThreadNativeWindowProcess001((int32_t*)(pipeFd), producer);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
    });

    int64_t data = 0;
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
    if (thread.joinable()) {
        thread.join();
    }
    close(pipeFd[0]);
    close(pipeFd[1]);
    producer = nullptr;
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

int32_t NativeWindowTest::CreateNativeWindowAndRequestBuffer002(sptr<IBufferProducer> producer,
    NativeWindow **nativeWindow)
{
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
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

int32_t NativeWindowTest::ThreadNativeWindowProcess002(int32_t *pipeFd, sptr<IBufferProducer> producer)
{
    int64_t data;
    NativeWindow *nativeWindow = nullptr;
    int32_t ret = CreateNativeWindowAndRequestBuffer002(producer, &nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        return -1;
    }

    data = ret;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);
    return 0;
}

HWTEST_F(NativeWindowTest, OH_NativeWindow_CreateNativeWindow002, Function | MediumTest | Level2)
{
    int32_t pipeFd[2] = {};
    pipe(pipeFd);

    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create("OH_NativeWindow_CreateNativeWindow002");
    cSurface->RegisterConsumerListener(this);
    auto producer = cSurface->GetProducer();

    std::thread thread([this, pipeFd, producer]() {
        int32_t ret = this->ThreadNativeWindowProcess002((int32_t*)(pipeFd), producer);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
    });

    int64_t data = 0;
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
    if (thread.joinable()) {
        thread.join();
    }
    close(pipeFd[0]);
    close(pipeFd[1]);
    producer = nullptr;
    cSurface = nullptr;
}

int32_t NativeWindowTest::CreateNativeWindowAndRequestBuffer003(uint64_t uniqueId, NativeWindow **nativeWindow)
{
    int32_t ret = OH_NativeWindow_CreateNativeWindowFromSurfaceId(uniqueId, nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        return ret;
    }
    struct NativeWindowBuffer *nativeWindowBuffer = nullptr;

    int32_t code = SET_BUFFER_GEOMETRY;
    int32_t height = 0x100;
    int32_t width = 0x100;
    OH_NativeWindow_NativeWindowHandleOpt(*nativeWindow, code, height, width);
    code = SET_FORMAT;
    int32_t format = GRAPHIC_PIXEL_FMT_RGBA_8888;
    OH_NativeWindow_NativeWindowHandleOpt(*nativeWindow, code, format);

    int32_t fenceFd = -1;
    struct Region *region = new Region();
    struct Region::Rect *rect = new Region::Rect();
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;
    region->rectNumber = 1;
    for (int32_t i = 0; i < QUEUE_SIZE; i++) {
        ret = OH_NativeWindow_NativeWindowRequestBuffer(*nativeWindow, &nativeWindowBuffer, &fenceFd);
        if (ret != OHOS::GSERROR_OK) {
            delete rect;
            delete region;
            return ret;
        }
        SetData(nativeWindowBuffer, *nativeWindow);

        ret = OH_NativeWindow_NativeWindowFlushBuffer(*nativeWindow, nativeWindowBuffer, -1, *region);
        if (ret != OHOS::GSERROR_OK) {
            delete rect;
            delete region;
            return ret;
        }
    }
    delete rect;
    delete region;
    return OHOS::GSERROR_OK;
}

int32_t NativeWindowTest::ThreadNativeWindowProcess003(int32_t *pipeFd, uint64_t uniqueId)
{
    int64_t data;
    NativeWindow *nativeWindow = nullptr;
    int32_t ret = CreateNativeWindowAndRequestBuffer003(uniqueId, &nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        return -1;
    }

    data = ret;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);
    return 0;
}

HWTEST_F(NativeWindowTest, OH_NativeWindow_CreateNativeWindowFromSurfaceId001, Function | MediumTest | Level2)
{
    int32_t pipeFd[2] = {};
    pipe(pipeFd);

    sptr<OHOS::IConsumerSurface> cSurface =
        IConsumerSurface::Create("OH_NativeWindow_CreateNativeWindowFromSurfaceId001");
    cSurface->RegisterConsumerListener(this);
    auto producer = cSurface->GetProducer();
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
    EXPECT_NE(pSurface, nullptr);
    EXPECT_EQ(cSurface->SetQueueSize(QUEUE_SIZE), OHOS::GSERROR_OK);

    uint64_t uniqueId = cSurface->GetUniqueId();
    std::thread thread([this, pipeFd, uniqueId]() {
        int32_t ret = this->ThreadNativeWindowProcess003((int32_t*)(pipeFd), uniqueId);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
    });

    int64_t data = 0;
    read(pipeFd[0], &data, sizeof(data));
    EXPECT_EQ(data, OHOS::GSERROR_OK);

    OHOS::sptr<SurfaceBuffer> buffer = nullptr;
    int32_t fence = -1;
    int64_t timestamp;
    Rect damage;
    EXPECT_EQ(g_onBufferAvailable_, QUEUE_SIZE);
    while (g_onBufferAvailable_ > 0) {
        auto ret = cSurface->AcquireBuffer(buffer, fence, timestamp, damage);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
        EXPECT_NE(buffer, nullptr);
        EXPECT_EQ(GetData(buffer), true);

        ret = cSurface->ReleaseBuffer(buffer, -1);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
        g_onBufferAvailable_--;
    }

    g_onBufferAvailable_ = 0;
    write(pipeFd[1], &data, sizeof(data));
    if (thread.joinable()) {
        thread.join();
    }
    close(pipeFd[0]);
    close(pipeFd[1]);
    producer = nullptr;
    cSurface = nullptr;
}

int32_t NativeWindowTest::RequestBuffer001(NativeWindow *nativeWindow)
{
    int32_t fenceFd = -1;
    struct Region *region = new Region();
    struct Region::Rect *rect = new Region::Rect();
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;
    region->rectNumber = 1;
    struct NativeWindowBuffer *nativeWindowBuffer = nullptr;
    int32_t ret;
    for (int32_t i = 0; i < QUEUE_SIZE; i++) {
        ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &nativeWindowBuffer, &fenceFd);
        if (ret != OHOS::GSERROR_OK) {
            delete rect;
            delete region;
            return ret;
        }
        SetData(nativeWindowBuffer, nativeWindow);

        ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, nativeWindowBuffer, -1, *region);
        if (ret != OHOS::GSERROR_OK) {
            delete rect;
            delete region;
            return ret;
        }
    }
    delete rect;
    delete region;
    return OHOS::GSERROR_OK;
}

int32_t NativeWindowTest::CreateNativeWindowAndRequestBuffer004(uint64_t uniqueId, NativeWindow **nativeWindow)
{
    int32_t ret = OH_NativeWindow_CreateNativeWindowFromSurfaceId(uniqueId, nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        return ret;
    }
    struct NativeWindowBuffer *nativeWindowBuffer = nullptr;

    int32_t code = SET_BUFFER_GEOMETRY;
    int32_t height = 0x100;
    int32_t width = 0x100;
    OH_NativeWindow_NativeWindowHandleOpt(*nativeWindow, code, height, width);
    code = SET_FORMAT;
    int32_t format = GRAPHIC_PIXEL_FMT_RGBA_8888;
    OH_NativeWindow_NativeWindowHandleOpt(*nativeWindow, code, format);

    int32_t fenceFd = -1;
    struct Region *region = new Region();
    struct Region::Rect *rect = new Region::Rect();
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;
    region->rectNumber = 1;
    for (int32_t i = 0; i < QUEUE_SIZE; i++) {
        ret = OH_NativeWindow_NativeWindowRequestBuffer(*nativeWindow, &nativeWindowBuffer, &fenceFd);
        if (ret != OHOS::GSERROR_OK) {
            delete rect;
            delete region;
            return ret;
        }
        SetData(nativeWindowBuffer, *nativeWindow);

        ret = OH_NativeWindow_NativeWindowFlushBuffer(*nativeWindow, nativeWindowBuffer, -1, *region);
        if (ret != OHOS::GSERROR_OK) {
            delete rect;
            delete region;
            return ret;
        }
    }
    delete rect;
    delete region;
    return OHOS::GSERROR_OK;
}

int32_t NativeWindowTest::ThreadNativeWindowProcess004(int32_t *pipeFd, uint64_t uniqueId)
{
    int64_t data;
    NativeWindow *nativeWindow = nullptr;
    int32_t ret = CreateNativeWindowAndRequestBuffer004(uniqueId, &nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        return -1;
    }

    data = ret;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    ret = RequestBuffer001(nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        return -1;
    }
    data = ret;
    write(pipeFd[1], &data, sizeof(data));
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);
    return 0;
}

HWTEST_F(NativeWindowTest, OH_NativeWindow_CreateNativeWindowFromSurfaceId002, Function | MediumTest | Level2)
{
    int32_t pipeFd[2] = {};
    pipe(pipeFd);

    sptr<OHOS::IConsumerSurface> cSurface =
        IConsumerSurface::Create("OH_NativeWindow_CreateNativeWindowFromSurfaceId002");
    cSurface->RegisterConsumerListener(this);
    auto producer = cSurface->GetProducer();
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
    EXPECT_NE(pSurface, nullptr);
    EXPECT_EQ(cSurface->SetQueueSize(QUEUE_SIZE), OHOS::GSERROR_OK);

    uint64_t uniqueId = cSurface->GetUniqueId();
    std::thread thread([this, pipeFd, uniqueId]() {
        int32_t ret = this->ThreadNativeWindowProcess004((int32_t*)(pipeFd), uniqueId);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
    });

    int64_t data = 0;
    read(pipeFd[0], &data, sizeof(data));
    EXPECT_EQ(data, OHOS::GSERROR_OK);

    OHOS::sptr<SurfaceBuffer> buffer = nullptr;
    int32_t fence = -1;
    int64_t timestamp;
    Rect damage;
    EXPECT_EQ(g_onBufferAvailable_, QUEUE_SIZE);
    while (g_onBufferAvailable_ > 0) {
        auto ret = cSurface->AcquireBuffer(buffer, fence, timestamp, damage);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
        EXPECT_NE(buffer, nullptr);
        EXPECT_EQ(GetData(buffer), true);

        ret = cSurface->ReleaseBuffer(buffer, -1);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
        g_onBufferAvailable_--;
    }

    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    EXPECT_EQ(data, OHOS::GSERROR_OK);
    EXPECT_EQ(g_onBufferAvailable_, QUEUE_SIZE);
    g_onBufferAvailable_ = 0;
    if (thread.joinable()) {
        thread.join();
    }
    close(pipeFd[0]);
    close(pipeFd[1]);
    producer = nullptr;
    cSurface = nullptr;
}

int32_t NativeWindowTest::CreateNativeWindowAndRequestBuffer005(uint64_t uniqueId, NativeWindow **nativeWindow)
{
    int32_t ret = OH_NativeWindow_CreateNativeWindowFromSurfaceId(uniqueId, nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        return ret;
    }
    
    struct NativeWindowBuffer *nativeWindowBuffer = nullptr;
    struct NativeWindowBuffer *lastNativeWindowBuffer = nullptr;

    int32_t code = SET_BUFFER_GEOMETRY;
    int32_t height = 0x100;
    int32_t width = 0x100;
    OH_NativeWindow_NativeWindowHandleOpt(*nativeWindow, code, height, width);
    code = SET_FORMAT;
    int32_t format = GRAPHIC_PIXEL_FMT_RGBA_8888;
    OH_NativeWindow_NativeWindowHandleOpt(*nativeWindow, code, format);

    int32_t fenceFd = -1;
    struct Region *region = new Region();
    struct Region::Rect *rect = new Region::Rect();
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;
    region->rectNumber = 1;
    float matrix[16] = {0};
    for (int32_t i = 0; i < QUEUE_SIZE; i++) {
        ret = OH_NativeWindow_NativeWindowRequestBuffer(*nativeWindow, &nativeWindowBuffer, &fenceFd);
        if (ret != OHOS::GSERROR_OK) {
            delete rect;
            delete region;
            return ret;
        }
        SetData(nativeWindowBuffer, *nativeWindow);

        ret = OH_NativeWindow_NativeWindowFlushBuffer(*nativeWindow, nativeWindowBuffer, -1, *region);
        if (ret != OHOS::GSERROR_OK) {
            delete rect;
            delete region;
            return ret;
        }
        ret = OH_NativeWindow_GetLastFlushedBufferV2(*nativeWindow, &lastNativeWindowBuffer, &fenceFd, matrix);
        if (ret != OHOS::GSERROR_OK) {
            delete rect;
            delete region;
            return ret;
        }
        if (!GetData(lastNativeWindowBuffer->sfbuffer)) {
            delete rect;
            delete region;
            return -1;
        }
    }
    delete rect;
    delete region;
    return OHOS::GSERROR_OK;
}

int32_t NativeWindowTest::ThreadNativeWindowProcess005(int32_t *pipeFd, uint64_t uniqueId)
{
    int64_t data;
    NativeWindow *nativeWindow = nullptr;
    int32_t ret = CreateNativeWindowAndRequestBuffer005(uniqueId, &nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        return -1;
    }

    data = ret;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    ret = RequestBuffer001(nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        return -1;
    }
    data = ret;
    write(pipeFd[1], &data, sizeof(data));
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);
    return 0;
}

HWTEST_F(NativeWindowTest, OH_NativeWindow_GetLastFlushedBufferV2001, Function | MediumTest | Level2)
{
    int32_t pipeFd[2] = {};
    pipe(pipeFd);

    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create("OH_NativeWindow_GetLastFlushedBufferV2001");
    cSurface->RegisterConsumerListener(this);
    auto producer = cSurface->GetProducer();
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
    EXPECT_NE(pSurface, nullptr);
    EXPECT_EQ(cSurface->SetQueueSize(QUEUE_SIZE), OHOS::GSERROR_OK);

    uint64_t uniqueId = cSurface->GetUniqueId();
    std::thread thread([this, pipeFd, uniqueId]() {
        int32_t ret = this->ThreadNativeWindowProcess005((int32_t*)(pipeFd), uniqueId);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
    });

    int64_t data = 0;
    read(pipeFd[0], &data, sizeof(data));
    EXPECT_EQ(data, OHOS::GSERROR_OK);

    OHOS::sptr<SurfaceBuffer> buffer = nullptr;
    int32_t fence = -1;
    int64_t timestamp;
    Rect damage;
    EXPECT_EQ(g_onBufferAvailable_, QUEUE_SIZE);
    while (g_onBufferAvailable_ > 0) {
        auto ret = cSurface->AcquireBuffer(buffer, fence, timestamp, damage);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
        EXPECT_NE(buffer, nullptr);
        EXPECT_EQ(GetData(buffer), true);

        ret = cSurface->ReleaseBuffer(buffer, -1);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
        g_onBufferAvailable_--;
    }

    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    EXPECT_EQ(data, OHOS::GSERROR_OK);
    EXPECT_EQ(g_onBufferAvailable_, QUEUE_SIZE);
    g_onBufferAvailable_ = 0;
    if (thread.joinable()) {
        thread.join();
    }
    close(pipeFd[0]);
    close(pipeFd[1]);
    producer = nullptr;
    cSurface = nullptr;
}

int32_t NativeWindowTest::ThreadNativeWindowProcess006(int32_t *pipeFd, uint64_t uniqueId)
{
    int64_t data;
    NativeWindow *nativeWindow = nullptr;
    int32_t ret = CreateNativeWindowAndRequestBuffer005(uniqueId, &nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        return -1;
    }
    ret = OH_NativeWindow_NativeObjectReference(nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        return -1;
    }
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);

    data = ret;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    ret = RequestBuffer001(nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        return -1;
    }
    data = ret;
    write(pipeFd[1], &data, sizeof(data));
    ret = OH_NativeWindow_NativeObjectUnreference(nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        return -1;
    }
    return 0;
}

HWTEST_F(NativeWindowTest, OH_NativeWindow_NativeObjectReference001, Function | MediumTest | Level2)
{
    int32_t pipeFd[2] = {};
    pipe(pipeFd);

    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create("OH_NativeWindow_NativeObjectReference001");
    cSurface->RegisterConsumerListener(this);
    auto producer = cSurface->GetProducer();
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
    EXPECT_NE(pSurface, nullptr);
    EXPECT_EQ(cSurface->SetQueueSize(QUEUE_SIZE), OHOS::GSERROR_OK);

    uint64_t uniqueId = cSurface->GetUniqueId();
    std::thread thread([this, pipeFd, uniqueId]() {
        int32_t ret = this->ThreadNativeWindowProcess006((int32_t*)(pipeFd), uniqueId);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
    });

    int64_t data = 0;
    read(pipeFd[0], &data, sizeof(data));
    EXPECT_EQ(data, OHOS::GSERROR_OK);

    OHOS::sptr<SurfaceBuffer> buffer = nullptr;
    int32_t fence = -1;
    int64_t timestamp;
    Rect damage;
    EXPECT_EQ(g_onBufferAvailable_, QUEUE_SIZE);
    while (g_onBufferAvailable_ > 0) {
        auto ret = cSurface->AcquireBuffer(buffer, fence, timestamp, damage);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
        EXPECT_NE(buffer, nullptr);
        EXPECT_EQ(GetData(buffer), true);

        ret = cSurface->ReleaseBuffer(buffer, -1);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
        g_onBufferAvailable_--;
    }

    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    EXPECT_EQ(data, OHOS::GSERROR_OK);
    EXPECT_EQ(g_onBufferAvailable_, QUEUE_SIZE);
    g_onBufferAvailable_ = 0;
    if (thread.joinable()) {
        thread.join();
    }
    close(pipeFd[0]);
    close(pipeFd[1]);
    producer = nullptr;
    cSurface = nullptr;
}

int32_t NativeWindowTest::CreateNativeWindowAndRequestBuffer007(sptr<IBufferProducer> producer,
    NativeWindow **nativeWindow)
{
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
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

int32_t NativeWindowTest::ThreadNativeWindowProcess007(int32_t *pipeFd,
    sptr<IBufferProducer> producer, uint64_t *uniqueId)
{
    int64_t data;
    NativeWindow *nativeWindow = nullptr;
    int32_t ret = CreateNativeWindowAndRequestBuffer007(producer, &nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        return -1;
    }
    ret = OH_NativeWindow_GetSurfaceId(nativeWindow, uniqueId);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        return -1;
    }

    data = ret;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);
    return 0;
}

HWTEST_F(NativeWindowTest, OH_NativeWindow_GetSurfaceId001, Function | MediumTest | Level2)
{
    int32_t pipeFd[2] = {};
    pipe(pipeFd);

    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create("OH_NativeWindow_CreateNativeWindow002");
    cSurface->RegisterConsumerListener(this);
    auto producer = cSurface->GetProducer();
    uint64_t cUniqueId = cSurface->GetUniqueId();

    std::thread thread([this, pipeFd, producer, cUniqueId]() {
        uint64_t uniqueId = 0;
        int32_t ret = this->ThreadNativeWindowProcess007((int32_t*)(pipeFd), producer, &uniqueId);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
        EXPECT_EQ(uniqueId, cUniqueId);
    });

    int64_t data = 0;
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
    if (thread.joinable()) {
        thread.join();
    }
    close(pipeFd[0]);
    close(pipeFd[1]);
    producer = nullptr;
    cSurface = nullptr;
}

int32_t NativeWindowTest::CreateNativeWindowAndAttachBuffer001(NativeWindow *nativeWindow)
{
    sptr<OnBufferAvailableTest> onBufferAvailableTest = new OnBufferAvailableTest();
    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create("CreateNativeWindowAndAttachBuffer001");
    cSurface->RegisterConsumerListener(onBufferAvailableTest);
    auto producer = cSurface->GetProducer();
    cSurface->SetQueueSize(QUEUE_SIZE);
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
    if (pSurface == nullptr) {
        return -1;
    }

    NativeWindow *nativeWindowNew = OH_NativeWindow_CreateNativeWindow(&pSurface);
    if (nativeWindowNew == nullptr) {
        return -1;
    }
    int32_t code = SET_BUFFER_GEOMETRY;
    int32_t height = 0x100;
    int32_t width = 0x100;
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindowNew, code, height, width);
    int32_t fenceFd = -1;
    struct Region *region = new Region();
    struct Region::Rect *rect = new Region::Rect();
    rect->w = 0x100;
    rect->h = 0x100;
    region->rects = rect;
    region->rectNumber = 1;
    struct NativeWindowBuffer *nativeWindowBuffer = nullptr;
    int32_t ret;
    for (int32_t i = 0; i < QUEUE_SIZE; i++) {
        ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowNew, &nativeWindowBuffer, &fenceFd);
        if (ret != OHOS::GSERROR_OK) {
            delete rect;
            delete region;
            return ret;
        }
        SetData(nativeWindowBuffer, nativeWindowNew);
        ret = OH_NativeWindow_NativeWindowDetachBuffer(nativeWindowNew, nativeWindowBuffer);
        if (ret != OHOS::GSERROR_OK) {
            delete rect;
            delete region;
            return ret;
        }
        ret = OH_NativeWindow_NativeWindowAttachBuffer(nativeWindow, nativeWindowBuffer);
        if (ret != OHOS::GSERROR_OK) {
            delete rect;
            delete region;
            return ret;
        }
        ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, nativeWindowBuffer, -1, *region);
        if (ret != OHOS::GSERROR_OK) {
            delete rect;
            delete region;
            return ret;
        }
    }
    delete rect;
    delete region;

    OH_NativeWindow_DestroyNativeWindow(nativeWindowNew);
    return OHOS::GSERROR_OK;
}

int32_t NativeWindowTest::CreateNativeWindowAndRequestBuffer008(uint64_t uniqueId, NativeWindow **nativeWindow)
{
    int32_t ret = OH_NativeWindow_CreateNativeWindowFromSurfaceId(uniqueId, nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        return ret;
    }

    int32_t code = SET_BUFFER_GEOMETRY;
    int32_t height = 0x100;
    int32_t width = 0x100;
    OH_NativeWindow_NativeWindowHandleOpt(*nativeWindow, code, height, width);
    code = SET_FORMAT;
    int32_t format = GRAPHIC_PIXEL_FMT_RGBA_8888;
    OH_NativeWindow_NativeWindowHandleOpt(*nativeWindow, code, format);

    return CreateNativeWindowAndAttachBuffer001(*nativeWindow);
}

int32_t NativeWindowTest::ThreadNativeWindowProcess008(int32_t *pipeFd, uint64_t uniqueId)
{
    int64_t data;
    NativeWindow *nativeWindow = nullptr;
    int32_t ret = CreateNativeWindowAndRequestBuffer008(uniqueId, &nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        return -1;
    }
    ret = OH_NativeWindow_NativeObjectReference(nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        return -1;
    }
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);

    data = ret;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    ret = RequestBuffer001(nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        data = ret;
        write(pipeFd[1], &data, sizeof(data));
        return -1;
    }
    data = ret;
    write(pipeFd[1], &data, sizeof(data));
    ret = OH_NativeWindow_NativeObjectUnreference(nativeWindow);
    if (ret != OHOS::GSERROR_OK) {
        return -1;
    }
    return 0;
}

HWTEST_F(NativeWindowTest, OH_NativeWindow_NativeWindowAttachBuffer001, Function | MediumTest | Level2)
{
    int32_t pipeFd[2] = {};
    pipe(pipeFd);

    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create("OH_NativeWindow_NativeWindowAttachBuffer001");
    cSurface->RegisterConsumerListener(this);
    auto producer = cSurface->GetProducer();
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
    EXPECT_NE(pSurface, nullptr);
    EXPECT_EQ(cSurface->SetQueueSize(QUEUE_SIZE), OHOS::GSERROR_OK);

    uint64_t uniqueId = cSurface->GetUniqueId();
    std::thread thread([this, pipeFd, uniqueId]() {
        int32_t ret = this->ThreadNativeWindowProcess008((int32_t*)(pipeFd), uniqueId);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
    });

    int64_t data = 0;
    read(pipeFd[0], &data, sizeof(data));
    EXPECT_EQ(data, OHOS::GSERROR_OK);

    OHOS::sptr<SurfaceBuffer> buffer = nullptr;
    int32_t fence = -1;
    int64_t timestamp;
    Rect damage;
    EXPECT_EQ(g_onBufferAvailable_, QUEUE_SIZE);
    while (g_onBufferAvailable_ > 0) {
        auto ret = cSurface->AcquireBuffer(buffer, fence, timestamp, damage);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
        EXPECT_NE(buffer, nullptr);
        EXPECT_EQ(GetData(buffer), true);

        ret = cSurface->ReleaseBuffer(buffer, -1);
        EXPECT_EQ(ret, OHOS::GSERROR_OK);
        g_onBufferAvailable_--;
    }

    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    EXPECT_EQ(data, OHOS::GSERROR_OK);
    EXPECT_EQ(g_onBufferAvailable_, QUEUE_SIZE);
    g_onBufferAvailable_ = 0;
    if (thread.joinable()) {
        thread.join();
    }
    close(pipeFd[0]);
    close(pipeFd[1]);
    producer = nullptr;
    cSurface = nullptr;
}

/*
 * CaseDescription: 1. preSetup: create native window
 *                  2. operation: set native window format is NATIVEBUFFER_PIXEL_FMT_RGBA16_FLOAT and request buffer
 *                  3. result: The requested buffer format attribute is NATIVEBUFFER_PIXEL_FMT_RGBA16_FLOAT,
 *                     and NATIVEBUFFER_PIXEL_FMT_RGBA16_FLOAT equal GRAPHIC_PIXEL_FMT_RGBA16_FLOAT
 */
HWTEST_F(NativeWindowTest, OH_NativeWindow_NativeWindowSetFormat001, Function | MediumTest | Level2)
{
    // preSetup
    sptr<IConsumerSurface> cSurface = IConsumerSurface::Create("SurfaceB");
    cSurface->RegisterConsumerListener(new OnBufferAvailableTest());
    auto producer = cSurface->GetProducer();
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
    EXPECT_NE(pSurface, nullptr);
    NativeWindow *nativeWindowNew = OH_NativeWindow_CreateNativeWindow(&pSurface);
    int32_t code = SET_BUFFER_GEOMETRY;
    int32_t height = 0x100;
    int32_t width = 0x100;
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindowNew, code, height, width);

    // operation
    code = SET_FORMAT;
    int32_t format = 10;
    EXPECT_EQ(NATIVEBUFFER_PIXEL_FMT_RGBA16_FLOAT - GRAPHIC_PIXEL_FMT_RGBA16_FLOAT, 0);
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindowNew, code, format);

    struct NativeWindowBuffer *nativeWindowBuffer = nullptr;
    int32_t fenceFd = -1;
    auto ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowNew, &nativeWindowBuffer, &fenceFd);
    
    // result
    if (ret == GSERROR_OK) {
        auto bufferHandle = OH_NativeWindow_GetBufferHandleFromNative(nativeWindowBuffer);
        EXPECT_EQ(bufferHandle->format, format);
    }
}
}
