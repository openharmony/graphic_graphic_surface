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

#include "consumer_surface.h"

#include <cinttypes>

#include "buffer_log.h"
#include "buffer_queue_producer.h"
#include "sync_fence.h"

namespace OHOS {
sptr<Surface> Surface::CreateSurfaceAsConsumer(std::string name)
{
    sptr<ConsumerSurface> surf = new ConsumerSurface(name);
    if (surf->Init() != GSERROR_OK) {
        BLOGE("consumer surf init failed");
        return nullptr;
    }
    return surf;
}

sptr<IConsumerSurface> IConsumerSurface::Create(std::string name)
{
    sptr<ConsumerSurface> surf = new ConsumerSurface(name);
    if (surf->Init() != GSERROR_OK) {
        BLOGE("consumer surf init failed");
        return nullptr;
    }
    return surf;
}

ConsumerSurface::ConsumerSurface(const std::string& name)
    : name_(name)
{
    consumer_ = nullptr;
    producer_ = nullptr;
}

ConsumerSurface::~ConsumerSurface()
{
    if (consumer_ != nullptr) {
        consumer_->OnConsumerDied();
        consumer_->SetStatus(false);
    }
    if (producer_ != nullptr) {
        BLOGI("~ConsumerSurface, producer_ sptrCnt: %{public}d, uniqueId: %{public}" PRIu64 ".",
            producer_->GetSptrRefCount(), uniqueId_);
    }
    consumer_ = nullptr;
    producer_ = nullptr;
}

GSError ConsumerSurface::Init()
{
    sptr<BufferQueue> queue_ = new BufferQueue(name_);
    producer_ = new BufferQueueProducer(queue_);
    consumer_ = new BufferQueueConsumer(queue_);
    uniqueId_ = GetUniqueId();
    BLOGD("ConsumerSurface Init, uniqueId: %{public}" PRIu64 ".", uniqueId_);
    return GSERROR_OK;
}

bool ConsumerSurface::IsConsumer() const
{
    return true;
}

sptr<IBufferProducer> ConsumerSurface::GetProducer() const
{
    return producer_;
}

GSError ConsumerSurface::AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                       int64_t& timestamp, Rect& damage)
{
    std::vector<Rect> damages;
    GSError ret = AcquireBuffer(buffer, fence, timestamp, damages);
    if (ret != GSERROR_OK) {
        return ret;
    }
    if (damages.size() == 1) {
        damage = damages[0];
        return GSERROR_OK;
    }
    BLOGE("damages is %{public}zu, uniqueId: %{public}" PRIu64 ".", damages.size(), uniqueId_);
    return GSERROR_INVALID_ARGUMENTS;
}

GSError ConsumerSurface::AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                       int64_t& timestamp, std::vector<Rect>& damages)
{
    if (consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->AcquireBuffer(buffer, fence, timestamp, damages);
}

GSError ConsumerSurface::AcquireBuffer(AcquireBufferReturnValue &returnValue, int64_t expectPresentTimestamp,
                                       bool isUsingAutoTimestamp)
{
    if (consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->AcquireBuffer(returnValue, expectPresentTimestamp, isUsingAutoTimestamp);
}

GSError ConsumerSurface::AcquireBuffer(AcquireBufferReturnValue &returnValue)
{
    if (consumer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return consumer_->AcquireBuffer(returnValue);
}

GSError ConsumerSurface::ReleaseBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence)
{
    if (buffer == nullptr || consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->ReleaseBuffer(buffer, fence);
}

GSError ConsumerSurface::ReleaseBuffer(uint32_t sequence, const sptr<SyncFence>& fence)
{
    if (sequence == 0 || consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->ReleaseBuffer(sequence, fence);
}

GSError ConsumerSurface::SetIsActiveGame(bool isTransactonActiveGame)
{
    if (consumer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return consumer_->SetIsActiveGame(isTransactonActiveGame);
}

GSError ConsumerSurface::AcquireBuffer(sptr<SurfaceBuffer>& buffer, int32_t& fence,
                                       int64_t& timestamp, Rect& damage)
{
    sptr<SyncFence> syncFence = SyncFence::InvalidFence();
    auto ret = AcquireBuffer(buffer, syncFence, timestamp, damage);
    if (ret != GSERROR_OK) {
        fence = -1;
        return ret;
    }
    fence = syncFence->Dup();
    return GSERROR_OK;
}

GSError ConsumerSurface::ReleaseBuffer(sptr<SurfaceBuffer>& buffer, int32_t fence)
{
    sptr<SyncFence> syncFence = new SyncFence(fence);
    return ReleaseBuffer(buffer, syncFence);
}

GSError ConsumerSurface::AttachBufferToQueue(sptr<SurfaceBuffer> buffer)
{
    if (buffer == nullptr || consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    buffer->SetConsumerAttachBufferFlag(true);
    return consumer_->AttachBufferToQueue(buffer);
}

GSError ConsumerSurface::DetachBufferFromQueue(sptr<SurfaceBuffer> buffer, bool isReserveSlot)
{
    if (buffer == nullptr || consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    buffer->SetConsumerAttachBufferFlag(false);
    return consumer_->DetachBufferFromQueue(buffer, isReserveSlot);
}

GSError ConsumerSurface::AttachBuffer(sptr<SurfaceBuffer>& buffer)
{
    if (buffer == nullptr || consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->AttachBuffer(buffer);
}

GSError ConsumerSurface::AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut)
{
    if (buffer == nullptr || consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->AttachBuffer(buffer, timeOut);
}

GSError ConsumerSurface::DetachBuffer(sptr<SurfaceBuffer>& buffer)
{
    if (buffer == nullptr || consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->DetachBuffer(buffer);
}

GSError ConsumerSurface::RegisterSurfaceDelegator(sptr<IRemoteObject> client)
{
    if (client == nullptr || consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->RegisterSurfaceDelegator(client, this);
}

bool ConsumerSurface::QueryIfBufferAvailable()
{
    if (consumer_ == nullptr) {
        return false;
    }
    return consumer_->QueryIfBufferAvailable();
}

uint32_t ConsumerSurface::GetQueueSize()
{
    if (producer_ == nullptr) {
        return 0;
    }
    return producer_->GetQueueSize();
}

GSError ConsumerSurface::SetQueueSize(uint32_t queueSize)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetQueueSize(queueSize);
}

const std::string& ConsumerSurface::GetName()
{
    return name_;
}

GSError ConsumerSurface::SetDefaultWidthAndHeight(int32_t width, int32_t height)
{
    if (consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->SetDefaultWidthAndHeight(width, height);
}

int32_t ConsumerSurface::GetDefaultWidth()
{
    if (producer_ == nullptr) {
        return -1;
    }
    return producer_->GetDefaultWidth();
}

int32_t ConsumerSurface::GetDefaultHeight()
{
    if (producer_ == nullptr) {
        return -1;
    }
    return producer_->GetDefaultHeight();
}

GSError ConsumerSurface::SetDefaultUsage(uint64_t usage)
{
    if (consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->SetDefaultUsage(usage);
}

uint64_t ConsumerSurface::GetDefaultUsage()
{
    if (producer_ == nullptr) {
        return 0;
    }
    return producer_->GetDefaultUsage();
}

GSError ConsumerSurface::SetUserData(const std::string& key, const std::string& val)
{
    std::lock_guard<std::mutex> lockGuard(lockMutex_);
    if (userData_.size() >= SURFACE_MAX_USER_DATA_COUNT) {
        BLOGE("userData_ size(%{public}zu) out, uniqueId: %{public}" PRIu64 ".", userData_.size(), uniqueId_);
        return GSERROR_OUT_OF_RANGE;
    }

    auto iterUserData = userData_.find(key);
    if (iterUserData != userData_.end() && iterUserData->second == val) {
        BLOGE("not find key:%{public}s, val:%{public}s exist, uniqueId: %{public}" PRIu64 ".",
            key.c_str(), val.c_str(), uniqueId_);
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

std::string ConsumerSurface::GetUserData(const std::string& key)
{
    std::lock_guard<std::mutex> lockGuard(lockMutex_);
    if (userData_.find(key) != userData_.end()) {
        return userData_[key];
    }

    return "";
}

GSError ConsumerSurface::RegisterConsumerListener(sptr<IBufferConsumerListener>& listener)
{
    if (listener == nullptr || consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->RegisterConsumerListener(listener);
}

GSError ConsumerSurface::RegisterConsumerListener(IBufferConsumerListenerClazz *listener)
{
    if (listener == nullptr || consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->RegisterConsumerListener(listener);
}

GSError ConsumerSurface::RegisterReleaseListener(OnReleaseFunc func)
{
    if (func == nullptr || consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->RegisterReleaseListener(func);
}

GSError ConsumerSurface::RegisterDeleteBufferListener(OnDeleteBufferFunc func, bool isForUniRedraw)
{
    if (func == nullptr || consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (isForUniRedraw) {
        bool hasRegistercallBackForRedraw = false;
        if (!hasRegistercallBackForRedraw_.compare_exchange_strong(hasRegistercallBackForRedraw, true)) {
            return GSERROR_OK;
        }
    } else {
        bool hasRegistercallBackForRT = false;
        if (!hasRegistercallBackForRT_.compare_exchange_strong(hasRegistercallBackForRT, true)) {
            return GSERROR_OK;
        }
    }
    return consumer_->RegisterDeleteBufferListener(func, isForUniRedraw);
}

GSError ConsumerSurface::UnregisterConsumerListener()
{
    if (consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->UnregisterConsumerListener();
}

GSError ConsumerSurface::RegisterUserDataChangeListener(const std::string& funcName, OnUserDataChangeFunc func)
{
    if (func == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    std::lock_guard<std::mutex> lockGuard(lockMutex_);
    if (onUserDataChange_.find(funcName) != onUserDataChange_.end()) {
        BLOGD("already register func: %{public}s, uniqueId: %{public}" PRIu64 ".",
            funcName.c_str(), uniqueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }
    
    onUserDataChange_[funcName] = func;
    return GSERROR_OK;
}

GSError ConsumerSurface::UnRegisterUserDataChangeListener(const std::string& funcName)
{
    std::lock_guard<std::mutex> lockGuard(lockMutex_);
    if (onUserDataChange_.erase(funcName) == 0) {
        BLOGD("no register funcName: %{public}s, uniqueId: %{public}" PRIu64 ".",
            funcName.c_str(), uniqueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }

    return GSERROR_OK;
}

GSError ConsumerSurface::ClearUserDataChangeListener()
{
    std::lock_guard<std::mutex> lockGuard(lockMutex_);
    onUserDataChange_.clear();
    return GSERROR_OK;
}

GSError ConsumerSurface::GoBackground()
{
    if (consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (producer_ != nullptr) {
        BLOGD("GoBackground, uniqueId: %{public}" PRIu64 ".", uniqueId_);
    }
    return consumer_->GoBackground();
}

uint64_t ConsumerSurface::GetUniqueId() const
{
    if (producer_ == nullptr) {
        return 0;
    }
    return producer_->GetUniqueId();
}

void ConsumerSurface::Dump(std::string& result) const
{
    if (consumer_ == nullptr) {
        return;
    }
    return consumer_->Dump(result);
}

void ConsumerSurface::DumpCurrentFrameLayer() const
{
    if (consumer_ == nullptr) {
        return;
    }
    return consumer_->DumpCurrentFrameLayer();
}

GSError ConsumerSurface::SetTransform(GraphicTransformType transform)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetTransform(transform);
}

GraphicTransformType ConsumerSurface::GetTransform() const
{
    if (consumer_ == nullptr) {
        return GraphicTransformType::GRAPHIC_ROTATE_BUTT;
    }
    return consumer_->GetTransform();
}

GSError ConsumerSurface::SetScalingMode(uint32_t sequence, ScalingMode scalingMode)
{
    if (producer_ == nullptr || scalingMode < ScalingMode::SCALING_MODE_FREEZE ||
        scalingMode > ScalingMode::SCALING_MODE_SCALE_FIT) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetScalingMode(sequence, scalingMode);
}

GSError ConsumerSurface::SetScalingMode(ScalingMode scalingMode)
{
    if (producer_ == nullptr || scalingMode < ScalingMode::SCALING_MODE_FREEZE ||
        scalingMode > ScalingMode::SCALING_MODE_SCALE_FIT) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetScalingMode(scalingMode);
}

GSError ConsumerSurface::GetScalingMode(uint32_t sequence, ScalingMode& scalingMode)
{
    if (consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->GetScalingMode(sequence, scalingMode);
}

GSError ConsumerSurface::SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData>& metaData)
{
    if (producer_ == nullptr || metaData.size() == 0) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetMetaData(sequence, metaData);
}

GSError ConsumerSurface::SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key,
                                        const std::vector<uint8_t>& metaData)
{
    if (producer_ == nullptr || key < GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X ||
        key > GraphicHDRMetadataKey::GRAPHIC_MATAKEY_HDR_VIVID || metaData.size() == 0) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetMetaDataSet(sequence, key, metaData);
}

GSError ConsumerSurface::QueryMetaDataType(uint32_t sequence, HDRMetaDataType& type) const
{
    if (consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->QueryMetaDataType(sequence, type);
}

GSError ConsumerSurface::GetMetaData(uint32_t sequence, std::vector<GraphicHDRMetaData>& metaData) const
{
    if (consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->GetMetaData(sequence, metaData);
}

GSError ConsumerSurface::GetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey& key,
                                        std::vector<uint8_t>& metaData) const
{
    if (consumer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->GetMetaDataSet(sequence, key, metaData);
}

GSError ConsumerSurface::SetTunnelHandle(const GraphicExtDataHandle *handle)
{
    if (producer_ == nullptr || handle == nullptr || handle->reserveInts == 0) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetTunnelHandle(handle);
}

sptr<SurfaceTunnelHandle> ConsumerSurface::GetTunnelHandle() const
{
    if (consumer_ == nullptr) {
        return nullptr;
    }
    return consumer_->GetTunnelHandle();
}

void ConsumerSurface::SetBufferHold(bool hold)
{
    if (consumer_ == nullptr) {
        return;
    }
    consumer_->SetBufferHold(hold);
}

GSError ConsumerSurface::SetPresentTimestamp(uint32_t sequence, const GraphicPresentTimestamp& timestamp)
{
    if (consumer_ == nullptr || timestamp.type == GraphicPresentTimestampType::GRAPHIC_DISPLAY_PTS_UNSUPPORTED) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return consumer_->SetPresentTimestamp(sequence, timestamp);
}

void ConsumerSurface::ConsumerRequestCpuAccess(bool on)
{
    if (consumer_ == nullptr) {
        return;
    }
    consumer_->ConsumerRequestCpuAccess(on);
}

GraphicTransformType ConsumerSurface::GetTransformHint() const
{
    if (producer_ == nullptr) {
        return GraphicTransformType::GRAPHIC_ROTATE_BUTT;
    }
    GraphicTransformType transformHint = GraphicTransformType::GRAPHIC_ROTATE_BUTT;
    if (producer_->GetTransformHint(transformHint) != GSERROR_OK) {
        BLOGE("GetTransformHint failed, uniqueId: %{public}" PRIu64 ".", uniqueId_);
        return GraphicTransformType::GRAPHIC_ROTATE_BUTT;
    }
    return transformHint;
}

GSError ConsumerSurface::SetTransformHint(GraphicTransformType transformHint)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetTransformHint(transformHint, 0); // broadcast to all producerSurfaces
}

GSError ConsumerSurface::SetSurfaceSourceType(OHSurfaceSource sourceType)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetSurfaceSourceType(sourceType);
}

OHSurfaceSource ConsumerSurface::GetSurfaceSourceType() const
{
    if (producer_ == nullptr) {
        return OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
    }
    OHSurfaceSource sourceType = OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
    if (producer_->GetSurfaceSourceType(sourceType) != GSERROR_OK) {
        BLOGE("GetSurfaceSourceType failed, uniqueId: %{public}" PRIu64 ".", uniqueId_);
        return OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
    }
    return sourceType;
}

GSError ConsumerSurface::SetSurfaceAppFrameworkType(std::string appFrameworkType)
{
    if (producer_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return producer_->SetSurfaceAppFrameworkType(appFrameworkType);
}

std::string ConsumerSurface::GetSurfaceAppFrameworkType() const
{
    if (producer_ == nullptr) {
        return "";
    }
    std::string appFrameworkType = "";
    if (producer_->GetSurfaceAppFrameworkType(appFrameworkType) != GSERROR_OK) {
        BLOGE("GetSurfaceAppFrameworkType failed, uniqueId: %{public}" PRIu64 ".", uniqueId_);
        return "";
    }
    return appFrameworkType;
}

float ConsumerSurface::GetHdrWhitePointBrightness() const
{
    if (consumer_ == nullptr) {
        return 0;
    }
    return consumer_->GetHdrWhitePointBrightness();
}

float ConsumerSurface::GetSdrWhitePointBrightness() const
{
    if (consumer_ == nullptr) {
        return 0;
    }
    return consumer_->GetSdrWhitePointBrightness();
}

GSError ConsumerSurface::GetSurfaceBufferTransformType(sptr<SurfaceBuffer> buffer,
    GraphicTransformType *transformType)
{
    if (buffer == nullptr || transformType == nullptr) {
        return SURFACE_ERROR_INVALID_PARAM;
    }
    *transformType = buffer->GetSurfaceBufferTransform();
    return GSERROR_OK;
}

GSError ConsumerSurface::IsSurfaceBufferInCache(uint32_t seqNum, bool& isInCache)
{
    if (consumer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return consumer_->IsSurfaceBufferInCache(seqNum, isInCache);
}

GSError ConsumerSurface::GetGlobalAlpha(int32_t &alpha)
{
    if (consumer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return consumer_->GetGlobalAlpha(alpha);
}

uint32_t ConsumerSurface::GetAvailableBufferCount() const
{
    if (consumer_ == nullptr) {
        return 0;
    }
    return consumer_->GetAvailableBufferCount();
}

GSError ConsumerSurface::GetLastFlushedDesiredPresentTimeStamp(int64_t &lastFlushedDesiredPresentTimeStamp) const
{
    if (consumer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return consumer_->GetLastFlushedDesiredPresentTimeStamp(lastFlushedDesiredPresentTimeStamp);
}

GSError ConsumerSurface::GetFrontDesiredPresentTimeStamp(int64_t &desiredPresentTimeStamp, bool &isAutoTimeStamp) const
{
    if (consumer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return consumer_->GetFrontDesiredPresentTimeStamp(desiredPresentTimeStamp, isAutoTimeStamp);
}

GSError ConsumerSurface::GetBufferSupportFastCompose(bool &bufferSupportFastCompose)
{
    if (consumer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    if (isFirstBuffer_.load()) {
        GSError ret = consumer_->GetBufferSupportFastCompose(bufferSupportFastCompose);
        if (ret == GSERROR_OK) {
            supportFastCompose_.store(bufferSupportFastCompose);
            isFirstBuffer_.store(false);
        }
        return ret;
    } else {
        bufferSupportFastCompose = supportFastCompose_.load();
        return GSERROR_OK;
    }
}

GSError ConsumerSurface::GetBufferCacheConfig(const sptr<SurfaceBuffer>& buffer, BufferRequestConfig& config)
{
    if (buffer == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (consumer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return consumer_->GetBufferCacheConfig(buffer, config);
}

GSError ConsumerSurface::GetCycleBuffersNumber(uint32_t& cycleBuffersNumber)
{
    if (consumer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return consumer_->GetCycleBuffersNumber(cycleBuffersNumber);
}

GSError ConsumerSurface::GetFrameGravity(int32_t &frameGravity)
{
    if (consumer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return consumer_->GetFrameGravity(frameGravity);
}

GSError ConsumerSurface::GetFixedRotation(int32_t &fixedRotation)
{
    if (consumer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return consumer_->GetFixedRotation(fixedRotation);
}

GSError ConsumerSurface::GetLastConsumeTime(int64_t &lastConsumeTime) const
{
    if (consumer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return consumer_->GetLastConsumeTime(lastConsumeTime);
}

GSError ConsumerSurface::SetMaxQueueSize(uint32_t queueSize)
{
    if (consumer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return consumer_->SetMaxQueueSize(queueSize);
}
GSError ConsumerSurface::GetMaxQueueSize(uint32_t &queueSize) const
{
    if (consumer_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return consumer_->GetMaxQueueSize(queueSize);
}
} // namespace OHOS
