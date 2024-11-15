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
#include <securec.h>
#include "native_window.h"
#include "data_generate.h"
#include "producer_surface.h"
#include "surface.h"
#include "sync_fence.h"
#include <buffer_producer_listener.h>
#include <buffer_extra_data_impl.h>
#include <sys/wait.h>

using namespace g_fuzzCommon;
namespace OHOS {
    sptr<OHOS::Surface> g_pSurface = nullptr;
    sptr<OHOS::IConsumerSurface> g_cSurface = nullptr;
    sptr<IBufferConsumerListener> g_listener = nullptr;
    sptr<OHOS::IBufferProducer> g_producer = nullptr;
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

    void CreateProducerSurface()
    {
        g_pid = fork();
        if (g_pid < 0) {
            exit(1);
        }
        if (g_pid == 0) {
            g_cSurface = IConsumerSurface::Create();
            g_listener = new BufferConsumerListener();
            g_cSurface->RegisterConsumerListener(g_listener);
            sptr<OHOS::IBufferProducer> g_producer = g_cSurface->GetProducer();
            g_pSurface = Surface::CreateSurfaceAsProducer(g_producer);

            close(g_pipeFd[1]);
            close(g_pipe1Fd[0]);
            char buf[10] = "start";
            write(g_pipe1Fd[1], buf, sizeof(buf));
            sleep(0);
            read(g_pipeFd[0], buf, sizeof(buf));
            close(g_pipeFd[0]);
            close(g_pipe1Fd[1]);
            exit(0);
        } else {
            close(g_pipeFd[0]);
            close(g_pipe1Fd[1]);
            char buf[10];
            read(g_pipe1Fd[0], buf, sizeof(buf));
        }
    }

    void DestroyProducerSurface()
    {
        g_pSurface = nullptr;
        g_producer = nullptr;
        g_cSurface = nullptr;
        g_listener = nullptr;

        char buf[10] = "over";
        write(g_pipeFd[1], buf, sizeof(buf));
        close(g_pipeFd[1]);
        close(g_pipe1Fd[0]);

        int32_t ret = 0;
        do {
            waitpid(g_pid, nullptr, 0);
        } while (ret == -1 && errno == EINTR);
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
        CreateProducerSurface();
        while (1) {
            if (g_pSurface != nullptr) {
                break;
            }
            usleep(10);
        }
        float brightness = GetData<float>();
        g_pSurface->SetHdrWhitePointBrightness(brightness);

        DestroyProducerSurface();

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