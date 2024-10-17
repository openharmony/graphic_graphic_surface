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

#include "buffer_log.h"
#include "buffer_extra_data_impl.h"
#include "buffer_producer_listener.h"
#include "sync_fence.h"
#include "native_window.h"
#include "surface_utils.h"
#include "metadata_helper.h"
 
using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
namespace OHOS {

sptr<Surface> Surface::CreateSurfaceAsProducer(sptr<IBufferProducer>& producer)
{
    if (producer == nullptr) {
        return nullptr;
    }

    sptr<ProducerSurface> surf = new ProducerSurface(producer);
    GSError ret = surf->Init();
    if (ret != GSERROR_OK) {
        BLOGE("producerSurface is nullptr");
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
    windowConfig_.width = initInfo_.width;
    windowConfig_.height = initInfo_.height;
    windowConfig_.usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_MEM_DMA;
    windowConfig_.format = GRAPHIC_PIXEL_FMT_RGBA_8888;
    windowConfig_.strideAlignment = 8;     // default stride is 8
    windowConfig_.timeout = 3000;          // default timeout is 3000 ms
    windowConfig_.colorGamut = GraphicColorGamut::GRAPHIC_COLOR_GAMUT_SRGB;
    windowConfig_.transform = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    BLOGD("ProducerSurface ctor");
}

ProducerSurface::~ProducerSurface()
{
    BLOGD("~ProducerSurface dtor, name: %{public}s, uniqueId: %{public}" PRIu64 ".", name_.c_str(), queueId_);
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
    inited_.store(true);
    BLOGD("Init name: %{public}s, uniqueId: %{public}" PRIu64 ".", name_.c_str(), queueId_);
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
    GSError ret;
    sptr<BufferExtraData> bedataimpl = new BufferExtraDataImpl;
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        ret = producer_->RequestBuffer(config, bedataimpl, retval);
        if (ret != GSERROR_OK) {
            if (ret == GSERROR_NO_CONSUMER) {
                CleanCacheLocked(false);
            }
            BLOGD("RequestBuffer ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, queueId_);
            return ret;
        }
        AddCacheLocked(bedataimpl, retval, config);
    }
    buffer = retval.buffer;
    fence = retval.fence;
    ret = SetMetadataValve(buffer);
    if (ret != GSERROR_OK) {
        BLOGD("SetMetadataValve ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, queueId_);
    }
    return ret;
}
 
GSError ProducerSurface::SetMetadataValve(sptr<SurfaceBuffer>& buffer)
{
    GSError ret = GSERROR_OK;
    std::vector<uint8_t> metaData;
    std::string value = GetUserData("ATTRKEY_COLORSPACE_INFO");
    if (!value.empty()) {
        ret = MetadataHelper::SetColorSpaceType(buffer, static_cast<CM_ColorSpaceType>(atoi(value.c_str())));
        if (ret != GSERROR_OK) {
            BLOGD("SetColorSpaceType ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, queueId_);
            return ret;
        }
    }
    value = GetUserData("OH_HDR_DYNAMIC_METADATA");
    if (!value.empty()) {
        metaData.resize(value.size());
        metaData.assign(value.begin(), value.end());
        ret = MetadataHelper::SetHDRStaticMetadata(buffer, metaData);
        if (ret != GSERROR_OK) {
            BLOGD("SetHDRStaticMetadata ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, queueId_);
            return ret;
        }
    }
    value = GetUserData("OH_HDR_STATIC_METADATA");
    if (!value.empty()) {
        metaData.resize(value.size());
        metaData.assign(value.begin(), value.end());
        ret = MetadataHelper::SetHDRStaticMetadata(buffer, metaData);
        if (ret != GSERROR_OK) {
            BLOGD("SetHDRStaticMetadata ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, queueId_);
            return ret;
        }
    }
    value = GetUserData("OH_HDR_METADATA_TYPE");
    if (!value.empty()) {
        ret = MetadataHelper::SetHDRMetadataType(buffer, static_cast<CM_HDR_Metadata_Type>(atoi(value.c_str())));
        if (ret != GSERROR_OK) {
            BLOGD("SetHDRMetadataType ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, queueId_);
        }
    }
    return ret;
}

GSError ProducerSurface::AddCacheLocked(sptr<BufferExtraData>& bedataimpl,
    IBufferProducer::RequestBufferReturnValue& retval, BufferRequestConfig& config)
{
    if (isDisconnected_) {
        isDisconnected_ = false;
    }
    // add cache
    if (retval.buffer != nullptr) {
        bufferProducerCache_[retval.sequence] = retval.buffer;
    } else if (bufferProducerCache_.find(retval.sequence) == bufferProducerCache_.end()) {
        BLOGE("cache not find buffer(%{public}u), uniqueId: %{public}" PRIu64 ".", retval.sequence, queueId_);
        return SURFACE_ERROR_UNKOWN;
    } else {
        retval.buffer = bufferProducerCache_[retval.sequence];
        retval.buffer->SetSurfaceBufferColorGamut(config.colorGamut);
        retval.buffer->SetSurfaceBufferTransform(config.transform);
    }
    if (retval.buffer != nullptr) {
        retval.buffer->SetExtraData(bedataimpl);
    }
    for (auto it = retval.deletingBuffers.begin(); it != retval.deletingBuffers.end(); it++) {
        uint32_t seqNum = static_cast<uint32_t>(*it);
        bufferProducerCache_.erase(seqNum);
        auto spNativeWindow = wpNativeWindow_.promote();
        if (spNativeWindow != nullptr) {
            auto& bufferCache = spNativeWindow->bufferCache_;
            if (bufferCache.find(seqNum) != bufferCache.end()) {
                NativeObjectUnreference(bufferCache[seqNum]);
                bufferCache.erase(seqNum);
            }
        }
    }
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
        BLOGD("RequestBuffers ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, queueId_);
        return ret;
    }
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
    if (buffers.size() == 0 || buffers.size() != fences.size() || producer_ == nullptr) {
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

GSError ProducerSurface::AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                       int64_t &timestamp, Rect &damage)
{
    return GSERROR_NOT_SUPPORT;
}
GSError ProducerSurface::ReleaseBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence)
{
    return GSERROR_NOT_SUPPORT;
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
        return GSERROR_INVALID_ARGUMENTS;
    }

    sptr<BufferExtraData> bedata = buffer->GetExtraData();
    if (bedata == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->CancelBuffer(buffer->GetSeqNum(), bedata);
}

GSError ProducerSurface::FlushBuffer(sptr<SurfaceBuffer>& buffer,
    int32_t fence, BufferFlushConfig& config)
{
    // fence need close?
    sptr<SyncFence> syncFence = new SyncFence(fence);
    return FlushBuffer(buffer, syncFence, config);
}

GSError ProducerSurface::AcquireBuffer(sptr<SurfaceBuffer>& buffer, int32_t &fence,
    int64_t &timestamp, Rect &damage)
{
    return GSERROR_NOT_SUPPORT;
}

GSError ProducerSurface::ReleaseBuffer(sptr<SurfaceBuffer>& buffer, int32_t fence)
{
    return GSERROR_NOT_SUPPORT;
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
    }
    return ret;
}

GSError ProducerSurface::DetachBufferFromQueue(sptr<SurfaceBuffer> buffer)
{
    if (buffer == nullptr || producer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    auto ret = producer_->DetachBufferFromQueue(buffer);
    if (ret == GSERROR_OK) {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        if (bufferProducerCache_.find(buffer->GetSeqNum()) == bufferProducerCache_.end()) {
            BLOGE("Detach buffer %{public}d, uniqueId: %{public}" PRIu64 ".", buffer->GetSeqNum(), queueId_);
            return SURFACE_ERROR_BUFFER_NOT_INCACHE;
        }
        bufferProducerCache_.erase(buffer->GetSeqNum());
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
    wpPSurfaceDelegator_ = surfaceDelegator;

    auto releaseBufferCallBack = [weakThis = wptr(this)] (const sptr<SurfaceBuffer>& buffer,
        const sptr<SyncFence>& fence) -> GSError {
        auto pSurface = weakThis.promote();
        if (pSurface == nullptr) {
            BLOGE("pSurface is nullptr");
            return GSERROR_INVALID_ARGUMENTS;
        }
        auto surfaceDelegator = pSurface->wpPSurfaceDelegator_.promote();
        if (surfaceDelegator == nullptr) {
            return GSERROR_INVALID_ARGUMENTS;
        }
        int error = surfaceDelegator->ReleaseBuffer(buffer, fence);
        return static_cast<GSError>(error);
    };
    RegisterReleaseListener(releaseBufferCallBack);
    return GSERROR_OK;
}

bool ProducerSurface::QueryIfBufferAvailable()
{
    return false;
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

GSError ProducerSurface::SetDefaultWidthAndHeight(int32_t width, int32_t height)
{
    return GSERROR_NOT_SUPPORT;
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
    GSError err = producer_->SetTransformHint(transformHint);
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
        BLOGE("SetUserData failed: key:%{public}s, val:%{public}s exist, uniqueId: %{public}" PRIu64 ".",
            key.c_str(), val.c_str(), queueId_);
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

GSError ProducerSurface::RegisterConsumerListener(sptr<IBufferConsumerListener>& listener)
{
    return GSERROR_NOT_SUPPORT;
}

GSError ProducerSurface::RegisterConsumerListener(IBufferConsumerListenerClazz *listener)
{
    return GSERROR_NOT_SUPPORT;
}

GSError ProducerSurface::UnregisterConsumerListener()
{
    return GSERROR_NOT_SUPPORT;
}

GSError ProducerSurface::RegisterReleaseListener(OnReleaseFunc func)
{
    if (func == nullptr || producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    listener_ = new BufferReleaseProducerListener(func);
    return producer_->RegisterReleaseListener(listener_);
}

GSError ProducerSurface::RegisterReleaseListener(OnReleaseFuncWithFence funcWithFence)
{
    if (funcWithFence == nullptr || producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    listener_ = new BufferReleaseProducerListener(nullptr, funcWithFence);
    return producer_->RegisterReleaseListener(listener_);
}

GSError ProducerSurface::UnRegisterReleaseListener()
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    wpPSurfaceDelegator_ = nullptr;
    return producer_->UnRegisterReleaseListener();
}

GSError ProducerSurface::RegisterDeleteBufferListener(OnDeleteBufferFunc func, bool isForUniRedraw)
{
    return GSERROR_NOT_SUPPORT;
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

void ProducerSurface::CleanAllLocked()
{
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
    BLOGD("CleanCacheLocked, uniqueId: %{public}" PRIu64 ".", queueId_);
    CleanAllLocked();
    return producer_->CleanCache(cleanAll);
}

GSError ProducerSurface::CleanCache(bool cleanAll)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    BLOGD("CleanCache, uniqueId: %{public}" PRIu64 ".", queueId_);
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        CleanAllLocked();
    }
    return producer_->CleanCache(cleanAll);
}

GSError ProducerSurface::GoBackground()
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    BLOGD("GoBackground, uniqueId: %{public}" PRIu64 ".", queueId_);
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        CleanAllLocked();
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

GSError ProducerSurface::IsSupportedAlloc(const std::vector<BufferVerifyAllocInfo>& infos,
                                          std::vector<bool>& supporteds)
{
    if (producer_ == nullptr || infos.size() == 0 || infos.size() != supporteds.size()) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->IsSupportedAlloc(infos, supporteds);
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
    BLOGD("Connect, uniqueId: %{public}" PRIu64 ".", queueId_);
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
    BLOGD("Disconnect, uniqueId: %{public}" PRIu64 ".", queueId_);
    CleanAllLocked();
    GSError ret = producer_->Disconnect();
    if (ret == GSERROR_OK) {
        isDisconnected_ = true;
    }
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

GSError ProducerSurface::GetScalingMode(uint32_t sequence, ScalingMode& scalingMode)
{
    return GSERROR_NOT_SUPPORT;
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

GSError ProducerSurface::QueryMetaDataType(uint32_t sequence, HDRMetaDataType &type) const
{
    return GSERROR_NOT_SUPPORT;
}

GSError ProducerSurface::GetMetaData(uint32_t sequence, std::vector<GraphicHDRMetaData> &metaData) const
{
    return GSERROR_NOT_SUPPORT;
}

GSError ProducerSurface::GetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey &key,
                                        std::vector<uint8_t> &metaData) const
{
    return GSERROR_NOT_SUPPORT;
}

GSError ProducerSurface::SetTunnelHandle(const GraphicExtDataHandle *handle)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetTunnelHandle(handle);
}

sptr<SurfaceTunnelHandle> ProducerSurface::GetTunnelHandle() const
{
    // not support
    return nullptr;
}

GSError ProducerSurface::SetPresentTimestamp(uint32_t sequence, const GraphicPresentTimestamp &timestamp)
{
    return GSERROR_NOT_SUPPORT;
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

int32_t ProducerSurface::GetDefaultFormat()
{
    return 0;
}

GSError ProducerSurface::SetDefaultFormat(int32_t format)
{
    return GSERROR_NOT_SUPPORT;
}

int32_t ProducerSurface::GetDefaultColorGamut()
{
    return 0;
}

GSError ProducerSurface::SetDefaultColorGamut(int32_t colorGamut)
{
    return GSERROR_NOT_SUPPORT;
}

sptr<NativeSurface> ProducerSurface::GetNativeSurface()
{
    return nullptr;
}

GSError ProducerSurface::SetWptrNativeWindowToPSurface(void* nativeWindow)
{
    NativeWindow *nw = reinterpret_cast<NativeWindow *>(nativeWindow);
    std::lock_guard<std::mutex> lockGuard(mutex_);
    wpNativeWindow_ = nw;
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

BufferRequestConfig* ProducerSurface::GetWindowConfig()
{
    return &windowConfig_;
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
} // namespace OHOS
