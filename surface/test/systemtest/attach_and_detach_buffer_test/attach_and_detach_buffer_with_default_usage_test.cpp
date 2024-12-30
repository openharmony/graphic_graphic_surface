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
#ifdef SUPPORT_ACCESS_TOKEN
#include <chrono>
#include <thread>
#include <unistd.h>
#include <gtest/gtest.h>
#include <iservice_registry.h>
#include <surface.h>
#include "accesstoken_kit.h"
#include "iconsumer_surface.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "sync_fence.h"
#include "external_window.h"
#include "native_window.h"
#include "iostream"
#include "buffer_consumer_listener.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class AttachBufferWithDefaultUsageTest : public testing::Test, public IBufferConsumerListenerClazz {
public:
    static void SetUpTestCase();
    void OnBufferAvailable() override;
};

void AttachBufferWithDefaultUsageTest::SetUpTestCase()
{
}

void AttachBufferWithDefaultUsageTest::OnBufferAvailable()
{
}


/*
* Function: AttachAndDetachBufferWithDefaultUsage
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. preSetup: create two surface（surfaceA and surfaceB）, surfaceA set default usage is "1",
*                     surfaceA request 3 buffer with request config usage is "8", and the buffer usage is "8|1" = "9".
*                  2. operation: surfaceA detach 3 buffer with usage is "9" and attach buffer to surfaceB, and
*                     surfaceB request 3 buffer, the requested buffer should be reused, shouled not realloc.
*                  3. result: the preSetup surfaceA buffer’s fd requal operation surfaceB reused buffer's fd
* @tc.require: issueI5I57K issueI5GMZN issueI5IWHW
 */
HWTEST_F(AttachBufferWithDefaultUsageTest, AttachAndDetachBufferWithDefaultUsage001, Function | MediumTest | Level2)
{
    int32_t queueSize = 3;
    // create surface A with default usage "1"
    sptr<IConsumerSurface> cSurfaceA = IConsumerSurface::Create("SurfaceA");
    sptr<IBufferConsumerListener> consumerListener = new BufferConsumerListener();
    cSurfaceA->RegisterConsumerListener(consumerListener);
    sptr<IBufferProducer> producerClient = cSurfaceA->GetProducer();
    sptr<Surface> pSurfaceA = Surface::CreateSurfaceAsProducer(producerClient);
    auto nativeWindowA = OH_NativeWindow_CreateNativeWindow(&pSurfaceA);
    uint64_t defaultUsageA = 1;
    int32_t ret = cSurfaceA->SetDefaultUsage(defaultUsageA);
    ASSERT_EQ(ret, GSERROR_OK);
    cSurfaceA->SetQueueSize(queueSize);

    // create surface B with default usage "0"
    sptr<IConsumerSurface> cSurfaceB = IConsumerSurface::Create("SurfaceB");
    sptr<IBufferConsumerListener> consumerListenerB = new BufferConsumerListener();
    cSurfaceB->RegisterConsumerListener(consumerListenerB);
    sptr<IBufferProducer> producerClientB = cSurfaceB->GetProducer();
    sptr<Surface> pSurfaceB = Surface::CreateSurfaceAsProducer(producerClientB);
    auto nativeWindowB = OH_NativeWindow_CreateNativeWindow(&pSurfaceB);
    cSurfaceB->SetQueueSize(queueSize);

    //surface A request and detach buffer with config usage is 8
    int32_t code = SET_BUFFER_GEOMETRY;
    int32_t height = 0x100;
    int32_t weight = 0x100;
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindowA, code, height, weight);
    ASSERT_EQ(ret, GSERROR_OK);
    code = SET_USAGE;
    uint64_t usageA = 8;
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindowA, code, usageA);
    ASSERT_EQ(ret, GSERROR_OK);
    int fenceFd = -1;
    vector<int32_t> bufferFds = {};
    for (int i = 0; i < queueSize; i++) {
        struct NativeWindowBuffer *nativeWindowBuffer = nullptr;
        ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowA, &nativeWindowBuffer, &fenceFd);
        ASSERT_EQ(ret, GSERROR_OK);
        ret = OH_NativeWindow_NativeWindowDetachBuffer(nativeWindowA, nativeWindowBuffer);
        ASSERT_EQ(ret, GSERROR_OK);
        ret = OH_NativeWindow_NativeWindowAttachBuffer(nativeWindowB, nativeWindowBuffer);
        ASSERT_EQ(ret, GSERROR_OK);
        struct Region *region = new Region();
        struct Region::Rect *rect = new Region::Rect();
        rect->w = 0x100;
        rect->h = 0x100;
        region->rects = rect;
        region->rectNumber = 1;
        ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindowB, nativeWindowBuffer, -1, *region);
        ASSERT_EQ(ret, GSERROR_OK);
        auto bufferHandle = OH_NativeWindow_GetBufferHandleFromNative(nativeWindowBuffer);
        bufferFds.push_back(bufferHandle->fd);
        ASSERT_EQ(bufferHandle->usage, usageA | defaultUsageA);
    }
    

    //surface B request buffer with config usage "8 | 1" = "9" and should not realloc buffer
    code = SET_BUFFER_GEOMETRY;
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindowB, code, height, weight);
    ASSERT_EQ(ret, GSERROR_OK);
    for (int i = 0; i < queueSize; i++) {
        //consumer buffer
        sptr<SurfaceBuffer> buffer = nullptr;
        int64_t timestamp;
        Rect damage;
        ret = cSurfaceB->AcquireBuffer(buffer, fenceFd, timestamp, damage);
        ASSERT_EQ(ret, GSERROR_OK);
        ret = cSurfaceB->ReleaseBuffer(buffer, fenceFd);
        ASSERT_EQ(ret, GSERROR_OK);

        //reuse buffer
        code = SET_USAGE;
        uint64_t usageB = usageA | defaultUsageA;
        ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindowA, code, usageB);
        ASSERT_EQ(ret, GSERROR_OK);
        struct NativeWindowBuffer *nativeWindowBuffer = nullptr;
        ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowB, &nativeWindowBuffer, &fenceFd);
        ASSERT_EQ(ret, GSERROR_OK);
        auto bufferHandle = OH_NativeWindow_GetBufferHandleFromNative(nativeWindowBuffer);
        bufferFds.push_back(bufferHandle->fd);
        ASSERT_EQ(bufferHandle->usage, usageB);
        ASSERT_EQ(bufferHandle->fd, bufferFds[i]);
    }
}

/*
* Function: AttachAndDetachBufferWithDefaultUsage
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. preSetup: create two surface（surfaceA and surfaceB）, surfaceA set default usage is "1",
*                     surfaceA request 3 buffer with request config usage is "8", and the buffer usage is "8|1" = "9".
*                  2. operation: surfaceA detach 3 buffer with usage is "9" and attach buffer to surfaceB, and
*                     surfaceB request 3 buffer, the requested buffer should be reused, shouled not realloc.
*                  3. result: the preSetup surfaceA buffer’s fd requal operation surfaceB reused buffer's fd
* @tc.require: issueI5I57K issueI5GMZN issueI5IWHW
 */
HWTEST_F(AttachBufferWithDefaultUsageTest, AttachAndDetachBufferWithDefaultUsage002, Function | MediumTest | Level2)
{
    int32_t queueSize = 3;
    // create surface A with default usage "0"
    sptr<IConsumerSurface> cSurfaceA = IConsumerSurface::Create("SurfaceA");
    sptr<IBufferConsumerListener> consumerListener = new BufferConsumerListener();
    cSurfaceA->RegisterConsumerListener(consumerListener);
    sptr<IBufferProducer> producerClient = cSurfaceA->GetProducer();
    sptr<Surface> pSurfaceA = Surface::CreateSurfaceAsProducer(producerClient);
    auto nativeWindowA = OH_NativeWindow_CreateNativeWindow(&pSurfaceA);
    cSurfaceA->SetQueueSize(queueSize);

    // create surface B with default usage "1"
    sptr<IConsumerSurface> cSurfaceB = IConsumerSurface::Create("SurfaceB");
    sptr<IBufferConsumerListener> consumerListenerB = new BufferConsumerListener();
    cSurfaceB->RegisterConsumerListener(consumerListenerB);
    sptr<IBufferProducer> producerClientB = cSurfaceB->GetProducer();
    sptr<Surface> pSurfaceB = Surface::CreateSurfaceAsProducer(producerClientB);
    auto nativeWindowB = OH_NativeWindow_CreateNativeWindow(&pSurfaceB);
    uint64_t defaultUsageB = 1;
    int32_t ret = cSurfaceA->SetDefaultUsage(defaultUsageB);
    ASSERT_EQ(ret, GSERROR_OK);
    cSurfaceB->SetQueueSize(queueSize);

    //surface A request and detach buffer with config usage is "9", attach to surface B and save the buffer fd.
    int32_t code = SET_BUFFER_GEOMETRY;
    int32_t height = 0x100;
    int32_t weight = 0x100;
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindowA, code, height, weight);
    ASSERT_EQ(ret, GSERROR_OK);
    code = SET_USAGE;
    uint64_t usageA = 9;
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindowA, code, usageA);
    ASSERT_EQ(ret, GSERROR_OK);
    int fenceFd = -1;
    vector<int32_t> bufferFds = {};
    for (int i = 0; i < queueSize; i++) {
        
        struct NativeWindowBuffer *nativeWindowBuffer = nullptr;
        ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowA, &nativeWindowBuffer, &fenceFd);
        ASSERT_EQ(ret, GSERROR_OK);
        ret = OH_NativeWindow_NativeWindowDetachBuffer(nativeWindowA, nativeWindowBuffer);
        ASSERT_EQ(ret, GSERROR_OK);
        ret = OH_NativeWindow_NativeWindowAttachBuffer(nativeWindowB, nativeWindowBuffer);
        ASSERT_EQ(ret, GSERROR_OK);
        struct Region *region = new Region();
        struct Region::Rect *rect = new Region::Rect();
        rect->w = 0x100;
        rect->h = 0x100;
        region->rects = rect;
        region->rectNumber = 1;
        ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindowB, nativeWindowBuffer, -1, *region);
        ASSERT_EQ(ret, GSERROR_OK);
        auto bufferHandle = OH_NativeWindow_GetBufferHandleFromNative(nativeWindowBuffer);
        bufferFds.push_back(bufferHandle->fd);
        ASSERT_EQ(bufferHandle->usage, usageA);
    }
    
    //surface B request buffer with config usage "8" and should not realloc buffer with default usage " "8 | 1" = "9"
    code = SET_BUFFER_GEOMETRY;
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindowB, code, height, weight);
    ASSERT_EQ(ret, GSERROR_OK);
    for (int i = 0; i < queueSize; i++) {
        //consumer buffer
        sptr<SurfaceBuffer> buffer = nullptr;
        int64_t timestamp;
        Rect damage;
        ret = cSurfaceB->AcquireBuffer(buffer, fenceFd, timestamp, damage);
        ASSERT_EQ(ret, GSERROR_OK);
        ret = cSurfaceB->ReleaseBuffer(buffer, fenceFd);
        ASSERT_EQ(ret, GSERROR_OK);

        //reuse buffer
        code = SET_USAGE;
        uint64_t usageB = 8;
        ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindowA, code, usageB);
        ASSERT_EQ(ret, GSERROR_OK);
        struct NativeWindowBuffer *nativeWindowBuffer = nullptr;
        ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindowB, &nativeWindowBuffer, &fenceFd);
        ASSERT_EQ(ret, GSERROR_OK);
        auto bufferHandle = OH_NativeWindow_GetBufferHandleFromNative(nativeWindowBuffer);
        bufferFds.push_back(bufferHandle->fd);
        ASSERT_EQ(bufferHandle->usage, usageB | defaultUsageB);
        ASSERT_EQ(bufferHandle->fd, bufferFds[i]);
    }
}
}
#endif