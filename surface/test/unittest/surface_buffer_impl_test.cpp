/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include <securec.h>
#include <gtest/gtest.h>
#include <surface.h>
#include <surface_buffer_impl.h>
#include <buffer_utils.h>
#include <metadata_helper.h>
#include "v1_1/buffer_handle_meta_key_type.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class SurfaceBufferImplTest : public testing::Test {
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
        .colorGamut = GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DCI_P3,
    };
    static inline sptr<SurfaceBuffer> buffer = nullptr;
    static inline int32_t val32 = 0;
    static inline int64_t val64 = 0;
};

void SurfaceBufferImplTest::SetUpTestCase()
{
    buffer = nullptr;
    val32 = 0;
    val64 = 0;
}

void SurfaceBufferImplTest::TearDownTestCase()
{
    buffer = nullptr;
}

/*
* Function: GetSeqNum
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. new SurfaceBufferImpl and GetSeqNum
*                  2. new SurfaceBufferImpl again and check GetSeqNum = oldSeq + 1
*                  3. set and verify the value of parameter isConsumerAttachBufferFlag_ is false
*                  4. set and verify the value of parameter isConsumerAttachBufferFlag_ is true
 */
HWTEST_F(SurfaceBufferImplTest, NewSeqIncrease001, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    int oldSeq = buffer->GetSeqNum();

    buffer = new SurfaceBufferImpl();
    ASSERT_EQ(oldSeq + 1, buffer->GetSeqNum());
    ASSERT_NE(0, buffer->GetBufferId());

    buffer->SetConsumerAttachBufferFlag(false);
    ASSERT_EQ(buffer->GetConsumerAttachBufferFlag(), false);
    buffer->SetConsumerAttachBufferFlag(true);
    ASSERT_EQ(buffer->GetConsumerAttachBufferFlag(), true);
}

/*
* Function: GetSeqNum
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. new 0xFFFF SurfaceBufferImpl and check SeqNum
*                  2. new SurfaceBufferImpl again and check GetSeqNum = oldSeq + 1
 */
HWTEST_F(SurfaceBufferImplTest, NewSeqIncrease002, TestSize.Level0)
{
    // the max seqNum low 16 bit is 0xFFFF
    uint32_t maxSeqNum = 0xFFFF;
    std::vector<sptr<SurfaceBuffer>> vecBuffer;
    for (uint32_t i = 0; i <= maxSeqNum; ++i) {
        sptr<SurfaceBuffer> newBuffer = new SurfaceBufferImpl();
        vecBuffer.push_back(newBuffer);
    }
    sptr<SurfaceBuffer> maxSeqBuffer = new SurfaceBufferImpl(maxSeqNum);
    ASSERT_EQ(maxSeqNum, maxSeqBuffer->GetSeqNum() & maxSeqNum);
    sptr<SurfaceBuffer> increbuffer = new SurfaceBufferImpl();
    int oldSeq = increbuffer->GetSeqNum();
    increbuffer = new SurfaceBufferImpl();
    ASSERT_EQ(oldSeq + 1, increbuffer->GetSeqNum());
}

/*
 * Function: CheckSeqNumExist
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl and CheckSeqNumExist
 */
HWTEST_F(SurfaceBufferImplTest, CheckSeqNumExist001, TestSize.Level0)
{
    sptr<SurfaceBuffer> bufferTemp = new SurfaceBufferImpl();
    sptr<SurfaceBuffer> bufferSeqExist = new SurfaceBufferImpl(bufferTemp->GetSeqNum());
    ASSERT_EQ(SurfaceBuffer::CheckSeqNumExist(bufferTemp->GetSeqNum()), true);
    // the max seqNum low 16 bit is 0xFFFF
    uint32_t maxSeqNum = 0xFFFF;
    ASSERT_EQ(SurfaceBuffer::CheckSeqNumExist(maxSeqNum), false);
}

/*
* Function: GenerateSequenceNumber
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. new SurfaceBufferImpl and GetSeqNum
*                  2. GenerateSequenceNumber seqNumLow and check retval is oldSeq + 1
*                  3. GenerateSequenceNumber 0xFFFF and check retval is 0
 */
HWTEST_F(SurfaceBufferImplTest, GenerateSequenceNumber001, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    uint32_t oldSeq = buffer->GetSeqNum();

    // the max seqNum low 16 bit is 0xFFFF
    uint32_t maxSeqNum = 0xFFFF;
    uint32_t seqNumLow = oldSeq & maxSeqNum;
    ASSERT_EQ((oldSeq + 1) & maxSeqNum, SurfaceBufferImpl::GenerateSequenceNumber(seqNumLow));
    ASSERT_EQ(0, SurfaceBufferImpl::GenerateSequenceNumber(maxSeqNum));
}

/*
* Function: check buffer state
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. check buffer state, such as bufferhandle, virAddr, fileDescriptor and size
 */
HWTEST_F(SurfaceBufferImplTest, State001, TestSize.Level0)
{
    ASSERT_EQ(buffer->GetBufferHandle(), nullptr);
    ASSERT_EQ(buffer->GetVirAddr(), nullptr);
    ASSERT_EQ(buffer->GetFileDescriptor(), -1);
    ASSERT_EQ(buffer->GetSize(), 0u);
}

/*
* Function: check buffer state
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetBufferHandle and Alloc
*                  2. check buffer state, such as bufferhandle, virAddr and size
*                  3. call Free
*                  4. check ret
 */
HWTEST_F(SurfaceBufferImplTest, State002, TestSize.Level0)
{
    ASSERT_EQ(buffer->GetBufferHandle(), nullptr);
    ASSERT_EQ(buffer->GetPhyAddr(), 0);
    ASSERT_EQ(buffer->GetStride(), -1);
    vector<uint32_t> keys;
    ASSERT_EQ(buffer->ListMetadataKeys(keys), GSERROR_NOT_INIT);
    ASSERT_EQ(buffer->EraseMetadataKey(1), GSERROR_NOT_INIT);
    GSError ret = buffer->Alloc(requestConfig);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ASSERT_NE(buffer->GetBufferHandle(), nullptr);
    ASSERT_NE(buffer->GetVirAddr(), nullptr);
    ASSERT_NE(buffer->GetSize(), 0u);
    ASSERT_EQ(buffer->GetFormat(), GRAPHIC_PIXEL_FMT_RGBA_8888);
    ASSERT_EQ(buffer->GetUsage(), BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA);
    ASSERT_EQ(buffer->GetSurfaceBufferColorGamut(), GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DCI_P3);

    buffer->SetBufferHandle(nullptr);
}

/*
* Function: parcel
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. new SurfaceBufferImpl and Alloc
*                  2. call Set data interface
*                  3. call WriteSurfaceBufferImpl and ReadSurfaceBufferImpl
*                  4. call Get data interface
*                  5. check ret
 */
HWTEST_F(SurfaceBufferImplTest, Parcel001, TestSize.Level0)
{
    sptr<SurfaceBuffer> sbi = new SurfaceBufferImpl(0);
    auto sret = sbi->Alloc(requestConfig);
    ASSERT_EQ(sret, OHOS::GSERROR_OK);

    MessageParcel parcel;
    WriteSurfaceBufferImpl(parcel, sbi->GetSeqNum(), sbi);

    sptr<SurfaceBuffer> buffer = nullptr;
    uint32_t seq;
    ReadSurfaceBufferImpl(parcel, seq, buffer);
    ASSERT_NE(buffer, nullptr);
}

/*
* Function: Create
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. Call SurfaceBuffer::Create()
*                  2. check ret
 */
HWTEST_F(SurfaceBufferImplTest, Create001, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    ASSERT_NE(buffer, nullptr);
}

/*
* Function: Alloc
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. Call SurfaceBuffer::Alloc()
*                  2. check ret
 */
HWTEST_F(SurfaceBufferImplTest, Alloc001, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = new SurfaceBufferImpl();
    sptr<SurfaceBuffer> bufferAlloc = new SurfaceBufferImpl();
    auto sret = bufferAlloc->Alloc(requestConfig, buffer);
    ASSERT_EQ(sret, OHOS::GSERROR_OK);
    sptr<SurfaceBuffer> bufferRealloc = new SurfaceBufferImpl();
    sret = bufferRealloc->Alloc(requestConfig, bufferAlloc);
    ASSERT_EQ(sret, OHOS::GSERROR_OK);
}

/*
* Function: Set/Get/List/Erase Metadata
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. new SurfaceBufferImpl and Alloc
*                  2. call Set Metadata interface
*                  3. call Get Metadata interface
*                  4. check ret
*                  5. call List Metadata keys interface
*                  6. check ret
*                  7. call Erase Metadata key interface
*                  8. call List Metadata keys interface again
*                  9. check ret
*/
HWTEST_F(SurfaceBufferImplTest, Metadata001, TestSize.Level0)
{
    using namespace HDI::Display::Graphic::Common::V1_0;

    sptr<SurfaceBuffer> sbi = new SurfaceBufferImpl(0);
    auto sret = sbi->Alloc(requestConfig);
    ASSERT_EQ(sret, OHOS::GSERROR_OK);

    uint32_t metadataKey = 2;

    uint32_t setMetadata = 4260097;
    std::vector<uint8_t> setData;
    ASSERT_EQ(MetadataHelper::ConvertMetadataToVec(setMetadata, setData), OHOS::GSERROR_OK);
    ASSERT_EQ(sbi->SetMetadata(0, setData), GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(sbi->SetMetadata(HDI::Display::Graphic::Common::V1_1::ATTRKEY_END, setData), GSERROR_INVALID_ARGUMENTS);
    sret = sbi->SetMetadata(metadataKey, setData);
    ASSERT_TRUE(sret == OHOS::GSERROR_OK || sret == GSERROR_HDI_ERROR);

    std::vector<uint8_t> getData;
    ASSERT_EQ(sbi->GetMetadata(0, getData), GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(sbi->GetMetadata(HDI::Display::Graphic::Common::V1_1::ATTRKEY_END, getData), GSERROR_INVALID_ARGUMENTS);
    sret = sbi->GetMetadata(metadataKey, getData);
    ASSERT_TRUE(sret == OHOS::GSERROR_OK || sret == GSERROR_HDI_ERROR);

    if (sret == OHOS::GSERROR_OK) {
        uint32_t getMetadata;
        ASSERT_EQ(MetadataHelper::ConvertVecToMetadata(getData, getMetadata), OHOS::GSERROR_OK);
        ASSERT_EQ(setMetadata, getMetadata);
    }

    std::vector<uint32_t> keys;

    sret = sbi->ListMetadataKeys(keys);
    ASSERT_TRUE(sret == OHOS::GSERROR_OK || sret == GSERROR_HDI_ERROR);
    if (sret == OHOS::GSERROR_OK) {
        ASSERT_EQ(sret, OHOS::GSERROR_OK);
        ASSERT_EQ(keys.size(), 1);
        ASSERT_EQ(keys[0], metadataKey);
    }

    ASSERT_EQ(sbi->EraseMetadataKey(0), GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(sbi->EraseMetadataKey(HDI::Display::Graphic::Common::V1_1::ATTRKEY_END), GSERROR_INVALID_ARGUMENTS);
    sret = sbi->EraseMetadataKey(metadataKey);
    ASSERT_TRUE(sret == OHOS::GSERROR_OK || sret == GSERROR_HDI_ERROR);

    sret = sbi->ListMetadataKeys(keys);
    ASSERT_TRUE(sret == OHOS::GSERROR_OK || sret == GSERROR_HDI_ERROR);
    if (sret == OHOS::GSERROR_OK) {
        ASSERT_EQ(keys.size(), 0);
    }
}

/*
 * Function: SetMetadata
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl and Alloc
 *                  2. call Set Metadata interface with disbale cache
                    3. check ret and metaDataCache_ should be empty
 */
HWTEST_F(SurfaceBufferImplTest, Metadata002, TestSize.Level0)
{
    using namespace HDI::Display::Graphic::Common::V1_0;

    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl(0);
    auto sret = sbi->Alloc(requestConfig);
    ASSERT_EQ(sret, OHOS::GSERROR_OK);

    uint32_t metadataKey = 2;

    uint32_t setMetadata = 4260097;
    std::vector<uint8_t> setData;
    ASSERT_EQ(MetadataHelper::ConvertMetadataToVec(setMetadata, setData), OHOS::GSERROR_OK);
    ASSERT_EQ(sbi->SetMetadata(0, setData, false), GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(sbi->SetMetadata(HDI::Display::Graphic::Common::V1_1::ATTRKEY_END, setData, false),
        GSERROR_INVALID_ARGUMENTS);
    sret = sbi->SetMetadata(metadataKey, setData, false);
    ASSERT_TRUE(sret == OHOS::GSERROR_OK || sret == GSERROR_HDI_ERROR);
    if (sret == OHOS::GSERROR_OK) {
        ASSERT_TRUE(sbi->metaDataCache_.empty());
    }
}

/*
 * Function: SetMetadata
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl and Alloc
 *                  2. call Set Metadata interface with enable cache
                    3. check ret and metaDataCache_ size be 1
 */
HWTEST_F(SurfaceBufferImplTest, Metadata003, TestSize.Level0)
{
    using namespace HDI::Display::Graphic::Common::V1_0;

    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl(0);
    auto sret = sbi->Alloc(requestConfig);
    ASSERT_EQ(sret, OHOS::GSERROR_OK);

    uint32_t metadataKey = 2;

    uint32_t setMetadata = 4260097;
    std::vector<uint8_t> setData;
    ASSERT_EQ(MetadataHelper::ConvertMetadataToVec(setMetadata, setData), OHOS::GSERROR_OK);
    ASSERT_EQ(sbi->SetMetadata(0, setData, true), GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(sbi->SetMetadata(HDI::Display::Graphic::Common::V1_1::ATTRKEY_END, setData, true),
        GSERROR_INVALID_ARGUMENTS);
    sret = sbi->SetMetadata(metadataKey, setData, true);
    ASSERT_TRUE(sret == OHOS::GSERROR_OK || sret == GSERROR_HDI_ERROR);
    if (sret == OHOS::GSERROR_OK) {
        ASSERT_TRUE(sbi->metaDataCache_.size() == 1);
    }
}


/*
* Function: BufferRequestConfig
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. new SurfaceBufferImpl
*                  2. call SetBufferRequestConfig interface using requestConfig and check ret
*                  3. call GetBufferRequestConfig interface using requestConfig and check ret
*                  4. call WriteBufferRequestConfig interface and check ret
*                  5. call ReadBufferRequestConfig interface and check ret
 */
HWTEST_F(SurfaceBufferImplTest, BufferRequestConfig001, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    MessageParcel parcel;
    buffer->SetBufferRequestConfig(requestConfig);
    ASSERT_EQ(buffer->GetBufferRequestConfig(), requestConfig);
    ASSERT_EQ(buffer->WriteBufferRequestConfig(parcel), GSERROR_OK);
    ASSERT_EQ(buffer->ReadBufferRequestConfig(parcel), GSERROR_OK);
}

/*
* Function: SetSurfaceBufferScalingMode&GetSurfaceBufferScalingMode
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. new SurfaceBufferImpl
*                  2. call GetSurfaceBufferScalingMode and check default is SCALING_MODE_SCALE_TO_WINDOW
*                  3. call SetSurfaceBufferScalingMode and GetSurfaceBufferScalingMode and check ret
*                  4. repeatly call SetSurfaceBufferScalingMode and GetSurfaceBufferScalingMode and check ret
 */
HWTEST_F(SurfaceBufferImplTest, SurfaceBufferScalingMode001, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    ASSERT_EQ(buffer->GetSurfaceBufferScalingMode(), ScalingMode::SCALING_MODE_SCALE_TO_WINDOW);
    buffer->SetSurfaceBufferScalingMode(ScalingMode::SCALING_MODE_SCALE_CROP);
    ASSERT_EQ(buffer->GetSurfaceBufferScalingMode(), ScalingMode::SCALING_MODE_SCALE_CROP);
    buffer->SetSurfaceBufferScalingMode(ScalingMode::SCALING_MODE_NO_SCALE_CROP);
    ASSERT_EQ(buffer->GetSurfaceBufferScalingMode(), ScalingMode::SCALING_MODE_NO_SCALE_CROP);
}

/*
* Function: SetBufferDeleteFromCacheFlag&GetBufferDeleteFromCacheFlag
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. new SurfaceBufferImpl
*                  2. call GetBufferDeleteFromCacheFlag and check default is false
*                  3. call SetBufferDeleteFromCacheFlag and GetBufferDeleteFromCacheFlag and check ret
*                  4. repeatly call SetBufferDeleteFromCacheFlag and GetBufferDeleteFromCacheFlag and check ret
 */
HWTEST_F(SurfaceBufferImplTest, BufferDeleteFromCacheFlag001, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    ASSERT_EQ(buffer->GetBufferDeleteFromCacheFlag(), false);
    buffer->SetBufferDeleteFromCacheFlag(true);
    ASSERT_EQ(buffer->GetBufferDeleteFromCacheFlag(), true);
    buffer->SetBufferDeleteFromCacheFlag(false);
    ASSERT_EQ(buffer->GetBufferDeleteFromCacheFlag(), false);
}

/*
* Function: TryReclaim
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. new SurfaceBufferImpl
*                  2. call TryReclaim and check ret
*                  3. call IsReclaimed and check ret
 */
HWTEST_F(SurfaceBufferImplTest, TryReclaim001, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    ASSERT_EQ(buffer->TryReclaim(), GSERROR_INVALID_ARGUMENTS);
    ASSERT_EQ(buffer->IsReclaimed(), false);
}

/*
* Function: TryReclaim
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. new SurfaceBufferImpl
*                  2. call SetBufferHandle and check buffer handle
*                  3. set fd of handle to -1
*                  4. call TryReclaim and check ret
*                  5. set fd of handle to 123
*                  6. call TryReclaim and check ret
 */
HWTEST_F(SurfaceBufferImplTest, TryReclaim002, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    ASSERT_EQ(buffer->GetBufferHandle(), nullptr);
    BufferHandle *handle = new BufferHandle();
    buffer->SetBufferHandle(handle);
    ASSERT_NE(buffer->GetBufferHandle(), nullptr);
    handle->fd = -1;
    ASSERT_EQ(buffer->TryReclaim(), GSERROR_INVALID_ARGUMENTS);
    handle->fd = 123;
    GSError ret = buffer->TryReclaim();
    if (buffer->IsReclaimed()) {
        printf("come into branch: isReclaimed = true\n");
        ASSERT_EQ(ret, GSERROR_OK);
    } else {
        printf("come into branch: isReclaimed = false\n");
        ASSERT_EQ(ret, GSERROR_API_FAILED);
    }
}

/*
* Function: TryReclaim
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. new SurfaceBufferImpl
*                  2. call SetBufferHandle and check buffer handle
*                  3. set fd of handle to 123
*                  4. call TryReclaim and check ret
*                  5. call TryReclaim again and check ret
 */
HWTEST_F(SurfaceBufferImplTest, TryReclaim003, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    ASSERT_EQ(buffer->GetBufferHandle(), nullptr);
    BufferHandle *handle = new BufferHandle();
    buffer->SetBufferHandle(handle);
    ASSERT_NE(buffer->GetBufferHandle(), nullptr);
    handle->fd = 123;
    GSError ret = buffer->TryReclaim();
    if (buffer->IsReclaimed()) {
        printf("come into branch: isReclaimed = true\n");
        ASSERT_EQ(ret, GSERROR_OK);
    } else {
        printf("come into branch: isReclaimed = false\n");
        ASSERT_EQ(ret, GSERROR_API_FAILED);
    }
    if (buffer->IsReclaimed()) {
        ret = buffer->TryReclaim();
        ASSERT_EQ(ret, GSERROR_INVALID_OPERATING);
    }
}

/*
* Function: TryResumeIfNeeded
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. new SurfaceBufferImpl
*                  2. call TryResumeIfNeeded and check ret
*                  3. call IsReclaimed and check ret
 */
HWTEST_F(SurfaceBufferImplTest, TryResumeIfNeeded001, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    ASSERT_EQ(buffer->TryResumeIfNeeded(), GSERROR_INVALID_OPERATING);
    ASSERT_EQ(buffer->IsReclaimed(), false);
}

/*
* Function: TryResumeIfNeeded
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. new SurfaceBufferImpl
*                  2. call SetBufferHandle and check buffer handle
*                  3. set fd of handle to 123
*                  4. call TryReclaim and check ret
*                  5. call TryResumeIfNeeded and check ret
 */
HWTEST_F(SurfaceBufferImplTest, TryResumeIfNeeded002, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    ASSERT_EQ(buffer->GetBufferHandle(), nullptr);
    BufferHandle *handle = new BufferHandle();
    buffer->SetBufferHandle(handle);
    ASSERT_NE(buffer->GetBufferHandle(), nullptr);
    handle->fd = 123;
    GSError ret = buffer->TryReclaim();
    if (buffer->IsReclaimed()) {
        printf("come into branch: isReclaimed = true\n");
        ASSERT_EQ(ret, GSERROR_OK);
        ASSERT_EQ(buffer->TryResumeIfNeeded(), GSERROR_OK);
        ASSERT_EQ(buffer->IsReclaimed(), false);
    } else {
        printf("come into branch: isReclaimed = false\n");
        ASSERT_EQ(ret, GSERROR_API_FAILED);
    }
}

/*
* Function: IsReclaimed
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. new SurfaceBufferImpl
*                  2. call IsReclaimed and check ret
 */
HWTEST_F(SurfaceBufferImplTest, IsReclaimed001, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    ASSERT_EQ(buffer->IsReclaimed(), false);
}

/*
* Function: InitMemMgrMembers
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. define a SurfaceBufferImpl object
*                  2. call InitMemMgrMembers and check value
*                  3. call InitMemMgrMembers again and check value
 */
HWTEST_F(SurfaceBufferImplTest, InitMemMgrMembers001, TestSize.Level0)
{
    SurfaceBufferImpl impl;
    impl.InitMemMgrMembers();
    ASSERT_EQ(impl.initMemMgrSucceed_, true);
    impl.InitMemMgrMembers();
    ASSERT_EQ(impl.initMemMgrSucceed_, true);
}

/*
 * Function: SurfaceBufferSyncFence
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. surfacebuffer add sync fence
 */
HWTEST_F(SurfaceBufferImplTest, SurfaceBufferSyncFence001, TestSize.Level0)
{
    SurfaceBufferImpl buffer;
    buffer.SetAndMergeSyncFence(nullptr);
    ASSERT_EQ(buffer.GetSyncFence(), nullptr);
    buffer.SetAndMergeSyncFence(SyncFence::INVALID_FENCE);
    ASSERT_NE(buffer.GetSyncFence(), nullptr);
    ASSERT_FALSE(buffer.GetSyncFence()->IsValid());
    buffer.SetAndMergeSyncFence(new SyncFence(0));
    ASSERT_NE(buffer.GetSyncFence(), nullptr);
    ASSERT_EQ(buffer.GetSyncFence()->Get(), 0);
}
}
