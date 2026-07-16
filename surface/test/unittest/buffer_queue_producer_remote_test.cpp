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
#include <gtest/gtest.h>
#include <iservice_registry.h>
#include <surface.h>
#include <buffer_extra_data_impl.h>
#include <buffer_utils.h>
#include <buffer_queue_producer.h>
#include "buffer_consumer_listener.h"
#include "sync_fence.h"
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "frame_report.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class BufferQueueProducerRemoteTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

    static inline BufferRequestConfig requestConfig = {
        .width = 0x100,
        .height = 0x100,
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    static inline BufferFlushConfigWithDamages flushConfig = {
        .damages = {
            {
                .w = 0x100,
                .h = 0x100,
            }
        },
    };
    static inline std::vector<uint32_t> deletingBuffers;
    static inline int64_t timestamp = 0;
    static inline std::vector<Rect> damages = {};
    static inline sptr<IBufferProducer> bp = nullptr;
    static inline sptr<BufferQueue> bq = nullptr;
    static inline sptr<BufferQueueProducer> bqp = nullptr;
    static inline sptr<BufferExtraData> bedata = nullptr;
};

void BufferQueueProducerRemoteTest::SetUpTestCase()
{
    bq = new BufferQueue("test");
    bqp = new BufferQueueProducer(bq);
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    bq->RegisterConsumerListener(listener);
    bp = bqp;
    bedata = new OHOS::BufferExtraDataImpl;
}

void BufferQueueProducerRemoteTest::TearDownTestCase()
{
    bedata = nullptr;
    bp = nullptr;
    bqp = nullptr;
    bq = nullptr;
}

sptr<SurfaceBuffer> CreateSurfaceBuffer(uint32_t pixelFormat, int32_t width, int32_t height)
{
    auto buffer = SurfaceBuffer::Create();
    if (nullptr == buffer) {
        printf("Create surface buffer failed\n");
        return nullptr;
    }
    BufferRequestConfig inputCfg;
    inputCfg.height = height;
    inputCfg.width = width;
    inputCfg.strideAlignment = width;
    inputCfg.usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE
        | BUFFER_USAGE_HW_RENDER | BUFFER_USAGE_HW_TEXTURE | BUFFER_USAGE_MEM_MMZ_CACHE;
    inputCfg.timeout = 0;
    inputCfg.format = pixelFormat;
    GSError err = buffer->Alloc(inputCfg);
    if (GSERROR_OK != err) {
        printf("Alloc surface buffer failed\n");
        return nullptr;
    }
    return buffer;
}

/*
* Function: AttachAndFlushBufferRemote with connectedPid_ matching activelyPid_
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. set connectedPid_ and activelyPid_ to the same value
*                  2. construct MessageParcel and call AttachAndFlushBufferRemote
*                  3. verify FrameReport active game logic is triggered
*                  4. verify AttachAndFlushBuffer succeeds
*/
HWTEST_F(BufferQueueProducerRemoteTest, AttachAndFlushBufferRemoteWithActiveGame001, TestSize.Level0)
{
    GSError ret;
    bp->SetQueueSize(8);
    int32_t testPid = getpid();
    int32_t origConnectedPid = bqp->connectedPid_;

    bqp->connectedPid_ = testPid;
    Rosen::FrameReport::GetInstance().SetGameScene(testPid, 2);
    EXPECT_TRUE(Rosen::FrameReport::GetInstance().IsActiveGameWithPid(testPid));

    auto buffer = CreateSurfaceBuffer(GRAPHIC_PIXEL_FMT_YCBCR_420_SP, 500, 500);
    ASSERT_NE(buffer, nullptr);
    sptr<SyncFence> fence = SyncFence::INVALID_FENCE;
    bool needMap = false;

    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    ret = WriteSurfaceBufferImpl(arguments, buffer->GetSeqNum(), buffer);
    EXPECT_EQ(ret, GSERROR_OK);
    ret = buffer->WriteBufferRequestConfig(arguments);
    EXPECT_EQ(ret, GSERROR_OK);
    sptr<BufferExtraData> bedataLocal = new BufferExtraDataImpl;
    ret = bedataLocal->WriteToParcel(arguments);
    EXPECT_EQ(ret, GSERROR_OK);
    EXPECT_TRUE(fence->WriteToMessageParcel(arguments));
    ret = WriteFlushConfig(arguments, flushConfig);
    EXPECT_EQ(ret, GSERROR_OK);
    EXPECT_TRUE(arguments.WriteBool(needMap));

    int32_t remoteRet = bqp->AttachAndFlushBufferRemote(arguments, reply, option);
    EXPECT_EQ(remoteRet, ERR_NONE);
    GSError sRet = static_cast<GSError>(reply.ReadInt32());
    EXPECT_EQ(sRet, GSERROR_OK);
    EXPECT_TRUE(Rosen::FrameReport::GetInstance().IsActiveGameWithPid(testPid));

    Rosen::FrameReport::GetInstance().SetGameScene(testPid, 0);
    bqp->connectedPid_ = origConnectedPid;
}

/*
* Function: AttachAndFlushBufferRemote with no active game (activelyPid_ = 0)
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. set connectedPid_ to valid pid, activelyPid_ remains 0
*                  2. construct MessageParcel and call AttachAndFlushBufferRemote
*                  3. verify FrameReport active game logic is not triggered
*                  4. verify AttachAndFlushBuffer still succeeds
*/
HWTEST_F(BufferQueueProducerRemoteTest, AttachAndFlushBufferRemoteWithNoActiveGame001, TestSize.Level0)
{
    GSError ret;
    bp->SetQueueSize(8);
    int32_t testPid = getpid();
    int32_t origConnectedPid = bqp->connectedPid_;

    bqp->connectedPid_ = testPid;
    Rosen::FrameReport::GetInstance().SetGameScene(testPid, 0);
    EXPECT_FALSE(Rosen::FrameReport::GetInstance().IsActiveGameWithPid(testPid));

    auto buffer = CreateSurfaceBuffer(GRAPHIC_PIXEL_FMT_YCBCR_420_SP, 500, 500);
    ASSERT_NE(buffer, nullptr);
    sptr<SyncFence> fence = SyncFence::INVALID_FENCE;
    bool needMap = false;

    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    ret = WriteSurfaceBufferImpl(arguments, buffer->GetSeqNum(), buffer);
    EXPECT_EQ(ret, GSERROR_OK);
    ret = buffer->WriteBufferRequestConfig(arguments);
    EXPECT_EQ(ret, GSERROR_OK);
    sptr<BufferExtraData> bedataLocal = new BufferExtraDataImpl;
    ret = bedataLocal->WriteToParcel(arguments);
    EXPECT_EQ(ret, GSERROR_OK);
    EXPECT_TRUE(fence->WriteToMessageParcel(arguments));
    ret = WriteFlushConfig(arguments, flushConfig);
    EXPECT_EQ(ret, GSERROR_OK);
    EXPECT_TRUE(arguments.WriteBool(needMap));

    int32_t remoteRet = bqp->AttachAndFlushBufferRemote(arguments, reply, option);
    EXPECT_EQ(remoteRet, ERR_NONE);
    GSError sRet = static_cast<GSError>(reply.ReadInt32());
    EXPECT_EQ(sRet, GSERROR_OK);
    EXPECT_FALSE(Rosen::FrameReport::GetInstance().IsActiveGameWithPid(testPid));

    bqp->connectedPid_ = origConnectedPid;
}

/*
* Function: AttachAndFlushBufferRemote with connectedPid_ = 0
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. set connectedPid_ = 0, activelyPid_ to valid pid
*                  2. construct MessageParcel and call AttachAndFlushBufferRemote
*                  3. verify IsActiveGameWithPid(0) returns false (pid <= 0)
*                  4. verify AttachAndFlushBuffer still succeeds
*/
HWTEST_F(BufferQueueProducerRemoteTest, AttachAndFlushBufferRemoteWithZeroPid001, TestSize.Level0)
{
    GSError ret;
    bp->SetQueueSize(8);
    int32_t origConnectedPid = bqp->connectedPid_;

    bqp->connectedPid_ = 0;
    Rosen::FrameReport::GetInstance().SetGameScene(1, 2);
    EXPECT_FALSE(Rosen::FrameReport::GetInstance().IsActiveGameWithPid(0));

    auto buffer = CreateSurfaceBuffer(GRAPHIC_PIXEL_FMT_YCBCR_420_SP, 500, 500);
    ASSERT_NE(buffer, nullptr);
    sptr<SyncFence> fence = SyncFence::INVALID_FENCE;
    bool needMap = false;

    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    ret = WriteSurfaceBufferImpl(arguments, buffer->GetSeqNum(), buffer);
    EXPECT_EQ(ret, GSERROR_OK);
    ret = buffer->WriteBufferRequestConfig(arguments);
    EXPECT_EQ(ret, GSERROR_OK);
    sptr<BufferExtraData> bedataLocal = new BufferExtraDataImpl;
    ret = bedataLocal->WriteToParcel(arguments);
    EXPECT_EQ(ret, GSERROR_OK);
    EXPECT_TRUE(fence->WriteToMessageParcel(arguments));
    ret = WriteFlushConfig(arguments, flushConfig);
    EXPECT_EQ(ret, GSERROR_OK);
    EXPECT_TRUE(arguments.WriteBool(needMap));

    int32_t remoteRet = bqp->AttachAndFlushBufferRemote(arguments, reply, option);
    EXPECT_EQ(remoteRet, ERR_NONE);
    GSError sRet = static_cast<GSError>(reply.ReadInt32());
    EXPECT_EQ(sRet, GSERROR_OK);

    Rosen::FrameReport::GetInstance().SetGameScene(1, 0);
    bqp->connectedPid_ = origConnectedPid;
}

/*
* Function: AttachAndFlushBufferRemote with connectedPid_ != activelyPid_
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. set connectedPid_ and activelyPid_ to different values
*                  2. construct MessageParcel and call AttachAndFlushBufferRemote
*                  3. verify IsActiveGameWithPid returns false (mismatch)
*                  4. verify AttachAndFlushBuffer still succeeds
*/
HWTEST_F(BufferQueueProducerRemoteTest, AttachAndFlushBufferRemotePidMismatch001, TestSize.Level0)
{
    GSError ret;
    bp->SetQueueSize(8);
    int32_t origConnectedPid = bqp->connectedPid_;

    bqp->connectedPid_ = 999;
    Rosen::FrameReport::GetInstance().SetGameScene(888, 2);
    EXPECT_FALSE(Rosen::FrameReport::GetInstance().IsActiveGameWithPid(999));
    EXPECT_TRUE(Rosen::FrameReport::GetInstance().IsActiveGameWithPid(888));

    auto buffer = CreateSurfaceBuffer(GRAPHIC_PIXEL_FMT_YCBCR_420_SP, 500, 500);
    ASSERT_NE(buffer, nullptr);
    sptr<SyncFence> fence = SyncFence::INVALID_FENCE;
    bool needMap = false;

    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    ret = WriteSurfaceBufferImpl(arguments, buffer->GetSeqNum(), buffer);
    EXPECT_EQ(ret, GSERROR_OK);
    ret = buffer->WriteBufferRequestConfig(arguments);
    EXPECT_EQ(ret, GSERROR_OK);
    sptr<BufferExtraData> bedataLocal = new BufferExtraDataImpl;
    ret = bedataLocal->WriteToParcel(arguments);
    EXPECT_EQ(ret, GSERROR_OK);
    EXPECT_TRUE(fence->WriteToMessageParcel(arguments));
    ret = WriteFlushConfig(arguments, flushConfig);
    EXPECT_EQ(ret, GSERROR_OK);
    EXPECT_TRUE(arguments.WriteBool(needMap));

    int32_t remoteRet = bqp->AttachAndFlushBufferRemote(arguments, reply, option);
    EXPECT_EQ(remoteRet, ERR_NONE);
    GSError sRet = static_cast<GSError>(reply.ReadInt32());
    EXPECT_EQ(sRet, GSERROR_OK);

    Rosen::FrameReport::GetInstance().SetGameScene(888, 0);
    bqp->connectedPid_ = origConnectedPid;
}

/*
* Function: SetQueueSize and GetQueueSize
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetQueueSize
*                  2. call SetQueueSize again with abnormal input
*                  3. check ret and call GetQueueSize
 */
HWTEST_F(BufferQueueProducerRemoteTest, QueueSize001, TestSize.Level0)
{
    GSError ret = bp->SetQueueSize(2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bp->SetQueueSize(SURFACE_MAX_QUEUE_SIZE + 1);
    ASSERT_NE(ret, OHOS::GSERROR_OK);

    ASSERT_EQ(bp->GetQueueSize(), 2u);
}

/*
* Function: RequestBuffer, CancelBuffer and AcquireBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer
*                  2. call CancelBuffer
*                  3. call AcquireBuffer and check ret
 */
HWTEST_F(BufferQueueProducerRemoteTest, ReqCan001, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    GSError ret = bp->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bp->CancelBuffer(retval.sequence, bedata);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->AcquireBuffer(retval.buffer, retval.fence, timestamp, damages);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer, CancelBuffer and AcquireBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer
*                  2. call CancelBuffer
*                  3. call CancelBuffer again
*                  4. call AcquireBuffer and check ret
 */
HWTEST_F(BufferQueueProducerRemoteTest, ReqCan002, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    GSError ret = bp->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bp->CancelBuffer(retval.sequence, bedata);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bp->CancelBuffer(retval.sequence, bedata);
    ASSERT_NE(ret, OHOS::GSERROR_OK);

    ret = bq->AcquireBuffer(retval.buffer, retval.fence, timestamp, damages);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer, CancelBuffer and AcquireBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer and CancelBuffer by different retval
*                  2. call AcquireBuffer and check ret
 */
HWTEST_F(BufferQueueProducerRemoteTest, ReqCan003, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval1;
    IBufferProducer::RequestBufferReturnValue retval2;
    IBufferProducer::RequestBufferReturnValue retval3;
    GSError ret;

    ret = bp->RequestBuffer(requestConfig, bedata, retval1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(retval1.buffer, nullptr);

    ret = bp->RequestBuffer(requestConfig, bedata, retval2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_NE(retval2.buffer, nullptr);

    ret = bp->RequestBuffer(requestConfig, bedata, retval3);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(retval3.buffer, nullptr);

    ret = bp->CancelBuffer(retval1.sequence, bedata);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bp->CancelBuffer(retval2.sequence, bedata);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bp->CancelBuffer(retval3.sequence, bedata);
    ASSERT_NE(ret, OHOS::GSERROR_OK);

    ret = bq->AcquireBuffer(retval1.buffer, retval1.fence, timestamp, damages);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer, FlushBuffer, AcquireBuffer and ReleaseBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer and FlushBuffer
*                  2. call AcquireBuffer and ReleaseBuffer
*                  3. call AcquireBuffer again
*                  4. check ret
 */
HWTEST_F(BufferQueueProducerRemoteTest, ReqFlu001, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    GSError ret = bp->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bp->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->AcquireBuffer(retval.buffer, retval.fence, timestamp, damages);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
    ret = bq->ReleaseBuffer(retval.buffer, releaseFence);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->AcquireBuffer(retval.buffer, retval.fence, timestamp, damages);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer, FlushBuffer, AcquireBuffer and ReleaseBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer and FlushBuffer
*                  2. call FlushBuffer again
*                  3. call AcquireBuffer and ReleaseBuffer
*                  4. call AcquireBuffer again
*                  5. check ret
*/
HWTEST_F(BufferQueueProducerRemoteTest, ReqFlu002, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    GSError ret = bp->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bp->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bp->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_NE(ret, OHOS::GSERROR_OK);

    ret = bq->AcquireBuffer(retval.buffer, retval.fence, timestamp, damages);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
    ret = bq->ReleaseBuffer(retval.buffer, releaseFence);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq->AcquireBuffer(retval.buffer, retval.fence, timestamp, damages);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
}

/*
* Function: SyncProducerCache
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SyncProducerCache and check the ret
*/
HWTEST_F(BufferQueueProducerRemoteTest, SyncProducerCache001, TestSize.Level0)
{
    std::map<uint32_t, sptr<SurfaceBuffer>> buffers;
    GSError ret = bp->SyncProducerCache(buffers);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: SyncProducerCache
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. request buffer and flush, then call SyncProducerCache
*/
HWTEST_F(BufferQueueProducerRemoteTest, SyncProducerCache002, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    GSError ret = bp->RequestBuffer(requestConfig, bedata, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bp->FlushBuffer(retval.sequence, bedata, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    std::map<uint32_t, sptr<SurfaceBuffer>> buffers;
    ret = bp->SyncProducerCache(buffers);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: AttachAndFlushBufferRemote with BufferHandle fields tampered
* Type: Security
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. create buffer and tamper with width field
*                 2. call AttachAndFlushBufferRemote
*                 3. verify it returns GSERROR_INVALID_OPERATING
*/
HWTEST_F(BufferQueueProducerRemoteTest, AttachAndFlushBufferRemoteTamperedWidth, TestSize.Level0)
{
    GSError ret;
    bp->SetQueueSize(8);

    auto buffer = CreateSurfaceBuffer(GRAPHIC_PIXEL_FMT_YCBCR_420_SP, 500, 500);
    ASSERT_NE(buffer, nullptr);

    BufferHandle* handle = buffer->GetBufferHandle();
    ASSERT_NE(handle, nullptr);
    int32_t originalWidth = handle->width;
    handle->width = originalWidth + 100;

    sptr<SyncFence> fence = SyncFence::INVALID_FENCE;
    bool needMap = false;

    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    ret = WriteSurfaceBufferImpl(arguments, buffer->GetSeqNum(), buffer);
    EXPECT_EQ(ret, GSERROR_OK);
    ret = buffer->WriteBufferRequestConfig(arguments);
    EXPECT_EQ(ret, GSERROR_OK);
    sptr<BufferExtraData> bedataLocal = new BufferExtraDataImpl;
    ret = bedataLocal->WriteToParcel(arguments);
    EXPECT_EQ(ret, GSERROR_OK);
    EXPECT_TRUE(fence->WriteToMessageParcel(arguments));
    ret = WriteFlushConfig(arguments, flushConfig);
    EXPECT_EQ(ret, GSERROR_OK);
    EXPECT_TRUE(arguments.WriteBool(needMap));

    int32_t remoteRet = bqp->AttachAndFlushBufferRemote(arguments, reply, option);
    EXPECT_EQ(remoteRet, ERR_INVALID_DATA);

    handle->width = originalWidth;
}

/*
* Function: AttachAndFlushBufferRemote with BufferHandle size tampered
* Type: Security
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. create buffer and tamper with size field
*                 2. call AttachAndFlushBufferRemote
*                 3. verify it returns GSERROR_INVALID_OPERATING
*/
HWTEST_F(BufferQueueProducerRemoteTest, AttachAndFlushBufferRemoteTamperedSize, TestSize.Level0)
{
    GSError ret;
    bp->SetQueueSize(8);

    auto buffer = CreateSurfaceBuffer(GRAPHIC_PIXEL_FMT_YCBCR_420_SP, 500, 500);
    ASSERT_NE(buffer, nullptr);

    BufferHandle* handle = buffer->GetBufferHandle();
    ASSERT_NE(handle, nullptr);
    int32_t originalSize = handle->size;
    handle->size = originalSize + 10000;

    sptr<SyncFence> fence = SyncFence::INVALID_FENCE;
    bool needMap = false;

    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    ret = WriteSurfaceBufferImpl(arguments, buffer->GetSeqNum(), buffer);
    EXPECT_EQ(ret, GSERROR_OK);
    ret = buffer->WriteBufferRequestConfig(arguments);
    EXPECT_EQ(ret, GSERROR_OK);
    sptr<BufferExtraData> bedataLocal = new BufferExtraDataImpl;
    ret = bedataLocal->WriteToParcel(arguments);
    EXPECT_EQ(ret, GSERROR_OK);
    EXPECT_TRUE(fence->WriteToMessageParcel(arguments));
    ret = WriteFlushConfig(arguments, flushConfig);
    EXPECT_EQ(ret, GSERROR_OK);
    EXPECT_TRUE(arguments.WriteBool(needMap));

    int32_t remoteRet = bqp->AttachAndFlushBufferRemote(arguments, reply, option);
    EXPECT_EQ(remoteRet, ERR_INVALID_DATA);

    handle->size = originalSize;
}

/*
* Function: AttachAndFlushBufferRemote with BufferHandle not tampered
* Type: Security
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. create buffer without tampering
*                 2. call AttachAndFlushBufferRemote
*                 3. verify it succeeds
*/
HWTEST_F(BufferQueueProducerRemoteTest, AttachAndFlushBufferRemoteNotTampered, TestSize.Level0)
{
    GSError ret;
    bp->SetQueueSize(8);

    auto buffer = CreateSurfaceBuffer(GRAPHIC_PIXEL_FMT_YCBCR_420_SP, 500, 500);
    ASSERT_NE(buffer, nullptr);

    sptr<SyncFence> fence = SyncFence::INVALID_FENCE;
    bool needMap = false;

    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    ret = WriteSurfaceBufferImpl(arguments, buffer->GetSeqNum(), buffer);
    EXPECT_EQ(ret, GSERROR_OK);
    ret = buffer->WriteBufferRequestConfig(arguments);
    EXPECT_EQ(ret, GSERROR_OK);
    sptr<BufferExtraData> bedataLocal = new BufferExtraDataImpl;
    ret = bedataLocal->WriteToParcel(arguments);
    EXPECT_EQ(ret, GSERROR_OK);
    EXPECT_TRUE(fence->WriteToMessageParcel(arguments));
    ret = WriteFlushConfig(arguments, flushConfig);
    EXPECT_EQ(ret, GSERROR_OK);
    EXPECT_TRUE(arguments.WriteBool(needMap));

    int32_t remoteRet = bqp->AttachAndFlushBufferRemote(arguments, reply, option);
    EXPECT_EQ(remoteRet, ERR_NONE);
    GSError sRet = static_cast<GSError>(reply.ReadInt32());
    EXPECT_EQ(sRet, GSERROR_OK);
}

/*
* Function: AttachBufferToQueueRemote with BufferHandle width tampered
* Type: Security
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. create buffer and tamper with width field
*                 2. call AttachBufferToQueueRemote
*                 3. verify it returns GSERROR_INVALID_OPERATING
*/
HWTEST_F(BufferQueueProducerRemoteTest, AttachBufferToQueueRemoteTamperedWidth, TestSize.Level0)
{
    GSError ret;
    bp->SetQueueSize(8);

    auto buffer = CreateSurfaceBuffer(GRAPHIC_PIXEL_FMT_RGBA_8888, 256, 256);
    ASSERT_NE(buffer, nullptr);

    BufferHandle* handle = buffer->GetBufferHandle();
    ASSERT_NE(handle, nullptr);
    int32_t originalWidth = handle->width;
    handle->width = originalWidth + 1000;

    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    ret = WriteSurfaceBufferImpl(arguments, buffer->GetSeqNum(), buffer);
    EXPECT_EQ(ret, GSERROR_OK);
    ret = buffer->WriteBufferRequestConfig(arguments);
    EXPECT_EQ(ret, GSERROR_OK);

    int32_t remoteRet = bqp->AttachBufferToQueueRemote(arguments, reply, option);
    EXPECT_EQ(remoteRet, ERR_INVALID_DATA);

    handle->width = originalWidth;
}

/*
* Function: AttachBufferToQueueRemote with BufferHandle size tampered
* Type: Security
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. create buffer and tamper with size field
*                 2. call AttachBufferToQueueRemote
*                 3. verify it returns GSERROR_INVALID_OPERATING
*/
HWTEST_F(BufferQueueProducerRemoteTest, AttachBufferToQueueRemoteTamperedSize, TestSize.Level0)
{
    GSError ret;
    bp->SetQueueSize(8);

    auto buffer = CreateSurfaceBuffer(GRAPHIC_PIXEL_FMT_RGBA_8888, 256, 256);
    ASSERT_NE(buffer, nullptr);

    BufferHandle* handle = buffer->GetBufferHandle();
    ASSERT_NE(handle, nullptr);
    int32_t originalSize = handle->size;
    handle->size = originalSize + 10000;

    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    ret = WriteSurfaceBufferImpl(arguments, buffer->GetSeqNum(), buffer);
    EXPECT_EQ(ret, GSERROR_OK);
    ret = buffer->WriteBufferRequestConfig(arguments);
    EXPECT_EQ(ret, GSERROR_OK);

    int32_t remoteRet = bqp->AttachBufferToQueueRemote(arguments, reply, option);
    EXPECT_EQ(remoteRet, ERR_INVALID_DATA);

    handle->size = originalSize;
}

/*
* Function: AttachBufferRemote with BufferHandle width tampered
* Type: Security
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. create buffer and tamper with width field
*                 2. call AttachBufferRemote
*                 3. verify it returns GSERROR_INVALID_OPERATING
*/
HWTEST_F(BufferQueueProducerRemoteTest, AttachBufferRemoteTamperedWidth, TestSize.Level0)
{
    GSError ret;
    bp->SetQueueSize(8);

    auto buffer = CreateSurfaceBuffer(GRAPHIC_PIXEL_FMT_RGBA_8888, 256, 256);
    ASSERT_NE(buffer, nullptr);

    BufferHandle* handle = buffer->GetBufferHandle();
    ASSERT_NE(handle, nullptr);
    int32_t originalWidth = handle->width;
    handle->width = originalWidth + 1000;

    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    ret = WriteSurfaceBufferImpl(arguments, buffer->GetSeqNum(), buffer);
    EXPECT_EQ(ret, GSERROR_OK);
    int32_t timeOut = 0;
    EXPECT_TRUE(arguments.WriteInt32(timeOut));

    int32_t remoteRet = bqp->AttachBufferRemote(arguments, reply, option);
    EXPECT_EQ(remoteRet, ERR_INVALID_DATA);

    handle->width = originalWidth;
}

/*
* Function: AttachBufferRemote with BufferHandle size tampered
* Type: Security
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. create buffer and tamper with size field
*                 2. call AttachBufferRemote
*                 3. verify it returns GSERROR_INVALID_OPERATING
*/
HWTEST_F(BufferQueueProducerRemoteTest, AttachBufferRemoteTamperedSize, TestSize.Level0)
{
    GSError ret;
    bp->SetQueueSize(8);

    auto buffer = CreateSurfaceBuffer(GRAPHIC_PIXEL_FMT_RGBA_8888, 256, 256);
    ASSERT_NE(buffer, nullptr);

    BufferHandle* handle = buffer->GetBufferHandle();
    ASSERT_NE(handle, nullptr);
    int32_t originalSize = handle->size;
    handle->size = originalSize + 10000;

    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    ret = WriteSurfaceBufferImpl(arguments, buffer->GetSeqNum(), buffer);
    EXPECT_EQ(ret, GSERROR_OK);
    int32_t timeOut = 0;
    EXPECT_TRUE(arguments.WriteInt32(timeOut));

    int32_t remoteRet = bqp->AttachBufferRemote(arguments, reply, option);
    EXPECT_EQ(remoteRet, ERR_INVALID_DATA);

    handle->size = originalSize;
}
}
