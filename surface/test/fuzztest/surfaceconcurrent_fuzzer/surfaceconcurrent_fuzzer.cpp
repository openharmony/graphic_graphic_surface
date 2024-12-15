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

#include "surfaceconcurrent_fuzzer.h"

#include <securec.h>
#include <atomic>

#include "data_generate.h"
#include "iconsumer_surface.h"
#include "producer_surface_delegator.h"
#include "surface.h"
#include "surface_buffer.h"
#include "surface_buffer_impl.h"
#include <iostream>

using namespace g_fuzzCommon;
namespace OHOS {
    static constexpr const uint32_t QUESIZE_RANGE = 10;
    static constexpr const uint32_t QUESIZE_MIN = 3;
    static constexpr const uint32_t BUFFERR_ROTATION_TIMES = 500;
    static constexpr const uint32_t BUFFERR_ROTATION_INTERVAL_MICRO_SECOND = 10;
    static constexpr const uint32_t CONCURRENCY_FUNCTION_TIMES = 500;
    static constexpr const uint32_t CONCURRENCY_FUNCTION_INTERVAL_MICRO_SECOND = 50;
    static constexpr const uint32_t ATTACHBUFFER_RETRY_TIMES = 10;
    BufferRequestConfig requestConfigTmp = {
        .width = 0x100,
        .height = 0x100,
        .strideAlignment = 0x8,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 1000,
    };
    int g_releaseFence = -1;
    BufferFlushConfig flushConfig = {
        .damage = {
            .w = 0x100,
            .h = 0x100,
        },
    };
    std::atomic<bool> g_isCleanCacheFinish = false;
    std::atomic<bool> g_isGoBackGroundFinish = false;

    void producerBufferFunc(sptr<Surface> pSurface)
    {
        for (uint32_t i = 0; i < BUFFERR_ROTATION_TIMES; i++) {
            usleep(BUFFERR_ROTATION_INTERVAL_MICRO_SECOND);
            sptr<SurfaceBuffer> requestBuffer = nullptr;
            pSurface->RequestBuffer(requestBuffer, g_releaseFence, requestConfigTmp);
            usleep(BUFFERR_ROTATION_INTERVAL_MICRO_SECOND);
            pSurface->FlushBuffer(requestBuffer, -1, flushConfig);
        }
        std::cout<< pSurface->GetName() <<"prudecer finish"<<std::endl;
    };

    void consumerBufferFunc(sptr<IConsumerSurface> cSurface)
    {
        int32_t flushFence;
        int64_t timestamp;
        OHOS::Rect damage;
        GSError ret;
        for (uint32_t i = 0; i < BUFFERR_ROTATION_TIMES; i++) {
            sptr<SurfaceBuffer> acquireBuffer;
            usleep(BUFFERR_ROTATION_INTERVAL_MICRO_SECOND);
            ret = cSurface->AcquireBuffer(acquireBuffer, flushFence, timestamp, damage);
            usleep(BUFFERR_ROTATION_INTERVAL_MICRO_SECOND);
            cSurface->ReleaseBuffer(acquireBuffer, -1);
        }
        std::cout<< cSurface->GetName() <<" consumerFunc finish"<<std::endl;
    }

    void cleanCacheFunc(sptr<Surface> pSurface)
    {
        for (uint32_t i = 0; i < CONCURRENCY_FUNCTION_TIMES; i++) {
            usleep(CONCURRENCY_FUNCTION_INTERVAL_MICRO_SECOND);
            pSurface->CleanCache(true);
        }
        std::cout<< pSurface->GetName() <<" cleanCacheFunc finish"<<std::endl;
    };

    void producerGoBackGroundFunc(sptr<Surface> pSurface)
    {
        for (uint32_t i = 0; i < CONCURRENCY_FUNCTION_TIMES; i++) {
            usleep(CONCURRENCY_FUNCTION_INTERVAL_MICRO_SECOND);
            pSurface->GoBackground();
        }
        std::cout<< pSurface->GetName() <<" producerGoBackGroundFunc finish"<<std::endl;
    };

    void consumerAttachFunc(sptr<IConsumerSurface> cSurface, sptr<SurfaceBuffer> buffer)
    {
        uint32_t ret = 1;
        uint32_t count = ATTACHBUFFER_RETRY_TIMES;
        while (ret != GSERROR_OK && count-- > 0) {
            usleep(CONCURRENCY_FUNCTION_INTERVAL_MICRO_SECOND);
            ret = cSurface->AttachBufferToQueue(buffer);
            std::cout<< cSurface->GetName() <<"Comsumer call AttachBufferToQueue result: "<< ret <<std::endl;
            ret = cSurface->ReleaseBuffer(buffer, -1);
        }
    };

    void producerAttachFunc(sptr<Surface> pSurface, sptr<SurfaceBuffer> buffer)
    {
        uint32_t ret = 1;
        uint32_t count = ATTACHBUFFER_RETRY_TIMES;
        while (ret != GSERROR_OK && count-- > 0) {
            usleep(CONCURRENCY_FUNCTION_INTERVAL_MICRO_SECOND);
            ret = pSurface->AttachBufferToQueue(buffer);
            std::cout<< pSurface->GetName() <<" producer call AttachBufferToQueue result: "<< ret <<std::endl;
            ret = pSurface->FlushBuffer(buffer, -1, flushConfig);
        }
    };

/*
* CaseDescription: 1. preSetup: create two surface（surfaceA and surfaceB）
*                  2. operation: While surfaceA and surfaceB are rotating the buffer, the producer of surfaceA detaches
*                     the buffer to surfaceB, and the producers of surfaceA and surfaceB perform cleanCache and
*                     GoBackground operations.
*                     Concurrent interface：
*                     1、RequestBuffer
*                     2、FlushBuffer
*                     3、AcquireBuffer
*                     4、ReleaseBuffer
*                     5、CleanCahce (producer)
*                     6、GoBackGround (producer)
*                     7、AttachBufferToQueue
*                     8、DetachBufferFromQueue (producer)
*                  3. result: buffer detach from surfaceA attach to surfaceB success multiple times without crash.
*/
    void BufferRotationConcurrentWithAttachBufferAndDetachBufferAndCleanCacheAndGoBackground()
    {
        std::cout<<"start"<<std::endl;
        // create surface A
        std::string name = GetStringFromData(STR_LEN);
        sptr<IConsumerSurface> cSurfaceA = IConsumerSurface::Create(name);
        sptr<IBufferConsumerListener> consumerListener = new BufferConsumerListener();
        cSurfaceA->RegisterConsumerListener(consumerListener);
        sptr<IBufferProducer> producerClient = cSurfaceA->GetProducer();
        sptr<Surface> pSurfaceA = Surface::CreateSurfaceAsProducer(producerClient);
       
        // create surface B
        sptr<IConsumerSurface> cSurfaceB = IConsumerSurface::Create("SurfaceB");
        sptr<IBufferConsumerListener> consumerListenerB = new BufferConsumerListener();
        cSurfaceB->RegisterConsumerListener(consumerListenerB);
        sptr<IBufferProducer> producerClientB = cSurfaceB->GetProducer();
        sptr<Surface> pSurfaceB = Surface::CreateSurfaceAsProducer(producerClientB);

        //init
        uint32_t queueSize = GetData<uint32_t>();
        queueSize = queueSize % QUESIZE_RANGE + QUESIZE_MIN;
        pSurfaceA->SetQueueSize(queueSize);
        pSurfaceB->SetQueueSize(queueSize);
        std::cout<<"Queue size: "<< queueSize <<std::endl;

        //create concurrent thread
        g_isCleanCacheFinish= false;
        g_isGoBackGroundFinish = false;
        std::thread consumerThread(consumerBufferFunc, cSurfaceA);
        std::thread consumerThreadB(consumerBufferFunc, cSurfaceB);
        std::thread cleanCacheThread(cleanCacheFunc, pSurfaceA);
        std::thread cleanCacheThreadB(cleanCacheFunc, pSurfaceB);
        std::thread goBackgroundThread(producerGoBackGroundFunc, pSurfaceA);
        std::thread goBackgroundThreadB(producerGoBackGroundFunc, pSurfaceB);
        std::thread producerThreadB(producerBufferFunc, pSurfaceB);
    
        std::vector<std::thread> attachThreadList;
        uint32_t detachBufferTrigger = GetData<uint32_t>();
        detachBufferTrigger = (detachBufferTrigger % QUESIZE_RANGE) + 1;
        std::cout<<"detachBuffer Trigger: "<< detachBufferTrigger <<std::endl;
        for (uint32_t i = 0; i < CONCURRENCY_FUNCTION_TIMES; i++) {
            usleep(BUFFERR_ROTATION_INTERVAL_MICRO_SECOND);
            sptr<SurfaceBuffer> requestBuffer = nullptr;
            auto ret = pSurfaceA->RequestBuffer(requestBuffer, g_releaseFence, requestConfigTmp);
            uint32_t count = 0;
            if (detachBufferTrigger == 0) {
                detachBufferTrigger++;
            }
            if (ret == GSERROR_OK && count++ % detachBufferTrigger == 0
                && (!g_isCleanCacheFinish|| !g_isGoBackGroundFinish)) {
                usleep(BUFFERR_ROTATION_INTERVAL_MICRO_SECOND);
                ret = pSurfaceA->DetachBufferFromQueue(requestBuffer);
                if (ret != GSERROR_OK) {
                    usleep(BUFFERR_ROTATION_INTERVAL_MICRO_SECOND);
                    pSurfaceA->FlushBuffer(requestBuffer, -1, flushConfig);
                }
                bool surfaceBIsProducer = GetData<bool>();
                if (surfaceBIsProducer) {
                    std::thread attachThread(consumerAttachFunc, cSurfaceB, requestBuffer);
                    attachThreadList.push_back(std::move(attachThread));
                } else {
                    std::thread attachThread(producerAttachFunc, pSurfaceB, requestBuffer);
                    attachThreadList.push_back(std::move(attachThread));
                }
                continue;
            }
            usleep(BUFFERR_ROTATION_INTERVAL_MICRO_SECOND);
            pSurfaceA->FlushBuffer(requestBuffer, -1, flushConfig);
        }

        // wait concurrent thread finish
        std::cout<< pSurfaceA->GetName() <<"prudecer finish"<<std::endl;
        for (auto& attachThread : attachThreadList) {
            if (attachThread.joinable()) {
                attachThread.join();
            }
        }
        consumerThread.join();
        cleanCacheThread.join();
        goBackgroundThread.join();
        consumerThreadB.join();
        cleanCacheThreadB.join();
        goBackgroundThreadB.join();
        producerThreadB.join();
        pSurfaceA = nullptr;
        producerClient = nullptr;
        cSurfaceA = nullptr;
        pSurfaceB = nullptr;
        producerClientB = nullptr;
        cSurfaceB = nullptr;
        std::cout<<"end"<<std::endl;
    }

    bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
    {
        if (data == nullptr) {
            return false;
        }
        BufferRotationConcurrentWithAttachBufferAndDetachBufferAndCleanCacheAndGoBackground();
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

