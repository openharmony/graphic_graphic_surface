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

#include "bufferutils_fuzzer.h"

#include <securec.h>
#include <vector>

#include "data_generate.h"
#include "surface_buffer.h"
#include "surface_buffer_impl.h"
#include "buffer_utils.h"
#include "sandbox_utils.h"
#include <message_parcel.h>

using namespace g_fuzzCommon;

namespace OHOS {
    bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
    {
        if (data == nullptr) {
            return false;
        }
        // initialize
        g_data = data;
        g_size = size;
        g_pos = 0;

        int fd = GetData<int>();
        BufferRequestConfig rqConfig = GetData<BufferRequestConfig>();
        BufferFlushConfigWithDamages flConfig = {
            .damages =  { GetData<OHOS::Rect>() },
            .timestamp = GetData<int64_t>(),
        };
        uint32_t seqNum = GetData<uint32_t>();
        uint32_t sequence = GetData<uint32_t>();
        BufferVerifyAllocInfo vaInfo = GetData<BufferVerifyAllocInfo>();
        GraphicHDRMetaData metaData = GetData<GraphicHDRMetaData>();
        uint8_t metaData2 = GetData<uint8_t>();
        uint32_t reserveInts = GetData<uint32_t>() % 0x100000; // no more than 0x100000
        MessageParcel parcel;
        sptr<SurfaceBuffer> buffer = new SurfaceBufferImpl(seqNum);
        WriteFileDescriptor(parcel, fd);
        ReadFileDescriptor(parcel, fd);
        WriteRequestConfig(parcel, rqConfig);
        ReadRequestConfig(parcel, rqConfig);
        WriteFlushConfig(parcel, flConfig);
        ReadFlushConfig(parcel, flConfig);
        WriteSurfaceBufferImpl(parcel, sequence, buffer);
        ReadSurfaceBufferImpl(parcel, sequence, buffer);
        std::vector<BufferVerifyAllocInfo> infos = {vaInfo};
        WriteVerifyAllocInfo(parcel, infos);
        ReadVerifyAllocInfo(parcel, infos);
        std::vector<GraphicHDRMetaData> metaDatas = {metaData};
        WriteHDRMetaData(parcel, metaDatas);
        ReadHDRMetaData(parcel, metaDatas);
        std::vector<uint8_t> metaDatas2 = {metaData2};
        WriteHDRMetaDataSet(parcel, metaDatas2);
        ReadHDRMetaDataSet(parcel, metaDatas2);
        GraphicExtDataHandle *handle = AllocExtDataHandle(reserveInts);
        WriteExtDataHandle(parcel, handle);
        sptr<SurfaceTunnelHandle> tunnelHandle = nullptr;
        ReadExtDataHandle(parcel, tunnelHandle);
        sptr<SurfaceTunnelHandle> tunnelHandle2 = new SurfaceTunnelHandle();
        ReadExtDataHandle(parcel, tunnelHandle);
        tunnelHandle2->SetHandle(handle);
        tunnelHandle2->GetHandle();
        tunnelHandle2->Different(tunnelHandle);
        FreeExtDataHandle(handle);
        DumpToFileAsync(GetRealPid(), "test", buffer);
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

