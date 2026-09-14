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
#include <array>
#include <cstddef>
#include <utility>
#include <securec.h>
#include <gtest/gtest.h>
#include <fcntl.h>
#include <surface.h>
#include <surface_buffer_impl.h>
#include <buffer_utils.h>
#include <metadata_helper.h>
#include "v1_1/buffer_handle_meta_key_type.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
uint64_t gBufferId = UINT64_MAX;
uint64_t gBufferId2 = UINT64_MAX;
uint64_t gBufferId3 = UINT64_MAX;
uint32_t gBufferDtorCbCount = 0;
// records the invocation order of the destructor callbacks, they must run in registration order
std::vector<uint32_t> gBufferDtorCbOrder;
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
    static void BufferDestructorCallBack(uint64_t bufferId);
    static void BufferDestructorCallBack2(uint64_t bufferId);
    static void BufferDestructorCallBack3(uint64_t bufferId);
};

void SurfaceBufferImplTest::BufferDestructorCallBack(uint64_t bufferId)
{
    gBufferId = bufferId;
    gBufferDtorCbCount++;
}

void SurfaceBufferImplTest::BufferDestructorCallBack2(uint64_t bufferId)
{
    gBufferId2 = bufferId;
}

void SurfaceBufferImplTest::BufferDestructorCallBack3(uint64_t bufferId)
{
    gBufferId3 = bufferId;
}

// keep the same values as MAX_BUFFER_DTOR_CB_NUM and LEGACY_BUFFER_DTOR_CB_NUM in surface_buffer_impl.cpp
constexpr uint32_t MAX_CB_NUM = 32;
constexpr uint32_t LEGACY_CB_NUM = 4;
// the slots left to the identity based registrations, plus one so that the last one is past their own limit
constexpr uint32_t SLOT_CB_NUM = MAX_CB_NUM - LEGACY_CB_NUM + 1;

// the identity of a registration is its function pointer, so filling the identity based registrations up to
// their own limit needs that many distinct addresses, and one template instantiation gives exactly one. each
// instantiation bumps its own counter, which keeps the linker from folding them into a single function under
// --icf=all, and lets a case check that every registered one of them was really invoked
std::array<uint32_t, SLOT_CB_NUM> gSlotCallBackHits = {};

template<uint32_t N>
void SlotBufferDestructorCallBack(uint64_t bufferId)
{
    (void)bufferId;
    gSlotCallBackHits[N]++;
    gBufferDtorCbCount++;
}

template<std::size_t... I>
constexpr std::array<void (*)(uint64_t), sizeof...(I)> MakeSlotCallBacks(std::index_sequence<I...>)
{
    return { &SlotBufferDestructorCallBack<static_cast<uint32_t>(I)>... };
}

constexpr auto gSlotCallBacks = MakeSlotCallBacks(std::make_index_sequence<SLOT_CB_NUM> {});

void SurfaceBufferImplTest::SetUpTestCase()
{
    buffer = nullptr;
    val32 = 0;
    val64 = 0;
    gBufferId = UINT64_MAX;
    gBufferId2 = UINT64_MAX;
    gBufferId3 = UINT64_MAX;
    gBufferDtorCbCount = 0;
    gBufferDtorCbOrder.clear();
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
* Function: SetSurfaceBufferVideoDimensionType&GetSurfaceBufferVideoDimensionType
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. new SurfaceBufferImpl
*                  2. call GetSurfaceBufferVideoDimensionType and check default is VIDEO_DIM_TYPE_2D
*                  3. call SetSurfaceBufferVideoDimensionType and GetSurfaceBufferVideoDimensionType and check ret
*                  4. repeatly call Set/GetSurfaceBufferVideoDimensionType and check ret
 */
HWTEST_F(SurfaceBufferImplTest, SurfaceBufferVideoDimensionType001, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    ASSERT_EQ(buffer->GetSurfaceBufferVideoDimensionType(), VideoDimType::VIDEO_DIM_TYPE_2D);
    buffer->SetSurfaceBufferVideoDimensionType(VideoDimType::VIDEO_DIM_TYPE_3D_TAB);
    ASSERT_EQ(buffer->GetSurfaceBufferVideoDimensionType(), VideoDimType::VIDEO_DIM_TYPE_3D_TAB);
    buffer->SetSurfaceBufferVideoDimensionType(VideoDimType::VIDEO_DIM_TYPE_3D_SBS);
    ASSERT_EQ(buffer->GetSurfaceBufferVideoDimensionType(), VideoDimType::VIDEO_DIM_TYPE_3D_SBS);
    buffer->SetSurfaceBufferVideoDimensionType(VideoDimType::VIDEO_DIM_TYPE_2D);
    ASSERT_EQ(buffer->GetSurfaceBufferVideoDimensionType(), VideoDimType::VIDEO_DIM_TYPE_2D);
}

/**
 * Function: SetBufferDeletedFlag & GetBufferDeletedFlag & ClearBufferDeletedFlag & IsBufferDeleted
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl
 *                  2. call GetBufferDeletedFlag and check ret
 *                  3. call SetBufferDeletedFlag and GetBufferDeletedFlag and check ret
 *                  4. repeatly call SetBufferDeletedFlag and GetBufferDeletedFlag and check ret
 */
HWTEST_F(SurfaceBufferImplTest, BufferDeletedFlag001, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    EXPECT_EQ(buffer->GetBufferDeletedFlag(), static_cast<OHOS::BufferDeletedFlag>(0));
    EXPECT_FALSE(buffer->IsBufferDeleted());

    buffer->SetBufferDeletedFlag(OHOS::BufferDeletedFlag::DELETED_FROM_CACHE);
    EXPECT_EQ(buffer->GetBufferDeletedFlag(), OHOS::BufferDeletedFlag::DELETED_FROM_CACHE);
    EXPECT_TRUE(buffer->IsBufferDeleted());

    buffer->ClearBufferDeletedFlag(OHOS::BufferDeletedFlag::DELETED_FROM_CACHE);
    EXPECT_FALSE(buffer->IsBufferDeleted());

    buffer->SetBufferDeletedFlag(OHOS::BufferDeletedFlag::DELETED_FROM_RS);
    EXPECT_EQ(buffer->GetBufferDeletedFlag(), OHOS::BufferDeletedFlag::DELETED_FROM_RS);
    EXPECT_TRUE(buffer->IsBufferDeleted());

    buffer->ClearBufferDeletedFlag(OHOS::BufferDeletedFlag::DELETED_FROM_RS);
    EXPECT_FALSE(buffer->IsBufferDeleted());
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
/*
 * Function: GetFlushedTimestamp
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: SetFlushTimestamp and GetFlushedTimestamp
 */
HWTEST_F(SurfaceBufferImplTest, GetFlushedTimestampCorrectnessTest, TestSize.Level0)
{
    sptr<SurfaceBufferImpl> bufferTmp = new SurfaceBufferImpl();
    uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    uint64_t flushedTimestamp = bufferTmp->GetFlushedTimestamp();
    ASSERT_EQ(flushedTimestamp, 0);

    bufferTmp->SetFlushTimestamp(now);
    std::cout << "now = " << now << std::endl;
    flushedTimestamp = bufferTmp->GetFlushedTimestamp();
    std::cout << "flushedTimestamp = " << flushedTimestamp << std::endl;
    ASSERT_EQ(flushedTimestamp, now);
}

/*
 * Function: CloneBufferHandle
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl
 *                  2. call SetBufferHandle and check buffer handle
 *                  3. set fd of handle to 123
 *                  4. call CloneBufferHandle and check ret
 */
HWTEST_F(SurfaceBufferImplTest, CloneBufferHandle001, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    ASSERT_EQ(buffer->GetBufferHandle(), nullptr);
    auto sret = buffer->Alloc(requestConfig);
    ASSERT_EQ(sret, OHOS::GSERROR_OK);
    ASSERT_NE(buffer->GetBufferHandle(), nullptr);
    BufferHandle* bufferHandle = buffer->CloneBufferHandle(buffer->GetBufferHandle());
    if (bufferHandle != nullptr) {
        BufferHandle *handle = buffer->GetBufferHandle();
        ASSERT_NE(bufferHandle, handle);
        ASSERT_EQ(bufferHandle->width, handle->width);
        ASSERT_EQ(bufferHandle->stride, handle->stride);
        ASSERT_EQ(bufferHandle->height, handle->height);
        ASSERT_EQ(bufferHandle->size, handle->size);
        ASSERT_EQ(bufferHandle->format, handle->format);
        ASSERT_EQ(bufferHandle->usage, handle->usage);
        ASSERT_EQ(bufferHandle->reserveFds, handle->reserveFds);
        ASSERT_EQ(bufferHandle->reserveInts, handle->reserveInts);
        ASSERT_NE(bufferHandle->fd, handle->fd);
    }
}

/*
 * Function: CloneBufferHandle
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl
 *                  2. call SetBufferHandle and check buffer handle
 *                  3. set fd of handle to -1
 *                  4. call CloneBufferHandle and check ret
 */
HWTEST_F(SurfaceBufferImplTest, CloneBufferHandle002, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    ASSERT_EQ(buffer->GetBufferHandle(), nullptr);
    BufferHandle *handle = new BufferHandle();
    buffer->SetBufferHandle(handle);
    ASSERT_NE(buffer->GetBufferHandle(), nullptr);
    handle->fd = -1;
    BufferHandle* bufferHandle = buffer->CloneBufferHandle(buffer->GetBufferHandle());
    ASSERT_EQ(bufferHandle, nullptr);
}

/*
 * Function: CloneBufferHandle
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl
 *                  2. call SetBufferHandle and check buffer handle
 *                  3. set handle to nullptr
 *                  4. call CloneBufferHandle and check ret
 */
HWTEST_F(SurfaceBufferImplTest, CloneBufferHandle003, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    ASSERT_EQ(buffer->GetBufferHandle(), nullptr);
    buffer->SetBufferHandle(nullptr);
    ASSERT_EQ(buffer->GetBufferHandle(), nullptr);
    BufferHandle* bufferHandle = buffer->CloneBufferHandle(buffer->GetBufferHandle());
    ASSERT_EQ(bufferHandle, nullptr);
}

/*
 * Function: RegisterBufferDestructorCallback001
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl
 *                  2. RegisterBufferDestructorCallback to SurfaceBuffer
 *                  3. CallBack will be exe when SurfaceBuffer is destructor
 *                  4. UnRegisterBufferDestructorCallback to SurfaceBuffer
 */
HWTEST_F(SurfaceBufferImplTest, RegisterBufferDestructorCallback001, TestSize.Level0)
{
    uint64_t bufferId = 0;
    gBufferId = UINT64_MAX;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferTmp->RegisterBufferDestructorCallback(nullptr);
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferId, UINT64_MAX);

    gBufferId = UINT64_MAX;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferId = bufferTmp->GetBufferId();
        bufferTmp->RegisterBufferDestructorCallback(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp->RegisterBufferDestructorCallback(nullptr); // invalid
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferId, bufferId);

    gBufferId = UINT64_MAX;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferId = bufferTmp->GetBufferId();
        bufferTmp->RegisterBufferDestructorCallback(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp->UnRegisterBufferDestructorCallback();
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferId, UINT64_MAX);
}

/*
 * Function: RegisterBufferDestructorCallback002
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl
 *                  2. register two different callbacks, they stand for two users
 *                  3. both callbacks are exe when SurfaceBuffer is destructor
 */
HWTEST_F(SurfaceBufferImplTest, RegisterBufferDestructorCallback002, TestSize.Level0)
{
    uint64_t bufferId = 0;
    gBufferId = UINT64_MAX;
    gBufferId2 = UINT64_MAX;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferId = bufferTmp->GetBufferId();
        bufferTmp->RegisterBufferDestructorCallback(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp->RegisterBufferDestructorCallback(&SurfaceBufferImplTest::BufferDestructorCallBack2);
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferId, bufferId);
    EXPECT_EQ(gBufferId2, bufferId);
}

/*
 * Function: RegisterBufferDestructorCallback003
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl
 *                  2. register the same function twice by RegisterBufferDestructorCallbackFunc
 *                  3. the callback is exe only once when SurfaceBuffer is destructor
 *                  4. register the same callback twice by RegisterBufferDestructorCallback
 *                  5. the callback carries no identity, so it is exe twice
 */
HWTEST_F(SurfaceBufferImplTest, RegisterBufferDestructorCallback003, TestSize.Level0)
{
    gBufferDtorCbCount = 0;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferDtorCbCount, 1U);

    gBufferDtorCbCount = 0;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        std::function<void(uint64_t)> callBack = &SurfaceBufferImplTest::BufferDestructorCallBack;
        bufferTmp->RegisterBufferDestructorCallback(callBack);
        bufferTmp->RegisterBufferDestructorCallback(callBack);
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferDtorCbCount, 2U);
}

/*
 * Function: RegisterBufferDestructorCallback004
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl
 *                  2. register a lambda callback and a function callback
 *                  3. both callbacks are exe when SurfaceBuffer is destructor
 */
HWTEST_F(SurfaceBufferImplTest, RegisterBufferDestructorCallback004, TestSize.Level0)
{
    uint64_t lambdaBufferId = UINT64_MAX;
    uint32_t lambdaCbCount = 0;
    gBufferId = UINT64_MAX;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        uint64_t bufferId = bufferTmp->GetBufferId();
        bufferTmp->RegisterBufferDestructorCallback([&lambdaBufferId, &lambdaCbCount](uint64_t id) {
            lambdaBufferId = id;
            lambdaCbCount++;
        });
        bufferTmp->RegisterBufferDestructorCallback(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp = nullptr;
        EXPECT_EQ(gBufferId, bufferId);
    }
    EXPECT_EQ(lambdaCbCount, 1U);
    EXPECT_NE(lambdaBufferId, UINT64_MAX);
}

/*
 * Function: RegisterBufferDestructorCallback005
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl
 *                  2. register more callbacks than the quota of the anonymous interface
 *                  3. only the first quota num callbacks are exe when SurfaceBuffer is destructor
 */
HWTEST_F(SurfaceBufferImplTest, RegisterBufferDestructorCallback005, TestSize.Level0)
{
    uint32_t lambdaCbCount = 0;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        for (uint32_t i = 0; i < LEGACY_CB_NUM + 4; i++) {
            bufferTmp->RegisterBufferDestructorCallback([&lambdaCbCount](uint64_t) {
                lambdaCbCount++;
            });
        }
        bufferTmp = nullptr;
    }
    // the registrations past the quota are dropped, and this interface returns void so it reports nothing
    EXPECT_EQ(lambdaCbCount, LEGACY_CB_NUM);
}

/*
 * Function: UnRegisterBufferDestructorCallback006
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl and register two callbacks of two users and one lambda callback
 *                  2. unregister one callback by UnRegisterBufferDestructorCallbackFunc
 *                  3. the unregistered callback is not exe, the others are exe when SurfaceBuffer is destructor
 */
HWTEST_F(SurfaceBufferImplTest, UnRegisterBufferDestructorCallback006, TestSize.Level0)
{
    uint64_t bufferId = 0;
    uint32_t lambdaCbCount = 0;
    gBufferId = UINT64_MAX;
    gBufferId2 = UINT64_MAX;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferId = bufferTmp->GetBufferId();
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack2);
        bufferTmp->RegisterBufferDestructorCallback([&lambdaCbCount](uint64_t) {
            lambdaCbCount++;
        });
        bufferTmp->UnRegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferId, UINT64_MAX);
    EXPECT_EQ(gBufferId2, bufferId);
    EXPECT_EQ(lambdaCbCount, 1U);
}

/*
 * Function: UnRegisterBufferDestructorCallback007
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl and register one callback
 *                  2. unregister with nullptr and with a function which is not registered
 *                  3. the registered callback is still exe when SurfaceBuffer is destructor
 */
HWTEST_F(SurfaceBufferImplTest, UnRegisterBufferDestructorCallback007, TestSize.Level0)
{
    uint64_t bufferId = 0;
    gBufferId = UINT64_MAX;
    gBufferId2 = UINT64_MAX;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferId = bufferTmp->GetBufferId();
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp->UnRegisterBufferDestructorCallbackFunc(nullptr);
        bufferTmp->UnRegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack2);
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferId, bufferId);
    EXPECT_EQ(gBufferId2, UINT64_MAX);
}

/*
 * Function: RegisterBufferDestructorCallback008
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl
 *                  2. register a lambda without capture and a lambda with capture
 *                  3. both of them are exe when SurfaceBuffer is destructor
 */
HWTEST_F(SurfaceBufferImplTest, RegisterBufferDestructorCallback008, TestSize.Level0)
{
    uint64_t bufferId = 0;
    uint32_t lambdaCbCount = 0;
    gBufferId2 = UINT64_MAX;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferId = bufferTmp->GetBufferId();
        bufferTmp->RegisterBufferDestructorCallback([](uint64_t id) {
            gBufferId2 = id;
        });
        bufferTmp->RegisterBufferDestructorCallback([&lambdaCbCount](uint64_t) {
            lambdaCbCount++;
        });
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferId2, bufferId);
    EXPECT_EQ(lambdaCbCount, 1U);
}

/*
 * Function: RegisterBufferDestructorCallback009
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl and register a null function by RegisterBufferDestructorCallbackFunc
 *                  2. unregister by both interfaces on a buffer which has no callback registered
 *                  3. the null callback is not exe, and the list is still usable after being cleared
 */
HWTEST_F(SurfaceBufferImplTest, RegisterBufferDestructorCallback009, TestSize.Level0)
{
    uint64_t bufferId = 0;
    // a null function pointer is ignored by the identity based register interface
    gBufferId = UINT64_MAX;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferTmp->RegisterBufferDestructorCallbackFunc(nullptr);
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferId, UINT64_MAX);

    // unregistering on an empty callback list is harmless, and the list still works afterwards
    gBufferId = UINT64_MAX;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferId = bufferTmp->GetBufferId();
        bufferTmp->UnRegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp->UnRegisterBufferDestructorCallback();
        bufferTmp->RegisterBufferDestructorCallback(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferId, bufferId);
}

/*
 * Function: RegisterBufferDestructorCallback010
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl and register three callbacks in order
 *                  2. they are exe in registration order when SurfaceBuffer is destructor
 *                  3. register a function and then a lambda, the function is exe first
 */
HWTEST_F(SurfaceBufferImplTest, RegisterBufferDestructorCallback010, TestSize.Level0)
{
    gBufferDtorCbOrder.clear();
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        for (uint32_t i = 0; i < 3; i++) {
            uint32_t order = i + 1;
            bufferTmp->RegisterBufferDestructorCallback([order](uint64_t) {
                gBufferDtorCbOrder.emplace_back(order);
            });
        }
        bufferTmp = nullptr;
    }
    ASSERT_EQ(gBufferDtorCbOrder.size(), 3U);
    EXPECT_EQ(gBufferDtorCbOrder[0], 1U);
    EXPECT_EQ(gBufferDtorCbOrder[1], 2U);
    EXPECT_EQ(gBufferDtorCbOrder[2], 3U);

    // the two register interfaces share one list, so the order is kept across them
    uint64_t bufferId = 0;
    bool funcRanFirst = false;
    gBufferId = UINT64_MAX;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferId = bufferTmp->GetBufferId();
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp->RegisterBufferDestructorCallback([&funcRanFirst, bufferId](uint64_t) {
            funcRanFirst = (gBufferId == bufferId);
        });
        bufferTmp = nullptr;
    }
    EXPECT_TRUE(funcRanFirst);
}

/*
 * Function: RegisterBufferDestructorCallback011
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl and register a callback which calls back into this buffer
 *                  2. the callback unregisters a registered function and registers a new one
 *                  3. no deadlock, and the batch being notified is not changed by the callback
 */
HWTEST_F(SurfaceBufferImplTest, RegisterBufferDestructorCallback011, TestSize.Level0)
{
    uint64_t bufferId = 0;
    uint32_t reentrantCount = 0;
    bool reentrantUnregRet = true;
    bool reentrantRegRet = false;
    gBufferId2 = UINT64_MAX;
    gBufferId3 = UINT64_MAX;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferId = bufferTmp->GetBufferId();
        SurfaceBuffer *rawBuffer = bufferTmp.GetRefPtr();
        bufferTmp->RegisterBufferDestructorCallback(
            [rawBuffer, &reentrantCount, &reentrantUnregRet, &reentrantRegRet](uint64_t) {
            reentrantCount++;
            // the callbacks run without the lock held, calling back into this buffer must not deadlock
            reentrantUnregRet =
                rawBuffer->UnRegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack2);
            reentrantRegRet =
                rawBuffer->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack3);
        });
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack2);
        bufferTmp = nullptr;
    }
    EXPECT_EQ(reentrantCount, 1U);
    // the registry is transferred out before the first callback runs, so it is already empty when the lambda
    // unregisters CallBack2 and there is nothing left to remove
    EXPECT_FALSE(reentrantUnregRet);
    // CallBack2 still runs, it was taken away with the registry before the notification started
    EXPECT_EQ(gBufferId2, bufferId);
    // CallBack3 goes into the emptied registry during the notification, so it is not in the batch being notified
    EXPECT_TRUE(reentrantRegRet);
    EXPECT_EQ(gBufferId3, UINT64_MAX);
}

/*
 * Function: UnRegisterBufferDestructorCallback012
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl and register two functions and one lambda
 *                  2. call the unparameterized UnRegisterBufferDestructorCallback
 *                  3. only the lambda is removed, both functions are still exe when SurfaceBuffer is destructor
 */
HWTEST_F(SurfaceBufferImplTest, UnRegisterBufferDestructorCallback012, TestSize.Level0)
{
    uint64_t bufferId = 0;
    uint32_t lambdaCbCount = 0;
    gBufferId = UINT64_MAX;
    gBufferId2 = UINT64_MAX;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferId = bufferTmp->GetBufferId();
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp->RegisterBufferDestructorCallback([&lambdaCbCount](uint64_t) {
            lambdaCbCount++;
        });
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack2);
        // the unparameterized interface removes only the registration carrying no identity, so the two identity
        // based ones survive and their modules are still notified
        bufferTmp->UnRegisterBufferDestructorCallback();
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferId, bufferId);
    EXPECT_EQ(gBufferId2, bufferId);
    EXPECT_EQ(lambdaCbCount, 0U);
}

/*
 * Function: UnRegisterBufferDestructorCallback013
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl and register three functions
 *                  2. unregister the middle one twice, and then unregister the last one
 *                  3. only the first callback is exe when SurfaceBuffer is destructor
 */
HWTEST_F(SurfaceBufferImplTest, UnRegisterBufferDestructorCallback013, TestSize.Level0)
{
    uint64_t bufferId = 0;
    gBufferId = UINT64_MAX;
    gBufferId2 = UINT64_MAX;
    gBufferId3 = UINT64_MAX;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferId = bufferTmp->GetBufferId();
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack2);
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack3);
        // erase the middle element, then erase it again, nothing is found and it must be harmless
        bufferTmp->UnRegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack2);
        bufferTmp->UnRegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack2);
        // erase the last element
        bufferTmp->UnRegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack3);
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferId, bufferId);
    EXPECT_EQ(gBufferId2, UINT64_MAX);
    EXPECT_EQ(gBufferId3, UINT64_MAX);
}

/*
 * Function: RegisterBufferDestructorCallback014
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. the anonymous interface is bounded by a quota of its own, the rest of them are dropped
 *                  2. a full anonymous quota still leaves the rest of the cap to the identity based ones
 *                  3. the identity based registrations stop at the cap minus the reserved quota
 *                  4. a repeated one is caught by the deduplication check instead, and the reserved quota is
 *                     still free at that point, so the anonymous interface still succeeds there
 */
HWTEST_F(SurfaceBufferImplTest, RegisterBufferDestructorCallback014, TestSize.Level0)
{
    uint64_t bufferId = 0;
    uint32_t lambdaCbCount = 0;
    gBufferId = UINT64_MAX;
    gBufferId2 = UINT64_MAX;
    gBufferDtorCbCount = 0;
    gSlotCallBackHits.fill(0);
    // the registrations are told apart by their function pointer, so the slot callbacks used below have to
    // stay distinct, a linker which folds identical functions together would silently break the filling
    for (uint32_t i = 1; i < SLOT_CB_NUM; i++) {
        ASSERT_NE(gSlotCallBacks[i], gSlotCallBacks[0]) << "slot callbacks were folded into one function";
    }
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferId = bufferTmp->GetBufferId();
        // register past the quota of the anonymous interface, only the first LEGACY_CB_NUM of them are kept
        for (uint32_t i = 0; i < LEGACY_CB_NUM + 4; i++) {
            bufferTmp->RegisterBufferDestructorCallback([&lambdaCbCount](uint64_t) {
                lambdaCbCount++;
            });
        }
        // the quota belongs to the anonymous interface alone and does not eat into the rest of the cap, so
        // these identity based registrations still succeed
        EXPECT_TRUE(bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack));
        EXPECT_TRUE(bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack2));
        bufferTmp = nullptr;
    }
    EXPECT_EQ(lambdaCbCount, LEGACY_CB_NUM);
    EXPECT_EQ(gBufferId, bufferId);
    EXPECT_EQ(gBufferId2, bufferId);
    EXPECT_EQ(gBufferDtorCbCount, 1U);

    lambdaCbCount = 0;
    gBufferDtorCbCount = 0;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        // take every slot which is left to the identity based registrations, each of them a distinct function
        for (uint32_t i = 0; i < MAX_CB_NUM - LEGACY_CB_NUM; i++) {
            EXPECT_TRUE(bufferTmp->RegisterBufferDestructorCallbackFunc(gSlotCallBacks[i]));
        }
        // one past their own limit is refused, the reserved quota is not available to them
        EXPECT_FALSE(bufferTmp->RegisterBufferDestructorCallbackFunc(gSlotCallBacks[MAX_CB_NUM - LEGACY_CB_NUM]));
        // the deduplication check runs before the limit is checked, so a function which is already in place
        // still succeeds even though their limit has been reached
        EXPECT_TRUE(bufferTmp->RegisterBufferDestructorCallbackFunc(gSlotCallBacks[0]));
        // the reserved quota is untouched by all of the above, this is what keeps the anonymous interface
        // usable once the identity based registrations have taken everything else
        bufferTmp->RegisterBufferDestructorCallback([&lambdaCbCount](uint64_t) {
            lambdaCbCount++;
        });
        bufferTmp = nullptr;
    }
    EXPECT_EQ(lambdaCbCount, 1U);
    EXPECT_EQ(gBufferDtorCbCount, MAX_CB_NUM - LEGACY_CB_NUM);
    // every registration which was accepted is invoked exactly once, the one which was refused never is
    for (uint32_t i = 0; i < MAX_CB_NUM - LEGACY_CB_NUM; i++) {
        EXPECT_EQ(gSlotCallBackHits[i], 1U);
    }
    EXPECT_EQ(gSlotCallBackHits[MAX_CB_NUM - LEGACY_CB_NUM], 0U);
}

/*
 * Function: RegisterBufferDestructorCallback015
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl with a sequence number, which is the other constructor
 *                  2. register a function and a lambda
 *                  3. both of them are exe when SurfaceBuffer is destructor
 */
HWTEST_F(SurfaceBufferImplTest, RegisterBufferDestructorCallback015, TestSize.Level0)
{
    uint64_t bufferId = 0;
    uint32_t lambdaCbCount = 0;
    gBufferId = UINT64_MAX;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl(1U);
        bufferId = bufferTmp->GetBufferId();
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp->RegisterBufferDestructorCallback([&lambdaCbCount](uint64_t) {
            lambdaCbCount++;
        });
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferId, bufferId);
    EXPECT_EQ(lambdaCbCount, 1U);
}

/*
 * Function: RegisterBufferDestructorCallback016
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl
 *                  2. check the return value of the identity based register and unregister interfaces
 *                  3. the return value tells the caller whether the registration is really in place
 *                  4. freeing one slot makes the list accept a new one again, and the quota reserved for the
 *                     anonymous interface stays usable all along
 */
HWTEST_F(SurfaceBufferImplTest, RegisterBufferDestructorCallback016, TestSize.Level0)
{
    gBufferDtorCbCount = 0;
    gSlotCallBackHits.fill(0);
    sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
    // a null callback is rejected by both interfaces
    EXPECT_FALSE(bufferTmp->RegisterBufferDestructorCallbackFunc(nullptr));
    EXPECT_FALSE(bufferTmp->UnRegisterBufferDestructorCallbackFunc(nullptr));
    // nothing is registered yet, so unregistering a valid function returns false as well
    EXPECT_FALSE(bufferTmp->UnRegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack));

    // the first registration succeeds, and registering the same function again succeeds idempotently
    EXPECT_TRUE(bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack));
    EXPECT_TRUE(bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack));
    // unregistering it succeeds once, the second time nothing is found so it returns false
    EXPECT_TRUE(bufferTmp->UnRegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack));
    EXPECT_FALSE(bufferTmp->UnRegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack));

    // fill the identity based registrations up to their own limit, then a further one is dropped and returns
    // false, which is how the caller learns that it will not be notified
    for (uint32_t i = 0; i < MAX_CB_NUM - LEGACY_CB_NUM; i++) {
        EXPECT_TRUE(bufferTmp->RegisterBufferDestructorCallbackFunc(gSlotCallBacks[i]));
    }
    EXPECT_FALSE(bufferTmp->RegisterBufferDestructorCallbackFunc(gSlotCallBacks[MAX_CB_NUM - LEGACY_CB_NUM]));

    // freeing one of them makes the list accept a new identity based registration again
    EXPECT_TRUE(bufferTmp->UnRegisterBufferDestructorCallbackFunc(gSlotCallBacks[0]));
    EXPECT_TRUE(bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack2));
    // the list is at their limit again, so the one which was refused before is still refused now
    EXPECT_FALSE(bufferTmp->RegisterBufferDestructorCallbackFunc(gSlotCallBacks[MAX_CB_NUM - LEGACY_CB_NUM]));

    // the quota reserved for the anonymous interface is untouched by all of the above, so it still succeeds
    bufferTmp->RegisterBufferDestructorCallback([](uint64_t) {});
    // unregistering a function which was never accepted returns false, and the unparameterized interface
    // removes the anonymous registration only
    EXPECT_FALSE(bufferTmp->UnRegisterBufferDestructorCallbackFunc(gSlotCallBacks[MAX_CB_NUM - LEGACY_CB_NUM]));
    bufferTmp->UnRegisterBufferDestructorCallback();

    // leave nothing behind, so that destructing this buffer notifies nobody and the global counter stays clean
    for (uint32_t i = 1; i < MAX_CB_NUM - LEGACY_CB_NUM; i++) {
        EXPECT_TRUE(bufferTmp->UnRegisterBufferDestructorCallbackFunc(gSlotCallBacks[i]));
    }
    EXPECT_TRUE(bufferTmp->UnRegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack2));
    bufferTmp = nullptr;
    EXPECT_EQ(gBufferDtorCbCount, 0U);
}

/*
 * Function: UnRegisterBufferDestructorCallback017
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. new SurfaceBufferImpl and register two functions only
 *                  2. the unparameterized UnRegisterBufferDestructorCallback removes none of them
 *                  3. register two lambdas, one call removes the earliest one only and the other is still exe
 *                  4. one call per registration drains them, the identity based registration is kept
 */
HWTEST_F(SurfaceBufferImplTest, UnRegisterBufferDestructorCallback017, TestSize.Level0)
{
    uint64_t bufferId = 0;
    uint32_t lambdaCbCount = 0;
    gBufferId = UINT64_MAX;
    gBufferId2 = UINT64_MAX;
    gBufferDtorCbCount = 0;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferId = bufferTmp->GetBufferId();
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack2);
        // both registrations carry an identity, so the unparameterized interface leaves them alone
        bufferTmp->UnRegisterBufferDestructorCallback();
        // two registrations carrying no identity, the second one stands for another module using this interface
        bufferTmp->RegisterBufferDestructorCallback([&lambdaCbCount](uint64_t) {
            lambdaCbCount++;
        });
        bufferTmp->RegisterBufferDestructorCallback([&lambdaCbCount](uint64_t) {
            lambdaCbCount++;
        });
        // they can not be told apart, so one call removes the earliest one only, which is the residual limitation
        // of an interface carrying no identity, while the identity based registrations are still in place
        bufferTmp->UnRegisterBufferDestructorCallback();
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferId, bufferId);
    EXPECT_EQ(gBufferId2, bufferId);
    EXPECT_EQ(gBufferDtorCbCount, 1U);
    // the second registration carrying no identity survived the single call and is still exe on destruction
    EXPECT_EQ(lambdaCbCount, 1U);

    // one call per registration drains them all, and the identity based registration is kept all along
    lambdaCbCount = 0;
    gBufferId = UINT64_MAX;
    gBufferDtorCbCount = 0;
    {
        sptr<SurfaceBuffer> bufferTmp = new SurfaceBufferImpl();
        bufferId = bufferTmp->GetBufferId();
        bufferTmp->RegisterBufferDestructorCallbackFunc(&SurfaceBufferImplTest::BufferDestructorCallBack);
        bufferTmp->RegisterBufferDestructorCallback([&lambdaCbCount](uint64_t) {
            lambdaCbCount++;
        });
        bufferTmp->RegisterBufferDestructorCallback([&lambdaCbCount](uint64_t) {
            lambdaCbCount++;
        });
        bufferTmp->UnRegisterBufferDestructorCallback();
        bufferTmp->UnRegisterBufferDestructorCallback();
        bufferTmp = nullptr;
    }
    EXPECT_EQ(gBufferId, bufferId);
    EXPECT_EQ(gBufferDtorCbCount, 1U);
    EXPECT_EQ(lambdaCbCount, 0U);
}

/*
 * Function: Write/Read all properties parcel
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: round-trip with and without handle, with sync fence
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcel001, TestSize.Level0)
{
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    // set some properties
    sbi->SetSurfaceBufferColorGamut(GraphicColorGamut::GRAPHIC_COLOR_GAMUT_SRGB);
    sbi->SetSurfaceBufferTransform(GraphicTransformType::GRAPHIC_ROTATE_180);
    sbi->SetSurfaceBufferScalingMode(ScalingMode::SCALING_MODE_NO_SCALE_CROP);
    sbi->SetSurfaceBufferVideoDimensionType(VideoDimType::VIDEO_DIM_TYPE_3D_TAB);
    sbi->SetSurfaceBufferWidth(11);
    sbi->SetSurfaceBufferHeight(22);
    sbi->SetCropMetadata({3, 4, 5, 6});
    sbi->SetAndMergeSyncFence(new SyncFence(0));

    // with no handle
    MessageParcel parcel1;
    ASSERT_EQ(sbi->WriteAllPropertiesToMessageParcel(parcel1), GSERROR_OK);
    sptr<SurfaceBufferImpl> sbiIn1 = new SurfaceBufferImpl();
    ASSERT_EQ(sbiIn1->ReadAllPropertiesFromMessageParcel(parcel1), GSERROR_OK);
    ASSERT_EQ(sbiIn1->GetSurfaceBufferColorGamut(), GraphicColorGamut::GRAPHIC_COLOR_GAMUT_SRGB);
    ASSERT_EQ(sbiIn1->GetSurfaceBufferTransform(), GraphicTransformType::GRAPHIC_ROTATE_180);
    ASSERT_EQ(sbiIn1->GetSurfaceBufferScalingMode(), ScalingMode::SCALING_MODE_NO_SCALE_CROP);
    ASSERT_EQ(sbiIn1->GetSurfaceBufferVideoDimensionType(), VideoDimType::VIDEO_DIM_TYPE_3D_TAB);
    Rect out{};
    ASSERT_TRUE(sbiIn1->GetCropMetadata(out));
    ASSERT_EQ(out.x, 3);
    ASSERT_EQ(out.y, 4);
    ASSERT_EQ(out.w, 5);
    ASSERT_EQ(out.h, 6);

    // with handle
    ASSERT_EQ(sbi->Alloc(requestConfig), OHOS::GSERROR_OK);
    MessageParcel parcel2;
    ASSERT_EQ(sbi->WriteAllPropertiesToMessageParcel(parcel2), GSERROR_OK);
    sptr<SurfaceBufferImpl> sbiIn2 = new SurfaceBufferImpl();
    ASSERT_EQ(sbiIn2->ReadAllPropertiesFromMessageParcel(parcel2), GSERROR_OK);
    ASSERT_NE(sbiIn2->GetBufferHandle(), nullptr);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel bad parcel (hasHandle=true but missing handle data)
 * Type: Function
 * Rank: Important(2)
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel001, TestSize.Level0)
{
    MessageParcel parcel;
    // Write sequenceNumber and bufferId
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    // Indicate handle exists but do not write the handle content
    ASSERT_TRUE(parcel.WriteBool(true));
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: Write/Read all properties parcel without sync fence
 * Type: Function
 * Rank: Important(2)
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelNoFence001, TestSize.Level0)
{
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    // leave syncFence_ as nullptr
    sbi->SetSurfaceBufferColorGamut(GraphicColorGamut::GRAPHIC_COLOR_GAMUT_SRGB);
    sbi->SetSurfaceBufferTransform(GraphicTransformType::GRAPHIC_ROTATE_NONE);
    sbi->SetSurfaceBufferScalingMode(ScalingMode::SCALING_MODE_SCALE_TO_WINDOW);
    sbi->SetSurfaceBufferVideoDimensionType(VideoDimType::VIDEO_DIM_TYPE_3D_TAB);
    sbi->SetSurfaceBufferWidth(33);
    sbi->SetSurfaceBufferHeight(44);
    sbi->SetCropMetadata({7, 8, 9, 10});

    MessageParcel parcel;
    ASSERT_EQ(sbi->WriteAllPropertiesToMessageParcel(parcel), GSERROR_OK);
    sptr<SurfaceBufferImpl> sbiIn = new SurfaceBufferImpl();
    ASSERT_EQ(sbiIn->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_OK);
    ASSERT_EQ(sbiIn->GetSurfaceBufferColorGamut(), GraphicColorGamut::GRAPHIC_COLOR_GAMUT_SRGB);
    ASSERT_EQ(sbiIn->GetSurfaceBufferTransform(), GraphicTransformType::GRAPHIC_ROTATE_NONE);
    ASSERT_EQ(sbiIn->GetSurfaceBufferScalingMode(), ScalingMode::SCALING_MODE_SCALE_TO_WINDOW);
    ASSERT_EQ(sbiIn->GetSurfaceBufferVideoDimensionType(), VideoDimType::VIDEO_DIM_TYPE_3D_TAB);
    Rect out{};
    ASSERT_TRUE(sbiIn->GetCropMetadata(out));
    ASSERT_EQ(out.x, 7);
    ASSERT_EQ(out.y, 8);
    ASSERT_EQ(out.w, 9);
    ASSERT_EQ(out.h, 10);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - invalid videoDimType should be reset to 2D
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: 1. write properties with VIDEO_DIM_TYPE_BUTT
 *                  2. read properties and verify videoDimType is reset to VIDEO_DIM_TYPE_2D
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadVideoDimType001, TestSize.Level0)
{
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    sbi->SetSurfaceBufferColorGamut(GraphicColorGamut::GRAPHIC_COLOR_GAMUT_SRGB);
    sbi->SetSurfaceBufferTransform(GraphicTransformType::GRAPHIC_ROTATE_NONE);
    sbi->SetSurfaceBufferScalingMode(ScalingMode::SCALING_MODE_SCALE_TO_WINDOW);
    sbi->SetSurfaceBufferVideoDimensionType(VideoDimType::VIDEO_DIM_TYPE_BUTT);
    sbi->SetSurfaceBufferWidth(33);
    sbi->SetSurfaceBufferHeight(44);
    sbi->SetCropMetadata({7, 8, 9, 10});

    MessageParcel parcel;
    ASSERT_EQ(sbi->WriteAllPropertiesToMessageParcel(parcel), GSERROR_OK);
    sptr<SurfaceBufferImpl> sbiIn = new SurfaceBufferImpl();
    ASSERT_EQ(sbiIn->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_OK);
    ASSERT_EQ(sbiIn->GetSurfaceBufferVideoDimensionType(), VideoDimType::VIDEO_DIM_TYPE_2D);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - failed to read basic info (sequenceNumber)
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test failure when reading sequenceNumber fails
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadBasicInfoFail001, TestSize.Level0)
{
    MessageParcel parcel;
    // Write only bufferId, missing sequenceNumber
    ASSERT_TRUE(parcel.WriteUint64(456));
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - failed to read basic info (bufferId)
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test failure when reading bufferId fails
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadBasicInfoFail002, TestSize.Level0)
{
    MessageParcel parcel;
    // Write only sequenceNumber, missing bufferId
    ASSERT_TRUE(parcel.WriteUint32(123));
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - failed to read hasHandle flag
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test failure when reading hasHandle flag fails
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadHasHandleFail, TestSize.Level0)
{
    MessageParcel parcel;
    // Write sequenceNumber and bufferId, but not hasHandle
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - failed to read color/transform info
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test failure when reading colorGamut fails
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadColorTransformFail001, TestSize.Level0)
{
    MessageParcel parcel;
    // Write sequenceNumber, bufferId, and hasHandle=false
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    ASSERT_TRUE(parcel.WriteBool(false));
    // Missing colorGamut, transform, scalingMode
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - failed to read color/transform info (partial)
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test failure when only colorGamut is written, transform fails
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadColorTransformFail002, TestSize.Level0)
{
    MessageParcel parcel;
    // Write sequenceNumber, bufferId, hasHandle=false, and only colorGamut
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    ASSERT_TRUE(parcel.WriteBool(false));
    ASSERT_TRUE(parcel.WriteUint32(1)); // colorGamut
    // Missing transform and scalingMode - this tests ReadUint32(transform) failure
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - failed to read color/transform info (partial 2)
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test failure when colorGamut and transform are written, scalingMode fails
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadColorTransformFail003, TestSize.Level0)
{
    MessageParcel parcel;
    // Write sequenceNumber, bufferId, hasHandle=false, colorGamut, and transform
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    ASSERT_TRUE(parcel.WriteBool(false));
    ASSERT_TRUE(parcel.WriteUint32(1)); // colorGamut
    ASSERT_TRUE(parcel.WriteUint32(2)); // transform
    // Missing scalingMode - this tests ReadUint32(scalingMode) failure
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - failed to read size info
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test failure when reading surfaceBufferWidth fails
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadSizeInfoFail001, TestSize.Level0)
{
    MessageParcel parcel;
    // Write up to scalingMode
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    ASSERT_TRUE(parcel.WriteBool(false));
    ASSERT_TRUE(parcel.WriteUint32(1)); // colorGamut
    ASSERT_TRUE(parcel.WriteUint32(2)); // transform
    ASSERT_TRUE(parcel.WriteUint32(3)); // scalingMode
    // Missing surfaceBufferWidth and surfaceBufferHeight
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - failed to read size info (partial)
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test failure when only surfaceBufferWidth is written, missing surfaceBufferHeight
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadSizeInfoFail002, TestSize.Level0)
{
    MessageParcel parcel;
    // Write up to surfaceBufferWidth
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    ASSERT_TRUE(parcel.WriteBool(false));
    ASSERT_TRUE(parcel.WriteUint32(1)); // colorGamut
    ASSERT_TRUE(parcel.WriteUint32(2)); // transform
    ASSERT_TRUE(parcel.WriteUint32(3)); // scalingMode
    ASSERT_TRUE(parcel.WriteInt32(100)); // surfaceBufferWidth
    // Missing surfaceBufferHeight
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - failed to read isReclaimed flag
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test failure when reading isReclaimed flag fails
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadIsReclaimedFail, TestSize.Level0)
{
    MessageParcel parcel;
    // Write up to surfaceBufferHeight
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    ASSERT_TRUE(parcel.WriteBool(false));
    ASSERT_TRUE(parcel.WriteUint32(1)); // colorGamut
    ASSERT_TRUE(parcel.WriteUint32(2)); // transform
    ASSERT_TRUE(parcel.WriteUint32(3)); // scalingMode
    ASSERT_TRUE(parcel.WriteInt32(100)); // surfaceBufferWidth
    ASSERT_TRUE(parcel.WriteInt32(200)); // surfaceBufferHeight
    // Missing isReclaimed
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - failed to read crop info
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test failure when reading crop_.x fails
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadCropInfoFail001, TestSize.Level0)
{
    MessageParcel parcel;
    // Write up to isReclaimed
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    ASSERT_TRUE(parcel.WriteBool(false));
    ASSERT_TRUE(parcel.WriteUint32(1)); // colorGamut
    ASSERT_TRUE(parcel.WriteUint32(2)); // transform
    ASSERT_TRUE(parcel.WriteUint32(3)); // scalingMode
    ASSERT_TRUE(parcel.WriteInt32(100)); // surfaceBufferWidth
    ASSERT_TRUE(parcel.WriteInt32(200)); // surfaceBufferHeight
    ASSERT_TRUE(parcel.WriteBool(false)); // isReclaimed
    // Missing crop info
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - failed to read crop info (partial)
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test failure when only crop_.x is written, crop_.y fails
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadCropInfoFail002, TestSize.Level0)
{
    MessageParcel parcel;
    // Write up to crop_.x
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    ASSERT_TRUE(parcel.WriteBool(false));
    ASSERT_TRUE(parcel.WriteUint32(1)); // colorGamut
    ASSERT_TRUE(parcel.WriteUint32(2)); // transform
    ASSERT_TRUE(parcel.WriteUint32(3)); // scalingMode
    ASSERT_TRUE(parcel.WriteInt32(100)); // surfaceBufferWidth
    ASSERT_TRUE(parcel.WriteInt32(200)); // surfaceBufferHeight
    ASSERT_TRUE(parcel.WriteBool(false)); // isReclaimed
    ASSERT_TRUE(parcel.WriteInt32(10)); // crop_.x
    // Missing crop_.y, crop_.w, crop_.h - this tests ReadInt32(crop_.y) failure
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - failed to read crop info (partial 2)
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test failure when crop_.x and crop_.y are written, crop_.w fails
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadCropInfoFail003, TestSize.Level0)
{
    MessageParcel parcel;
    // Write up to crop_.y
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    ASSERT_TRUE(parcel.WriteBool(false));
    ASSERT_TRUE(parcel.WriteUint32(1)); // colorGamut
    ASSERT_TRUE(parcel.WriteUint32(2)); // transform
    ASSERT_TRUE(parcel.WriteUint32(3)); // scalingMode
    ASSERT_TRUE(parcel.WriteInt32(100)); // surfaceBufferWidth
    ASSERT_TRUE(parcel.WriteInt32(200)); // surfaceBufferHeight
    ASSERT_TRUE(parcel.WriteBool(false)); // isReclaimed
    ASSERT_TRUE(parcel.WriteInt32(10)); // crop_.x
    ASSERT_TRUE(parcel.WriteInt32(20)); // crop_.y
    // Missing crop_.w and crop_.h - this tests ReadInt32(crop_.w) failure
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - failed to read crop info (partial 3)
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test failure when crop_.x, crop_.y, crop_.w are written, crop_.h fails
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadCropInfoFail004, TestSize.Level0)
{
    MessageParcel parcel;
    // Write up to crop_.w
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    ASSERT_TRUE(parcel.WriteBool(false));
    ASSERT_TRUE(parcel.WriteUint32(1)); // colorGamut
    ASSERT_TRUE(parcel.WriteUint32(2)); // transform
    ASSERT_TRUE(parcel.WriteUint32(3)); // scalingMode
    ASSERT_TRUE(parcel.WriteInt32(100)); // surfaceBufferWidth
    ASSERT_TRUE(parcel.WriteInt32(200)); // surfaceBufferHeight
    ASSERT_TRUE(parcel.WriteBool(false)); // isReclaimed
    ASSERT_TRUE(parcel.WriteInt32(10)); // crop_.x
    ASSERT_TRUE(parcel.WriteInt32(20)); // crop_.y
    ASSERT_TRUE(parcel.WriteInt32(30)); // crop_.w
    // Missing crop_.h
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - failed to read hasSyncFence flag
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test failure when reading hasSyncFence flag fails
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadHasSyncFenceFail, TestSize.Level0)
{
    MessageParcel parcel;
    // Write up to crop_.h
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    ASSERT_TRUE(parcel.WriteBool(false));
    ASSERT_TRUE(parcel.WriteUint32(1)); // colorGamut
    ASSERT_TRUE(parcel.WriteUint32(2)); // transform
    ASSERT_TRUE(parcel.WriteUint32(3)); // scalingMode
    ASSERT_TRUE(parcel.WriteInt32(100)); // surfaceBufferWidth
    ASSERT_TRUE(parcel.WriteInt32(200)); // surfaceBufferHeight
    ASSERT_TRUE(parcel.WriteBool(false)); // isReclaimed
    ASSERT_TRUE(parcel.WriteInt32(10)); // crop_.x
    ASSERT_TRUE(parcel.WriteInt32(20)); // crop_.y
    ASSERT_TRUE(parcel.WriteInt32(30)); // crop_.w
    ASSERT_TRUE(parcel.WriteInt32(40)); // crop_.h
    // Missing hasSyncFence
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - hasSyncFence true but missing fence data
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test when hasSyncFence is true but fence data is incomplete.
 *                 Note: SyncFence::ReadFromMessageParcel never returns nullptr, it returns
 *                 INVALID_FENCE when data is invalid, so the read succeeds but stores INVALID_FENCE.
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadSyncFenceIncomplete, TestSize.Level0)
{
    MessageParcel parcel;
    // Write up to hasSyncFence = true, then incomplete fence data
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    ASSERT_TRUE(parcel.WriteBool(false));
    ASSERT_TRUE(parcel.WriteUint32(1)); // colorGamut
    ASSERT_TRUE(parcel.WriteUint32(2)); // transform
    ASSERT_TRUE(parcel.WriteUint32(3)); // scalingMode
    ASSERT_TRUE(parcel.WriteInt32(100)); // surfaceBufferWidth
    ASSERT_TRUE(parcel.WriteInt32(200)); // surfaceBufferHeight
    ASSERT_TRUE(parcel.WriteBool(false)); // isReclaimed
    ASSERT_TRUE(parcel.WriteInt32(10)); // crop_.x
    ASSERT_TRUE(parcel.WriteInt32(20)); // crop_.y
    ASSERT_TRUE(parcel.WriteInt32(30)); // crop_.w
    ASSERT_TRUE(parcel.WriteInt32(40)); // crop_.h
    ASSERT_TRUE(parcel.WriteBool(true)); // hasSyncFence = true
    // Write incomplete fence data (just a valid int32, missing fd)
    ASSERT_TRUE(parcel.WriteInt32(0)); // fence flag (valid)
    // Missing file descriptor - ReadFileDescriptor will return -1
    // Write videoDimType (now at the end of parcel, after syncFence)
    ASSERT_TRUE(parcel.WriteUint32(static_cast<uint32_t>(VideoDimType::VIDEO_DIM_TYPE_2D)));
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    // The read should succeed because SyncFence::ReadFromMessageParcel returns INVALID_FENCE (not nullptr)
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_OK);
    // Verify that syncFence is set but is invalid (INVALID_FENCE)
    sptr<SyncFence> fence = sbi->GetSyncFence();
    ASSERT_NE(fence, nullptr);
    ASSERT_FALSE(fence->IsValid());
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - hasHandle=true with handle=nullptr
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test when hasHandle is true but ReadBufferHandle returns nullptr
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_HandleNullWhenHasHandleTrue, TestSize.Level0)
{
    MessageParcel parcel;
    // Write sequenceNumber, bufferId, and hasHandle = true
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    ASSERT_TRUE(parcel.WriteBool(true));
    // Write invalid handle data (write bool instead of proper handle)
    ASSERT_TRUE(parcel.WriteBool(false));
    // Write remaining valid data to avoid early return
    ASSERT_TRUE(parcel.WriteUint32(1)); // colorGamut
    ASSERT_TRUE(parcel.WriteUint32(2)); // transform
    ASSERT_TRUE(parcel.WriteUint32(3)); // scalingMode
    ASSERT_TRUE(parcel.WriteInt32(100)); // surfaceBufferWidth
    ASSERT_TRUE(parcel.WriteInt32(200)); // surfaceBufferHeight
    ASSERT_TRUE(parcel.WriteBool(false)); // isReclaimed
    ASSERT_TRUE(parcel.WriteInt32(10)); // crop_.x
    ASSERT_TRUE(parcel.WriteInt32(20)); // crop_.y
    ASSERT_TRUE(parcel.WriteInt32(30)); // crop_.w
    ASSERT_TRUE(parcel.WriteInt32(40)); // crop_.h
    ASSERT_TRUE(parcel.WriteBool(false)); // hasSyncFence
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_API_FAILED);
}

/*
 * Function: ReadAllPropertiesFromMessageParcel - hasSyncFence true with negative fence value
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test when hasSyncFence is true and fence int32 value is negative,
 *                 which causes SyncFence::ReadFromMessageParcel to return INVALID_FENCE.
 *                 The read should succeed because INVALID_FENCE is a valid sptr (not nullptr).
 */
HWTEST_F(SurfaceBufferImplTest, AllPropertiesParcelBadParcel_ReadSyncFenceNegative, TestSize.Level0)
{
    MessageParcel parcel;
    // Write complete valid data up to hasSyncFence
    ASSERT_TRUE(parcel.WriteUint32(123));
    ASSERT_TRUE(parcel.WriteUint64(456));
    ASSERT_TRUE(parcel.WriteBool(false));
    ASSERT_TRUE(parcel.WriteUint32(1)); // colorGamut
    ASSERT_TRUE(parcel.WriteUint32(2)); // transform
    ASSERT_TRUE(parcel.WriteUint32(3)); // scalingMode
    ASSERT_TRUE(parcel.WriteInt32(100)); // surfaceBufferWidth
    ASSERT_TRUE(parcel.WriteInt32(200)); // surfaceBufferHeight
    ASSERT_TRUE(parcel.WriteBool(false)); // isReclaimed
    ASSERT_TRUE(parcel.WriteInt32(10)); // crop_.x
    ASSERT_TRUE(parcel.WriteInt32(20)); // crop_.y
    ASSERT_TRUE(parcel.WriteInt32(30)); // crop_.w
    ASSERT_TRUE(parcel.WriteInt32(40)); // crop_.h
    ASSERT_TRUE(parcel.WriteBool(true)); // hasSyncFence = true
    // Write fence data with negative value (indicates invalid fence)
    ASSERT_TRUE(parcel.WriteInt32(-1)); // negative fence value
    // Write videoDimType (now at the end of parcel, after syncFence)
    ASSERT_TRUE(parcel.WriteUint32(static_cast<uint32_t>(VideoDimType::VIDEO_DIM_TYPE_2D)));
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    // The read should succeed because SyncFence::ReadFromMessageParcel returns INVALID_FENCE when fence < 0
    ASSERT_EQ(sbi->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_OK);
    // Verify that syncFence is set but is invalid (INVALID_FENCE)
    sptr<SyncFence> fence = sbi->GetSyncFence();
    ASSERT_NE(fence, nullptr);
    ASSERT_EQ(fence, SyncFence::INVALID_FENCE);
    ASSERT_FALSE(fence->IsValid());
}

/*
 * Function: WriteAllPropertiesToMessageParcel with all properties set
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test writing all properties including isReclaimed=true
 */
HWTEST_F(SurfaceBufferImplTest, WriteAllPropertiesWithReclaimedFlag, TestSize.Level0)
{
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    sbi->SetSurfaceBufferColorGamut(GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DISPLAY_P3);
    sbi->SetSurfaceBufferTransform(GraphicTransformType::GRAPHIC_ROTATE_270);
    sbi->SetSurfaceBufferScalingMode(ScalingMode::SCALING_MODE_SCALE_CROP);
    sbi->SetSurfaceBufferWidth(1920);
    sbi->SetSurfaceBufferHeight(1080);
    sbi->SetCropMetadata({0, 0, 1920, 1080});

    // Manually set isReclaimed_ to true (accessible because test is built with -Dprivate=public)
    sbi->isReclaimed_.store(true);

    MessageParcel parcel;
    ASSERT_EQ(sbi->WriteAllPropertiesToMessageParcel(parcel), GSERROR_OK);

    sptr<SurfaceBufferImpl> sbiIn = new SurfaceBufferImpl();
    ASSERT_EQ(sbiIn->ReadAllPropertiesFromMessageParcel(parcel), GSERROR_OK);
    ASSERT_EQ(sbiIn->GetSurfaceBufferColorGamut(), GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DISPLAY_P3);
    ASSERT_EQ(sbiIn->GetSurfaceBufferTransform(), GraphicTransformType::GRAPHIC_ROTATE_270);
    ASSERT_EQ(sbiIn->GetSurfaceBufferScalingMode(), ScalingMode::SCALING_MODE_SCALE_CROP);
    ASSERT_EQ(sbiIn->GetSurfaceBufferWidth(), 1920);
    ASSERT_EQ(sbiIn->GetSurfaceBufferHeight(), 1080);
    Rect out{};
    ASSERT_TRUE(sbiIn->GetCropMetadata(out));
    ASSERT_EQ(out.x, 0);
    ASSERT_EQ(out.y, 0);
    ASSERT_EQ(out.w, 1920);
    ASSERT_EQ(out.h, 1080);
    ASSERT_EQ(sbiIn->IsReclaimed(), true);
}

/*
 * Function: ReadBufferRequestConfig - invalid videoDimType should be reset to 2D
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: 1. write request config with VIDEO_DIM_TYPE_BUTT
 *                  2. read request config and verify videoDimType is reset to VIDEO_DIM_TYPE_2D
 */
HWTEST_F(SurfaceBufferImplTest, ReadBufferRequestConfigBadVideoDimType001, TestSize.Level0)
{
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    BufferRequestConfig config = {};
    config.width = 100;
    config.height = 200;
    config.strideAlignment = 8;
    config.format = GRAPHIC_PIXEL_FMT_RGBA_8888;
    config.usage = 0;
    config.timeout = 0;
    config.colorGamut = GraphicColorGamut::GRAPHIC_COLOR_GAMUT_SRGB;
    config.transform = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    sbi->SetBufferRequestConfig(config);
    sbi->SetSurfaceBufferScalingMode(ScalingMode::SCALING_MODE_SCALE_TO_WINDOW);
    sbi->SetSurfaceBufferVideoDimensionType(VideoDimType::VIDEO_DIM_TYPE_BUTT);

    MessageParcel parcel;
    ASSERT_EQ(sbi->WriteBufferRequestConfig(parcel), GSERROR_OK);
    sptr<SurfaceBufferImpl> sbiIn = new SurfaceBufferImpl();
    ASSERT_EQ(sbiIn->ReadBufferRequestConfig(parcel), GSERROR_OK);
    ASSERT_EQ(sbiIn->GetSurfaceBufferVideoDimensionType(), VideoDimType::VIDEO_DIM_TYPE_2D);
}

/*
 * Function: WriteBufferProperty and ReadBufferProperty round trip
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test writing and reading buffer properties through MessageParcel
 */
HWTEST_F(SurfaceBufferImplTest, BufferPropertyParcelRoundTrip001, TestSize.Level0)
{
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    sbi->SetSurfaceBufferColorGamut(GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DISPLAY_P3);
    sbi->SetSurfaceBufferTransform(GraphicTransformType::GRAPHIC_ROTATE_270);
    sbi->SetSurfaceBufferScalingMode(ScalingMode::SCALING_MODE_SCALE_CROP);
    sbi->SetSurfaceBufferWidth(320);
    sbi->SetSurfaceBufferHeight(240);

    BufferRequestConfig requestConfig = {
        .width = 320,
        .height = 240,
        .strideAlignment = 16,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE,
        .timeout = 99,
        .colorGamut = GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DISPLAY_P3,
        .transform = GraphicTransformType::GRAPHIC_ROTATE_270,
    };
    sbi->SetBufferRequestConfig(requestConfig);

    MessageParcel parcel;
    ASSERT_EQ(sbi->WriteBufferProperty(parcel), GSERROR_OK);

    sptr<SurfaceBufferImpl> sbiIn = new SurfaceBufferImpl();
    ASSERT_EQ(sbiIn->ReadBufferProperty(parcel), GSERROR_OK);
    ASSERT_EQ(sbiIn->GetSurfaceBufferWidth(), 320);
    ASSERT_EQ(sbiIn->GetSurfaceBufferHeight(), 240);
    ASSERT_EQ(sbiIn->GetSurfaceBufferColorGamut(), GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DISPLAY_P3);
    ASSERT_EQ(sbiIn->GetSurfaceBufferTransform(), GraphicTransformType::GRAPHIC_ROTATE_270);
    ASSERT_EQ(sbiIn->GetSurfaceBufferScalingMode(), ScalingMode::SCALING_MODE_SCALE_CROP);

    BufferRequestConfig actualConfig = sbiIn->GetBufferRequestConfig();
    ASSERT_EQ(actualConfig.width, 320);
    ASSERT_EQ(actualConfig.height, 240);
    ASSERT_EQ(actualConfig.strideAlignment, 16);
    ASSERT_EQ(actualConfig.format, GRAPHIC_PIXEL_FMT_RGBA_8888);
    ASSERT_EQ(actualConfig.usage, BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE);
    ASSERT_EQ(actualConfig.timeout, 99);
    ASSERT_EQ(actualConfig.colorGamut, GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DISPLAY_P3);
    ASSERT_EQ(actualConfig.transform, GraphicTransformType::GRAPHIC_ROTATE_270);
}

/*
 * Function: ReadBufferProperty failure when parcel data is incomplete
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test the read failure branch when the last property is missing
 */
HWTEST_F(SurfaceBufferImplTest, BufferPropertyReadFailMissingTransform001, TestSize.Level0)
{
    MessageParcel parcel;
    ASSERT_TRUE(parcel.WriteInt32(320));
    ASSERT_TRUE(parcel.WriteInt32(240));
    ASSERT_TRUE(parcel.WriteInt32(16));
    ASSERT_TRUE(parcel.WriteInt32(GRAPHIC_PIXEL_FMT_RGBA_8888));
    ASSERT_TRUE(parcel.WriteUint64(BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE));
    ASSERT_TRUE(parcel.WriteInt32(99));
    ASSERT_TRUE(parcel.WriteUint32(static_cast<uint32_t>(GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DISPLAY_P3)));
    ASSERT_TRUE(parcel.WriteUint32(static_cast<uint32_t>(GraphicTransformType::GRAPHIC_ROTATE_270)));
    ASSERT_TRUE(parcel.WriteUint32(static_cast<uint32_t>(ScalingMode::SCALING_MODE_SCALE_CROP)));
    // Missing the final transform field

    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    ASSERT_EQ(sbi->ReadBufferProperty(parcel), GSERROR_API_FAILED);
}

/*
 * Function: WriteBufferProperty failure when parcel is full
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test the write failure branch when MessageParcel has no writable space
 */
HWTEST_F(SurfaceBufferImplTest, BufferPropertyWriteFailWhenParcelFull001, TestSize.Level0)
{
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    BufferRequestConfig requestConfig = {
        .width = 320,
        .height = 240,
        .strideAlignment = 16,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE,
        .timeout = 99,
        .colorGamut = GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DISPLAY_P3,
        .transform = GraphicTransformType::GRAPHIC_ROTATE_270,
    };
    sbi->SetBufferRequestConfig(requestConfig);
    sbi->SetSurfaceBufferColorGamut(GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DISPLAY_P3);
    sbi->SetSurfaceBufferTransform(GraphicTransformType::GRAPHIC_ROTATE_270);
    sbi->SetSurfaceBufferScalingMode(ScalingMode::SCALING_MODE_SCALE_CROP);

    constexpr size_t BUFFER_SIZE = 200 * 1024;
    std::vector<uint8_t> fillBuffer(BUFFER_SIZE, 0xFF);

    MessageParcel parcel;
    ASSERT_EQ(sbi->WriteBufferProperty(parcel), GSERROR_OK);
}

/*
 * Function: ReadFromBufferInfo
 * Type: Function
 * Rank: Important(2)
 * CaseDescription: Test copying buffer info into SurfaceBufferImpl
 */
HWTEST_F(SurfaceBufferImplTest, ReadFromBufferInfo001, TestSize.Level0)
{
    sptr<SurfaceBufferImpl> sbi = new SurfaceBufferImpl();
    RSBufferInfo bufferInfo = {
        .surfaceBufferColorGamut = GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DISPLAY_P3,
        .transform = GraphicTransformType::GRAPHIC_ROTATE_180,
        .scalingMode = ScalingMode::SCALING_MODE_NO_SCALE_CROP,
        .surfaceBufferWidth = 1234,
        .surfaceBufferHeight = 5678,
        .sequence = 42,
        .bufferRequestConfig = {
            .width = 1234,
            .height = 5678,
            .strideAlignment = 8,
            .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
            .usage = BUFFER_USAGE_CPU_READ,
            .timeout = 77,
            .colorGamut = GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DISPLAY_P3,
            .transform = GraphicTransformType::GRAPHIC_ROTATE_180,
        },
    };

    ASSERT_EQ(sbi->ReadFromBufferInfo(bufferInfo), GSERROR_OK);
    ASSERT_EQ(sbi->GetSurfaceBufferColorGamut(), GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DISPLAY_P3);
    ASSERT_EQ(sbi->GetSurfaceBufferTransform(), GraphicTransformType::GRAPHIC_ROTATE_180);
    ASSERT_EQ(sbi->GetSurfaceBufferScalingMode(), ScalingMode::SCALING_MODE_NO_SCALE_CROP);
    ASSERT_EQ(sbi->GetSurfaceBufferWidth(), 1234);
    ASSERT_EQ(sbi->GetSurfaceBufferHeight(), 5678);

    BufferRequestConfig actualConfig = sbi->GetBufferRequestConfig();
    ASSERT_EQ(actualConfig.width, 1234);
    ASSERT_EQ(actualConfig.height, 5678);
    ASSERT_EQ(actualConfig.strideAlignment, 8);
    ASSERT_EQ(actualConfig.format, GRAPHIC_PIXEL_FMT_RGBA_8888);
    ASSERT_EQ(actualConfig.usage, BUFFER_USAGE_CPU_READ);
    ASSERT_EQ(actualConfig.timeout, 77);
    ASSERT_EQ(actualConfig.colorGamut, GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DISPLAY_P3);
    ASSERT_EQ(actualConfig.transform, GraphicTransformType::GRAPHIC_ROTATE_180);
}

/*
* Function: SyncFence::ReadFromMessageParcel
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call ReadFromMessageParcel with empty parcel (ReadInt32 fails)
*                  2. check ret is INVALID_FENCE
*/
HWTEST_F(SurfaceBufferImplTest, SyncFenceReadFromMessageParcelEmpty001, TestSize.Level0)
{
    MessageParcel parcel;
    // empty parcel: ReadInt32 fails, should return INVALID_FENCE directly
    sptr<SyncFence> fence = SyncFence::ReadFromMessageParcel(parcel, nullptr);
    ASSERT_EQ(fence, SyncFence::INVALID_FENCE);
    ASSERT_FALSE(fence->IsValid());
}

/*
 * Function: SetSingleBufferModeTest
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 */
HWTEST_F(SurfaceBufferImplTest, SetSingleBufferModeTest001, TestSize.Level0)
{
    buffer = new SurfaceBufferImpl();
    buffer->GetAndResetSingleBufferMode();
    buffer->SetSingleBufferMode(SingleBufferMode::SINGLE_BUFFER_MODE_TO_SINGLE);
    ASSERT_EQ(buffer->GetAndResetSingleBufferMode(), SingleBufferMode::SINGLE_BUFFER_MODE_TO_SINGLE);
}
}
