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

#include "bufferqueueproducer_fuzzer.h"
#include <iremote_proxy.h>
#include <iremote_object.h>
#include <securec.h>
#include "buffer_queue.h"
#include "buffer_queue_producer.h"
#include "data_generate.h"
#include "surface.h"
#include "surface_buffer.h"
#include "surface_buffer_impl.h"
#include "buffer_extra_data.h"
#include "buffer_extra_data_impl.h"
#include "sync_fence.h"
#include <buffer_producer_listener.h>

using namespace g_fuzzCommon;
namespace OHOS {
    sptr<BufferExtraData> GetBufferExtraDataFromData()
    {
        // get data
        std::string keyInt32 = GetStringFromData(STR_LEN);
        int32_t valueInt32 = GetData<int32_t>();
        std::string keyInt64 = GetStringFromData(STR_LEN);
        int64_t valueInt64 = GetData<int64_t>();
        std::string keyDouble = GetStringFromData(STR_LEN);
        double valueDouble = GetData<double>();
        std::string keyStr = GetStringFromData(STR_LEN);
        std::string valueStr = GetStringFromData(STR_LEN);

        // test
        sptr<BufferExtraData> bedata = new BufferExtraDataImpl();
        bedata->ExtraSet(keyInt32, valueInt32);
        bedata->ExtraSet(keyInt64, valueInt64);
        bedata->ExtraSet(keyDouble, valueDouble);
        bedata->ExtraSet(keyStr, valueStr);

        bedata->ExtraGet(keyInt32, valueInt32);
        bedata->ExtraGet(keyInt64, valueInt64);
        bedata->ExtraGet(keyDouble, valueDouble);
        bedata->ExtraGet(keyStr, valueStr);
        return bedata;
    }
    void BufferQueueProducerFuzzTest(const sptr<BufferQueueProducer> &bqp)
    {
        // get data
        std::string name = GetStringFromData(STR_LEN);
        uint32_t queueSize = GetData<uint32_t>();
        uint64_t usage = GetData<uint64_t>();
        GraphicTransformType transform = GetData<GraphicTransformType >();
        uint32_t sequence = GetData<uint32_t>();
        std::vector<GraphicHDRMetaData> metaData;
        for (int i = 0; i < 10; i++) { // add 10 elements to the vector
            GraphicHDRMetaData hDRMetaData = GetData<GraphicHDRMetaData>();
            metaData.push_back(hDRMetaData);
        }
        GraphicHDRMetadataKey key = GetData<GraphicHDRMetadataKey>();
        std::vector<uint8_t> metaDataSet;
        for (int i = 0; i < 10; i++) { // add 10 elements to the vector
            uint8_t metaDataElement = GetData<uint8_t>();
            metaDataSet.push_back(metaDataElement);
        }
        std::string result = GetStringFromData(STR_LEN);
        bool status = GetData<bool>();
        uint32_t reserveInts = GetData<uint32_t>() % 0x100000; // no more than 0x100000
        OHSurfaceSource sourceType = GetData<OHSurfaceSource>();
        std::string appFrameworkType = GetStringFromData(STR_LEN);

        // test
        bqp->SetQueueSize(queueSize);
        bqp->SetDefaultUsage(usage);
        bqp->SetTransform(transform);
        bqp->SetMetaData(sequence, metaData);
        bqp->SetMetaDataSet(sequence, key, metaDataSet);
        bqp->SetStatus(status);
        GraphicExtDataHandle *handle = AllocExtDataHandle(reserveInts);
        sptr<SurfaceTunnelHandle> tunnelHandle = new SurfaceTunnelHandle();
        tunnelHandle->SetHandle(handle);
        FreeExtDataHandle(handle);
        bqp->SetSurfaceSourceType(sourceType);
        bqp->GetSurfaceSourceType(sourceType);
        bqp->SetSurfaceAppFrameworkType(appFrameworkType);
        ScalingMode mode = GetData<ScalingMode>();
        bqp->SetScalingMode(mode);
        bool bufferHold = GetData<bool>();
        bqp->SetBufferHold(bufferHold);
        int64_t time = 0;
        GraphicPresentTimestampType timestampType = GetData<GraphicPresentTimestampType>();
        bqp->GetPresentTimestamp(sequence, timestampType, time);
    }

    void BufferQueueProducerFuzzTest1(const sptr<BufferQueueProducer> &bqp)
    {
        bqp->GetNativeSurface();
        bqp->GetStatus();
        bool cleanAll = GetData<bool>();
        bqp->CleanCache(cleanAll);
        sptr<OHOS::SurfaceBuffer> buffer1 = SurfaceBuffer::Create();
        sptr<OHOS::SurfaceBuffer> buffer = SurfaceBuffer::Create();
        int32_t timeOut = 0;
        bqp->AttachBuffer(buffer1, timeOut);
        bqp->DetachBuffer(buffer1);
        bqp->GetUniqueId();
        float matrix[16] = {0};
        bool isUseNewMatrix = GetData<bool>();
        sptr<SyncFence> syncFence = new SyncFence(-1);
        bqp->GetLastFlushedBuffer(buffer, syncFence, matrix, isUseNewMatrix);
        bqp->AttachBufferToQueue(buffer);
        bqp->DetachBufferFromQueue(buffer);
        bqp->UnRegisterReleaseListener();
        GraphicTransformType transformType = GetData<GraphicTransformType>();
        bqp->SetTransform(transformType);
        bqp->GetTransform(transformType);
        bqp->SetTransformHint(transformType);
        bqp->GetTransformHint(transformType);
        uint64_t uniqueId = 0;
        std::string name = "";
        bqp->GetNameAndUniqueId(name, uniqueId);
        float brightness = GetData<float>();
        bqp->SetHdrWhitePointBrightness(brightness);
        BufferQueueProducerFuzzTest(bqp);
        uint32_t code = GetData<uint32_t>();
        MessageParcel arguments;
        MessageParcel reply;
        MessageOption option;
        bqp->OnRemoteRequest(code, arguments, reply, option);
        for (auto iter : bqp->memberFuncMap_) {
            iter.second(reinterpret_cast<BufferQueueProducer*>(bqp.GetRefPtr()), arguments, reply, option);
        }
    }

    bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
    {
        if (data == nullptr) {
            return false;
        }

        // initialize
        g_data = data;
        g_size = size;
        g_pos = 0;
        // get data
        uint32_t seqNum = GetData<uint32_t>();
        BufferRequestConfig requestConfig = GetData<BufferRequestConfig>();
        OHOS::Rect rect = GetData<OHOS::Rect>();
        int64_t timestamp = GetData<int64_t>();
        BufferFlushConfigWithDamages flushConfig = {.damages =  { rect }, .timestamp = timestamp};
        uint32_t sequence = GetData<uint32_t>();

        // test
        std::string name = GetStringFromData(STR_LEN);
        sptr<BufferQueue> bq = new BufferQueue(name);
        sptr<BufferQueueProducer> bqp = new BufferQueueProducer(bq);
        sptr<OHOS::SurfaceBuffer> buffer = new SurfaceBufferImpl(seqNum);
        sptr<BufferExtraData> bedata = GetBufferExtraDataFromData();
        IBufferProducer::RequestBufferReturnValue retval;
        retval.buffer = buffer;
        OnReleaseFunc onBufferRelease = nullptr;
        sptr<IProducerListener> listener = new BufferReleaseProducerListener(onBufferRelease);
        bqp->RegisterReleaseListener(listener);
        OnDeleteBufferFunc deleteBufferFunc;
        bqp->RequestBuffer(requestConfig, bedata, retval);
        bqp->GoBackground();
        bqp->CancelBuffer(sequence, bedata);
        sptr<SyncFence> syncFence = SyncFence::INVALID_FENCE;
        bqp->FlushBuffer(sequence, bedata, syncFence, flushConfig);
        std::vector<uint32_t> sequences;
        std::vector<sptr<BufferExtraData>> bedataimpls;
        std::vector<sptr<SyncFence>> fences;
        std::vector<BufferFlushConfigWithDamages> flushConfigs;
        for (size_t i = 0; i < sequences.size(); i++) {
            flushConfigs.emplace_back(flushConfig);
            fences.emplace_back(new SyncFence(-1));
        }
        bqp->FlushBuffers(sequences, bedataimpls, fences, flushConfigs);
        BufferQueueProducerFuzzTest1(bqp);
        return true;
    }
}
/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}