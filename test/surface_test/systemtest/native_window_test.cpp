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
    close(pipeFd[0]);
    close(pipeFd[1]);
    if (thread.joinable()) {
        thread.join();
    }
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
    close(pipeFd[0]);
    close(pipeFd[1]);
    if (thread.joinable()) {
        thread.join();
    }
    producer = nullptr;
    cSurface = nullptr;
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
    close(pipeFd[0]);
    close(pipeFd[1]);
    if (thread.joinable()) {
        thread.join();
    }
    producer = nullptr;
    cSurface = nullptr;
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
    close(pipeFd[0]);
    close(pipeFd[1]);
    if (thread.joinable()) {
        thread.join();
    }
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
    close(pipeFd[0]);
    close(pipeFd[1]);
    if (thread.joinable()) {
        thread.join();
    }
    producer = nullptr;
    cSurface = nullptr;
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
}
