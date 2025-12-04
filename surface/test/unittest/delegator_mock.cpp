/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <dlfcn.h>
#include <cstdio>
#include <cstring>
#include "delegator_mock.h"
#include "delegator_adapter.h"
#include "surface_type.h"
#include "delegator_adapter.h"
#include "sync_fence.h"

namespace OHOS {
namespace Rosen {

bool g_dlopenNull = false;
bool g_producerSurfaceDelegatorCreateNull = false;
bool g_producerSurfaceDelegatorDestroyNull = false;
bool g_producerSurfaceDelegatorSetSurfaceNull = false;
bool g_producerSurfaceDelegatorSetClientNull = false;
bool g_producerSurfaceDelegatorReleaseBufferNull = false;
bool g_consumerSurfaceDelegatorCreateNull = false;
bool g_consumerSurfaceDelegatorSetClientNull = false;
bool g_consumerSurfaceDelegatorSetSurfaceNull = false;
bool g_consumerSurfaceDelegatorDequeueBufferNull = false;
bool g_consumerSurfaceDelegatorQueueBufferNull = false;
bool g_consumerSurfaceDelegatorDestroyNull = false;

uintptr_t MockProducerSurfaceDelegatorCreate()
{
    return 0x16;
}

void MockProducerSurfaceDelegatorDestroy()
{
}

void MockProducerSurfaceDelegatorSetSurface()
{
}

bool MockProducerSurfaceDelegatorSetClient()
{
    return true;
}

GSError MockProducerSurfaceDelegatorReleaseBuffer()
{
    return GSERROR_OK;
}

uintptr_t MockConsumerSurfaceDelegatorCreate()
{
    return 0x11;
}

bool MockConsumerSurfaceDelegatorSetClient()
{
    return true;
}

void MockConsumerSurfaceDelegatorSetSurface()
{
}

GSError MockConsumerSurfaceDelegatorDequeueBuffer()
{
    return GSERROR_OK;
}

GSError MockConsumerSurfaceDelegatorQueueBuffer()
{
    return GSERROR_OK;
}

void MockConsumerSurfaceDelegatorDestroy()
{
}

static void* HandleProducerSurfaceDelegatorCreate()
{
    if (g_producerSurfaceDelegatorCreateNull) {
        return nullptr;
    }
    return reinterpret_cast<void *>(MockProducerSurfaceDelegatorCreate);
}

static void* HandleProducerSurfaceDelegatorDestroy()
{
    if (g_producerSurfaceDelegatorDestroyNull) {
        return nullptr;
    }
    return reinterpret_cast<void *>(MockProducerSurfaceDelegatorDestroy);
}

static void* HandleProducerSurfaceDelegatorSetSurface()
{
    if (g_producerSurfaceDelegatorSetSurfaceNull) {
        return nullptr;
    }
    return reinterpret_cast<void *>(MockProducerSurfaceDelegatorSetSurface);
}

static void* HandleProducerSurfaceDelegatorSetClient()
{
    if (g_producerSurfaceDelegatorSetClientNull) {
        return nullptr;
    }
    return reinterpret_cast<void *>(MockProducerSurfaceDelegatorSetClient);
}

static void* HandleProducerSurfaceDelegatorReleaseBuffer()
{
    if (g_producerSurfaceDelegatorReleaseBufferNull) {
        return nullptr;
    }
    return reinterpret_cast<void *>(MockProducerSurfaceDelegatorReleaseBuffer);
}

static void* HandleConsumerSurfaceDelegatorCreate()
{
    if (g_consumerSurfaceDelegatorCreateNull) {
        return nullptr;
    }
    return reinterpret_cast<void *>(MockConsumerSurfaceDelegatorCreate);
}

static void* HandleConsumerSurfaceDelegatorSetClient()
{
    if (g_consumerSurfaceDelegatorSetClientNull) {
        return nullptr;
    }
    return reinterpret_cast<void *>(MockConsumerSurfaceDelegatorSetClient);
}

static void* HandleConsumerSurfaceDelegatorSetSurface()
{
    if (g_consumerSurfaceDelegatorSetSurfaceNull) {
        return nullptr;
    }
    return reinterpret_cast<void *>(MockConsumerSurfaceDelegatorSetSurface);
}

static void* HandleConsumerSurfaceDelegatorDequeueBuffer()
{
    if (g_consumerSurfaceDelegatorDequeueBufferNull) {
        return nullptr;
    }
    return reinterpret_cast<void *>(MockConsumerSurfaceDelegatorDequeueBuffer);
}

static void* HandleConsumerSurfaceDelegatorQueueBuffer()
{
    if (g_consumerSurfaceDelegatorQueueBufferNull) {
        return nullptr;
    }
    return reinterpret_cast<void *>(MockConsumerSurfaceDelegatorQueueBuffer);
}

static void* HandleConsumerSurfaceDelegatorDestroy()
{
    if (g_consumerSurfaceDelegatorDestroyNull) {
        return nullptr;
    }
    return reinterpret_cast<void *>(MockConsumerSurfaceDelegatorDestroy);
}

struct SymbolHandler {
    const char* name;
    void* (*handler)();
};

struct SymbolHandler g_symbolHandlerTable[] = {
    { "ProducerSurfaceDelegatorCreate", HandleProducerSurfaceDelegatorCreate },
    { "ProducerSurfaceDelegatorDestroy", HandleProducerSurfaceDelegatorDestroy },
    { "ProducerSurfaceDelegatorSetSurface", HandleProducerSurfaceDelegatorSetSurface },
    { "ProducerSurfaceDelegatorSetClient", HandleProducerSurfaceDelegatorSetClient },
    { "ProducerSurfaceDelegatorReleaseBuffer", HandleProducerSurfaceDelegatorReleaseBuffer },
    { "ConsumerSurfaceDelegatorCreate", HandleConsumerSurfaceDelegatorCreate },
    { "ConsumerSurfaceDelegatorSetClient", HandleConsumerSurfaceDelegatorSetClient },
    { "ConsumerSurfaceDelegatorSetSurface", HandleConsumerSurfaceDelegatorSetSurface },
    { "ConsumerSurfaceDelegatorDequeueBuffer", HandleConsumerSurfaceDelegatorDequeueBuffer },
    { "ConsumerSurfaceDelegatorQueueBuffer", HandleConsumerSurfaceDelegatorQueueBuffer },
    { "ConsumerSurfaceDelegatorDestroy", HandleConsumerSurfaceDelegatorDestroy },
};

extern "C" void *dlsym(void *handle, const char *symbol)
{
    for (auto& handler : g_symbolHandlerTable) {
        if (strcmp(symbol, handler.name) == 0) {
            return handler.handler();
        }
    }
    return nullptr;
}

extern "C" void *dlopen(const char *filename, int flags)
{
    if (strcmp(filename, "libdelegator.z.so") == 0) {
        if (g_dlopenNull) {
            return nullptr;
        }
        return reinterpret_cast<void *>(0x1234);
    }
    return nullptr;
}

extern "C" int dlclose(void *handle)
{
    if (handle == reinterpret_cast<void *>(0x1234)) {
        return 0;
    }
    return -1;
}

}  // namespace Rosen
}  // namespace OHOS