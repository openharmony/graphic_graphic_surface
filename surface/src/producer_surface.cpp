/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "producer_surface.h"

#include <cinttypes>
#include <sys/ioctl.h>
 
#include <linux/dma-buf.h>

#include "buffer_log.h"
#include "buffer_extra_data_impl.h"
#include "buffer_producer_listener.h"
#include "sync_fence.h"
#include "native_window.h"
#include "surface_utils.h"
#include "surface_trace.h"
#include "metadata_helper.h"

#define DMA_BUF_SET_TYPE _IOW(DMA_BUF_BASE, 2, const char *)

using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
namespace OHOS {
constexpr int32_t FORCE_GLOBAL_ALPHA_MIN = -1;
constexpr int32_t FORCE_GLOBAL_ALPHA_MAX = 255;
sptr<Surface> Surface::CreateSurfaceAsProducer(sptr<IBufferProducer>& producer)
{
    if (producer == nullptr) {
        return nullptr;
    }

    sptr<ProducerSurface> surf = new ProducerSurface(producer);
    GSError ret = surf->Init();
    if (ret != GSERROR_OK) {
        BLOGE("producer surf init failed");
        return nullptr;
    }
    auto utils = SurfaceUtils::GetInstance();
    utils->Add(surf->GetUniqueId(), surf);
    return surf;
}

ProducerSurface::ProducerSurface(sptr<IBufferProducer>& producer)
{
    producer_ = producer;
    GetProducerInitInfo(initInfo_);
    lastSetTransformHint_ = static_cast<GraphicTransformType>(initInfo_.transformHint);
    windowConfig_.width = initInfo_.width;
    windowConfig_.height = initInfo_.height;
    windowConfig_.usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_MEM_DMA;
    windowConfig_.format = GRAPHIC_PIXEL_FMT_RGBA_8888;
    windowConfig_.strideAlignment = 8;     // default stride is 8
    windowConfig_.timeout = 3000;          // default timeout is 3000 ms
    windowConfig_.colorGamut = GraphicColorGamut::GRAPHIC_COLOR_GAMUT_SRGB;
    windowConfig_.transform = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    BLOGD("ProducerSurface ctor, name: %{public}s, uniqueId: %{public}" PRIu64 ", appName: %{public}s, isInHebcList:"
        " %{public}d.", initInfo_.name.c_str(), initInfo_.uniqueId, initInfo_.appName.c_str(), initInfo_.isInHebcList);
    RequestPropertyListenerInner([this](SurfaceProperty property) { return PropertyChangeCallback(property); },
        initInfo_.producerId);
}

ProducerSurface::~ProducerSurface()
{
    BLOGD("~ProducerSurface dtor, name: %{public}s, uniqueId: %{public}" PRIu64 ".", name_.c_str(), queueId_);
    UnRegisterPropertyListenerInner(initInfo_.producerId);
    Disconnect();
    auto utils = SurfaceUtils::GetInstance();
    utils->Remove(GetUniqueId());
}

GSError ProducerSurface::GetProducerInitInfo(ProducerInitInfo& info)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->GetProducerInitInfo(info);
}

GSError ProducerSurface::Init()
{
    if (inited_.load()) {
        return GSERROR_OK;
    }
    name_ = initInfo_.name;
    queueId_ = initInfo_.uniqueId;
    bufferName_ = initInfo_.bufferName;
    inited_.store(true);
    return GSERROR_OK;
}

bool ProducerSurface::IsConsumer() const
{
    return false;
}

sptr<IBufferProducer> ProducerSurface::GetProducer() const
{
    return producer_;
}

GSError ProducerSurface::RequestBuffer(sptr<SurfaceBuffer>& buffer,
                                       sptr<SyncFence>& fence, BufferRequestConfig& config)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    IBufferProducer::RequestBufferReturnValue retval;
    sptr<BufferExtraData> bedataimpl = new BufferExtraDataImpl;

    std::lock_guard<std::mutex> lockGuard(mutex_);
    GSError ret = producer_->RequestBuffer(config, bedataimpl, retval);
#ifdef HIPERF_TRACE_ENABLE
    BLOGW("hiperf_surface RequestBuffer %{public}lx %{public}u %{public}u %{public}u",
        config.usage, config.format, config.width, config.height);
#endif
    if (ret != GSERROR_OK) {
        if (ret == GSERROR_NO_CONSUMER) {
            CleanCacheLocked(false);
        }
        /**
         * if server is connected, but result is failed.
         * client needs to synchronize status.
         */
        if (retval.isConnected) {
            isDisconnected_ = false;
        }
        BLOGD("RequestBuffer ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, queueId_);
        return ret;
    }
    isDisconnected_ = false;
    AddCacheLocked(bedataimpl, retval, config);
    buffer = retval.buffer;
    fence = retval.fence;

    if (SetMetadataValue(buffer) != GSERROR_OK) {
        BLOGD("SetMetadataValue fail, uniqueId: %{public}" PRIu64 ".", queueId_);
    }
    return ret;
}

GSError ProducerSurface::SetMetadataValue(sptr<SurfaceBuffer>& buffer)
{
    GSError ret = GSERROR_OK;
    std::vector<uint8_t> metaData;
    std::string value = GetUserData("ATTRKEY_COLORSPACE_INFO");
    if (!value.empty()) {
        ret = MetadataHelper::SetColorSpaceType(buffer, static_cast<CM_ColorSpaceType>(atoi(value.c_str())));
        if (ret != GSERROR_OK) {
            return ret;
        }
    }
    value = GetUserData("OH_HDR_DYNAMIC_METADATA");
    if (!value.empty()) {
        metaData.resize(value.size());
        metaData.assign(value.begin(), value.end());
        ret = MetadataHelper::SetHDRDynamicMetadata(buffer, metaData);
        if (ret != GSERROR_OK) {
            return ret;
        }
    }
    value = GetUserData("OH_HDR_STATIC_METADATA");
    if (!value.empty()) {
        metaData.resize(value.size());
        metaData.assign(value.begin(), value.end());
        ret = MetadataHelper::SetHDRStaticMetadata(buffer, metaData);
        if (ret != GSERROR_OK) {
            return ret;
        }
    }
    value = GetUserData("OH_HDR_METADATA_TYPE");
    if (!value.empty()) {
        ret = MetadataHelper::SetHDRMetadataType(buffer, static_cast<CM_HDR_Metadata_Type>(atoi(value.c_str())));
    }
    return ret;
}

void ProducerSurface::SetBufferConfigLocked(sptr<BufferExtraData>& bedataimpl,
    IBufferProducer::RequestBufferReturnValue& retval, BufferRequestConfig& config)
{
    if (retval.buffer != nullptr) {
        retval.buffer->SetSurfaceBufferColorGamut(config.colorGamut);
        retval.buffer->SetSurfaceBufferTransform(config.transform);
        retval.buffer->SetExtraData(bedataimpl);
    }
}

void ProducerSurface::DeleteCacheBufferLocked(sptr<BufferExtraData>& bedataimpl,
    IBufferProducer::RequestBufferReturnValue& retval, BufferRequestConfig& config)
{
    for (auto it = retval.deletingBuffers.begin(); it != retval.deletingBuffers.end(); it++) {
        uint32_t seqNum = static_cast<uint32_t>(*it);
        bufferProducerCache_.erase(seqNum);
        auto spNativeWindow = wpNativeWindow_.promote();
        if (spNativeWindow != nullptr) {
            auto& bufferCache = spNativeWindow->bufferCache_;
            auto iter = bufferCache.find(seqNum);
            if (iter != bufferCache.end()) {
                NativeObjectUnreference(iter->second);
                bufferCache.erase(iter);
            }
        }
    }
}

GSError ProducerSurface::AddCacheLocked(sptr<BufferExtraData>& bedataimpl,
    IBufferProducer::RequestBufferReturnValue& retval, BufferRequestConfig& config)
{
    // add cache
    if (retval.buffer != nullptr) {
        bufferProducerCache_[retval.sequence] = retval.buffer;
        ReleasePreCacheBuffer(static_cast<int>(bufferProducerCache_.size()));
        if (bufferName_ != "") {
            int fd = retval.buffer->GetFileDescriptor();
            if (fd > 0) {
                ioctl(fd, DMA_BUF_SET_TYPE, bufferName_.c_str());
            }
        }
    } else {
        auto it = bufferProducerCache_.find(retval.sequence);
        if (it == bufferProducerCache_.end()) {
            DeleteCacheBufferLocked(bedataimpl, retval, config);
            BLOGE("cache not find buffer(%{public}u), uniqueId: %{public}" PRIu64 ".", retval.sequence, queueId_);
            return SURFACE_ERROR_UNKOWN;
        } else {
            retval.buffer = it->second;
        }
    }
    SetBufferConfigLocked(bedataimpl, retval, config);
    DeleteCacheBufferLocked(bedataimpl, retval, config);
    return SURFACE_ERROR_OK;
}

GSError ProducerSurface::RequestBuffers(std::vector<sptr<SurfaceBuffer>>& buffers,
    std::vector<sptr<SyncFence>>& fences, BufferRequestConfig& config)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    std::vector<IBufferProducer::RequestBufferReturnValue> retvalues;
    retvalues.resize(SURFACE_MAX_QUEUE_SIZE);
    std::vector<sptr<BufferExtraData>> bedataimpls;
    for (size_t i = 0; i < retvalues.size(); ++i) {
        sptr<BufferExtraData> bedataimpl = new BufferExtraDataImpl;
        bedataimpls.emplace_back(bedataimpl);
    }
    std::lock_guard<std::mutex> lockGuard(mutex_);
    GSError ret = producer_->RequestBuffers(config, bedataimpls, retvalues);
    if (ret != GSERROR_NO_BUFFER && ret != GSERROR_OK) {
        /**
         * if server is connected, but result is failed.
         * client needs to synchronize status.
         */
        if (retvalues[0].isConnected) {
            isDisconnected_ = false;
        }
        BLOGD("RequestBuffers ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, queueId_);
        return ret;
    }
    isDisconnected_ = false;
    for (size_t i = 0; i < retvalues.size(); ++i) {
        AddCacheLocked(bedataimpls[i], retvalues[i], config);
        buffers.emplace_back(retvalues[i].buffer);
        fences.emplace_back(retvalues[i].fence);
    }
    return GSERROR_OK;
}

GSError ProducerSurface::FlushBuffer(sptr<SurfaceBuffer>& buffer,
                                     const sptr<SyncFence>& fence, BufferFlushConfig& config)
{
    BufferFlushConfigWithDamages configWithDamages;
    configWithDamages.damages.push_back(config.damage);
    configWithDamages.timestamp = config.timestamp;
    configWithDamages.desiredPresentTimestamp = config.desiredPresentTimestamp;
    return FlushBuffer(buffer, fence, configWithDamages);
}

GSError ProducerSurface::FlushBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
                                     BufferFlushConfigWithDamages& config)
{
    if (buffer == nullptr || fence == nullptr || producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }

    sptr<BufferExtraData> bedata = buffer->GetExtraData();
    if (bedata == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    auto ret = producer_->FlushBuffer(buffer->GetSeqNum(), bedata, fence, config);
    if (ret == GSERROR_NO_CONSUMER) {
        CleanCache();
        BLOGD("FlushBuffer ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, queueId_);
    }
    return ret;
}

GSError ProducerSurface::FlushBuffers(const std::vector<sptr<SurfaceBuffer>>& buffers,
    const std::vector<sptr<SyncFence>>& fences, const std::vector<BufferFlushConfigWithDamages>& configs)
{
    if (buffers.size() == 0 || buffers.size() != fences.size() || producer_ == nullptr
        || buffers.size() > SURFACE_MAX_QUEUE_SIZE) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    for (size_t i = 0; i < buffers.size(); ++i) {
        if (buffers[i] == nullptr || fences[i] == nullptr) {
            BLOGE("buffer or fence is nullptr, i: %{public}zu, uniqueId: %{public}" PRIu64 ".", i, queueId_);
            return GSERROR_INVALID_ARGUMENTS;
        }
    }
    std::vector<sptr<BufferExtraData>> bedata;
    bedata.reserve(buffers.size());
    std::vector<uint32_t> sequences;
    sequences.reserve(buffers.size());
    for (uint32_t i = 0; i < buffers.size(); ++i) {
        bedata.emplace_back(buffers[i]->GetExtraData());
        sequences.emplace_back(buffers[i]->GetSeqNum());
    }
    auto ret = producer_->FlushBuffers(sequences, bedata, fences, configs);
    if (ret == GSERROR_NO_CONSUMER) {
        CleanCache();
        BLOGD("FlushBuffers ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, queueId_);
    }
    return ret;
}

GSError ProducerSurface::GetLastFlushedBuffer(sptr<SurfaceBuffer>& buffer,
    sptr<SyncFence>& fence, float matrix[16], bool isUseNewMatrix)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->GetLastFlushedBuffer(buffer, fence, matrix, isUseNewMatrix);
}

GSError ProducerSurface::RequestBuffer(sptr<SurfaceBuffer>& buffer,
    int32_t& fence, BufferRequestConfig& config)
{
    sptr<SyncFence> syncFence = SyncFence::InvalidFence();
    auto ret = RequestBuffer(buffer, syncFence, config);
    if (ret != GSERROR_OK) {
        fence = -1;
        return ret;
    }
    fence = syncFence->Dup();
    return GSERROR_OK;
}

GSError ProducerSurface::CancelBuffer(sptr<SurfaceBuffer>& buffer)
{
    if (buffer == nullptr || producer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }

    sptr<BufferExtraData> bedata = buffer->GetExtraData();
    if (bedata == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return producer_->CancelBuffer(buffer->GetSeqNum(), bedata);
}

GSError ProducerSurface::FlushBuffer(sptr<SurfaceBuffer>& buffer,
    int32_t fence, BufferFlushConfig& config)
{
    sptr<SyncFence> syncFence = new SyncFence(fence);
    return FlushBuffer(buffer, syncFence, config);
}

GSError ProducerSurface::AttachBufferToQueue(sptr<SurfaceBuffer> buffer)
{
    if (buffer == nullptr || producer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    auto ret = producer_->AttachBufferToQueue(buffer);
    if (ret == GSERROR_OK) {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        if (bufferProducerCache_.find(buffer->GetSeqNum()) != bufferProducerCache_.end()) {
            BLOGE("Attach buffer %{public}d, uniqueId: %{public}" PRIu64 ".", buffer->GetSeqNum(), queueId_);
            return SURFACE_ERROR_BUFFER_IS_INCACHE;
        }
        bufferProducerCache_[buffer->GetSeqNum()] = buffer;
        ReleasePreCacheBuffer(static_cast<int>(bufferProducerCache_.size()));
    }
    return ret;
}

GSError ProducerSurface::DetachBufferFromQueue(sptr<SurfaceBuffer> buffer, bool isReserveSlot)
{
    (void)isReserveSlot;
    if (buffer == nullptr || producer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    auto ret = producer_->DetachBufferFromQueue(buffer);
    if (ret == GSERROR_OK) {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        auto it = bufferProducerCache_.find(buffer->GetSeqNum());
        if (it == bufferProducerCache_.end()) {
            BLOGE("Detach buffer %{public}d, uniqueId: %{public}" PRIu64 ".", buffer->GetSeqNum(), queueId_);
            return SURFACE_ERROR_BUFFER_NOT_INCACHE;
        }
        bufferProducerCache_.erase(it);
    }
    return ret;
}

GSError ProducerSurface::AttachBuffer(sptr<SurfaceBuffer>& buffer)
{
    if (buffer == nullptr || producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }

    return producer_->AttachBuffer(buffer);
}

GSError ProducerSurface::AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut)
{
    if (buffer == nullptr || producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }

    return producer_->AttachBuffer(buffer, timeOut);
}

GSError ProducerSurface::DetachBuffer(sptr<SurfaceBuffer>& buffer)
{
    if (buffer == nullptr || producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->DetachBuffer(buffer);
}

GSError ProducerSurface::RegisterSurfaceDelegator(sptr<IRemoteObject> client)
{
    if (client == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    sptr<ProducerSurfaceDelegator> surfaceDelegator = ProducerSurfaceDelegator::Create();
    if (surfaceDelegator == nullptr) {
        BLOGE("surfaceDelegator is nullptr");
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (!surfaceDelegator->SetClient(client)) {
        BLOGE("SetClient failed");
        return GSERROR_INVALID_ARGUMENTS;
    }

    surfaceDelegator->SetSurface(this);
    {
        std::lock_guard<std::mutex> lockGuard(delegatorMutex_);
        wpPSurfaceDelegator_ = surfaceDelegator;
    }

    auto releaseBufferCallBack = [weakThis = wptr(this)] (const sptr<SurfaceBuffer>& buffer,
        const sptr<SyncFence>& fence) -> GSError {
        auto pSurface = weakThis.promote();
        if (pSurface == nullptr) {
            BLOGE("pSurface is nullptr");
            return GSERROR_INVALID_ARGUMENTS;
        }
        sptr<ProducerSurfaceDelegator> surfaceDelegator = nullptr;
        {
            std::lock_guard<std::mutex> lockGuard(pSurface->delegatorMutex_);
            surfaceDelegator = pSurface->wpPSurfaceDelegator_.promote();
        }
        if (surfaceDelegator == nullptr) {
            return GSERROR_INVALID_ARGUMENTS;
        }
        int error = surfaceDelegator->ReleaseBuffer(buffer, fence);
        return static_cast<GSError>(error);
    };
    RegisterReleaseListenerBackup(releaseBufferCallBack);
    return GSERROR_OK;
}

uint32_t ProducerSurface::GetQueueSize()
{
    if (producer_ == nullptr) {
        return 0;
    }
    return producer_->GetQueueSize();
}

GSError ProducerSurface::SetQueueSize(uint32_t queueSize)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetQueueSize(queueSize);
}

const std::string& ProducerSurface::GetName()
{
    if (!inited_.load()) {
        BLOGW("ProducerSurface is not initialized.");
    }
    return name_;
}

int32_t ProducerSurface::GetDefaultWidth()
{
    if (producer_ == nullptr) {
        return -1;
    }
    return producer_->GetDefaultWidth();
}

int32_t ProducerSurface::GetDefaultHeight()
{
    if (producer_ == nullptr) {
        return -1;
    }
    return producer_->GetDefaultHeight();
}

GraphicTransformType ProducerSurface::GetTransformHint() const
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return lastSetTransformHint_;
}

GSError ProducerSurface::SetTransformHint(GraphicTransformType transformHint)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        if (lastSetTransformHint_ == transformHint) {
            return GSERROR_OK;
        }
    }
    GSError err = producer_->SetTransformHint(transformHint, initInfo_.producerId);
    if (err == GSERROR_OK) {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        lastSetTransformHint_ = transformHint;
    }
    return err;
}

GSError ProducerSurface::SetDefaultUsage(uint64_t usage)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetDefaultUsage(usage);
}

uint64_t ProducerSurface::GetDefaultUsage()
{
    if (producer_ == nullptr) {
        return 0;
    }
    return producer_->GetDefaultUsage();
}

GSError ProducerSurface::SetSurfaceSourceType(OHSurfaceSource sourceType)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetSurfaceSourceType(sourceType);
}

OHSurfaceSource ProducerSurface::GetSurfaceSourceType() const
{
    if (producer_ == nullptr) {
        return OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
    }
    OHSurfaceSource sourceType = OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
    if (producer_->GetSurfaceSourceType(sourceType) != GSERROR_OK) {
        BLOGE("GetSurfaceSourceType failed, uniqueId: %{public}" PRIu64 ".", queueId_);
        return OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
    }
    return sourceType;
}

GSError ProducerSurface::SetSurfaceAppFrameworkType(std::string appFrameworkType)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetSurfaceAppFrameworkType(appFrameworkType);
}

std::string ProducerSurface::GetSurfaceAppFrameworkType() const
{
    if (producer_ == nullptr) {
        return "";
    }
    std::string appFrameworkType = "";
    if (producer_->GetSurfaceAppFrameworkType(appFrameworkType) != GSERROR_OK) {
        BLOGE("GetSurfaceAppFrameworkType failed, uniqueId: %{public}" PRIu64 ".", queueId_);
        return "";
    }
    return appFrameworkType;
}

GSError ProducerSurface::SetUserData(const std::string& key, const std::string& val)
{
    std::lock_guard<std::mutex> lockGuard(lockMutex_);
    if (userData_.size() >= SURFACE_MAX_USER_DATA_COUNT) {
        BLOGE("userData_ size out: %{public}zu, uniqueId: %{public}" PRIu64 ".", userData_.size(), queueId_);
        return GSERROR_OUT_OF_RANGE;
    }

    auto iterUserData = userData_.find(key);
    if (iterUserData != userData_.end() && iterUserData->second == val) {
        return GSERROR_API_FAILED;
    }

    userData_[key] = val;
    auto iter = onUserDataChange_.begin();
    while (iter != onUserDataChange_.end()) {
        if (iter->second != nullptr) {
            iter->second(key, val);
        }
        iter++;
    }

    return GSERROR_OK;
}

std::string ProducerSurface::GetUserData(const std::string& key)
{
    std::lock_guard<std::mutex> lockGuard(lockMutex_);
    if (userData_.find(key) != userData_.end()) {
        return userData_[key];
    }

    return "";
}

GSError ProducerSurface::RegisterReleaseListener(OnReleaseFunc func)
{
    if (func == nullptr || producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    sptr<IProducerListener> listener;
    {
        std::lock_guard<std::mutex> lockGuard(listenerMutex_);
        listener_ = new BufferReleaseProducerListener(func);
        listener = listener_;
    }
    return producer_->RegisterReleaseListener(listener);
}

GSError ProducerSurface::RegisterReleaseListener(OnReleaseFuncWithFence funcWithFence)
{
    if (funcWithFence == nullptr || producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    sptr<IProducerListener> listener;
    {
        std::lock_guard<std::mutex> lockGuard(listenerMutex_);
        listener_ = new BufferReleaseProducerListener(nullptr, funcWithFence);
        listener = listener_;
    }
    return producer_->RegisterReleaseListener(listener);
}

GSError ProducerSurface::PropertyChangeCallback(const SurfaceProperty& property)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    lastSetTransformHint_ = property.transformHint;
    return GSERROR_OK;
}

GSError ProducerSurface::RegisterPropertyListenerInner(OnPropertyChangeFunc func, uint64_t producerId)
{
    if (func == nullptr || producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    sptr<IProducerListener> listener;
    {
        std::lock_guard<std::mutex> lockGuard(listenerMutex_);
        propertyListener_ = new PropertyChangeProducerListener(func);
        listener = producerListener_;
    }
    return producer_->RegisterPropertyListener(listener, producerId);
}

GSError ProducerSurface::UnRegisterPropertyListenerInner(uint64_t producerId)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    {
        std::lock_guard<std::mutex> lockGuard(listenerMutex_);
        if (propertyListener_ != nullptr) {
            propertyListener_->ResetReleaseFunc();
        }
    }
    return producer_->UnRegisterPropertyListener(producerId);
}

GSError ProducerSurface::RegisterReleaseListenerBackup(OnReleaseFuncWithFence funcWithFence)
{
    if (funcWithFence == nullptr || producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    sptr<IProducerListener> listener;
    {
        std::lock_guard<std::mutex> lockGuard(listenerMutex_);
        listenerBackup_ = new BufferReleaseProducerListener(nullptr, funcWithFence);
        listener = listenerBackup_;
    }
    return producer_->RegisterReleaseListenerBackup(listener);
}

GSError ProducerSurface::UnRegisterReleaseListener()
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    {
        std::lock_guard<std::mutex> lockGuard(listenerMutex_);
        if (listener_ != nullptr) {
            listener_->ResetReleaseFunc();
        }
    }
    return producer_->UnRegisterReleaseListener();
}

GSError ProducerSurface::UnRegisterReleaseListenerBackup()
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    {
        std::lock_guard<std::mutex> lockGuard(delegatorMutex_);
        wpPSurfaceDelegator_ = nullptr;
    }
    {
        std::lock_guard<std::mutex> lockGuard(listenerMutex_);
        if (listenerBackup_ != nullptr) {
            listenerBackup_->ResetReleaseFunc();
        }
    }
    return producer_->UnRegisterReleaseListenerBackup();
}

GSError ProducerSurface::RegisterUserDataChangeListener(const std::string& funcName, OnUserDataChangeFunc func)
{
    if (func == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    std::lock_guard<std::mutex> lockGuard(lockMutex_);
    if (onUserDataChange_.find(funcName) != onUserDataChange_.end()) {
        BLOGD("already register func: %{public}s, uniqueId: %{public}" PRIu64 ".",
            funcName.c_str(), queueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }

    onUserDataChange_[funcName] = func;
    return GSERROR_OK;
}

GSError ProducerSurface::UnRegisterUserDataChangeListener(const std::string& funcName)
{
    std::lock_guard<std::mutex> lockGuard(lockMutex_);
    if (onUserDataChange_.erase(funcName) == 0) {
        BLOGD("no register funcName: %{public}s, uniqueId: %{public}" PRIu64 ".", funcName.c_str(), queueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }

    return GSERROR_OK;
}

GSError ProducerSurface::ClearUserDataChangeListener()
{
    std::lock_guard<std::mutex> lockGuard(lockMutex_);
    onUserDataChange_.clear();
    return GSERROR_OK;
}

bool ProducerSurface::IsRemote()
{
    if (producer_ == nullptr || producer_->AsObject() == nullptr) {
        return false;
    }
    return producer_->AsObject()->IsProxyObject();
}

void ProducerSurface::CleanAllLocked(uint32_t *bufSeqNum)
{
    if (bufSeqNum && bufferProducerCache_.find(*bufSeqNum) != bufferProducerCache_.end()) {
        preCacheBuffer_ = bufferProducerCache_[*bufSeqNum];
    }
    bufferProducerCache_.clear();
    auto spNativeWindow = wpNativeWindow_.promote();
    if (spNativeWindow != nullptr) {
        auto& bufferCache = spNativeWindow->bufferCache_;
        for (auto& [seqNum, buffer] : bufferCache) {
            NativeObjectUnreference(buffer);
        }
        bufferCache.clear();
    }
}

GSError ProducerSurface::CleanCacheLocked(bool cleanAll)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    uint32_t bufSeqNum = 0;
    GSError ret = producer_->CleanCache(cleanAll, &bufSeqNum);
    CleanAllLocked(&bufSeqNum);
    if (cleanAll) {
        preCacheBuffer_ = nullptr;
    }
    return ret;
}

GSError ProducerSurface::CleanCache(bool cleanAll)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    std::lock_guard<std::mutex> lockGuard(mutex_);
    uint32_t bufSeqNum = 0;
    GSError ret = producer_->CleanCache(cleanAll, &bufSeqNum);
    CleanAllLocked(&bufSeqNum);
    if (cleanAll) {
        preCacheBuffer_ = nullptr;
    }
    return ret;
}

GSError ProducerSurface::GoBackground()
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        CleanAllLocked(nullptr);
    }
    return producer_->GoBackground();
}

uint64_t ProducerSurface::GetUniqueId() const
{
    if (!inited_.load()) {
        BLOGW("ProducerSurface is not initialized.");
    }
    return queueId_;
}

GSError ProducerSurface::SetTransform(GraphicTransformType transform)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetTransform(transform);
}

GraphicTransformType ProducerSurface::GetTransform() const
{
    if (producer_ == nullptr) {
        return GraphicTransformType::GRAPHIC_ROTATE_BUTT;
    }
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_BUTT;
    if (producer_->GetTransform(transform) != GSERROR_OK) {
        BLOGE("GetTransform failed, uniqueId: %{public}" PRIu64 ".", queueId_);
        return GraphicTransformType::GRAPHIC_ROTATE_BUTT;
    }
    return transform;
}

GSError ProducerSurface::Connect()
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (!isDisconnected_) {
        BLOGE("Surface has been connect, uniqueId: %{public}" PRIu64 ".", queueId_);
        return SURFACE_ERROR_CONSUMER_IS_CONNECTED;
    }
    GSError ret = producer_->Connect();
    if (ret == GSERROR_OK) {
        isDisconnected_ = false;
    }
    return ret;
}

GSError ProducerSurface::Disconnect()
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (isDisconnected_) {
        BLOGD("Surface is disconnect, uniqueId: %{public}" PRIu64 ".", queueId_);
        return SURFACE_ERROR_CONSUMER_DISCONNECTED;
    }
    uint32_t bufSeqNum = 0;
    GSError ret = producer_->Disconnect(&bufSeqNum);
    if (ret == GSERROR_OK) {
        isDisconnected_ = true;
    }
    CleanAllLocked(&bufSeqNum);
    return ret;
}

GSError ProducerSurface::ConnectStrictly()
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    std::lock_guard<std::mutex> lockGuard(mutex_);
    GSError ret = producer_->ConnectStrictly();
    return ret;
}

GSError ProducerSurface::DisconnectStrictly()
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    std::lock_guard<std::mutex> lockGuard(mutex_);
    GSError ret = producer_->DisconnectStrictly();
    return ret;
}

GSError ProducerSurface::SetScalingMode(uint32_t sequence, ScalingMode scalingMode)
{
    if (producer_ == nullptr || scalingMode < ScalingMode::SCALING_MODE_FREEZE ||
        scalingMode > ScalingMode::SCALING_MODE_SCALE_FIT) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetScalingMode(sequence, scalingMode);
}

GSError ProducerSurface::SetScalingMode(ScalingMode scalingMode)
{
    if (producer_ == nullptr || scalingMode < ScalingMode::SCALING_MODE_FREEZE ||
        scalingMode > ScalingMode::SCALING_MODE_SCALE_FIT) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetScalingMode(scalingMode);
}

void ProducerSurface::SetBufferHold(bool hold)
{
    if (producer_ == nullptr) {
        return;
    }
    producer_->SetBufferHold(hold);
}

GSError ProducerSurface::SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData>& metaData)
{
    if (producer_ == nullptr || metaData.size() == 0) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetMetaData(sequence, metaData);
}

GSError ProducerSurface::SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key,
                                        const std::vector<uint8_t>& metaData)
{
    if (producer_ == nullptr || key < GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X ||
        key > GraphicHDRMetadataKey::GRAPHIC_MATAKEY_HDR_VIVID || metaData.size() == 0) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetMetaDataSet(sequence, key, metaData);
}

GSError ProducerSurface::SetTunnelHandle(const GraphicExtDataHandle *handle)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetTunnelHandle(handle);
}

GSError ProducerSurface::GetPresentTimestamp(uint32_t sequence, GraphicPresentTimestampType type,
                                             int64_t& time) const
{
    if (producer_ == nullptr || type <= GraphicPresentTimestampType::GRAPHIC_DISPLAY_PTS_UNSUPPORTED ||
        type > GraphicPresentTimestampType::GRAPHIC_DISPLAY_PTS_TIMESTAMP) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->GetPresentTimestamp(sequence, type, time);
}

GSError ProducerSurface::SetWptrNativeWindowToPSurface(void* nativeWindow)
{
    if (nativeWindow == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    NativeWindow *nw = reinterpret_cast<NativeWindow *>(nativeWindow);
    std::lock_guard<std::mutex> lockGuard(mutex_);
    wpNativeWindow_ = nw;
    return GSERROR_OK;
}

GSError ProducerSurface::SetBufferName(const std::string &name)
{
    bufferName_ = name;
    producer_->SetBufferName(name);
    return GSERROR_OK;
}

void ProducerSurface::SetRequestWidthAndHeight(int32_t width, int32_t height)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    requestWidth_ = width;
    requestHeight_ = height;
}

int32_t ProducerSurface::GetRequestWidth()
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return requestWidth_;
}

int32_t ProducerSurface::GetRequestHeight()
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return requestHeight_;
}

void ProducerSurface::SetWindowConfig(const BufferRequestConfig& config)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    windowConfig_ = config;
}

void ProducerSurface::SetWindowConfigWidthAndHeight(int32_t width, int32_t height)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    windowConfig_.width = width;
    windowConfig_.height = height;
}

void ProducerSurface::SetWindowConfigStride(int32_t stride)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    windowConfig_.strideAlignment = stride;
}

void ProducerSurface::SetWindowConfigFormat(int32_t format)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    windowConfig_.format = format;
}

void ProducerSurface::SetWindowConfigUsage(uint64_t usage)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    windowConfig_.usage = usage;
}

void ProducerSurface::SetWindowConfigTimeout(int32_t timeout)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    windowConfig_.timeout = timeout;
}

void ProducerSurface::SetWindowConfigColorGamut(GraphicColorGamut colorGamut)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    windowConfig_.colorGamut = colorGamut;
}

void ProducerSurface::SetWindowConfigTransform(GraphicTransformType transform)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    windowConfig_.transform = transform;
}

BufferRequestConfig ProducerSurface::GetWindowConfig()
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return windowConfig_;
}

GSError ProducerSurface::SetHdrWhitePointBrightness(float brightness)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (brightness < 0.0 || brightness > 1.0) {
        BLOGE("brightness:%{public}f, uniqueId: %{public}" PRIu64 ".", brightness, queueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetHdrWhitePointBrightness(brightness);
}

GSError ProducerSurface::SetSdrWhitePointBrightness(float brightness)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (brightness < 0.0 || brightness > 1.0) {
        BLOGE("brightness:%{public}f, uniqueId: %{public}" PRIu64 ".", brightness, queueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetSdrWhitePointBrightness(brightness);
}

GSError ProducerSurface::AcquireLastFlushedBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
    float matrix[16], uint32_t matrixSize, bool isUseNewMatrix)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->AcquireLastFlushedBuffer(buffer, fence, matrix, matrixSize, isUseNewMatrix);
}

GSError ProducerSurface::ReleaseLastFlushedBuffer(sptr<SurfaceBuffer> buffer)
{
    if (buffer == nullptr || producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->ReleaseLastFlushedBuffer(buffer->GetSeqNum());
}

GSError ProducerSurface::SetGlobalAlpha(int32_t alpha)
{
    if (producer_ == nullptr || alpha < FORCE_GLOBAL_ALPHA_MIN || alpha > FORCE_GLOBAL_ALPHA_MAX) {
        BLOGE("Invalid producer global alpha value: %{public}d, queueId: %{public}" PRIu64 ".", alpha, queueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetGlobalAlpha(alpha);
}

GSError ProducerSurface::UpdateCacheLocked(sptr<BufferExtraData>& bedataimpl,
    IBufferProducer::RequestBufferReturnValue& retval, BufferRequestConfig& config)
{
    // add cache
    if (retval.buffer == nullptr) {
        auto it = bufferProducerCache_.find(retval.sequence);
        if (it == bufferProducerCache_.end()) {
            DeleteCacheBufferLocked(bedataimpl, retval, config);
            BLOGE("cache not find buffer(%{public}u), uniqueId: %{public}" PRIu64 ".", retval.sequence, queueId_);
            return SURFACE_ERROR_UNKOWN;
        } else {
            retval.buffer = it->second;
        }
    }
    bufferProducerCache_.erase(retval.sequence);
    SetBufferConfigLocked(bedataimpl, retval, config);
    DeleteCacheBufferLocked(bedataimpl, retval, config);
    return SURFACE_ERROR_OK;
}

GSError ProducerSurface::RequestAndDetachBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                                BufferRequestConfig& config)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    IBufferProducer::RequestBufferReturnValue retval;
    sptr<BufferExtraData> bedataimpl = new BufferExtraDataImpl;

    std::lock_guard<std::mutex> lockGuard(mutex_);
    GSError ret = producer_->RequestAndDetachBuffer(config, bedataimpl, retval);
    if (ret != GSERROR_OK) {
        if (ret == GSERROR_NO_CONSUMER) {
            CleanCacheLocked(false);
        }
        /**
         * if server is connected, but result is failed.
         * client needs to synchronize status.
         */
        if (retval.isConnected) {
            isDisconnected_ = false;
        }
        BLOGD("RequestAndDetachBuffer ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, queueId_);
        return ret;
    }
    isDisconnected_ = false;
    UpdateCacheLocked(bedataimpl, retval, config);
    buffer = retval.buffer;
    fence = retval.fence;

    if (SetMetadataValue(buffer) != GSERROR_OK) {
        BLOGD("SetMetadataValue fail, uniqueId: %{public}" PRIu64 ".", queueId_);
    }
    return ret;
}

GSError ProducerSurface::AttachAndFlushBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
                                              BufferFlushConfig& config, bool needMap)
{
    if (buffer == nullptr || fence == nullptr || producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }

    sptr<BufferExtraData> bedata = buffer->GetExtraData();
    if (bedata == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }

    BufferFlushConfigWithDamages configWithDamages;
    configWithDamages.damages.push_back(config.damage);
    configWithDamages.timestamp = config.timestamp;
    configWithDamages.desiredPresentTimestamp = config.desiredPresentTimestamp;

    auto ret = producer_->AttachAndFlushBuffer(buffer, bedata, fence, configWithDamages, needMap);
    if (ret == GSERROR_OK) {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        if (bufferProducerCache_.find(buffer->GetSeqNum()) != bufferProducerCache_.end()) {
            BLOGE("Attach buffer %{public}d, uniqueId: %{public}" PRIu64 ".", buffer->GetSeqNum(), queueId_);
            return SURFACE_ERROR_BUFFER_IS_INCACHE;
        }
        bufferProducerCache_[buffer->GetSeqNum()] = buffer;
    } else if (ret == GSERROR_NO_CONSUMER) {
        CleanCache();
        BLOGD("FlushBuffer ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, queueId_);
    }
    return ret;
}

void ProducerSurface::ReleasePreCacheBuffer(int bufferCacheSize)
{
    // client must have more than two buffer, otherwise RS can not delete the prebuffer.
    // Because RS has two buffer(pre and cur).
    const int deletePreCacheBufferThreshold = 2; // 2 is delete threshold.
    if (bufferCacheSize >= deletePreCacheBufferThreshold) {
        preCacheBuffer_ = nullptr;
    }
}

GSError ProducerSurface::GetCycleBuffersNumber(uint32_t& cycleBuffersNumber)
{
    if (producer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return producer_->GetCycleBuffersNumber(cycleBuffersNumber);
}

GSError ProducerSurface::SetCycleBuffersNumber(uint32_t cycleBuffersNumber)
{
    if (producer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return producer_->SetCycleBuffersNumber(cycleBuffersNumber);
}
} // namespace OHOS
