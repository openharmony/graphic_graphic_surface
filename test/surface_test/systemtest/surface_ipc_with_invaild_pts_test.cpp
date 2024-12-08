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
#include "iostream"
using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class SurfaceIPCWithInvalidPTSTest : public testing::Test, public IBufferConsumerListenerClazz {
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
    static inline int64_t desiredPresentTimestamp = 0;
    static constexpr const int32_t WAIT_SYSTEM_ABILITY_GET_PRODUCER_TIMES = 1000;
    static constexpr const int64_t ONE_SECOND_TIMESTAMP = 1e9;
};

void SurfaceIPCWithInvalidPTSTest::SetUpTestCase()
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
    desiredPresentTimestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    flushConfig = {
        .damage = {
            .w = 0x100,
            .h = 0x100,
        },
        .desiredPresentTimestamp = desiredPresentTimestamp,
    };
}

void SurfaceIPCWithInvalidPTSTest::OnBufferAvailable()
{
}

static inline GSError OnBufferRelease(sptr<SurfaceBuffer> &buffer)
{
    return GSERROR_OK;
}

sptr<OHOS::Surface> SurfaceIPCWithInvalidPTSTest::CreateSurface()
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

pid_t SurfaceIPCWithInvalidPTSTest::ChildProcessMain()
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
    int64_t invalidDesiredPresentTimestamp[3] = {-1, desiredPresentTimestamp, desiredPresentTimestamp};
    for (int i = 0; i < bufferNum; i++) {
        auto code = SET_UI_TIMESTAMP;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, static_cast<uint64_t>(desiredPresentTimestamp));
        sRet = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &nativeWindowBuffer, &fenceFd);
        if (sRet != OHOS::GSERROR_OK) {
            data = sRet;
            write(pipeFd[1], &data, sizeof(data));
            exit(0);
        }
        code = SET_DESIRED_PRESENT_TIMESTAMP;
        OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, invalidDesiredPresentTimestamp[i]);
        struct Region *region = new Region();
        struct Region::Rect *rect = new Region::Rect();
        rect->w = 0x100;
        rect->h = 0x100;
        region->rects = rect;
        region->rectNumber = 1;
        sRet = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, nativeWindowBuffer, -1, *region);
        if (sRet != OHOS::GSERROR_OK) {
            data = sRet;
            write(pipeFd[1], &data, sizeof(data));
            exit(0);
        }
    }

    data = sRet;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    if (sRet != OHOS::GSERROR_OK) {
        data = sRet;
        write(pipeFd[1], &data, sizeof(data));
        exit(0);
    }
    pSurface->UnRegisterReleaseListener();
    close(pipeFd[0]);
    close(pipeFd[1]);
    exit(0);
    return 0;
}

/*
* Function: produce and consumer surface by IPC
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. produce surface, fill buffer
*                  2. consume surface and check buffer
*                  3. call RequestBuffer in this process, check sRet and buffer
* @tc.require: issueI5I57K issueI5GMZN issueI5IWHW
 */
HWTEST_F(SurfaceIPCWithInvalidPTSTest, BufferIPC001, Function | MediumTest | Level2)
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

    int64_t data = 3;
    write(pipeFd[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeFd[0], &data, sizeof(data));
    EXPECT_EQ(data, OHOS::GSERROR_OK);

    //消费者消费buffer
    IConsumerSurface::AcquireBufferReturnValue returnValue = {
        .buffer =nullptr,
        .fence = new SyncFence(-1),
    };
    //1 - do not enable pts, and acquire front buffer
    auto sRet = cSurface->AcquireBuffer(returnValue, desiredPresentTimestamp, false);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    EXPECT_NE(returnValue.buffer, nullptr);

    //2 - do not enable pts, and acquire front buffer
    sRet = cSurface->AcquireBuffer(returnValue, desiredPresentTimestamp - ONE_SECOND_TIMESTAMP - 1, false);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    EXPECT_NE(returnValue.buffer, nullptr);

    //3 - do not enable pts, and acquire front buffer
    sRet = cSurface->AcquireBuffer(returnValue, desiredPresentTimestamp, false);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    EXPECT_NE(returnValue.buffer, nullptr);

    //4 - No buffer, make sure that all 3 buffers have been consumed
    sRet = cSurface->AcquireBuffer(returnValue, desiredPresentTimestamp, false);
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
}
