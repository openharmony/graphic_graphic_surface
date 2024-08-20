/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "surface_fuzzer.h"

#include <securec.h>

#include "data_generate.h"
#include "iconsumer_surface.h"
#include "metadata_helper.h"
#include "producer_surface_delegator.h"
#include "surface.h"
#include "surface_buffer.h"
#include "surface_buffer_impl.h"
#include "sync_fence.h"
#include <iostream>

using namespace g_fuzzCommon;
using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
namespace OHOS {
    class TestConsumerListenerClazz : public IBufferConsumerListenerClazz {
    public:
        void OnBufferAvailable() override
        {
        }
    };
    GSError OnBufferRelease(sptr<SurfaceBuffer> &buffer)
    {
        return GSERROR_OK;
    }

    void BufferDeleteCbFunc(int32_t seqNum)
    {
        (void)seqNum;
    }

    void MetadataHelperFuzzTest()
    {
        CM_ColorSpaceInfo colorSpaceInfo1;
        MetadataHelper::ConvertColorSpaceTypeToInfo(CM_SRGB_FULL, colorSpaceInfo1);
        CM_ColorSpaceInfo colorSpaceInfo = GetData<CM_ColorSpaceInfo>();
        CM_ColorSpaceType colorSpaceType;
        MetadataHelper::ConvertColorSpaceInfoToType(colorSpaceInfo, colorSpaceType);

        sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
        MetadataHelper::SetColorSpaceInfo(buffer, colorSpaceInfo);
        MetadataHelper::GetColorSpaceInfo(buffer, colorSpaceInfo);

        MetadataHelper::SetColorSpaceType(buffer, CM_SRGB_FULL);
        MetadataHelper::GetColorSpaceType(buffer, colorSpaceType);

        MetadataHelper::SetHDRMetadataType(buffer, CM_VIDEO_HDR_VIVID);
        CM_HDR_Metadata_Type hdrMetadataType;
        MetadataHelper::GetHDRMetadataType(buffer, hdrMetadataType);

        HdrStaticMetadata metadataSet = GetData<HdrStaticMetadata>();
        MetadataHelper::SetHDRStaticMetadata(buffer, metadataSet);
        MetadataHelper::GetHDRStaticMetadata(buffer, metadataSet);
        std::vector<uint8_t> metadataSets;
        for (uint32_t i = 0; i < UINT8_MAX; i++) {
            metadataSets.emplace_back(GetData<uint8_t>());
        }
        MetadataHelper::SetHDRDynamicMetadata(buffer, metadataSets);
        MetadataHelper::GetHDRDynamicMetadata(buffer, metadataSets);

        std::vector<uint8_t> hdrStaticMetadata;
        for (uint32_t i = 0; i < UINT8_MAX; i++) {
            hdrStaticMetadata.emplace_back(GetData<uint8_t>());
        }
        MetadataHelper::SetHDRStaticMetadata(buffer, hdrStaticMetadata);
        MetadataHelper::GetHDRStaticMetadata(buffer, hdrStaticMetadata);
    }

    void SurfaceFuzzTest4(sptr<OHOS::Surface> pSurface, sptr<OHOS::IConsumerSurface> cSurface)
    {
        int32_t width = GetData<int32_t>();
        int32_t height = GetData<int32_t>();
        cSurface->SetDefaultWidthAndHeight(width, height);
        uint64_t usage = GetData<uint64_t>();
        cSurface->SetDefaultUsage(usage);
        GraphicTransformType transform = GetData<GraphicTransformType>();
        pSurface->SetTransform(transform);
        cSurface->SetTransform(transform);
        OHSurfaceSource sourceType = GetData<OHSurfaceSource>();
        pSurface->SetSurfaceSourceType(sourceType);
        cSurface->SetSurfaceSourceType(sourceType);
        std::string appFrameworkType = GetStringFromData(STR_LEN);
        cSurface->SetSurfaceAppFrameworkType(appFrameworkType);
        pSurface->SetSurfaceAppFrameworkType(appFrameworkType);
        std::string key = "";
        cSurface->GetUserData(key);
        cSurface->GetQueueSize();
        cSurface->GetSurfaceSourceType();
        cSurface->GetSurfaceAppFrameworkType();
    }

    void SurfaceFuzzTest3(sptr<OHOS::Surface> pSurface, sptr<OHOS::IConsumerSurface> cSurface)
    {
        std::vector<sptr<SurfaceBuffer>> buffers;
        std::vector<sptr<SyncFence>> fences;
        BufferRequestConfig config = GetData<BufferRequestConfig>();
        pSurface->RequestBuffers(buffers, fences, config);
        BufferFlushConfigWithDamages flushConfig = GetData<BufferFlushConfigWithDamages>();
        std::vector<BufferFlushConfigWithDamages> flushConfigs;
        for (size_t i = 0; i < buffers.size(); i++) {
            flushConfigs.emplace_back(flushConfig);
            fences.emplace_back(new SyncFence(-1));
        }
        pSurface->FlushBuffers(buffers, fences, flushConfigs);
        ScalingMode scalingMode = GetData<ScalingMode>();
        uint32_t sequence = GetData<uint32_t>();
        cSurface->SetScalingMode(sequence, scalingMode);
        cSurface->GetScalingMode(sequence, scalingMode);
        GraphicHDRMetaData metaData = GetData<GraphicHDRMetaData>();
        std::vector<GraphicHDRMetaData> metaDatas = {metaData};
        uint8_t metaData2 = GetData<uint8_t>();
        std::vector<uint8_t> metaDatas2 = {metaData2};

        cSurface->SetMetaData(sequence, metaDatas);
        GraphicHDRMetadataKey metakey = GetData<GraphicHDRMetadataKey>();
        cSurface->SetMetaDataSet(sequence, metakey, metaDatas2);
        HDRMetaDataType metaType = GetData<HDRMetaDataType>();
        cSurface->QueryMetaDataType(sequence, metaType);
        cSurface->GetMetaData(sequence, metaDatas);
        cSurface->GetMetaDataSet(sequence, metakey, metaDatas2);
        GraphicPresentTimestamp ptimestamp = GetData<GraphicPresentTimestamp>();
        cSurface->SetPresentTimestamp(sequence, ptimestamp);
        cSurface->GetTunnelHandle();
        bool hold = GetData<bool>();
        cSurface->SetBufferHold(hold);
        cSurface->GoBackground();
        cSurface->GetTransform();
        GraphicTransformType hintType = GetData<GraphicTransformType>();
        cSurface->SetTransformHint(hintType);
        cSurface->GetTransformHint();
        cSurface->UnregisterConsumerListener();
        bool isIncache = GetData<bool>();
        cSurface->IsSurfaceBufferInCache(sequence, isIncache);
        pSurface->UnRegisterReleaseListener();
    }

    void SurfaceFuzzTest1(sptr<OHOS::Surface> pSurface, sptr<OHOS::IConsumerSurface> cSurface)
    {
        pSurface->IsConsumer();
        cSurface->IsConsumer();
        bool cleanAll = GetData<bool>();
        pSurface->CleanCache(cleanAll);
        pSurface->QueryIfBufferAvailable();
        pSurface->GetName();
        pSurface->GoBackground();
        pSurface->Connect();
        uint64_t defaultUsage = GetData<uint64_t>();
        pSurface->SetDefaultUsage(defaultUsage);
        pSurface->GetDefaultUsage();
        std::string userKey = GetStringFromData(STR_LEN);
        pSurface->GetUserData(userKey);
        sptr<SyncFence> syncFence = new SyncFence(-1);
        uint32_t queueSize = GetData<uint32_t>();
        int32_t fenceFd = syncFence->Get();
        sptr<OHOS::SurfaceBuffer> buffer = new SurfaceBufferImpl(GetData<uint32_t>());
        BufferRequestConfig requestConfig = GetData<BufferRequestConfig>();
        BufferFlushConfig flushConfig = GetData<BufferFlushConfig>();
        pSurface->RequestBuffer(buffer, fenceFd, requestConfig);
        pSurface->CancelBuffer(buffer);
        pSurface->FlushBuffer(buffer, fenceFd, flushConfig);
        pSurface->AttachBuffer(buffer);
        pSurface->DetachBuffer(buffer);
        int32_t timeout = 0;
        pSurface->AttachBuffer(buffer, timeout);
        pSurface->DetachBuffer(buffer);
        pSurface->SetQueueSize(queueSize);
        Rect damage = GetData<Rect>();
        int64_t timestamp = GetData<int64_t>();
        cSurface->AcquireBuffer(buffer, fenceFd, timestamp, damage);
        cSurface->ReleaseBuffer(buffer, fenceFd);
        cSurface->AttachBuffer(buffer);
        cSurface->DetachBuffer(buffer);
        sptr<SurfaceBuffer> buffer1 = SurfaceBuffer::Create();
        cSurface->AttachBuffer(buffer1, timeout);
        cSurface->DetachBuffer(buffer1);
        cSurface->AttachBufferToQueue(buffer);
        cSurface->DetachBufferFromQueue(buffer);
        cSurface->SetQueueSize(queueSize);
    }

    void SurfaceFuzzTest2()
    {
        // get data
        std::string name = GetStringFromData(STR_LEN);
        bool isShared = GetData<bool>();
        std::string key = GetStringFromData(STR_LEN);
        std::string val = GetStringFromData(STR_LEN);
        BufferVerifyAllocInfo info = GetData<BufferVerifyAllocInfo>();
        uint32_t sequence = GetData<uint32_t>();
        ScalingMode scalingMode = GetData<ScalingMode>();
        GraphicPresentTimestampType type = GetData<GraphicPresentTimestampType>();
        int64_t time = GetData<int64_t>();
        std::string result = GetStringFromData(STR_LEN);
        sptr<OHOS::IConsumerSurface> cSurface = OHOS::IConsumerSurface::Create(name, isShared);
        auto producer = cSurface->GetProducer();
        sptr<OHOS::Surface> pSurface = OHOS::Surface::CreateSurfaceAsProducer(producer);
        sptr<ProducerSurfaceDelegator> surfaceDelegator = ProducerSurfaceDelegator::Create();
        cSurface->RegisterSurfaceDelegator(surfaceDelegator->AsObject());
        cSurface->QueryIfBufferAvailable();
        sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
        cSurface->RegisterConsumerListener(listener);
        TestConsumerListenerClazz* listenerClazz = new TestConsumerListenerClazz();
        cSurface->RegisterConsumerListener(listenerClazz);
        cSurface->RegisterReleaseListener([](sptr<SurfaceBuffer> &buffer) { return GSERROR_OK; });
        cSurface->RegisterDeleteBufferListener(BufferDeleteCbFunc);
        std::string funcName = GetStringFromData(STR_LEN);
        cSurface->RegisterUserDataChangeListener(funcName,
            [](const std::string& key, const std::string& val) {});
        pSurface->SetUserData(key, val);
        bool supported = GetData<bool>();
        std::vector<bool> supporteds = {supported};
        std::vector<BufferVerifyAllocInfo> infos = {info};
        pSurface->IsSupportedAlloc(infos, supporteds);
        pSurface->SetScalingMode(sequence, scalingMode);
        pSurface->GetPresentTimestamp(sequence, type, time);
        cSurface->SetUserData(key, val);
        cSurface->Dump(result);
        cSurface->GetDefaultWidth();
        cSurface->GetDefaultUsage();
        cSurface->GetHdrWhitePointBrightness();
        cSurface->GetName();
        cSurface->GetDefaultHeight();
        cSurface->GetSdrWhitePointBrightness();
        SurfaceFuzzTest1(pSurface, cSurface);
        SurfaceFuzzTest3(pSurface, cSurface);
        SurfaceFuzzTest4(pSurface, cSurface);
        MetadataHelperFuzzTest();
    }

    bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
    {
        if (data == nullptr) {
            return false;
        }
        g_data = data;
        g_size = size;
        g_pos = 0;
        std::string name = GetStringFromData(STR_LEN);
        bool isShared = GetData<bool>();
        uint32_t seqNum = GetData<uint32_t>();
        sptr<OHOS::IConsumerSurface> cSurface = OHOS::IConsumerSurface::Create(name, isShared);
        auto producer = cSurface->GetProducer();
        sptr<OHOS::Surface> pSurface = OHOS::Surface::CreateSurfaceAsProducer(producer);
        sptr<ProducerSurfaceDelegator> surfaceDelegator = ProducerSurfaceDelegator::Create();
        pSurface->RegisterSurfaceDelegator(surfaceDelegator->AsObject());
        pSurface->RegisterReleaseListener([](const sptr<SurfaceBuffer> &buffer, const sptr<SyncFence> &fence) ->
            GSError {
                return GSERROR_OK;
        });
        std::string funcName = GetStringFromData(STR_LEN);
        pSurface->RegisterUserDataChangeListener(funcName,
            [](const std::string& key, const std::string& value) -> void {});
        sptr<OHOS::SurfaceBuffer> buffer = new SurfaceBufferImpl(seqNum);
        sptr<SyncFence> syncFence = SyncFence::INVALID_FENCE;
        float matrix[16];
        uint32_t matrixSize = GetData<uint32_t>();
        bool isUseNewMatrix = GetData<bool>();
        pSurface->AcquireLastFlushedBuffer(buffer, syncFence, matrix, matrixSize, isUseNewMatrix);
        pSurface->ReleaseLastFlushedBuffer(buffer);
        GraphicTransformType transformType;
        cSurface->GetSurfaceBufferTransformType(buffer, &transformType);
        SurfaceFuzzTest2();
        pSurface->UnRegisterUserDataChangeListener(funcName);
        pSurface->ClearUserDataChangeListener();
        cSurface->ClearUserDataChangeListener();
        return true;
    }
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}

