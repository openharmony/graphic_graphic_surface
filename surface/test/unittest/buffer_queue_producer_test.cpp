/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include <fcntl.h>
#include <surface.h>
#include <sys/mman.h>
#include <buffer_extra_data_impl.h>
#include <buffer_queue_producer.h>
#include "buffer_consumer_listener.h"
#include "buffer_utils.h"
#include "consumer_surface.h"
#include "frame_report.h"
#include "sync_fence.h"
#include "producer_surface_delegator.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class BufferQueueProducerTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp();
    void TearDown();

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
protected:
    int64_t timestamp_ = 0;
    std::vector<Rect> damages_ = {};
    sptr<BufferQueueProducer> bqp_ = nullptr;
    sptr<BufferQueue> bq_ = nullptr;
    sptr<BufferExtraData> bedata_ = nullptr;
};


void BufferQueueProducerTest::SetUp()
{
    bq_ = new BufferQueue("test");
    sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
    bq_->RegisterConsumerListener(listener);
    if (bqp_ == nullptr) {
        bqp_ = new BufferQueueProducer(bq_);
    }
    if (bedata_ == nullptr) {
        bedata_ = new OHOS::BufferExtraDataImpl;
    }
}

void BufferQueueProducerTest::TearDown()
{
    damages_.clear();
    std::vector<Rect> tmp;
    std::swap(damages_, tmp);
    if (bqp_ != nullptr) {
        bqp_->SetStatus(false);
        bqp_ = nullptr;
    }
    bq_ = nullptr;
    if (bedata_ != nullptr) {
        bedata_ = nullptr;
    }
}

/*
* Function: SetQueueSize and GetQueueSize
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetQueueSize and get default value
*                  2. call SetQueueSize
*                  3. call SetQueueSize again with abnormal value
*                  4. call GetQueueSize for BufferQueueProducer and BufferQueue
*                  5. check ret
 */
HWTEST_F(BufferQueueProducerTest, QueueSize001, TestSize.Level0)
{
    ASSERT_EQ(bqp_->GetQueueSize(), (uint32_t)SURFACE_DEFAULT_QUEUE_SIZE);

    GSError ret = bqp_->SetQueueSize(2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bqp_->SetQueueSize(SURFACE_MAX_QUEUE_SIZE + 1);
    EXPECT_NE(ret, OHOS::GSERROR_OK);

    EXPECT_EQ(bqp_->GetQueueSize(), 2u);
    EXPECT_EQ(bq_->GetQueueSize(), 2u);
}

/*
* Function: SetStatus and GetStatus
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetStatus with false and check get status value
*                  2. call SetStatus with true and check get status value
 */
HWTEST_F(BufferQueueProducerTest, Status001, TestSize.Level0)
{
    bqp_->SetStatus(false);
    EXPECT_EQ(bqp_->GetStatus(), false);
    bqp_->SetStatus(true);
    EXPECT_EQ(bqp_->GetStatus(), true);
}

/*
* Function: RequestBuffer and CancelBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer
*                  2. call CancelBuffer
*                  3. check ret
 */
HWTEST_F(BufferQueueProducerTest, ReqCan001, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    GSError ret = bqp_->RequestBuffer(requestConfig, bedata_, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bqp_->CancelBuffer(retval.sequence, bedata_);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer and CancelBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer
*                  2. call CancelBuffer 2 times
*                  3. check ret
 */
HWTEST_F(BufferQueueProducerTest, ReqCan002, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    GSError ret = bqp_->RequestBuffer(requestConfig, bedata_, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bqp_->CancelBuffer(retval.sequence, bedata_);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bqp_->CancelBuffer(retval.sequence, bedata_);
    ASSERT_NE(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer, and CancelBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer and CancelBuffer by different retval
*                  2. check ret
 */
HWTEST_F(BufferQueueProducerTest, ReqCan003, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval1;
    IBufferProducer::RequestBufferReturnValue retval2;
    IBufferProducer::RequestBufferReturnValue retval3;

    auto ret = bqp_->RequestBuffer(requestConfig, bedata_, retval1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    EXPECT_NE(retval1.buffer, nullptr);

    ret = bqp_->RequestBuffer(requestConfig, bedata_, retval2);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);
    EXPECT_NE(retval2.buffer, nullptr);

    ret = bqp_->RequestBuffer(requestConfig, bedata_, retval3);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);
    EXPECT_NE(retval3.buffer, nullptr);

    ret = bqp_->CancelBuffer(retval1.sequence, bedata_);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    ret = bqp_->CancelBuffer(retval2.sequence, bedata_);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    ret = bqp_->CancelBuffer(retval3.sequence, bedata_);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer, FlushBuffer, AcquireBuffer and ReleaseBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer and FlushBuffer
*                  2. call AcquireBuffer and ReleaseBuffer
*                  3. check ret
 */
HWTEST_F(BufferQueueProducerTest, ReqFlu001, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    GSError ret = bqp_->RequestBuffer(requestConfig, bedata_, retval);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bqp_->FlushBuffer(retval.sequence, bedata_, acquireFence, flushConfig);
    EXPECT_EQ(ret, OHOS::GSERROR_OK);

    ret = bq_->AcquireBuffer(retval.buffer, retval.fence, timestamp_, damages_);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
    ret = bq_->ReleaseBuffer(retval.buffer, releaseFence);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: RequestBuffer, FlushBuffer, AcquireBuffer and ReleaseBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call RequestBuffer and FlushBuffer
*                  2. call FlushBuffer again
*                  3. call AcquireBuffer and ReleaseBuffer
*                  4. check ret
 */
HWTEST_F(BufferQueueProducerTest, ReqFlu002, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retval;
    GSError ret = bqp_->RequestBuffer(requestConfig, bedata_, retval);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bqp_->FlushBuffer(retval.sequence, bedata_, acquireFence, flushConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = bqp_->FlushBuffer(retval.sequence, bedata_, acquireFence, flushConfig);
    ASSERT_NE(ret, OHOS::GSERROR_OK);

    ret = bq_->AcquireBuffer(retval.buffer, retval.fence, timestamp_, damages_);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    sptr<SyncFence> releaseFence = SyncFence::INVALID_FENCE;
    ret = bq_->ReleaseBuffer(retval.buffer, releaseFence);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    int32_t timeOut = 1;
    ret = bqp_->AttachBuffer(retval.buffer, timeOut);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: AttachBuffer and DetachBuffer
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call AttachBuffer and DetachBuffer
* 4. check ret
*/
HWTEST_F(BufferQueueProducerTest, AttachAndDetachBuffer001, TestSize.Level0)
{
    IBufferProducer::RequestBufferReturnValue retVal;
    sptr<SurfaceBuffer> &buffer = retVal.buffer;

    sptr<BufferQueue> bufferQueue = nullptr;
    sptr<BufferQueueProducer> bqpTmp = new BufferQueueProducer(bufferQueue);

    GSError ret = bqpTmp->AttachBuffer(buffer);
    ASSERT_EQ(ret, GSERROR_INVALID_ARGUMENTS);
    ret = bqpTmp->DetachBuffer(buffer);
    ASSERT_EQ(ret, GSERROR_INVALID_ARGUMENTS);
    bqpTmp = nullptr;
}

/*
* Function: AttachAndDetachBufferRemote
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call AttachBufferRemote, DetachBufferRemote
* 4. check ret
*/
HWTEST_F(BufferQueueProducerTest, AttachAndDetachBufferRemote, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteInt32(5); // write sequence
    MessageParcel reply;
    reply.WriteInt32(6);
    MessageOption option;
    int32_t ret = bqp_->AttachBufferRemote(arguments, reply, option);
    ASSERT_EQ(ret, ERR_INVALID_DATA);
    ret = bqp_->DetachBufferRemote(arguments, reply, option);
    ASSERT_EQ(ret, ERR_NONE);
}

/*
* Function: SetTunnelHandle
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetTunnelHandle
* 4. check ret
*/
HWTEST_F(BufferQueueProducerTest, SetTunnelHandle, TestSize.Level0)
{
    EXPECT_EQ(bqp_->SetTunnelHandle(nullptr), GSERROR_OK);
    GraphicExtDataHandle *handle = static_cast<GraphicExtDataHandle *>(
        malloc(sizeof(GraphicExtDataHandle) + sizeof(int32_t)));
    handle->fd = -1;
    handle->reserveInts = 1;
    handle->reserve[0] = 0;
    GSError ret = bqp_->SetTunnelHandle(handle);
    EXPECT_EQ(ret, GSERROR_NO_ENTRY);
    free(handle);
}

/*
 * Function: SetTunnelHandleRemote
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetTunnelHandleRemote
 * 4. check ret
 */
HWTEST_F(BufferQueueProducerTest, SetTunnelHandleRemote, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteBool(true);
    MessageParcel reply;
    reply.WriteInt32(6);
    MessageOption option;
    int32_t ret = bqp_->SetTunnelHandleRemote(arguments, reply, option);
    EXPECT_EQ(ret, GSERROR_BINDER);
}

/*
 * Function: SetTunnelHandleRemoteErr001
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetTunnelHandleRemote
 * 4. check ret
 */
HWTEST_F(BufferQueueProducerTest, SetTunnelHandleRemoteErr001, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteBool(true);

    MessageParcel reply;
    MessageOption option;

    uint32_t reserveInts = 5;
    reply.WriteInt32(reserveInts);
    int32_t ret = bqp_->SetTunnelHandleRemote(arguments, reply, option);
    EXPECT_EQ(ret, GSERROR_BINDER);
}

/*
 * Function: SetTunnelHandleRemoteErr002
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetTunnelHandleRemote
 * 4. check ret
 */
HWTEST_F(BufferQueueProducerTest, SetTunnelHandleRemoteErr002, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteBool(true);

    MessageParcel reply;
    MessageOption option;

    uint32_t reserveInts = 5;
    reply.WriteInt32(reserveInts);
    for (uint32_t i = 0; i < reserveInts; i++) {
        arguments.WriteInt32(6);
    }
    int32_t ret = bqp_->SetTunnelHandleRemote(arguments, reply, option);
    EXPECT_EQ(ret, GSERROR_BINDER);
}

/*
 * Function: SetTunnelHandleRemoteOk
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetTunnelHandleRemote
 * 4. check ret
 */
HWTEST_F(BufferQueueProducerTest, SetTunnelHandleRemoteOk, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteBool(true);

    MessageParcel reply;
    MessageOption option;

    uint32_t reserveInts = 5;
    arguments.WriteInt32(reserveInts);
    for (uint32_t i = 0; i < reserveInts; i++) {
        arguments.WriteInt32(6);
    }
    arguments.WriteInt32(5);
    int32_t ret = bqp_->SetTunnelHandleRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/*
* Function: SetTunnelHandleRemote002
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetTunnelHandleRemote
*                  2. check ret and errno
*/
HWTEST_F(BufferQueueProducerTest, SetTunnelHandleRemote002, TestSize.Level0)
{
    system("mount -o remount, rw /");
    system("mkdir -p /data/SurfaceTestTmp");
    system("touch /data/SurfaceTestTmp/test.txt");
    int32_t fd = -1;
    {
        sptr<BufferQueueProducer> tmpBufferQueueProducer = nullptr;
        sptr<BufferQueue> tmpBufferQueue = nullptr;
        tmpBufferQueue = new BufferQueue("SetTunnelHandleRemoteTest");
        sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
        tmpBufferQueue->RegisterConsumerListener(listener);
        if (tmpBufferQueueProducer == nullptr) {
            tmpBufferQueueProducer = new BufferQueueProducer(tmpBufferQueue);
        }

        GraphicExtDataHandle *handle =
            static_cast<GraphicExtDataHandle *>(malloc(sizeof(GraphicExtDataHandle) + sizeof(int32_t) * 1));
        EXPECT_NE(handle, nullptr);
        if (handle) {
            MessageParcel arguments, reply;
            MessageOption option;
            std::string testPath = "/data/SurfaceTestTmp/test.txt";
            errno = 0; // reset errno
            fd = open(testPath.c_str(), O_RDONLY);
            EXPECT_GE(fd, 0);
            handle->fd = fd;
            handle->reserveInts = 1;
            handle->reserve[0] = 0;
            EXPECT_EQ(arguments.WriteBool(true), true);
            EXPECT_EQ(WriteExtDataHandle(arguments, handle), GSERROR_OK);
            EXPECT_EQ(tmpBufferQueueProducer->SetTunnelHandleRemote(arguments, reply, option), ERR_NONE);
            free(handle);
            handle = nullptr;
        }
    }

    /* tmpBufferQueueProducer and tmpBufferQueue has been destructed,
     * and the fd dup by WriteExtDataHandle has been closed.
     * so the global variable errno is expected to be 0.
     */
    EXPECT_EQ(errno, 0);

    if (fd > 0) {
        EXPECT_EQ(close(fd), 0);
    }
    system("rm -rf /data/SurfaceTestTmp");
}

/*
* Function: GetPresentTimestampRemote
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetPresentTimestampRemote
* 4. check ret
*/
HWTEST_F(BufferQueueProducerTest, GetPresentTimestampRemote, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteInt32(5);
    MessageParcel reply;
    reply.WriteInt32(6);
    MessageOption option;
    int32_t ret = bqp_->GetPresentTimestampRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/*
* Function: SetSurfaceSourceType and GetSurfaceSourceType
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetSurfaceSourceType for default
*                  2. call SetSurfaceSourceType and check the ret
*/
HWTEST_F(BufferQueueProducerTest, SurfaceSourceType001, TestSize.Level0)
{
    OHSurfaceSource sourceType;
    GSError ret = bqp_->GetSurfaceSourceType(sourceType);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    ASSERT_EQ(sourceType, OH_SURFACE_SOURCE_DEFAULT);

    ret = bqp_->SetSurfaceSourceType(OH_SURFACE_SOURCE_VIDEO);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    bqp_->GetSurfaceSourceType(sourceType);
    ASSERT_EQ(sourceType, OH_SURFACE_SOURCE_VIDEO);
}

/*
* Function: SetSurfaceAppFrameworkType and GetSurfaceAppFrameworkType
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetSurfaceAppFrameworkType for default
*                  2. call SetSurfaceAppFrameworkType and check the ret
*/
HWTEST_F(BufferQueueProducerTest, SurfaceAppFrameworkType001, TestSize.Level0)
{
    std::string appFrameworkType;
    bqp_->GetSurfaceAppFrameworkType(appFrameworkType);
    ASSERT_EQ(appFrameworkType, "");

    GSError ret = bqp_->SetSurfaceAppFrameworkType("test");
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
    bqp_->GetSurfaceAppFrameworkType(appFrameworkType);
    ASSERT_EQ(appFrameworkType, "test");
}

/*
* Function: SetSurfaceSourceTypeRemote
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetSurfaceSourceTypeRemote
* 4. check ret
*/
HWTEST_F(BufferQueueProducerTest, SetSurfaceSourceTypeRemote001, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteInt32(1);
    MessageParcel reply;
    reply.WriteInt32(6);
    MessageOption option;
    int32_t ret = bqp_->SetSurfaceSourceTypeRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/*
* Function: GetSurfaceSourceTypeRemote
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetSurfaceSourceTypeRemote
* 4. check ret
*/
HWTEST_F(BufferQueueProducerTest, GetSurfaceSourceTypeRemote001, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteInt32(5);
    MessageParcel reply;
    reply.WriteInt32(6);
    MessageOption option;
    int32_t ret = bqp_->GetSurfaceSourceTypeRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/*
* Function: SetSurfaceAppFrameworkTypeRemote
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetSurfaceAppFrameworkTypeRemote
* 4. check ret
*/
HWTEST_F(BufferQueueProducerTest, SetSurfaceAppFrameworkTypeRemote001, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteString("test");
    MessageParcel reply;
    reply.WriteInt32(6);
    MessageOption option;
    int32_t ret = bqp_->SetSurfaceAppFrameworkTypeRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/*
* Function: GetSurfaceAppFrameworkTypeRemote
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetSurfaceAppFrameworkTypeRemote
* 4. check ret
*/
HWTEST_F(BufferQueueProducerTest, GetSurfaceAppFrameworkTypeRemote001, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteInt32(5);
    MessageParcel reply;
    reply.WriteInt32(6);
    MessageOption option;
    int32_t ret = bqp_->GetSurfaceAppFrameworkTypeRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/*
* Function: SetWhitePointBrightness
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetHdrWhitePointBrightness, SetSdrWhitePointBrightness
*                  2. check ret
*/
HWTEST_F(BufferQueueProducerTest, SetWhitePointBrightness001, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteFloat(1);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->SetHdrWhitePointBrightnessRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
    ret = bqp_->SetSdrWhitePointBrightnessRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/*
* Function: SetDefaultUsage
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetDefaultUsage
*                  2. check ret
*/
HWTEST_F(BufferQueueProducerTest, SetDefaultUsage001, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteUint64(0);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->SetDefaultUsageRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/*
* Function: SetBufferHold
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetBufferHold
*                  2. check ret
*/
HWTEST_F(BufferQueueProducerTest, SetBufferHold001, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteBool(false);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->SetBufferHoldRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/*
* Function: SetBufferReallocFlag
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetBufferReallocFlag
*                  2. check ret
*/
HWTEST_F(BufferQueueProducerTest, SetBufferReallocFlag001, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteBool(false);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->SetBufferReallocFlagRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/*
* Function: TransformHint
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call TransformHint
*                  2. check ret
*/
HWTEST_F(BufferQueueProducerTest, TransformHint001, TestSize.Level0)
{
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_FLIP_H;

    MessageParcel arguments;
    arguments.WriteUint32(transform);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->SetBufferHoldRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
    int32_t err = reply.ReadInt32();
    EXPECT_EQ(err, OHOS::GSERROR_OK);

    ret = bqp_->GetTransformHintRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
    err = reply.ReadInt32();
    EXPECT_EQ(err, OHOS::GSERROR_OK);
}


/*
* Function: BufferQueueProducer member function nullptr test
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: BufferQueueProducer member function nullptr test
 */
HWTEST_F(BufferQueueProducerTest, NullTest, TestSize.Level0)
{
    sptr<BufferQueue> bqTmp = nullptr;
    sptr<BufferQueueProducer> bqpTmp = new BufferQueueProducer(bqTmp);
    IBufferProducer::RequestBufferReturnValue retval;
    GSError ret = bqpTmp->RequestBuffer(requestConfig, bedata_, retval);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_UNKOWN);
    ret = bqpTmp->RequestAndDetachBuffer(requestConfig, bedata_, retval);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_UNKOWN);
    ret = bqpTmp->CancelBuffer(retval.sequence, bedata_);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_UNKOWN);
    sptr<SyncFence> acquireFence = SyncFence::INVALID_FENCE;
    ret = bqpTmp->FlushBuffer(retval.sequence, bedata_, acquireFence, flushConfig);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_UNKOWN);
    ret = bqpTmp->AttachAndFlushBuffer(retval.buffer, bedata_, acquireFence, flushConfig, true);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_UNKOWN);
    ret = bqpTmp->GetLastFlushedBuffer(retval.buffer, acquireFence, nullptr, false);
    EXPECT_EQ(ret, OHOS::SURFACE_ERROR_UNKOWN);
    EXPECT_EQ(bqpTmp->AttachBuffer(retval.buffer), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->DetachBuffer(retval.buffer), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->GetQueueSize(), 0);
    EXPECT_EQ(bqpTmp->SetQueueSize(0), OHOS::GSERROR_INVALID_ARGUMENTS);
    std::string name = "test";
    EXPECT_EQ(bqpTmp->GetName(name), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->GetDefaultWidth(), 0);
    EXPECT_EQ(bqpTmp->GetDefaultHeight(), 0);
    EXPECT_EQ(bqpTmp->GetDefaultUsage(), 0);
    EXPECT_EQ(bqpTmp->GetUniqueId(), 0);
    sptr<IProducerListener> listener = nullptr;
    EXPECT_EQ(bqpTmp->RegisterReleaseListener(listener), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->RegisterReleaseListenerBackup(listener), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->UnRegisterReleaseListener(), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->UnRegisterReleaseListenerBackup(), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->SetTransform(GraphicTransformType::GRAPHIC_FLIP_H), OHOS::GSERROR_INVALID_ARGUMENTS);
    std::vector<BufferVerifyAllocInfo> infos;
    uint64_t uniqueId;
    EXPECT_EQ(bqpTmp->GetNameAndUniqueId(name, uniqueId), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->Disconnect(nullptr), OHOS::SURFACE_ERROR_UNKOWN);
    EXPECT_EQ(bqpTmp->SetScalingMode(0, ScalingMode::SCALING_MODE_FREEZE), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->SetScalingMode(ScalingMode::SCALING_MODE_FREEZE), OHOS::GSERROR_INVALID_ARGUMENTS);
    std::vector<GraphicHDRMetaData> meta;
    EXPECT_EQ(bqpTmp->SetMetaData(0, meta), OHOS::GSERROR_INVALID_ARGUMENTS);
    std::vector<uint8_t> metaData;
    EXPECT_EQ(bqpTmp->SetMetaDataSet(0, GraphicHDRMetadataKey::GRAPHIC_MATAKEY_BLUE_PRIMARY_X,
        metaData), OHOS::GSERROR_INVALID_ARGUMENTS);
    sptr<SurfaceTunnelHandle> handle = nullptr;
    EXPECT_EQ(bqpTmp->SetTunnelHandle(handle), OHOS::GSERROR_INVALID_ARGUMENTS);
    int64_t time = 0;
    EXPECT_EQ(bqpTmp->GetPresentTimestamp(0, GraphicPresentTimestampType::GRAPHIC_DISPLAY_PTS_DELAY,
        time), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->GetStatus(), false);
    bqpTmp->SetStatus(false);
    EXPECT_EQ(bqpTmp->AcquireLastFlushedBuffer(retval.buffer, acquireFence, nullptr, 0, false),
        OHOS::SURFACE_ERROR_UNKOWN);
    EXPECT_EQ(bqpTmp->ReleaseLastFlushedBuffer(0), OHOS::SURFACE_ERROR_UNKOWN);
    bqpTmp->OnBufferProducerRemoteDied();
    EXPECT_EQ(bqpTmp->SetBufferHold(false), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->SetBufferReallocFlag(false), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->SetSdrWhitePointBrightness(1), OHOS::SURFACE_ERROR_UNKOWN);
    EXPECT_EQ(bqpTmp->SetHdrWhitePointBrightness(1), OHOS::SURFACE_ERROR_UNKOWN);
    EXPECT_EQ(bqpTmp->SetSurfaceAppFrameworkType(name), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->GetSurfaceAppFrameworkType(name), OHOS::GSERROR_INVALID_ARGUMENTS);
    OHSurfaceSource sourceType = OH_SURFACE_SOURCE_DEFAULT;
    EXPECT_EQ(bqpTmp->SetSurfaceSourceType(sourceType), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->GetSurfaceSourceType(sourceType), OHOS::GSERROR_INVALID_ARGUMENTS);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_FLIP_H;
    EXPECT_EQ(bqpTmp->SetTransformHint(transform, 0), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->GetTransformHint(transform), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->GoBackground(), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->CleanCache(true, nullptr), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->SetDefaultUsage(0), OHOS::GSERROR_INVALID_ARGUMENTS);
    EXPECT_EQ(bqpTmp->AttachBufferToQueue(nullptr), OHOS::SURFACE_ERROR_UNKOWN);
    EXPECT_EQ(bqpTmp->DetachBufferFromQueue(nullptr), OHOS::SURFACE_ERROR_UNKOWN);
    ProducerInitInfo info;
    EXPECT_EQ(bqpTmp->GetProducerInitInfo(info), OHOS::SURFACE_ERROR_UNKOWN);
    vector<uint32_t> sequences;
    vector<sptr<SyncFence>> fences;
    vector<sptr<BufferExtraData>> bedata;
    vector<BufferFlushConfigWithDamages> damages;
    vector<BufferQueueProducer::RequestBufferReturnValue> retvalues;
    BufferRequestConfig config;
    EXPECT_EQ(bqpTmp->RequestBuffers(config, bedata, retvalues), OHOS::SURFACE_ERROR_UNKOWN);
    EXPECT_EQ(bqpTmp->FlushBuffers(sequences, bedata, fences, damages), OHOS::SURFACE_ERROR_UNKOWN);
    EXPECT_EQ(bqpTmp->SetGlobalAlpha(-1), OHOS::SURFACE_ERROR_UNKOWN);
    EXPECT_EQ(bqpTmp->SetFrameGravity(-1), OHOS::SURFACE_ERROR_UNKOWN);
    EXPECT_EQ(bqpTmp->SetFixedRotation(-1), OHOS::SURFACE_ERROR_UNKOWN);
    EXPECT_EQ(bqpTmp->PreAllocBuffers(config, 0), OHOS::SURFACE_ERROR_UNKOWN);
    EXPECT_EQ(bqpTmp->SetRequestBufferNoblockMode(true), OHOS::SURFACE_ERROR_UNKOWN);
    EXPECT_EQ(bqpTmp->SetAlphaType(GraphicAlphaType::GRAPHIC_ALPHATYPE_UNKNOWN), OHOS::SURFACE_ERROR_UNKOWN);
    bqTmp = nullptr;
    bqpTmp = nullptr;
}

/*
* Function: SetRequestBufferNoblockMode
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetRequestBufferNoblockMode
*                  2. check ret
*/
HWTEST_F(BufferQueueProducerTest, SetRequestBufferNoblockMode, TestSize.Level0)
{
    ASSERT_EQ(bqp_->SetRequestBufferNoblockMode(true), OHOS::GSERROR_OK);
    bool noblockMode = false;
    ASSERT_EQ(bq_->GetRequestBufferNoblockMode(noblockMode), OHOS::GSERROR_OK);
    ASSERT_EQ(noblockMode, true);

    ASSERT_EQ(bqp_->SetRequestBufferNoblockMode(false), OHOS::GSERROR_OK);
    ASSERT_EQ(bq_->GetRequestBufferNoblockMode(noblockMode), OHOS::GSERROR_OK);
    ASSERT_EQ(noblockMode, false);
}

/*
* Function: SetRequestBufferNoblockModeRemote
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call SetRequestBufferNoblockModeRemote
*                  2. check ret
 */
HWTEST_F(BufferQueueProducerTest, SetRequestBufferNoblockModeRemote, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteBool(false);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->SetRequestBufferNoblockModeRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/*
* Function: CheckIsAliveTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: CheckIsAliveTest member function test
 */
HWTEST_F(BufferQueueProducerTest, CheckIsAliveTest, TestSize.Level0)
{
    bqp_->magicNum_ = 0;
    EXPECT_EQ(bqp_->CheckIsAlive(), false);
    bqp_->magicNum_ = BufferQueueProducer::MAGIC_INIT;
    EXPECT_EQ(bqp_->CheckIsAlive(), true);
}

/*
* Function: UnRegisterPropertyListener
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: UnRegisterPropertyListener member function test
 */
HWTEST_F(BufferQueueProducerTest, UnRegisterPropertyListenerTest, TestSize.Level0)
{
    sptr<BufferQueue> bqtmp = nullptr;
    sptr<BufferQueueProducer> bqptmp = new BufferQueueProducer(bqtmp);
    EXPECT_EQ(bqptmp->UnRegisterPropertyListener(0), OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: RegisterPropertyListener
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: RegisterPropertyListener member function test
 */
HWTEST_F(BufferQueueProducerTest, RegisterPropertyListenerTest, TestSize.Level0)
{
    sptr<IProducerListener> listener = nullptr;
    sptr<BufferQueue> bqtmp = nullptr;
    sptr<BufferQueueProducer> bqptmp = new BufferQueueProducer(bqtmp);
    EXPECT_EQ(bqptmp->RegisterPropertyListener(listener, 0), OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: RegisterPropertyListenerRemote
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: RegisterPropertyListenerRemote member function test
 */
HWTEST_F(BufferQueueProducerTest, RegisterPropertyListenerRemote001, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteBool(false);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->RegisterPropertyListenerRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_INVALID_REPLY);
}

/*
* Function: UnRegisterPropertyListenerRemote
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: UnRegisterPropertyListenerRemote member function test
 */
HWTEST_F(BufferQueueProducerTest, UnRegisterPropertyListenerRemote001, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteInt64(0);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->UnRegisterPropertyListenerRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/*
* Function: GetProducerInitInfoRemote
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: GetProducerInitInfoRemote member function test
 */
HWTEST_F(BufferQueueProducerTest, GetProducerInitInfoRemote001, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteBool(false);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->GetProducerInitInfoRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_INVALID_DATA);
}
/*
 * Function: SetLppShareFdRemote
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: SetLppShareFdRemote member function test
 */
HWTEST_F(BufferQueueProducerTest, SetLppShareFdRemote001, TestSize.Level0)
{
    int fd = -1;
    bool state = false;
    MessageParcel arguments;
    arguments.WriteFileDescriptor(fd);
    arguments.WriteBool(state);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->SetLppShareFdRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_INVALID_VALUE);
}
/*
 * Function: SetLppShareFdRemote
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: SetLppShareFdRemote member function test
 */
HWTEST_F(BufferQueueProducerTest, SetLppShareFdRemote002, TestSize.Level0)
{
    int fd = open("/dev/lpptest", O_RDWR | O_CREAT, static_cast<mode_t>(0600));
    ASSERT_NE(fd, -1);
    ASSERT_NE(ftruncate(fd, 0x1000), -1);
    bool state = false;
    MessageParcel arguments;
    arguments.WriteFileDescriptor(fd);
    arguments.WriteBool(state);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->SetLppShareFdRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}
/*
 * Function: SetLppShareFd
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: SetLppShareFd member function test
 */
HWTEST_F(BufferQueueProducerTest, SetLppShareFd001, TestSize.Level0)
{
    int fd = -1;
    bool state = false;
    sptr<BufferQueue> bqtmp = nullptr;
    sptr<BufferQueueProducer> bqptmp = new BufferQueueProducer(bqtmp);
    bqptmp->bufferQueue_ = nullptr;
    int ret = bqptmp->SetLppShareFd(fd, state);
    ASSERT_EQ(ret, SURFACE_ERROR_UNKOWN);
}

/*
 * Function: SetLppShareFd
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: SetLppShareFd member function test
 */
HWTEST_F(BufferQueueProducerTest, SetLppShareFd002, TestSize.Level0)
{
    int fd = -1;
    bool state = false;
    sptr<BufferQueue> bqtmp = new BufferQueue("test");
    sptr<BufferQueueProducer> bqptmp = new BufferQueueProducer(bqtmp);
    int ret = bqptmp->SetLppShareFd(fd, state);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
 * Function: SetAlphaTypeRemote
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: call SetAlphaTypeRemote
 */
HWTEST_F(BufferQueueProducerTest, SetAlphaTypeRemote, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteInt32(1);
    MessageParcel reply;
    reply.WriteInt32(2);
    MessageOption option;
    int32_t ret = bqp_->SetAlphaTypeRemote(arguments, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/*
 * Function: SetAlphaType
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: SetAlphaType member function test
 */
HWTEST_F(BufferQueueProducerTest, SetAlphaTypeTest, TestSize.Level0)
{
    GraphicAlphaType alphaType = GraphicAlphaType::GRAPHIC_ALPHATYPE_OPAQUE;
    sptr<BufferQueue> bqtmp = new BufferQueue("test");
    sptr<BufferQueueProducer> bqptmp = new BufferQueueProducer(bqtmp);
    int ret = bqptmp->SetAlphaType(alphaType);
    ASSERT_EQ(ret, GSERROR_OK);
}

/*
 * Function: RequestBufferRemoteParseErr
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RequestBufferRemote
 * 4. check ret
 */
HWTEST_F(BufferQueueProducerTest, RequestBufferRemoteParseErr, TestSize.Level0)
{
    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->RequestBufferRemote(arguments, reply, option);
    EXPECT_EQ(ret, GSERROR_BINDER);
}

/*
 * Function: RequestBuffersRemoteParseErr
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RequestBuffersRemote
 * 4. check ret
 */
HWTEST_F(BufferQueueProducerTest, RequestBuffersRemoteParseErr, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteInt32(5);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->RequestBuffersRemote(arguments, reply, option);
    EXPECT_EQ(ret, GSERROR_BINDER);
}

/*
 * Function: RequestBuffersRemoteParseErr001
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RequestBuffersRemote
 * 4. check ret
 */
HWTEST_F(BufferQueueProducerTest, RequestBuffersRemoteParseErr001, TestSize.Level0)
{
    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->RequestBuffersRemote(arguments, reply, option);
    EXPECT_EQ(ret, GSERROR_BINDER);
}

/*
 * Function: AttachBufferRemoteParseErr
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call AttachBufferRemote
 * 4. check ret
 */
HWTEST_F(BufferQueueProducerTest, AttachBufferRemoteParseErr, TestSize.Level0)
{
    MessageParcel arguments;
    arguments.WriteInt32(5);
    arguments.WriteBool(true);
    // reserveFds, reserveInts
    arguments.WriteInt32(1);
    arguments.WriteInt32(1);
    // handle: width, stride, height, size, format, usage, phyAddr
    for (uint32_t i =  0; i < 6; i++) {
        arguments.WriteInt32(1);
    }
    arguments.WriteInt64(0x100);
    arguments.WriteBool(true);
    arguments.WriteBool(true);
    int fd = open("/dev/lpptest", O_RDWR | O_CREAT, static_cast<mode_t>(0600));
    arguments.WriteFileDescriptor(fd);
    arguments.WriteFileDescriptor(fd);
    arguments.WriteInt32(1);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->AttachBufferRemote(arguments, reply, option);
    EXPECT_EQ(ret, GSERROR_BINDER);
}

/*
 * Function: SetQueueSizeRemoteParseErr
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call SetQueueSizeRemote
 * 4. check ret
 */
HWTEST_F(BufferQueueProducerTest, SetQueueSizeRemoteParseErr, TestSize.Level0)
{
    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->SetQueueSizeRemote(arguments, reply, option);
    EXPECT_EQ(ret, GSERROR_BINDER);
}

/*
 * Function: CleanCacheRemoteParseErr
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call CleanCacheRemote
 * 4. check ret
 */
HWTEST_F(BufferQueueProducerTest, CleanCacheRemoteParseErr, TestSize.Level0)
{
    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->CleanCacheRemote(arguments, reply, option);
    EXPECT_EQ(ret, GSERROR_BINDER);
}

/*
 * Function: AcquireLastFlushedBufferRemoteParseErr
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call AcquireLastFlushedBufferRemote
 * 4. check ret
 */
HWTEST_F(BufferQueueProducerTest, AcquireLastFlushedBufferRemoteParseErr, TestSize.Level0)
{
    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->AcquireLastFlushedBufferRemote(arguments, reply, option);
    EXPECT_EQ(ret, GSERROR_BINDER);
}

/*
 * Function: RequestAndDetachBufferRemoteParseErr
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call RequestAndDetachBufferRemote
 * 4. check ret
 */
HWTEST_F(BufferQueueProducerTest, RequestAndDetachBufferRemoteParseErr, TestSize.Level0)
{
    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->RequestAndDetachBufferRemote(arguments, reply, option);
    EXPECT_EQ(ret, GSERROR_BINDER);
}

/*
 * Function: PreAllocBuffersRemoteParseErr
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call PreAllocBuffersRemote
 * 4. check ret
 */
HWTEST_F(BufferQueueProducerTest, PreAllocBuffersRemoteParseErr, TestSize.Level0)
{
    MessageParcel arguments;
    MessageParcel reply;
    MessageOption option;
    int32_t ret = bqp_->PreAllocBuffersRemote(arguments, reply, option);
    EXPECT_EQ(ret, GSERROR_BINDER);
}
}
