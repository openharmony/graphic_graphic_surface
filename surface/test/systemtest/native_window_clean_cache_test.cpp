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
#include <surface.h>
#include "accesstoken_kit.h"
#include "iconsumer_surface.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "sync_fence.h"
#include "external_window.h"
#include "native_window.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class NativeWindowCleanCacheTest : public testing::Test, public IBufferConsumerListenerClazz {
public:
    static void SetUpTestCase();
    void OnBufferAvailable() override;
    OHOS::GSError SetData(sptr<SurfaceBuffer> &buffer, sptr<Surface> &pSurface);
    bool GetData(sptr<SurfaceBuffer> &buffer);
    pid_t ChildProcessMain();
    sptr<OHOS::Surface> CreateSurface();

    static inline sptr<IConsumerSurface> cSurface = nullptr;
    static inline int32_t pipeFd[2] = {};
    static inline int32_t ipcSystemAbilityID = 34156;
    static inline BufferRequestConfig requestConfig = {};
    static inline BufferFlushConfig flushConfig = {};

    static constexpr const int32_t WAIT_SYSTEM_ABILITY_GET_PRODUCER_TIMES = 1000;
};

void NativeWindowCleanCacheTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << getpid();
    requestConfig = {
        .width = 0x100,  // small
        .height = 0x100, // small
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };

    flushConfig = {
        .damage = {
            .w = 0x100,
            .h = 0x100,
        }
    };
}

void NativeWindowCleanCacheTest::OnBufferAvailable()
{
}

static inline GSError OnBufferRelease(sptr<SurfaceBuffer> &buffer)
{
    return GSERROR_OK;
}

sptr<OHOS::Surface> NativeWindowCleanCacheTest::CreateSurface()
{
    sptr<IRemoteObject> robj = nullptr;
    int i = 0;
    while (i++ < WAIT_SYSTEM_ABILITY_GET_PRODUCER_TIMES) {
        auto sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        robj = sam->GetSystemAbility(ipcSystemAbilityID);
        if (robj != nullptr) {
            break;
        }
        usleep(1);
    }

    auto producer = iface_cast<IBufferProducer>(robj);
    return Surface::CreateSurfaceAsProducer(producer);
}

pid_t NativeWindowCleanCacheTest::ChildProcessMain()
{
    pipe(pipeFd);
    pid_t pid = fork();
    if (pid != 0) {
        return pid;
    }

    int64_t data;
    int64_t bufferNum;
    read(pipeFd[0], &bufferNum, sizeof(bufferNum));

    auto pSurface = CreateSurface();
    auto nativeWindow = OH_NativeWindow_CreateNativeWindow(&pSurface);
    int32_t code = SET_BUFFER_GEOMETRY;
    int32_t height = 0x100;
    int32_t weight = 0x100;
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, height, weight);
    code = SET_FORMAT;
    int32_t format = GRAPHIC_PIXEL_FMT_RGBA_8888;
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, format);
    pSurface->RegisterReleaseListener(OnBufferRelease);

    struct NativeWindowBuffer *nativeWindowBuffer = nullptr;
    int32_t sRet;
    int fenceFd = -1;
    for (int i = 0; i < bufferNum; i++) {
        sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &nativeWindowBuffer, &fenceFd);
        if (sRet != OHOS::GSERROR_OK) {
            std::cout<<"OH_NativeWindow_NativeWindowRequestBuffer ret:"<<sRet<<std::endl;
            data = sRet;
            write(pipeFd[1], &data, sizeof(data));
            exit(0);
        }
        struct Region *region = new Region();
        struct Region::Rect *rect = new Region::Rect();
        rect->w = 0x100;
        rect->h = 0x100;
        region->rects = rect;
        region->rectNumber = 1;
        sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, nativeWindowBuffer, -1, *region);
        if (sRet != OHOS::GSERROR_OK) {
            std::cout<<"OH_NativeWindow_NativeWindowFlushBuffer ret:"<<sRet<<std::endl;
            data = sRet;
            write(pipeFd[1], &data, sizeof(data));
            exit(0);
        }
    }
    sRet = OH_NativeWindow_CleanCache(nativeWindow);
    
    data = sRet;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    pSurface->UnRegisterReleaseListener();
    close(pipeFd[0]);
    close(pipeFd[1]);
    exit(0);
    return 0;
}

/*
* Function: NativeWindowCleanCache
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. preSetUp: native window flush 2 buffer
*                  2. operation: native window clean cache success
*                  3. result: consumer surface acquire buffer failed and no buffer in cache
 */
HWTEST_F(NativeWindowCleanCacheTest, CleanCache001, Function | MediumTest | Level2)
{
    //生产者生产buffer
    auto pid = ChildProcessMain();
    ASSERT_GE(pid, 0);

    uint64_t tokenId;
    const char *perms[2];
    perms[0] = "ohos.permission.DISTRIBUTED_DATASYNC";
    perms[1] = "ohos.permission.CAMERA";
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 2,
        .aclsNum = 0,
        .dcaps = NULL,
        .perms = perms,
        .acls = NULL,
        .processName = "dcamera_client_demo",
        .aplStr = "system_basic",
    };
    tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    int32_t rett = Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    ASSERT_EQ(rett, Security::AccessToken::RET_SUCCESS);
    cSurface = IConsumerSurface::Create("test");
    cSurface->RegisterConsumerListener(this);
    auto producer = cSurface->GetProducer();
    auto sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    sam->AddSystemAbility(ipcSystemAbilityID, producer->AsObject());

    int64_t data = 2;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    EXPECT_EQ(data, OHOS::GSERROR_OK);

    //消费者消费buffer
    IConsumerSurface::AcquireBufferReturnValue returnValue = {
        .buffer =nullptr,
        .fence = new SyncFence(-1),
    };
    //Branch1 - No buffer after clean cache
    auto sRet = cSurface->AcquireBuffer(returnValue, 0, false);
    EXPECT_EQ(sRet, GSERROR_NO_BUFFER);


    //close resource
    write(pipeFd[1], &data, sizeof(data));
    close(pipeFd[0]);
    close(pipeFd[1]);
    sam->RemoveSystemAbility(ipcSystemAbilityID);
    int32_t ret = 0;
    do {
        waitpid(pid, nullptr, 0);
    } while (ret == -1 && errno == EINTR);
}

/*
* Function: NativeWindowCleanCache
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. preSetUp: native window has no connect to comsumer
*                  2. operation: native window clean cache failed 
*                  3. result: failed and return error code GSERROR_CONSUMER_DISCONNECTED
 */
HWTEST_F(NativeWindowCleanCacheTest, CleanCache002, Function | MediumTest | Level2)
{
    auto cSurface = IConsumerSurface::Create("test");
    cSurface->RegisterConsumerListener(this);
    auto producer = cSurface->GetProducer();
    auto pSurface = Surface::CreateSurfaceAsProducer(producer);
    NativeWindow* nativeWindow = nullptr;
    auto ret = OH_NativeWindow_CreateNativeWindowFromSurfaceId(pSurface->GetUniqueId(), &nativeWindow);
    ret = OH_NativeWindow_CleanCache(nativeWindow);
    
    ASSERT_EQ(ret, OHOS::GSERROR_CONSUMER_DISCONNECTED);
}
}
