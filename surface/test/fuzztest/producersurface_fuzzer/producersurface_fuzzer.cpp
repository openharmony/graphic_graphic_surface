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

#include "producersurface_fuzzer.h"
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
#include <buffer_client_producer.h>
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include <iservice_registry.h>
#include <sys/wait.h>

using namespace g_fuzzCommon;
namespace OHOS {
    sptr<IRemoteObject> g_robj = nullptr;
    sptr<IBufferProducer> g_bufferClientProducer = nullptr;
    pid_t g_pid = 0;
    int g_pipeFd[2] = {};
    int g_pipe1Fd[2] = {};

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

    bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
    {
        if (data == nullptr) {
            return false;
        }

        // initialize
        g_data = data;
        g_size = size;
        g_pos = 0;
        // test

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