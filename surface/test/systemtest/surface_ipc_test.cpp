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
class SurfaceIPCTest : public testing::Test, public IBufferConsumerListenerClazz {
public:
    static void SetUpTestCase();
    void OnBufferAvailable() override;
    OHOS::GSError SetData(sptr<SurfaceBuffer> &buffer, sptr<Surface> &pSurface);
    bool GetData(sptr<SurfaceBuffer> &buffer);
    pid_t ChildProcessMain();
    sptr<OHOS::Surface> CreateSurface();

    static inline sptr<IConsumerSurface> cSurface = nullptr;
    static inline int32_t pipeMain[2] = {};
    static inline int32_t pipeChild[2] = {};
    static inline int32_t ipcSystemAbilityID = 34156;
    static inline BufferRequestConfig requestConfig = {};
    static inline BufferFlushConfig flushConfig = {};
};

void SurfaceIPCTest::SetUpTestCase()
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

void SurfaceIPCTest::OnBufferAvailable()
{
}

static inline GSError OnBufferRelease(sptr<SurfaceBuffer> &buffer)
{
    return GSERROR_OK;
}

OHOS::GSError SurfaceIPCTest::SetData(sptr<SurfaceBuffer> &buffer, sptr<Surface> &pSurface)
{
    buffer->GetExtraData()->ExtraSet("123", 0x123);
    buffer->GetExtraData()->ExtraSet("345", (int64_t)0x345);
    buffer->GetExtraData()->ExtraSet("567", "567");

    uint32_t reserveInts = 1;
    GraphicExtDataHandle *handle = AllocExtDataHandle(reserveInts);
    handle->reserve[0] = 1;
    OHOS::GSError ret = pSurface->SetTunnelHandle(handle);
    FreeExtDataHandle(handle);
    handle = nullptr;
    return ret;
}

bool SurfaceIPCTest::GetData(sptr<SurfaceBuffer> &buffer)
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

    sptr<SurfaceTunnelHandle> handleGet = nullptr;
    handleGet = cSurface->GetTunnelHandle();
    if ((handleGet == nullptr) || (handleGet->GetHandle()->fd != -1) ||
        (handleGet->GetHandle()->reserveInts != 1) || (handleGet->GetHandle()->reserve[0] != 1)) {
            return false;
    }

    GraphicPresentTimestamp timestamp = {GRAPHIC_DISPLAY_PTS_DELAY, 1};  // mock data for test
    auto sRet = cSurface->SetPresentTimestamp(buffer->GetSeqNum(), timestamp);
    return (sRet == OHOS::GSERROR_OK);
}

sptr<OHOS::Surface> SurfaceIPCTest::CreateSurface()
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

pid_t SurfaceIPCTest::ChildProcessMain()
{
    pipe(pipeMain);
    pipe(pipeChild);
    pid_t pid = fork();
    if (pid != 0) {
        return pid;
    }

    int64_t data;
    read(pipeMain[0], &data, sizeof(data));

    auto pSurface = CreateSurface();
    pSurface->RegisterReleaseListener(OnBufferRelease);
    sptr<SurfaceBuffer> buffer = nullptr;
    int releaseFence = -1;
    auto sRet = pSurface->RequestBuffer(buffer, releaseFence, requestConfig);
    if (sRet != OHOS::GSERROR_OK) {
        data = sRet;
        write(pipeChild[1], &data, sizeof(data));
        exit(0);
    }
    sRet = SetData(buffer, pSurface);
    if (sRet != OHOS::GSERROR_OK) {
        data = sRet;
        write(pipeChild[1], &data, sizeof(data));
        exit(0);
    }

    sRet = pSurface->FlushBuffer(buffer, -1, flushConfig);
    data = sRet;
    write(pipeChild[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeMain[0], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    GraphicPresentTimestampType type = GraphicPresentTimestampType::GRAPHIC_DISPLAY_PTS_DELAY;
    int64_t time = 0;
    sRet = pSurface->GetPresentTimestamp(buffer->GetSeqNum(), type, time);
    if (sRet != OHOS::GSERROR_OK || time != 1) {
        data = sRet;
        write(pipeChild[1], &data, sizeof(data));
        exit(0);
    }
    pSurface->UnRegisterReleaseListener();
    close(pipeMain[0]);
    close(pipeMain[1]);
    close(pipeChild[0]);
    close(pipeChild[1]);
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
HWTEST_F(SurfaceIPCTest, BufferIPC001, TestSize.Level0)
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
    write(pipeMain[1], &data, sizeof(data));
    usleep(1000); // sleep 1000 microseconds (equals 1 milliseconds)
    read(pipeChild[0], &data, sizeof(data));
    EXPECT_EQ(data, OHOS::GSERROR_OK);

    sptr<SurfaceBuffer> buffer = nullptr;
    int32_t fence = -1;
    int64_t timestamp;
    Rect damage;
    auto sRet = cSurface->AcquireBuffer(buffer, fence, timestamp, damage);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    EXPECT_NE(buffer, nullptr);
    EXPECT_EQ(GetData(buffer), true);

    sRet = cSurface->ReleaseBuffer(buffer, -1);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);

    // RequestBuffer cannot be called in two processes
    auto pSurfaceSecond = Surface::CreateSurfaceAsProducer(producer);
    sptr<SurfaceBuffer> bufferSecond = nullptr;
    int releaseFence = -1;
    sRet = pSurfaceSecond->RequestBuffer(bufferSecond, releaseFence, requestConfig);
    ASSERT_EQ(sRet, GSERROR_CONSUMER_IS_CONNECTED);
    ASSERT_EQ(bufferSecond, nullptr);

    //close resource
    write(pipeMain[1], &data, sizeof(data));
    close(pipeMain[0]);
    close(pipeMain[1]);
    close(pipeChild[0]);
    close(pipeChild[1]);
    sam->RemoveSystemAbility(ipcSystemAbilityID);
    int32_t ret = 0;
    do {
        waitpid(pid, nullptr, 0);
    } while (ret == -1 && errno == EINTR);
}

/*
* Function: disconnect
* Type: Function
* Rank: Important(1)
* EnvConditions: N/A
* CaseDescription: 1. call Disconnect in other process, check sRet
 */
HWTEST_F(SurfaceIPCTest, Disconnect001, TestSize.Level0)
{
    cSurface->RegisterConsumerListener(this);
    auto producer = cSurface->GetProducer();
    auto pSurface = Surface::CreateSurfaceAsProducer(producer);
    auto sRet = pSurface->Disconnect();
    ASSERT_EQ(sRet, GSERROR_CONSUMER_DISCONNECTED);  // Disconnect cannot be called in two processes
}
}