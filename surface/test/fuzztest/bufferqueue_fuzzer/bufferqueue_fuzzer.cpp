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

#include "bufferqueue_fuzzer.h"

#include <securec.h>

#include "buffer_queue.h"
#include "data_generate.h"
#include "surface.h"
#include "surface_buffer.h"
#include "surface_buffer_impl.h"
#include "buffer_extra_data.h"
#include "buffer_extra_data_impl.h"
#include "sync_fence.h"

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

    void BufferQueueFuzzTest2()
    {
        std::string name = GetStringFromData(STR_LEN);
        uint32_t queueSize = GetData<uint32_t>();
        int32_t width = GetData<int32_t>();
        int32_t height = GetData<int32_t>();
        uint64_t usage = GetData<uint64_t>();
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
        bool flag = GetData<bool>();
        std::string result = GetStringFromData(STR_LEN);
        bool status = GetData<bool>();
        GraphicPresentTimestamp timestamp = GetData<GraphicPresentTimestamp>();
        OHSurfaceSource sourceType = GetData<OHSurfaceSource>();
        std::string appFrameworkType = GetStringFromData(STR_LEN);
        // test
        sptr<BufferQueue> bufferqueue = new BufferQueue(name);
        bufferqueue->SetQueueSize(queueSize);
        bufferqueue->SetDefaultWidthAndHeight(width, height);
        bufferqueue->SetDefaultUsage(usage);
        GraphicTransformType transform = GetData<GraphicTransformType >();
        bufferqueue->SetTransform(transform);
        bufferqueue->SetMetaData(sequence, metaData);
        bufferqueue->SetMetaDataSet(sequence, key, metaDataSet);
        bufferqueue->SetProducerCacheCleanFlag(flag);
        bufferqueue->Dump(result);
        bufferqueue->SetStatus(status);
        bufferqueue->SetPresentTimestamp(sequence, timestamp);
        bufferqueue->SetSurfaceSourceType(sourceType);
        bufferqueue->SetSurfaceAppFrameworkType(appFrameworkType);
        int64_t time = 0;
        GraphicPresentTimestampType timestampType = GetData<GraphicPresentTimestampType>();
        bufferqueue->GetPresentTimestamp(sequence, timestampType, time);
    }

    void BufferQueueFuzzTest1()
    {
        int32_t timeOut = 0;
        std::string name = GetStringFromData(STR_LEN);
        sptr<BufferQueue> bufferqueue = new BufferQueue(name);
        uint32_t seqNum = GetData<uint32_t>();
        BufferRequestConfig requestConfig = GetData<BufferRequestConfig>();
        OHOS::Rect rect = GetData<OHOS::Rect>();
        int64_t timestamp = GetData<int64_t>();
        BufferFlushConfigWithDamages flushConfig = {.damages =  { rect }, .timestamp = timestamp};
        uint32_t sequence = GetData<uint32_t>();
        std::vector<Rect> damages;
        sptr<OHOS::SurfaceBuffer> buffer = new SurfaceBufferImpl(seqNum);
        sptr<OHOS::SurfaceBuffer> buffer1 = SurfaceBuffer::Create();
        sptr<BufferExtraData> bedata = GetBufferExtraDataFromData();
        IBufferProducer::RequestBufferReturnValue retval;
        retval.buffer = buffer;
        bufferqueue->RegisterConsumerListener(nullptr);
        OnReleaseFunc func;
        bufferqueue->RegisterReleaseListener(func);
        OnDeleteBufferFunc deleteBufferFunc;
        bool isForUniRedraw = GetData<bool>();
        bufferqueue->RegisterDeleteBufferListener(deleteBufferFunc, isForUniRedraw);
        bufferqueue->RequestBuffer(requestConfig, bedata, retval);
        bufferqueue->ReuseBuffer(requestConfig, bedata, retval);
        bufferqueue->CancelBuffer(sequence, bedata);
        sptr<SyncFence> syncFence = SyncFence::INVALID_FENCE;
        bufferqueue->FlushBuffer(sequence, bedata, syncFence, flushConfig);
        bufferqueue->DoFlushBuffer(sequence, bedata, syncFence, flushConfig);
        bufferqueue->AcquireBuffer(buffer, syncFence, timestamp, damages);
        bufferqueue->ReleaseBuffer(buffer, syncFence);
        bufferqueue->AttachBuffer(buffer1, timeOut);
        bufferqueue->DetachBuffer(buffer1);
        float matrix[16] = {0};
        uint32_t matrixSize = GetData<uint32_t>();
        bool isUseNewMatrix = GetData<bool>();
        bool needRecordSequence = GetData<bool>();
        bufferqueue->GetLastFlushedBuffer(buffer, syncFence, matrix, matrixSize, isUseNewMatrix, needRecordSequence);
        bufferqueue->UnregisterConsumerListener();
        bufferqueue->UnRegisterProducerReleaseListener();
        GraphicTransformType transformType = GetData<GraphicTransformType>();
        bufferqueue->SetTransform(transformType);
        bufferqueue->GetTransform();
        bufferqueue->SetTransformHint(transformType);
        bufferqueue->GetSurfaceSourceType();
        float brightness = GetData<float>();
        bufferqueue->SetHdrWhitePointBrightness(brightness);
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
        std::string name = GetStringFromData(STR_LEN);
        sptr<BufferQueue> bufferqueue = new BufferQueue(name);
        bool cleanAll = GetData<bool>();
        bufferqueue->CleanCache(cleanAll);
        bufferqueue->GetHdrWhitePointBrightness();
        bufferqueue->GetUniqueId();
        bufferqueue->GoBackground();
        ScalingMode mode = GetData<ScalingMode>();
        bufferqueue->SetScalingMode(mode);
        bool bufferHold = GetData<bool>();
        bufferqueue->SetBufferHold(bufferHold);
        uint32_t reserveInts = GetData<uint32_t>() % 0x100000; // no more than 0x100000
        GraphicExtDataHandle *handle = AllocExtDataHandle(reserveInts);
        sptr<SurfaceTunnelHandle> tunnelHandle = new SurfaceTunnelHandle();
        tunnelHandle->SetHandle(handle);
        bufferqueue->SetTunnelHandle(tunnelHandle);
        FreeExtDataHandle(handle);
        BufferQueueFuzzTest1();
        BufferQueueFuzzTest2();

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

