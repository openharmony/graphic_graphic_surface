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
#include "sync_fence.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class SurfaceBatchOptWithConnectStrictlyTest : public testing::Test, public IBufferConsumerListenerClazz {
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
    static inline uint32_t queueSize = 3;
};

void SurfaceBatchOptWithConnectStrictlyTest::SetUpTestCase()
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
}

void SurfaceBatchOptWithConnectStrictlyTest::OnBufferAvailable()
{
}

static inline GSError OnBufferRelease(sptr<SurfaceBuffer> &buffer)
{
    return GSERROR_OK;
}

sptr<OHOS::Surface> SurfaceBatchOptWithConnectStrictlyTest::CreateSurface()
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

pid_t SurfaceBatchOptWithConnectStrictlyTest::ChildProcessMain()
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

    // batch requst buffer and batch flush buffer need more buffer queue size
    pSurface->SetQueueSize(queueSize);

    auto handleConfig = [](BufferFlushConfigWithDamages &config) -> void {
        config.damages.reserve(1);
        OHOS::Rect damage = {
            .x = 0,
            .y = 0,
            .w = 0x100,
            .h = 0x100
        };
        config.damages.emplace_back(damage);
        config.timestamp = 0;
    };
    
    // Branch1: producer batch request Buffer failed with GSERROR_CONSUMER_DISCONNECTED after disconnect strictly,
    //          but batch producer Request Buffer success after enable connect strictly.
    pSurface->DisconnectStrictly();
    pSurface->Disconnect();
    std::vector<sptr<SurfaceBuffer>> buffers1;
    std::vector<sptr<SyncFence>> releaseFences1;
    auto sRet = pSurface->RequestBuffers(buffers1, releaseFences1, requestConfig);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    pSurface->ConnectStrictly();
    sRet = pSurface->RequestBuffers(buffers1, releaseFences1, requestConfig);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    std::vector<sptr<SyncFence>> flushFences;
    std::vector<BufferFlushConfigWithDamages> configs;
    flushFences.resize(buffers1.size());
    configs.reserve(buffers1.size());
    for (uint32_t i = 0; i < buffers1.size(); ++i) {
        flushFences[i] = new SyncFence(-1);
        BufferFlushConfigWithDamages config;
        handleConfig(config);
        configs.emplace_back(config);
    }
    sRet = pSurface->FlushBuffers(buffers1, flushFences, configs);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    data = sRet;
    write(pipeWrite[1], &data, sizeof(data));
    read(pipeRead[0], &data, sizeof(data));

    // Branch2: producer batch request buffer success when ConnectStricyly(), batch flush buffer failed with
    //          GSERROR_CONSUMER_DISCONNECTED after disconnect strictly, but producer flush buffer success after
    //          connect strictly.

    std::vector<sptr<SurfaceBuffer>> buffers2;
    std::vector<sptr<SyncFence>> releaseFences2;
    pSurface->ConnectStrictly();
    sRet = pSurface->RequestBuffers(buffers2, releaseFences2, requestConfig);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    pSurface->DisconnectStrictly();
    flushFences.resize(buffers2.size());
    configs.reserve(buffers2.size());
    for (uint32_t i = 0; i < buffers2.size(); ++i) {
        flushFences[i] = new SyncFence(-1);
        BufferFlushConfigWithDamages config;
        handleConfig(config);
        configs.emplace_back(config);
    }
    sRet = pSurface->FlushBuffers(buffers2, flushFences, configs);
    EXPECT_EQ(sRet, OHOS::GSERROR_CONSUMER_DISCONNECTED);
    pSurface->ConnectStrictly();
    sRet = pSurface->FlushBuffers(buffers2, flushFences, configs);
    EXPECT_EQ(sRet, OHOS::GSERROR_OK);
    data = sRet;
    write(pipeWrite[1], &data, sizeof(data));
    read(pipeRead[0], &data, sizeof(data));

    write(pipeWrite[1], &data, sizeof(data));
    read(pipeRead[0], &data, sizeof(data));
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
*       Branch1： 1. PreSet: create producer surface and disconnect strictly
*                 2. Operation: producer failed to batch request buffer with error code GSERROR_CONSUMER_DISCONNECTED,
*                                 but succeeded after being connected strictly.
*       Branch2:  1. PreSet: producer connect strictly on request buffer and disconnect strictly to flush buffer
*                 2. Operation: the producer failed to batch flush buffer with error code GSERROR_CONSUMER_DISCONNECTED,
*                                 but succeeded after being connected strictly.
*/
HWTEST_F(SurfaceBatchOptWithConnectStrictlyTest, BufferIPC001, TestSize.Level0)
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

    // i is branch number
    for (int i = 0 ; i < 2 ; i++) {
        for (int j = 0 ; j < queueSize ; j++) {
            sptr<SurfaceBuffer> buffer = nullptr;
            int32_t fence = -1;
            int64_t timestamp;
            Rect damage;
            auto sRet = cSurface->AcquireBuffer(buffer, fence, timestamp, damage);
            EXPECT_EQ(sRet, OHOS::GSERROR_OK);
            EXPECT_NE(buffer, nullptr);
            sRet = cSurface->ReleaseBuffer(buffer, -1);
            EXPECT_EQ(sRet, OHOS::GSERROR_OK);
            data = sRet;
        }
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
