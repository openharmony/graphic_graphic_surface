/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class SurfaceIPCWithConnectStrictlyTest : public testing::Test, public IBufferConsumerListenerClazz {
public:
    static void SetUpTestCase();
    void OnBufferAvailable() override;
    bool GetData(sptr<SurfaceBuffer> &buffer);
    pid_t ChildProcessMain();
    sptr<OHOS::Surface> CreateSurface();

    static inline sptr<IConsumerSurface> cSurface = nullptr;
    static inline int32_t pipeRead[2] = {};
    static inline int32_t pipeWrite[2] = {};
    static inline int32_t ipcSystemAbilityID = 34156;
    static inline BufferRequestConfig requestConfig = {};
    static inline BufferFlushConfig flushConfig = {};
};

void SurfaceIPCWithConnectStrictlyTest::SetUpTestCase()
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
    flushConfig = { .damage = {
        .w = 0x100,
        .h = 0x100,
    } };
}

void SurfaceIPCWithConnectStrictlyTest::OnBufferAvailable()
{
}

static inline GSError OnBufferRelease(sptr<SurfaceBuffer> &buffer)
{
    return GSERROR_OK;
}

sptr<OHOS::Surface> SurfaceIPCWithConnectStrictlyTest::CreateSurface()
{
    sptr<IRemoteObject> robj = nullptr;
    while (true) {
        auto sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        robj = sam->GetSystemAbility(ipcSystemAbilityID);
        if (robj != nullptr) {
            break;
        }
        sleep(0);
    }

    auto producer = iface_cast<IBufferProducer>(robj);
    return Surface::CreateSurfaceAsProducer(producer);
}

pid_t SurfaceIPCWithConnectStrictlyTest::ChildProcessMain()
{
    pipe(pipeRead);
    pipe(pipeWrite);
    pid_t pid = fork();
    if (pid != 0) {
        return pid;
    }

    int64_t data;
    read(pipeRead[0], &data, sizeof(data));

    auto pSurface = CreateSurface();
    pSurface->RegisterReleaseListener(OnBufferRelease);
    int releaseFence = -1;

    // Branch1: producer Surface Request Buffer failed with GSERROR_CONSUMER_DISCONNECTED after disconnect strictly,
    //          but producer Surface Request Buffer success after enable connect strictly.
    pSurface->DisconnectStrictly();
    pSurface->Disconnect();
    sptr<SurfaceBuffer> buffer1 = nullptr;
    auto sRet = pSurface->RequestBuffer(buffer1, releaseFence, requestConfig);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    pSurface->ConnectStrictly();
    sRet = pSurface->RequestBuffer(buffer1, releaseFence, requestConfig);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    sRet = pSurface->FlushBuffer(buffer1, -1, flushConfig);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    data = sRet;
    write(pipeWrite[1], &data, sizeof(data));
    read(pipeRead[0], &data, sizeof(data));

    // Branch2: producer request buffer success when ConnectStricyly(), flush buffer failed with
    //          GSERROR_CONSUMER_DISCONNECTED after disconnect strictly, but producer flush buffer success after
    //          connect strictly.

    sptr<SurfaceBuffer> buffer2 = nullptr;
    pSurface->ConnectStrictly();
    sRet = pSurface->RequestBuffer(buffer2, releaseFence, requestConfig);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    pSurface->DisconnectStrictly();
    sRet = pSurface->FlushBuffer(buffer2, -1, flushConfig);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    pSurface->ConnectStrictly();
    sRet = pSurface->FlushBuffer(buffer2, -1, flushConfig);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    data = sRet;
    write(pipeWrite[1], &data, sizeof(data));
    read(pipeRead[0], &data, sizeof(data));

    write(pipeWrite[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeRead[0], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    pSurface->UnRegisterReleaseListener();
    close(pipeRead[0]);
    close(pipeRead[1]);
    close(pipeWrite[0]);
    close(pipeWrite[1]);
    exit(0);
    return 0;
}

/*
* Function: produce and consumer surface by IPC
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* Case1Description:
*         Branch1： 1. PreSet: create producer surface and disconnect strictly
*                   2. Operation: the producer failed to request buffer with error code GSERROR_CONSUMER_DISCONNECTED,
*                                 but succeeded after being connected strictly.
*         Branch2:  1. PreSet: producer connect strictly on request buffer and disconnect strictly to flush buffer
*                   2. Operation: the producer failed to flush buffer with error code GSERROR_CONSUMER_DISCONNECTED,
*                                 but succeeded after being connected strictly.
*/
HWTEST_F(SurfaceIPCWithConnectStrictlyTest, BufferIPC001, Function | MediumTest | Level2)
{
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

    int64_t data = 0;
    write(pipeRead[1], &data, sizeof(data));
    read(pipeWrite[0], &data, sizeof(data));
    EXPECT_EQ(data, OHOS::GSERROR_OK);

    // requested 3 buffer in 3 branch
    for (int i = 0 ; i < 2 ; i++) {
        sptr<SurfaceBuffer> buffer = nullptr;
        int32_t fence = -1;
        int64_t timestamp;
        Rect damage;
        auto sRet = cSurface->AcquireBuffer(buffer, fence, timestamp, damage);
        EXPECT_EQ(sRet, OHOS::GSERROR_OK);
        EXPECT_NE(buffer, nullptr);
        write(pipeRead[1], &data, sizeof(data));
        read(pipeWrite[0], &data, sizeof(data));
        EXPECT_EQ(data, OHOS::GSERROR_OK);
    }

    //close resource
    write(pipeRead[1], &data, sizeof(data));
    close(pipeRead[0]);
    close(pipeRead[1]);
    close(pipeWrite[0]);
    close(pipeWrite[1]);
    sam->RemoveSystemAbility(ipcSystemAbilityID);
    int32_t ret = 0;
    do {
        waitpid(pid, nullptr, 0);
    } while (ret == -1 && errno == EINTR);
}
}
