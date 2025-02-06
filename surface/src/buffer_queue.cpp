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

#include "buffer_queue.h"
#include <algorithm>
#include <fstream>
#include <linux/dma-buf.h>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <cinttypes>
#include <unistd.h>
#include <parameters.h>

#include "acquire_fence_manager.h"
#include "buffer_utils.h"
#include "buffer_log.h"
#include "hebc_white_list.h"
#include "hitrace_meter.h"
#include "metadata_helper.h"
#include "sandbox_utils.h"
#include "surface_buffer_impl.h"
#include "sync_fence.h"
#include "sync_fence_tracker.h"
#include "surface_utils.h"
#include "surface_trace.h"
#include "v2_0/buffer_handle_meta_key_type.h"

namespace OHOS {
namespace {
constexpr int32_t FORCE_GLOBAL_ALPHA_MIN = -1;
constexpr int32_t FORCE_GLOBAL_ALPHA_MAX = 255;
constexpr uint32_t UNIQUE_ID_OFFSET = 32;
constexpr uint32_t BUFFER_MEMSIZE_RATE = 1024;
constexpr uint32_t BUFFER_MEMSIZE_FORMAT = 2;
constexpr uint32_t MAXIMUM_LENGTH_OF_APP_FRAMEWORK = 64;
constexpr uint32_t INVALID_SEQUENCE = 0xFFFFFFFF;
constexpr uint32_t ONE_SECOND_TIMESTAMP = 1e9;
constexpr const char* BUFFER_SUPPORT_FASTCOMPOSE = "SupportFastCompose";
}

static const std::map<BufferState, std::string> BufferStateStrs = {
    {BUFFER_STATE_RELEASED,                    "0 <released>"},
    {BUFFER_STATE_REQUESTED,                   "1 <requested>"},
    {BUFFER_STATE_FLUSHED,                     "2 <flushed>"},
    {BUFFER_STATE_ACQUIRED,                    "3 <acquired>"},
    {BUFFER_STATE_ATTACHED,                    "4 <attached>"},
};

static uint64_t GetUniqueIdImpl()
{
    static std::atomic<uint32_t> counter { 0 };
    static uint64_t id = static_cast<uint64_t>(GetRealPid()) << UNIQUE_ID_OFFSET;
    return id | counter.fetch_add(1, std::memory_order_relaxed);
}

static bool IsLocalRender()
{
    std::ifstream procfile("/proc/self/cmdline");
    if (!procfile.is_open()) {
        BLOGE("Error opening procfile!");
        return false;
    }
    std::string processName;
    std::getline(procfile, processName);
    procfile.close();
    std::string target = "/system/bin/render_service";
    bool result = processName.substr(0, target.size()) == target;
    return result;
}

BufferQueue::BufferQueue(const std::string &name)
    : name_(name), uniqueId_(GetUniqueIdImpl()), isLocalRender_(IsLocalRender())
{
    BLOGD("BufferQueue ctor, uniqueId: %{public}" PRIu64 ".", uniqueId_);
    acquireLastFlushedBufSequence_ = INVALID_SEQUENCE;

    if (isLocalRender_) {
        if (!HebcWhiteList::GetInstance().Init()) {
            BLOGW("HebcWhiteList init failed");
        }
    }
}

BufferQueue::~BufferQueue()
{
    BLOGD("~BufferQueue dtor, uniqueId: %{public}" PRIu64 ".", uniqueId_);
    for (auto &[id, _] : bufferQueueCache_) {
        OnBufferDeleteForRS(id);
    }
}

uint32_t BufferQueue::GetUsedSize()
{
    return static_cast<uint32_t>(bufferQueueCache_.size());
}

GSError BufferQueue::GetProducerInitInfo(ProducerInitInfo &info)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    info.name = name_;
    info.width = defaultWidth_;
    info.height = defaultHeight_;
    info.uniqueId = uniqueId_;
    info.isInHebcList = HebcWhiteList::GetInstance().Check(info.appName);
    info.bufferName = bufferName_;
    return GSERROR_OK;
}

GSError BufferQueue::PopFromFreeListLocked(sptr<SurfaceBuffer> &buffer,
    const BufferRequestConfig &config)
{
    for (auto it = freeList_.begin(); it != freeList_.end(); it++) {
        auto mapIter = bufferQueueCache_.find(*it);
        if (mapIter != bufferQueueCache_.end() && mapIter->second.config == config) {
            if (mapIter->first == acquireLastFlushedBufSequence_) {
                continue;
            }
            buffer = mapIter->second.buffer;
            freeList_.erase(it);
            return GSERROR_OK;
        }
    }

    if (freeList_.empty() || GetUsedSize() < bufferQueueSize_ ||
        (freeList_.size() == 1 && freeList_.front() == acquireLastFlushedBufSequence_)) {
        buffer = nullptr;
        return GSERROR_NO_BUFFER;
    }

    if (freeList_.front() == acquireLastFlushedBufSequence_) {
        freeList_.pop_front();
        freeList_.push_back(acquireLastFlushedBufSequence_);
    }

    buffer = bufferQueueCache_[freeList_.front()].buffer;
    buffer->SetSurfaceBufferColorGamut(config.colorGamut);
    buffer->SetSurfaceBufferTransform(config.transform);
    freeList_.pop_front();
    return GSERROR_OK;
}

GSError BufferQueue::PopFromDirtyListLocked(sptr<SurfaceBuffer> &buffer)
{
    if (!dirtyList_.empty()) {
        buffer = bufferQueueCache_[dirtyList_.front()].buffer;
        dirtyList_.pop_front();
        return GSERROR_OK;
    } else {
        buffer = nullptr;
        return GSERROR_NO_BUFFER;
    }
}

GSError BufferQueue::CheckRequestConfig(const BufferRequestConfig &config)
{
    if (config.colorGamut <= GraphicColorGamut::GRAPHIC_COLOR_GAMUT_INVALID ||
        config.colorGamut > GraphicColorGamut::GRAPHIC_COLOR_GAMUT_DISPLAY_BT2020 + 1) {
        BLOGW("colorGamut is %{public}d, uniqueId: %{public}" PRIu64 ".",
            static_cast<uint32_t>(config.colorGamut), uniqueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }

    if (config.transform < GraphicTransformType::GRAPHIC_ROTATE_NONE ||
        config.transform >= GraphicTransformType::GRAPHIC_ROTATE_BUTT) {
        BLOGW("transform is %{public}d, uniqueId: %{public}" PRIu64 ".", config.transform, uniqueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }
    return GSERROR_OK;
}

GSError BufferQueue::CheckFlushConfig(const BufferFlushConfigWithDamages &config)
{
    for (decltype(config.damages.size()) i = 0; i < config.damages.size(); i++) {
        if (config.damages[i].w < 0 || config.damages[i].h < 0) {
            BLOGW("damages[%{public}zu].w is %{public}d, .h is %{public}d, uniqueId: %{public}" PRIu64 ".",
                i, config.damages[i].w, config.damages[i].h, uniqueId_);
            return GSERROR_INVALID_ARGUMENTS;
        }
    }
    return GSERROR_OK;
}

bool BufferQueue::QueryIfBufferAvailable()
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    bool ret = !freeList_.empty() || (GetUsedSize() < bufferQueueSize_);
    return ret;
}

static GSError DelegatorDequeueBuffer(wptr<ConsumerSurfaceDelegator>& delegator,
                                      const BufferRequestConfig& config,
                                      sptr<BufferExtraData>& bedata,
                                      struct IBufferProducer::RequestBufferReturnValue& retval)
{
    auto consumerDelegator = delegator.promote();
    if (consumerDelegator == nullptr) {
        BLOGE("Consumer surface delegator has been expired");
        return GSERROR_INVALID_ARGUMENTS;
    }
    auto ret = consumerDelegator->DequeueBuffer(config, bedata, retval);
    if (ret != GSERROR_OK) {
        BLOGE("Consumer surface delegator failed to dequeuebuffer, err: %{public}d", ret);
        return ret;
    }

    ret = retval.buffer->Map();
    if (ret != GSERROR_OK) {
        BLOGE("Buffer map failed, err: %{public}d", ret);
        return ret;
    }
    retval.buffer->SetSurfaceBufferWidth(retval.buffer->GetWidth());
    retval.buffer->SetSurfaceBufferHeight(retval.buffer->GetHeight());

    return GSERROR_OK;
}

static void SetReturnValue(sptr<SurfaceBuffer>& buffer, sptr<BufferExtraData>& bedata,
                           struct IBufferProducer::RequestBufferReturnValue& retval)
{
    retval.sequence = buffer->GetSeqNum();
    bedata = buffer->GetExtraData();
    retval.fence = SyncFence::InvalidFence();
}

void BufferQueue::SetSurfaceBufferHebcMetaLocked(sptr<SurfaceBuffer> buffer)
{
    using namespace HDI::Display::Graphic::Common;
    // usage does not contain BUFFER_USAGE_CPU_HW_BOTH, just return
    if (!(buffer->GetUsage() & BUFFER_USAGE_CPU_HW_BOTH)) {
        return;
    }

    V2_0::BufferHandleAttrKey key = V2_0::BufferHandleAttrKey::ATTRKEY_REQUEST_ACCESS_TYPE;
    std::vector<uint8_t> values;
    if (isCpuAccessable_) { // hebc is off
        values.emplace_back(static_cast<uint8_t>(V2_0::HebcAccessType::HEBC_ACCESS_CPU_ACCESS));
    } else { // hebc is on
        values.emplace_back(static_cast<uint8_t>(V2_0::HebcAccessType::HEBC_ACCESS_HW_ONLY));
    }

    buffer->SetMetadata(key, values);
}

void BufferQueue::SetBatchHandle(bool batch)
{
    std::unique_lock<std::mutex> lock(mutex_);
    isBatch_ = batch;
}

GSError BufferQueue::RequestBufferCheckStatus()
{
    if (isBatch_) {
        return GSERROR_OK;
    }
    if (!GetStatusLocked()) {
        SURFACE_TRACE_NAME_FMT("RequestBufferCheckStatus status wrong,"
            "surface name: %s queueId: %" PRIu64 " status: %u", name_.c_str(), uniqueId_, GetStatusLocked());
        BLOGN_FAILURE_RET(GSERROR_NO_CONSUMER);
    }
    std::lock_guard<std::mutex> lockGuard(listenerMutex_);
    if (listener_ == nullptr && listenerClazz_ == nullptr) {
        SURFACE_TRACE_NAME_FMT("RequestBufferCheckStatus no listener, surface name: %s queueId: %" PRIu64,
            name_.c_str(), uniqueId_);
        BLOGN_FAILURE_RET(SURFACE_ERROR_CONSUMER_UNREGISTER_LISTENER);
    }

    return GSERROR_OK;
}

bool BufferQueue::WaitForCondition()
{
    return (!freeList_.empty() && !(freeList_.size() == 1 && freeList_.front() == acquireLastFlushedBufSequence_)) ||
        (GetUsedSize() < bufferQueueSize_) || !GetStatusLocked();
}

void BufferQueue::RequestBufferDebugInfoLocked()
{
    SURFACE_TRACE_NAME_FMT("lockLastFlushedBuffer seq: %u", acquireLastFlushedBufSequence_);
    std::map<BufferState, int32_t> bufferState;
    for (auto &[id, ele] : bufferQueueCache_) {
        SURFACE_TRACE_NAME_FMT("request buffer id: %u state: %u", id, ele.state);
        bufferState[ele.state] += 1;
    }
    std::string str = std::to_string(uniqueId_) +
        ", Released: " + std::to_string(bufferState[BUFFER_STATE_RELEASED]) +
        " Requested: " + std::to_string(bufferState[BUFFER_STATE_REQUESTED]) +
        " Flushed: " + std::to_string(bufferState[BUFFER_STATE_FLUSHED]) +
        " Acquired: " + std::to_string(bufferState[BUFFER_STATE_ACQUIRED]);
    if (str.compare(requestBufferStateStr_) != 0) {
        requestBufferStateStr_ = str;
        BLOGE("all buffer are using, uniqueId: %{public}s", str.c_str());
    }
}

GSError BufferQueue::RequestBufferLocked(const BufferRequestConfig &config, sptr<BufferExtraData> &bedata,
    struct IBufferProducer::RequestBufferReturnValue &retval, std::unique_lock<std::mutex> &lock)
{
    GSError ret = RequestBufferCheckStatus();
    if (ret != GSERROR_OK) {
        return ret;
    }

    // check param
    BufferRequestConfig updateConfig = config;
    updateConfig.usage |= defaultUsage_;
    ret = CheckRequestConfig(updateConfig);
    if (ret != GSERROR_OK) {
        BLOGE("CheckRequestConfig ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, uniqueId_);
        return SURFACE_ERROR_UNKOWN;
    }
    isAllocatingBufferCon_.wait(lock, [this]() { return !isAllocatingBuffer_; });
    SURFACE_TRACE_NAME_FMT("RequestBuffer name: %s queueId: %" PRIu64 " queueSize: %u",
        name_.c_str(), uniqueId_, bufferQueueSize_);
    // dequeue from free list
    sptr<SurfaceBuffer>& buffer = retval.buffer;
    ret = PopFromFreeListLocked(buffer, updateConfig);
    if (ret == GSERROR_OK) {
        return ReuseBuffer(updateConfig, bedata, retval, lock);
    }

    // check queue size
    if (GetUsedSize() >= bufferQueueSize_) {
        waitReqCon_.wait_for(lock, std::chrono::milliseconds(config.timeout),
            [this]() { return WaitForCondition(); });
        if (!GetStatusLocked() && !isBatch_) {
            SURFACE_TRACE_NAME_FMT("Status wrong, status: %d", GetStatusLocked());
            BLOGN_FAILURE_RET(GSERROR_NO_CONSUMER);
        }
        // try dequeue from free list again
        ret = PopFromFreeListLocked(buffer, updateConfig);
        if (ret == GSERROR_OK) {
            return ReuseBuffer(updateConfig, bedata, retval, lock);
        } else if (GetUsedSize() >= bufferQueueSize_) {
            RequestBufferDebugInfoLocked();
            return GSERROR_NO_BUFFER;
        }
    }

    ret = AllocBuffer(buffer, updateConfig, lock);
    if (ret == GSERROR_OK) {
        AddDeletingBuffersLocked(retval.deletingBuffers);
        SetSurfaceBufferHebcMetaLocked(buffer);
        SetSurfaceBufferGlobalAlphaUnlocked(buffer);
        SetReturnValue(buffer, bedata, retval);
    } else {
        BLOGE("Fail to alloc or map Buffer[%{public}d %{public}d] ret: %{public}d, uniqueId: %{public}" PRIu64,
            config.width, config.height, ret, uniqueId_);
    }
    return ret;
}

GSError BufferQueue::RequestBuffer(const BufferRequestConfig &config, sptr<BufferExtraData> &bedata,
    struct IBufferProducer::RequestBufferReturnValue &retval)
{
    if (wpCSurfaceDelegator_ != nullptr) {
        return DelegatorDequeueBuffer(wpCSurfaceDelegator_, config, bedata, retval);
    }
    std::unique_lock<std::mutex> lock(mutex_);
    return RequestBufferLocked(config, bedata, retval, lock);
}

GSError BufferQueue::SetProducerCacheCleanFlag(bool flag)
{
    std::unique_lock<std::mutex> lock(mutex_);
    return SetProducerCacheCleanFlagLocked(flag, lock);
}

GSError BufferQueue::SetProducerCacheCleanFlagLocked(bool flag, std::unique_lock<std::mutex> &lock)
{
    isAllocatingBufferCon_.wait(lock, [this]() { return !isAllocatingBuffer_; });
    producerCacheClean_ = flag;
    producerCacheList_.clear();
    return GSERROR_OK;
}

bool BufferQueue::CheckProducerCacheListLocked()
{
    for (auto &[id, _] : bufferQueueCache_) {
        if (std::find(producerCacheList_.begin(), producerCacheList_.end(), id) == producerCacheList_.end()) {
            return false;
        }
    }
    return true;
}

GSError BufferQueue::ReallocBufferLocked(const BufferRequestConfig &config,
    struct IBufferProducer::RequestBufferReturnValue &retval, std::unique_lock<std::mutex> &lock)
{
    DeleteBufferInCacheNoWaitForAllocatingState(retval.sequence);

    sptr<SurfaceBuffer> buffer = nullptr;
    auto sret = AllocBuffer(buffer, config, lock);
    if (sret != GSERROR_OK) {
        BLOGE("AllocBuffer failed: %{public}d, uniqueId: %{public}" PRIu64 ".", sret, uniqueId_);
        return sret;
    }

    retval.buffer = buffer;
    retval.sequence = buffer->GetSeqNum();
    bufferQueueCache_[retval.sequence].config = config;
    return GSERROR_OK;
}

void BufferQueue::AddDeletingBuffersLocked(std::vector<uint32_t> &deletingBuffers)
{
    deletingBuffers.reserve(deletingBuffers.size() + deletingList_.size());
    deletingBuffers.insert(deletingBuffers.end(), deletingList_.begin(), deletingList_.end());
    deletingList_.clear();
}

GSError BufferQueue::ReuseBuffer(const BufferRequestConfig &config, sptr<BufferExtraData> &bedata,
    struct IBufferProducer::RequestBufferReturnValue &retval, std::unique_lock<std::mutex> &lock)
{
    if (retval.buffer == nullptr) {
        BLOGE("input buffer is null, uniqueId: %{public}" PRIu64 ".", uniqueId_);
        return SURFACE_ERROR_UNKOWN;
    }
    retval.sequence = retval.buffer->GetSeqNum();
    if (bufferQueueCache_.find(retval.sequence) == bufferQueueCache_.end()) {
        BLOGE("cache not find the buffer(%{public}u), uniqueId: %{public}" PRIu64 ".", retval.sequence, uniqueId_);
        return SURFACE_ERROR_UNKOWN;
    }
    auto &cacheConfig = bufferQueueCache_[retval.sequence].config;
    SURFACE_TRACE_NAME_FMT("ReuseBuffer config width: %d height: %d usage: %llu format: %d id: %u",
        cacheConfig.width, cacheConfig.height, cacheConfig.usage, cacheConfig.format, retval.sequence);

    bool needRealloc = (config != bufferQueueCache_[retval.sequence].config);
    // config, realloc
    if (needRealloc) {
        auto sret = ReallocBufferLocked(config, retval, lock);
        if (sret != GSERROR_OK) {
            return sret;
        }
    }

    bufferQueueCache_[retval.sequence].state = BUFFER_STATE_REQUESTED;
    retval.fence = bufferQueueCache_[retval.sequence].fence;
    bedata = retval.buffer->GetExtraData();
    SetSurfaceBufferHebcMetaLocked(retval.buffer);
    SetSurfaceBufferGlobalAlphaUnlocked(retval.buffer);

    auto &dbs = retval.deletingBuffers;
    AddDeletingBuffersLocked(dbs);

    if (needRealloc || producerCacheClean_ || retval.buffer->GetConsumerAttachBufferFlag()) {
        if (producerCacheClean_) {
            producerCacheList_.push_back(retval.sequence);
            if (CheckProducerCacheListLocked()) {
                SetProducerCacheCleanFlagLocked(false, lock);
            }
        }
        retval.buffer->SetConsumerAttachBufferFlag(false);
    } else {
        retval.buffer = nullptr;
    }

    SURFACE_TRACE_NAME_FMT("%s:%u", name_.c_str(), retval.sequence);
    if (IsTagEnabled(HITRACE_TAG_GRAPHIC_AGP) && isLocalRender_) {
        static SyncFenceTracker releaseFenceThread("Release Fence");
        releaseFenceThread.TrackFence(retval.fence);
    }
    return GSERROR_OK;
}

GSError BufferQueue::CancelBufferLocked(uint32_t sequence, sptr<BufferExtraData> bedata)
{
    if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
        return SURFACE_ERROR_BUFFER_NOT_INCACHE;
    }

    if (bufferQueueCache_[sequence].state != BUFFER_STATE_REQUESTED &&
        bufferQueueCache_[sequence].state != BUFFER_STATE_ATTACHED) {
        return SURFACE_ERROR_BUFFER_STATE_INVALID;
    }
    bufferQueueCache_[sequence].state = BUFFER_STATE_RELEASED;
    freeList_.push_back(sequence);
    if (bufferQueueCache_[sequence].buffer == nullptr) {
        BLOGE("cache buffer is nullptr, sequence:%{public}u, uniqueId: %{public}" PRIu64 ".", sequence, uniqueId_);
        return SURFACE_ERROR_UNKOWN;
    }
    bufferQueueCache_[sequence].buffer->SetExtraData(bedata);

    waitReqCon_.notify_all();
    waitAttachCon_.notify_all();

    return GSERROR_OK;
}

GSError BufferQueue::CancelBuffer(uint32_t sequence, sptr<BufferExtraData> bedata)
{
    SURFACE_TRACE_NAME_FMT("CancelBuffer name: %s queueId: %" PRIu64 " sequence: %u",
        name_.c_str(), uniqueId_, sequence);
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return CancelBufferLocked(sequence, bedata);
}

GSError BufferQueue::CheckBufferQueueCacheLocked(uint32_t sequence)
{
    if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
        BLOGE("no find seq: %{public}u, uniqueId: %{public}" PRIu64 ".", sequence, uniqueId_);
        return SURFACE_ERROR_BUFFER_NOT_INCACHE;
    }

    auto &state = bufferQueueCache_[sequence].state;
    if (state != BUFFER_STATE_REQUESTED && state != BUFFER_STATE_ATTACHED) {
        BLOGE("seq: %{public}u, invalid state %{public}d, uniqueId: %{public}" PRIu64 ".",
            sequence, state, uniqueId_);
        return SURFACE_ERROR_BUFFER_STATE_INVALID;
    }
    return GSERROR_OK;
}

GSError BufferQueue::CheckBufferQueueCache(uint32_t sequence)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return CheckBufferQueueCacheLocked(sequence);
}

GSError BufferQueue::DelegatorQueueBuffer(uint32_t sequence, sptr<SyncFence> fence)
{
    auto consumerDelegator = wpCSurfaceDelegator_.promote();
    if (consumerDelegator == nullptr) {
        BLOGE("Consumer surface delegator has been expired");
        return GSERROR_INVALID_ARGUMENTS;
    }
    sptr<SurfaceBuffer> buffer = nullptr;
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
            return GSERROR_NO_ENTRY;
        }
        bufferQueueCache_[sequence].state = BUFFER_STATE_ACQUIRED;
        buffer = bufferQueueCache_[sequence].buffer;
    }
    GSError ret = consumerDelegator->QueueBuffer(buffer, fence->Get());
    if (ret != GSERROR_OK) {
        BLOGE("Consumer surface delegator failed to queuebuffer");
    }
    ret = ReleaseBuffer(buffer, SyncFence::InvalidFence());
    if (ret != GSERROR_OK) {
        BLOGE("Consumer surface delegator failed to releasebuffer");
    }
    return ret;
}

void BufferQueue::CallConsumerListener()
{
    SURFACE_TRACE_NAME_FMT("CallConsumerListener");
    sptr<IBufferConsumerListener> listener;
    IBufferConsumerListenerClazz *listenerClazz;
    {
        std::lock_guard<std::mutex> lockGuard(listenerMutex_);
        listener = listener_;
        listenerClazz = listenerClazz_;
    }
    if (listener != nullptr) {
        listener->OnBufferAvailable();
    } else if (listenerClazz != nullptr) {
        listenerClazz->OnBufferAvailable();
    }
}

GSError BufferQueue::FlushBuffer(uint32_t sequence, sptr<BufferExtraData> bedata,
    sptr<SyncFence> fence, const BufferFlushConfigWithDamages &config)
{
    SURFACE_TRACE_NAME_FMT("FlushBuffer name: %s queueId: %" PRIu64 " sequence: %u",
        name_.c_str(), uniqueId_, sequence);
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        if (!GetStatusLocked()) {
            SURFACE_TRACE_NAME_FMT("status: %d", GetStatusLocked());
            BLOGN_FAILURE_RET(GSERROR_NO_CONSUMER);
        }
    }
    // check param
    auto sret = CheckFlushConfig(config);
    if (sret != GSERROR_OK) {
        BLOGE("CheckFlushConfig ret: %{public}d, uniqueId: %{public}" PRIu64 ".", sret, uniqueId_);
        return sret;
    }

    sret = CheckBufferQueueCache(sequence);
    if (sret != GSERROR_OK) {
        return sret;
    }

    bool listenerNullCheck = false;
    {
        std::lock_guard<std::mutex> lockGuard(listenerMutex_);
        if (listener_ == nullptr && listenerClazz_ == nullptr) {
            listenerNullCheck = true;
        }
    }
    if (listenerNullCheck) {
        SURFACE_TRACE_NAME("listener is nullptr");
        BLOGE("listener is nullptr, uniqueId: %{public}" PRIu64 ".", uniqueId_);
        CancelBuffer(sequence, bedata);
        return SURFACE_ERROR_CONSUMER_UNREGISTER_LISTENER;
    }

    sret = DoFlushBuffer(sequence, bedata, fence, config);
    if (sret != GSERROR_OK) {
        return sret;
    }
    CallConsumerListener();

    if (wpCSurfaceDelegator_ != nullptr) {
        sret = DelegatorQueueBuffer(sequence, fence);
    }
    return sret;
}

GSError BufferQueue::GetLastFlushedBuffer(sptr<SurfaceBuffer>& buffer,
    sptr<SyncFence>& fence, float matrix[16], uint32_t matrixSize, bool isUseNewMatrix, bool needRecordSequence)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (needRecordSequence && acquireLastFlushedBufSequence_ != INVALID_SEQUENCE) {
        BLOGE("last flushed buffer(%{public}d) is using, uniqueId: %{public}" PRIu64 ".",
            acquireLastFlushedBufSequence_, uniqueId_);
        return SURFACE_ERROR_BUFFER_STATE_INVALID;
    }
    if (bufferQueueCache_.find(lastFlusedSequence_) == bufferQueueCache_.end()) {
        BLOGE("cache ont find the buffer(%{public}u), uniqueId: %{public}" PRIu64 ".", lastFlusedSequence_, uniqueId_);
        return SURFACE_ERROR_UNKOWN;
    }
    auto &state = bufferQueueCache_[lastFlusedSequence_].state;
    if (state == BUFFER_STATE_REQUESTED) {
        BLOGE("seq: %{public}u, invalid state %{public}d, uniqueId: %{public}" PRIu64 ".",
            lastFlusedSequence_, state, uniqueId_);
        return SURFACE_ERROR_BUFFER_STATE_INVALID;
    }
    buffer = bufferQueueCache_[lastFlusedSequence_].buffer;
    auto usage = buffer->GetUsage();
    if (usage & BUFFER_USAGE_PROTECTED) {
        BLOGE("lastFlusedSeq: %{public}u, usage: %{public}" PRIu64 ", uniqueId: %{public}" PRIu64 ".",
            lastFlusedSequence_, usage, uniqueId_);
        return SURFACE_ERROR_NOT_SUPPORT;
    }

    fence = lastFlusedFence_;
    Rect damage = {};
    damage.w = buffer->GetWidth();
    damage.h = buffer->GetHeight();

    auto utils = SurfaceUtils::GetInstance();
    if (isUseNewMatrix) {
        utils->ComputeTransformMatrixV2(matrix, matrixSize, buffer, lastFlushedTransform_, damage);
    } else {
        utils->ComputeTransformMatrix(matrix, matrixSize, buffer, lastFlushedTransform_, damage);
    }

    if (needRecordSequence) {
        acquireLastFlushedBufSequence_ = lastFlusedSequence_;
        SURFACE_TRACE_NAME_FMT("GetLastFlushedBuffer(needRecordSequence) name: %s queueId: %" PRIu64 " seq: %u",
            name_.c_str(), uniqueId_, acquireLastFlushedBufSequence_);
    }
    return GSERROR_OK;
}

GSError BufferQueue::AcquireLastFlushedBuffer(sptr<SurfaceBuffer> &buffer, sptr<SyncFence> &fence,
    float matrix[16], uint32_t matrixSize, bool isUseNewMatrix)
{
    return GetLastFlushedBuffer(buffer, fence, matrix, matrixSize, isUseNewMatrix, true);
}

GSError BufferQueue::ReleaseLastFlushedBuffer(uint32_t sequence)
{
    SURFACE_TRACE_NAME_FMT("ReleaseLastFlushedBuffer name: %s queueId: %" PRIu64 " seq: %u",
        name_.c_str(), uniqueId_, sequence);
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (acquireLastFlushedBufSequence_ == INVALID_SEQUENCE || acquireLastFlushedBufSequence_ != sequence) {
        BLOGE("ReleaseLastFlushedBuffer lastFlushBuffer:%{public}d sequence:%{public}d, uniqueId: %{public}" PRIu64,
            acquireLastFlushedBufSequence_, sequence, uniqueId_);
        return SURFACE_ERROR_BUFFER_STATE_INVALID;
    }
    acquireLastFlushedBufSequence_ = INVALID_SEQUENCE;
    waitReqCon_.notify_all();
    return GSERROR_OK;
}

GSError BufferQueue::DoFlushBufferLocked(uint32_t sequence, sptr<BufferExtraData> bedata,
    sptr<SyncFence> fence, const BufferFlushConfigWithDamages &config, std::unique_lock<std::mutex> &lock)
{
    if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
        BLOGE("bufferQueueCache not find sequence:%{public}u, uniqueId: %{public}" PRIu64 ".", sequence, uniqueId_);
        return SURFACE_ERROR_BUFFER_NOT_INCACHE;
    }
    if (bufferQueueCache_[sequence].isDeleting) {
        DeleteBufferInCache(sequence, lock);
        BLOGD("DoFlushBuffer delete seq: %{public}d, uniqueId: %{public}" PRIu64 ".", sequence, uniqueId_);
        CountTrace(HITRACE_TAG_GRAPHIC_AGP, name_, static_cast<int32_t>(dirtyList_.size()));
        return GSERROR_OK;
    }

    bufferQueueCache_[sequence].buffer->SetExtraData(bedata);
    int32_t supportFastCompose = 0;
    bufferQueueCache_[sequence].buffer->GetExtraData()->ExtraGet(
        BUFFER_SUPPORT_FASTCOMPOSE, supportFastCompose);
    bufferQueueCache_[sequence].buffer->SetSurfaceBufferTransform(transform_);

    uint64_t usage = static_cast<uint32_t>(bufferQueueCache_[sequence].config.usage);
    if (usage & BUFFER_USAGE_CPU_WRITE) {
        // api flush
        auto sret = bufferQueueCache_[sequence].buffer->FlushCache();
        if (sret != GSERROR_OK) {
            BLOGE("FlushCache ret: %{public}d, seq: %{public}u, uniqueId: %{public}" PRIu64 ".",
                sret, sequence, uniqueId_);
            return sret;
        }
    }
    // if failed, avoid to state rollback
    bufferQueueCache_[sequence].state = BUFFER_STATE_FLUSHED;
    bufferQueueCache_[sequence].fence = fence;
    bufferQueueCache_[sequence].damages = config.damages;
    dirtyList_.push_back(sequence);
    lastFlusedSequence_ = sequence;
    lastFlusedFence_ = fence;
    lastFlushedTransform_ = transform_;
    bufferSupportFastCompose_ = (bool)supportFastCompose;

    SetDesiredPresentTimestampAndUiTimestamp(sequence, config.desiredPresentTimestamp, config.timestamp);
    lastFlushedDesiredPresentTimeStamp_ = bufferQueueCache_[sequence].desiredPresentTimestamp;
    bool traceTag = IsTagEnabled(HITRACE_TAG_GRAPHIC_AGP);
    if (isLocalRender_) {
        AcquireFenceTracker::TrackFence(fence, traceTag);
    }
    // if you need dump SurfaceBuffer to file, you should execute hdc shell param set persist.dumpbuffer.enabled 1
    // and reboot your device
    static bool dumpBufferEnabled = system::GetParameter("persist.dumpbuffer.enabled", "0") != "0";
    if (dumpBufferEnabled) {
        // Wait for the status of the fence to change to SIGNALED.
        fence->Wait(-1);
        DumpToFileAsync(GetRealPid(), name_, bufferQueueCache_[sequence].buffer);
    }

    CountTrace(HITRACE_TAG_GRAPHIC_AGP, name_, static_cast<int32_t>(dirtyList_.size()));
    return GSERROR_OK;
}

GSError BufferQueue::DoFlushBuffer(uint32_t sequence, sptr<BufferExtraData> bedata,
    sptr<SyncFence> fence, const BufferFlushConfigWithDamages &config)
{
    SURFACE_TRACE_NAME_FMT("DoFlushBuffer name: %s queueId: %" PRIu64 " seq: %u",
        name_.c_str(), uniqueId_, sequence);
    std::unique_lock<std::mutex> lock(mutex_);
    return DoFlushBufferLocked(sequence, bedata, fence, config, lock);
}

void BufferQueue::SetDesiredPresentTimestampAndUiTimestamp(uint32_t sequence, int64_t desiredPresentTimestamp,
                                                           uint64_t uiTimestamp)
{
    bufferQueueCache_[sequence].isAutoTimestamp = false;
    if (desiredPresentTimestamp <= 0) {
        if (desiredPresentTimestamp == 0 && uiTimestamp != 0
            && uiTimestamp <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
            bufferQueueCache_[sequence].desiredPresentTimestamp = static_cast<int64_t>(uiTimestamp);
        } else {
            bufferQueueCache_[sequence].desiredPresentTimestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            bufferQueueCache_[sequence].isAutoTimestamp = true;
        }
    } else {
        bufferQueueCache_[sequence].desiredPresentTimestamp = desiredPresentTimestamp;
    }
    bufferQueueCache_[sequence].timestamp = static_cast<int64_t>(uiTimestamp);
}

void BufferQueue::LogAndTraceAllBufferInBufferQueueCache()
{
    std::map<BufferState, int32_t> bufferState;
    for (auto &[id, ele] : bufferQueueCache_) {
        SURFACE_TRACE_NAME_FMT("acquire buffer id: %d state: %d desiredPresentTimestamp: %" PRId64
            " isAotuTimestamp: %d", id, ele.state, ele.desiredPresentTimestamp, ele.isAutoTimestamp);
        bufferState[ele.state] += 1;
    }
    std::string str = std::to_string(uniqueId_) +
        ", Released: " + std::to_string(bufferState[BUFFER_STATE_RELEASED]) +
        " Requested: " + std::to_string(bufferState[BUFFER_STATE_REQUESTED]) +
        " Flushed: " + std::to_string(bufferState[BUFFER_STATE_FLUSHED]) +
        " Acquired: " + std::to_string(bufferState[BUFFER_STATE_ACQUIRED]);
    if (str.compare(acquireBufferStateStr_) != 0) {
        acquireBufferStateStr_ = str;
        BLOGE("there is no dirty buffer or no dirty buffer ready, uniqueId: %{public}s", str.c_str());
    }
}

GSError BufferQueue::AcquireBuffer(sptr<SurfaceBuffer> &buffer,
    sptr<SyncFence> &fence, int64_t &timestamp, std::vector<Rect> &damages)
{
    SURFACE_TRACE_NAME_FMT("AcquireBuffer name: %s queueId: %" PRIu64, name_.c_str(), uniqueId_);
    // dequeue from dirty list
    std::lock_guard<std::mutex> lockGuard(mutex_);
    GSError ret = PopFromDirtyListLocked(buffer);
    if (ret == GSERROR_OK) {
        uint32_t sequence = buffer->GetSeqNum();
        bufferQueueCache_[sequence].state = BUFFER_STATE_ACQUIRED;

        fence = bufferQueueCache_[sequence].fence;
        timestamp = bufferQueueCache_[sequence].timestamp;
        damages = bufferQueueCache_[sequence].damages;
        SURFACE_TRACE_NAME_FMT("acquire buffer sequence: %u desiredPresentTimestamp: %" PRId64 " isAotuTimestamp: %d",
            sequence, bufferQueueCache_[sequence].desiredPresentTimestamp,
            bufferQueueCache_[sequence].isAutoTimestamp);
    } else if (ret == GSERROR_NO_BUFFER) {
        LogAndTraceAllBufferInBufferQueueCache();
    }

    CountTrace(HITRACE_TAG_GRAPHIC_AGP, name_, static_cast<int32_t>(dirtyList_.size()));
    return ret;
}

GSError BufferQueue::AcquireBuffer(IConsumerSurface::AcquireBufferReturnValue &returnValue,
                                   int64_t expectPresentTimestamp, bool isUsingAutoTimestamp)
{
    SURFACE_TRACE_NAME_FMT("AcquireBuffer with PresentTimestamp name: %s queueId: %" PRIu64 " queueSize: %u"
        "expectPresentTimestamp: %" PRId64, name_.c_str(), uniqueId_, bufferQueueSize_, expectPresentTimestamp);
    if (expectPresentTimestamp <= 0) {
        return AcquireBuffer(returnValue.buffer, returnValue.fence, returnValue.timestamp, returnValue.damages);
    }
    std::vector<BufferAndFence> dropBuffers;
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        std::list<uint32_t>::iterator frontSequence = dirtyList_.begin();
        if (frontSequence == dirtyList_.end()) {
            LogAndTraceAllBufferInBufferQueueCache();
            return GSERROR_NO_BUFFER;
        }
        int64_t frontDesiredPresentTimestamp = bufferQueueCache_[*frontSequence].desiredPresentTimestamp;
        bool frontIsAutoTimestamp = bufferQueueCache_[*frontSequence].isAutoTimestamp;
        if (!frontIsAutoTimestamp && frontDesiredPresentTimestamp > expectPresentTimestamp
            && frontDesiredPresentTimestamp - ONE_SECOND_TIMESTAMP <= expectPresentTimestamp) {
            SURFACE_TRACE_NAME_FMT("Acquire no buffer ready");
            LogAndTraceAllBufferInBufferQueueCache();
            return GSERROR_NO_BUFFER_READY;
        }
        while (!(frontIsAutoTimestamp && !isUsingAutoTimestamp)
            && frontDesiredPresentTimestamp <= expectPresentTimestamp) {
            BufferElement& frontBufferElement = bufferQueueCache_[*frontSequence];
            if (++frontSequence == dirtyList_.end()) {
                BLOGD("Buffer seq(%{public}d) is the last buffer, do acquire.", dirtyList_.front());
                break;
            }
            BufferElement& secondBufferElement = bufferQueueCache_[*frontSequence];
            if ((secondBufferElement.isAutoTimestamp && !isUsingAutoTimestamp)
                || secondBufferElement.desiredPresentTimestamp > expectPresentTimestamp) {
                BLOGD("Next dirty buffer desiredPresentTimestamp: %{public}" PRId64 " not match expectPresentTimestamp"
                    ": %{public}" PRId64 ".", secondBufferElement.desiredPresentTimestamp, expectPresentTimestamp);
                break;
            }
            SURFACE_TRACE_NAME_FMT("DropBuffer name: %s queueId: %" PRIu64 " ,buffer seq: %u , buffer "
                "desiredPresentTimestamp: %" PRId64 " acquire expectPresentTimestamp: %" PRId64, name_.c_str(),
                uniqueId_, frontBufferElement.buffer->GetSeqNum(), frontBufferElement.desiredPresentTimestamp,
                expectPresentTimestamp);
            DropFirstDirtyBuffer(frontBufferElement, secondBufferElement, frontDesiredPresentTimestamp,
                                 frontIsAutoTimestamp, dropBuffers);
        }
        if (!frontIsAutoTimestamp && !IsPresentTimestampReady(frontDesiredPresentTimestamp, expectPresentTimestamp)) {
            SURFACE_TRACE_NAME_FMT("Acquire no buffer ready");
            LogAndTraceAllBufferInBufferQueueCache();
            return GSERROR_NO_BUFFER_READY;
        }
    }
    ReleaseDropBuffers(dropBuffers);
    return AcquireBuffer(returnValue.buffer, returnValue.fence, returnValue.timestamp, returnValue.damages);
}

void BufferQueue::DropFirstDirtyBuffer(BufferElement &frontBufferElement, BufferElement &secondBufferElement,
                                       int64_t &frontDesiredPresentTimestamp, bool &frontIsAutoTimestamp,
                                       std::vector<BufferAndFence> &dropBuffers)
{
    dirtyList_.pop_front();
    frontBufferElement.state = BUFFER_STATE_ACQUIRED;
    dropBuffers.emplace_back(frontBufferElement.buffer, frontBufferElement.fence);
    frontDesiredPresentTimestamp = secondBufferElement.desiredPresentTimestamp;
    frontIsAutoTimestamp = secondBufferElement.isAutoTimestamp;
}

void BufferQueue::ReleaseDropBuffers(std::vector<BufferAndFence> &dropBuffers)
{
    for (auto& dropBuffer : dropBuffers) {
        auto ret = ReleaseBuffer(dropBuffer.first, dropBuffer.second);
        if (ret != GSERROR_OK) {
            BLOGE("DropBuffer failed, ret: %{public}d, sequeue: %{public}u, uniqueId: %{public}" PRIu64 ".",
                ret, dropBuffer.first->GetSeqNum(), uniqueId_);
        }
    }
}

bool BufferQueue::IsPresentTimestampReady(int64_t desiredPresentTimestamp, int64_t expectPresentTimestamp)
{
    if (desiredPresentTimestamp <= expectPresentTimestamp) {
        return true;
    }
    if (desiredPresentTimestamp - ONE_SECOND_TIMESTAMP > expectPresentTimestamp) {
        return true;
    }
    return false;
}

void BufferQueue::ListenerBufferReleasedCb(sptr<SurfaceBuffer> &buffer, const sptr<SyncFence> &fence)
{
    {
        std::lock_guard<std::mutex> lockGuard(onBufferReleaseMutex_);
        if (onBufferRelease_ != nullptr) {
            SURFACE_TRACE_NAME_FMT("OnBufferRelease_ sequence: %u", buffer->GetSeqNum());
            sptr<SurfaceBuffer> buf = buffer;
            (void)onBufferRelease_(buf);
        }
    }

    sptr<IProducerListener> listener;
    sptr<IProducerListener> listenerBackup;
    {
        std::lock_guard<std::mutex> lockGuard(producerListenerMutex_);
        listener = producerListener_;
        listenerBackup = producerListenerBackup_;
    }

    if (listener != nullptr) {
        SURFACE_TRACE_NAME_FMT("onBufferReleasedForProducer sequence: %u", buffer->GetSeqNum());
        if (listener->OnBufferReleased() != GSERROR_OK) {
            BLOGE("seq: %{public}u, OnBufferReleased faile, uniqueId: %{public}" PRIu64 ".",
                buffer->GetSeqNum(), uniqueId_);
        }
    }

    if (listenerBackup != nullptr) {
        SURFACE_TRACE_NAME_FMT("onBufferReleasedBackupForProducer sequence: %u", buffer->GetSeqNum());
        if (listenerBackup->OnBufferReleasedWithFence(buffer, fence) != GSERROR_OK) {
            BLOGE("seq: %{public}u, OnBufferReleasedWithFence failed, uniqueId: %{public}" PRIu64 ".",
                buffer->GetSeqNum(), uniqueId_);
        }
    }
    std::lock_guard<std::mutex> lockGuard(mutex_);
    OnBufferDeleteCbForHardwareThreadLocked(buffer);
}

void BufferQueue::OnBufferDeleteCbForHardwareThreadLocked(const sptr<SurfaceBuffer> &buffer) const
{
    if (onBufferDeleteForRSHardwareThread_ != nullptr) {
        onBufferDeleteForRSHardwareThread_(buffer->GetSeqNum());
    }
}

GSError BufferQueue::ReleaseBuffer(sptr<SurfaceBuffer> &buffer, const sptr<SyncFence>& fence)
{
    if (buffer == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }

    uint32_t sequence = buffer->GetSeqNum();
    SURFACE_TRACE_NAME_FMT("ReleaseBuffer name: %s queueId: %" PRIu64 " seq: %u", name_.c_str(), uniqueId_, sequence);
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
            SURFACE_TRACE_NAME_FMT("buffer not found in cache");
            BLOGE("cache not find the buffer(%{public}u), uniqueId: %{public}" PRIu64 ".", sequence, uniqueId_);
            OnBufferDeleteCbForHardwareThreadLocked(buffer);
            return SURFACE_ERROR_BUFFER_NOT_INCACHE;
        }

        const auto &state = bufferQueueCache_[sequence].state;
        if (state != BUFFER_STATE_ACQUIRED && state != BUFFER_STATE_ATTACHED) {
            SURFACE_TRACE_NAME_FMT("invalid state: %u", state);
            BLOGD("invalid state: %{public}d, uniqueId: %{public}" PRIu64 ".", state, uniqueId_);
            return SURFACE_ERROR_BUFFER_STATE_INVALID;
        }

        bufferQueueCache_[sequence].state = BUFFER_STATE_RELEASED;
        bufferQueueCache_[sequence].fence = fence;

        if (bufferQueueCache_[sequence].isDeleting) {
            DeleteBufferInCache(sequence, lock);
        } else {
            freeList_.push_back(sequence);
        }
        waitReqCon_.notify_all();
        waitAttachCon_.notify_all();
    }
    ListenerBufferReleasedCb(buffer, fence);

    return GSERROR_OK;
}

GSError BufferQueue::AllocBuffer(sptr<SurfaceBuffer> &buffer,
    const BufferRequestConfig &config, std::unique_lock<std::mutex> &lock)
{
    sptr<SurfaceBuffer> bufferImpl = new SurfaceBufferImpl();
    uint32_t sequence = bufferImpl->GetSeqNum();
    SURFACE_TRACE_NAME_FMT("AllocBuffer name: %s queueId: %" PRIu64 ", config width: %d height: %d usage: %llu format:"
        " %d id: %u", name_.c_str(), uniqueId_, config.width, config.height, config.usage, config.format, sequence);
    ScalingMode scalingMode = scalingMode_;
    int32_t connectedPid = connectedPid_;
    isAllocatingBuffer_ = true;
    lock.unlock();
    GSError ret = bufferImpl->Alloc(config);
    lock.lock();
    isAllocatingBuffer_ = false;
    isAllocatingBufferCon_.notify_all();
    if (ret != GSERROR_OK) {
        BLOGE("Alloc failed, sequence:%{public}u, ret:%{public}d, uniqueId: %{public}" PRIu64 ".",
            sequence, ret, uniqueId_);
        return SURFACE_ERROR_UNKOWN;
    }

    bufferImpl->SetSurfaceBufferScalingMode(scalingMode);
    BufferElement ele = {
        .buffer = bufferImpl,
        .state = BUFFER_STATE_REQUESTED,
        .isDeleting = false,
        .config = config,
        .fence = SyncFence::InvalidFence(),
    };

    if (config.usage & BUFFER_USAGE_PROTECTED) {
        BLOGD("usage is BUFFER_USAGE_PROTECTED, uniqueId: %{public}" PRIu64 ".", uniqueId_);
        bufferQueueCache_[sequence] = ele;
        buffer = bufferImpl;
        return SURFACE_ERROR_OK;
    }

    ret = bufferImpl->Map();
    if (ret == GSERROR_OK) {
        bufferQueueCache_[sequence] = ele;
        buffer = bufferImpl;
    } else {
        BLOGE("Map failed, seq:%{public}u, ret:%{public}d, uniqueId: %{public}" PRIu64 ".",
            sequence, ret, uniqueId_);
        return SURFACE_ERROR_UNKOWN;
    }

    BufferHandle* bufferHandle = bufferImpl->GetBufferHandle();
    if (connectedPid != 0 && bufferHandle != nullptr) {
        ioctl(bufferHandle->fd, DMA_BUF_SET_NAME_A, std::to_string(connectedPid).c_str());
    }

    return SURFACE_ERROR_OK;
}

void BufferQueue::OnBufferDeleteForRS(uint32_t sequence)
{
    if (onBufferDeleteForRSMainThread_ != nullptr) {
        onBufferDeleteForRSMainThread_(sequence);
    }
    if (onBufferDeleteForRSHardwareThread_ != nullptr) {
        onBufferDeleteForRSHardwareThread_(sequence);
    }
}

void BufferQueue::DeleteBufferInCacheNoWaitForAllocatingState(uint32_t sequence)
{
    auto it = bufferQueueCache_.find(sequence);
    if (it != bufferQueueCache_.end()) {
        OnBufferDeleteForRS(sequence);
        BLOGD("DeleteBufferInCache seq: %{public}u, uniqueId: %{public}" PRIu64 ".", sequence, uniqueId_);
        bufferQueueCache_.erase(it);
        deletingList_.push_back(sequence);
    }
}

void BufferQueue::DeleteBufferInCache(uint32_t sequence, std::unique_lock<std::mutex> &lock)
{
    isAllocatingBufferCon_.wait(lock, [this]() { return !isAllocatingBuffer_; });
    DeleteBufferInCacheNoWaitForAllocatingState(sequence);
}

uint32_t BufferQueue::GetQueueSize()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return bufferQueueSize_;
}

void BufferQueue::DeleteBuffersLocked(int32_t count, std::unique_lock<std::mutex> &lock)
{
    SURFACE_TRACE_NAME_FMT("DeleteBuffersLocked count: %d", count);
    if (count <= 0) {
        return;
    }

    isAllocatingBufferCon_.wait(lock, [this]() { return !isAllocatingBuffer_; });
    while (!freeList_.empty()) {
        DeleteBufferInCacheNoWaitForAllocatingState(freeList_.front());
        freeList_.pop_front();
        count--;
        if (count <= 0) {
            return;
        }
    }

    while (!dirtyList_.empty()) {
        DeleteBufferInCacheNoWaitForAllocatingState(dirtyList_.front());
        dirtyList_.pop_front();
        count--;
        if (count <= 0) {
            return;
        }
    }

    for (auto&& ele : bufferQueueCache_) {
        ele.second.isDeleting = true;
        // we don't have to do anything
        count--;
        if (count <= 0) {
            break;
        }
    }
}

GSError BufferQueue::AttachBufferUpdateStatus(std::unique_lock<std::mutex> &lock, uint32_t sequence, int32_t timeOut)
{
    BufferState state = bufferQueueCache_[sequence].state;
    if (state == BUFFER_STATE_RELEASED) {
        bufferQueueCache_[sequence].state = BUFFER_STATE_ATTACHED;
    } else {
        waitAttachCon_.wait_for(lock, std::chrono::milliseconds(timeOut),
            [this, sequence]() { return (bufferQueueCache_[sequence].state == BUFFER_STATE_RELEASED); });
        if (bufferQueueCache_[sequence].state == BUFFER_STATE_RELEASED) {
            bufferQueueCache_[sequence].state = BUFFER_STATE_ATTACHED;
        } else {
            BLOGN_FAILURE_RET(SURFACE_ERROR_BUFFER_STATE_INVALID);
        }
    }

    for (auto iter = freeList_.begin(); iter != freeList_.end(); iter++) {
        if (sequence == *iter) {
            freeList_.erase(iter);
            break;
        }
    }
    return GSERROR_OK;
}

void BufferQueue::AttachBufferUpdateBufferInfo(sptr<SurfaceBuffer>& buffer, bool needMap)
{
    if (needMap) {
        buffer->Map();
    }
    buffer->SetSurfaceBufferWidth(buffer->GetWidth());
    buffer->SetSurfaceBufferHeight(buffer->GetHeight());
}

GSError BufferQueue::AttachBufferToQueueLocked(sptr<SurfaceBuffer> buffer, InvokerType invokerType, bool needMap)
{
    uint32_t sequence = buffer->GetSeqNum();
    if (GetUsedSize() >= bufferQueueSize_) {
        BLOGE("seq: %{public}u, buffer queue size:%{public}u, used size:%{public}u,"
            "uniqueId: %{public}" PRIu64 ".", sequence, bufferQueueSize_, GetUsedSize(), uniqueId_);
        return SURFACE_ERROR_BUFFER_QUEUE_FULL;
    }
    if (bufferQueueCache_.find(sequence) != bufferQueueCache_.end()) {
        BLOGE("seq: %{public}u, buffer is already in cache, uniqueId: %{public}" PRIu64 ".",
            sequence, uniqueId_);
        return SURFACE_ERROR_BUFFER_IS_INCACHE;
    }
    buffer->SetSurfaceBufferScalingMode(scalingMode_);
    BufferElement ele;
    ele = {
        .buffer = buffer,
        .isDeleting = false,
        .config = buffer->GetBufferRequestConfig(),
        .fence = SyncFence::InvalidFence(),
    };
    if (invokerType == InvokerType::PRODUCER_INVOKER) {
        ele.state = BUFFER_STATE_REQUESTED;
    } else {
        ele.state = BUFFER_STATE_ACQUIRED;
    }
    AttachBufferUpdateBufferInfo(buffer, needMap);
    bufferQueueCache_[sequence] = ele;
    return GSERROR_OK;
}

GSError BufferQueue::AttachBufferToQueue(sptr<SurfaceBuffer> buffer, InvokerType invokerType)
{
    SURFACE_TRACE_NAME_FMT("AttachBufferToQueue name: %s queueId: %" PRIu64 " sequence: %u invokerType: %u",
        name_.c_str(), uniqueId_, buffer->GetSeqNum(), invokerType);
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return AttachBufferToQueueLocked(buffer, invokerType, true);
}

GSError BufferQueue::DetachBufferFromQueueLocked(uint32_t sequence, InvokerType invokerType,
    std::unique_lock<std::mutex> &lock)
{
    if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
        BLOGE("seq: %{public}u, not find in cache, uniqueId: %{public}" PRIu64 ".",
            sequence, uniqueId_);
        return SURFACE_ERROR_BUFFER_NOT_INCACHE;
    }
    if (invokerType == InvokerType::PRODUCER_INVOKER) {
        if (bufferQueueCache_[sequence].state != BUFFER_STATE_REQUESTED) {
            BLOGE("seq: %{public}u, state: %{public}d, uniqueId: %{public}" PRIu64 ".",
                sequence, bufferQueueCache_[sequence].state, uniqueId_);
            return SURFACE_ERROR_BUFFER_STATE_INVALID;
        }
        OnBufferDeleteForRS(sequence);
        bufferQueueCache_.erase(sequence);
    } else {
        if (bufferQueueCache_[sequence].state != BUFFER_STATE_ACQUIRED) {
            BLOGE("seq: %{public}u, state: %{public}d, uniqueId: %{public}" PRIu64 ".",
                sequence, bufferQueueCache_[sequence].state, uniqueId_);
            return SURFACE_ERROR_BUFFER_STATE_INVALID;
        }
        DeleteBufferInCache(sequence, lock);
    }
    return GSERROR_OK;
}

GSError BufferQueue::DetachBufferFromQueue(sptr<SurfaceBuffer> buffer, InvokerType invokerType)
{
    SURFACE_TRACE_NAME_FMT("DetachBufferFromQueue name: %s queueId: %" PRIu64 " sequence: %u invokerType%u",
        name_.c_str(), uniqueId_, buffer->GetSeqNum(), invokerType);
    std::unique_lock<std::mutex> lock(mutex_);
    uint32_t sequence = buffer->GetSeqNum();
    auto ret = DetachBufferFromQueueLocked(sequence, invokerType, lock);
    if (ret != GSERROR_OK) {
        return ret;
    }

    return GSERROR_OK;
}

GSError BufferQueue::AttachBuffer(sptr<SurfaceBuffer> &buffer, int32_t timeOut)
{
    SURFACE_TRACE_NAME_FMT("%s", __func__);
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        if (!GetStatusLocked()) {
            BLOGN_FAILURE_RET(GSERROR_NO_CONSUMER);
        }
    }
    {
        std::lock_guard<std::mutex> lockGuard(listenerMutex_);
        if (listener_ == nullptr && listenerClazz_ == nullptr) {
            BLOGN_FAILURE_RET(SURFACE_ERROR_CONSUMER_UNREGISTER_LISTENER);
        }
    }

    if (buffer == nullptr) {
        BLOGN_FAILURE_RET(GSERROR_INVALID_OPERATING);
    }

    uint32_t sequence = buffer->GetSeqNum();
    std::unique_lock<std::mutex> lock(mutex_);
    if (bufferQueueCache_.find(sequence) != bufferQueueCache_.end()) {
        return AttachBufferUpdateStatus(lock, sequence, timeOut);
    }

    buffer->SetSurfaceBufferScalingMode(scalingMode_);
    BufferElement ele = {
        .buffer = buffer,
        .state = BUFFER_STATE_ATTACHED,
        .config = {
            .width = buffer->GetWidth(), .height = buffer->GetHeight(), .strideAlignment = 0x8,
            .format = buffer->GetFormat(), .usage = buffer->GetUsage(), .timeout = timeOut,
        },
        .damages = { { .w = buffer->GetWidth(), .h = buffer->GetHeight(), } },
    };
    AttachBufferUpdateBufferInfo(buffer, true);
    int32_t usedSize = static_cast<int32_t>(GetUsedSize());
    int32_t queueSize = static_cast<int32_t>(bufferQueueSize_);
    if (usedSize >= queueSize) {
        int32_t freeSize = static_cast<int32_t>(dirtyList_.size() + freeList_.size());
        if (freeSize >= usedSize - queueSize + 1) {
            DeleteBuffersLocked(usedSize - queueSize + 1, lock);
            bufferQueueCache_[sequence] = ele;
            return GSERROR_OK;
        } else {
            BLOGN_FAILURE_RET(GSERROR_OUT_OF_RANGE);
        }
    } else {
        bufferQueueCache_[sequence] = ele;
        return GSERROR_OK;
    }
}

GSError BufferQueue::DetachBuffer(sptr<SurfaceBuffer> &buffer)
{
    SURFACE_TRACE_NAME_FMT("%s", __func__);
    if (buffer == nullptr) {
        BLOGN_FAILURE_RET(GSERROR_INVALID_ARGUMENTS);
    }

    std::lock_guard<std::mutex> lockGuard(mutex_);
    uint32_t sequence = buffer->GetSeqNum();
    if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
        return GSERROR_NO_ENTRY;
    }

    if (bufferQueueCache_[sequence].state == BUFFER_STATE_REQUESTED) {
        BLOGD("DetachBuffer requested seq: %{public}u, uniqueId: %{public}" PRIu64 ".", sequence, uniqueId_);
    } else if (bufferQueueCache_[sequence].state == BUFFER_STATE_ACQUIRED) {
        BLOGD("DetachBuffer acquired seq: %{public}u, uniqueId: %{public}" PRIu64 ".", sequence, uniqueId_);
    } else {
        BLOGE("DetachBuffer invalid state: %{public}d, seq: %{public}u, uniqueId: %{public}" PRIu64 ".",
            bufferQueueCache_[sequence].state, sequence, uniqueId_);
        return GSERROR_NO_ENTRY;
    }
    OnBufferDeleteForRS(sequence);
    bufferQueueCache_.erase(sequence);
    return GSERROR_OK;
}

GSError BufferQueue::RegisterSurfaceDelegator(sptr<IRemoteObject> client, sptr<Surface> cSurface)
{
    sptr<ConsumerSurfaceDelegator> surfaceDelegator = ConsumerSurfaceDelegator::Create();
    if (surfaceDelegator == nullptr) {
        BLOGE("Failed to register consumer delegator because the surface delegator is nullptr");
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (!surfaceDelegator->SetClient(client)) {
        BLOGE("Failed to set client");
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (!surfaceDelegator->SetBufferQueue(this)) {
        BLOGE("Failed to set bufferqueue");
        return GSERROR_INVALID_ARGUMENTS;
    }

    surfaceDelegator->SetSurface(cSurface);
    wpCSurfaceDelegator_ = surfaceDelegator;
    return GSERROR_OK;
}

GSError BufferQueue::SetQueueSize(uint32_t queueSize)
{
    if (queueSize == 0) {
        BLOGW("queue size: %{public}u, uniqueId: %{public}" PRIu64 ".", queueSize, uniqueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }

    if (queueSize > SURFACE_MAX_QUEUE_SIZE) {
        BLOGW("invalid queueSize: %{public}u, uniqueId: %{public}" PRIu64 ".",
            queueSize, uniqueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    if (bufferQueueSize_ > queueSize) {
        DeleteBuffersLocked(bufferQueueSize_ - queueSize, lock);
    }
    // if increase the queue size, try to wakeup the blocked thread
    if (queueSize > bufferQueueSize_) {
        bufferQueueSize_ = queueSize;
        waitReqCon_.notify_all();
    } else {
        bufferQueueSize_ = queueSize;
    }

    return GSERROR_OK;
}

GSError BufferQueue::GetName(std::string &name)
{
    name = name_;
    return GSERROR_OK;
}

GSError BufferQueue::RegisterConsumerListener(sptr<IBufferConsumerListener> &listener)
{
    std::lock_guard<std::mutex> lockGuard(listenerMutex_);
    listener_ = listener;
    return GSERROR_OK;
}

GSError BufferQueue::RegisterConsumerListener(IBufferConsumerListenerClazz *listener)
{
    std::lock_guard<std::mutex> lockGuard(listenerMutex_);
    listenerClazz_ = listener;
    return GSERROR_OK;
}

GSError BufferQueue::UnregisterConsumerListener()
{
    std::lock_guard<std::mutex> lockGuard(listenerMutex_);
    listener_ = nullptr;
    listenerClazz_ = nullptr;
    return GSERROR_OK;
}

GSError BufferQueue::RegisterReleaseListener(OnReleaseFunc func)
{
    std::lock_guard<std::mutex> lockGuard(onBufferReleaseMutex_);
    onBufferRelease_ = func;
    return GSERROR_OK;
}

GSError BufferQueue::RegisterProducerReleaseListener(sptr<IProducerListener> listener)
{
    std::lock_guard<std::mutex> lockGuard(producerListenerMutex_);
    producerListener_ = listener;
    return GSERROR_OK;
}

GSError BufferQueue::RegisterProducerReleaseListenerBackup(sptr<IProducerListener> listener)
{
    std::lock_guard<std::mutex> lockGuard(producerListenerMutex_);
    producerListenerBackup_ = listener;
    return GSERROR_OK;
}

GSError BufferQueue::UnRegisterProducerReleaseListener()
{
    std::lock_guard<std::mutex> lockGuard(producerListenerMutex_);
    producerListener_ = nullptr;
    return GSERROR_OK;
}

GSError BufferQueue::UnRegisterProducerReleaseListenerBackup()
{
    std::lock_guard<std::mutex> lockGuard(producerListenerMutex_);
    producerListenerBackup_ = nullptr;
    return GSERROR_OK;
}

GSError BufferQueue::RegisterDeleteBufferListener(OnDeleteBufferFunc func, bool isForUniRedraw)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (isForUniRedraw) {
        if (onBufferDeleteForRSHardwareThread_ != nullptr) {
            return GSERROR_OK;
        }
        onBufferDeleteForRSHardwareThread_ = func;
    } else {
        if (onBufferDeleteForRSMainThread_ != nullptr) {
            return GSERROR_OK;
        }
        onBufferDeleteForRSMainThread_ = func;
    }
    return GSERROR_OK;
}

GSError BufferQueue::SetDefaultWidthAndHeight(int32_t width, int32_t height)
{
    if (width <= 0) {
        BLOGW("width is %{public}d, uniqueId: %{public}" PRIu64 ".", width, uniqueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }

    if (height <= 0) {
        BLOGW("height is %{public}d, uniqueId: %{public}" PRIu64 ".", height, uniqueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }
    std::lock_guard<std::mutex> lockGuard(mutex_);
    defaultWidth_ = width;
    defaultHeight_ = height;
    return GSERROR_OK;
}

int32_t BufferQueue::GetDefaultWidth()
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return defaultWidth_;
}

int32_t BufferQueue::GetDefaultHeight()
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return defaultHeight_;
}

GSError BufferQueue::SetDefaultUsage(uint64_t usage)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    defaultUsage_ = usage;
    return GSERROR_OK;
}

uint64_t BufferQueue::GetDefaultUsage()
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return defaultUsage_;
}

void BufferQueue::ClearLocked(std::unique_lock<std::mutex> &lock)
{
    isAllocatingBufferCon_.wait(lock, [this]() { return !isAllocatingBuffer_; });
    for (auto &[id, _] : bufferQueueCache_) {
        OnBufferDeleteForRS(id);
    }
    bufferQueueCache_.clear();
    freeList_.clear();
    dirtyList_.clear();
    deletingList_.clear();
}

GSError BufferQueue::GoBackground()
{
    sptr<IBufferConsumerListener> listener;
    IBufferConsumerListenerClazz *listenerClazz;
    {
        std::lock_guard<std::mutex> lockGuard(listenerMutex_);
        listener = listener_;
        listenerClazz = listenerClazz_;
    }
    if (listener != nullptr) {
        SURFACE_TRACE_NAME_FMT("OnGoBackground name: %s queueId: %" PRIu64, name_.c_str(), uniqueId_);
        listener->OnGoBackground();
    } else if (listenerClazz != nullptr) {
        SURFACE_TRACE_NAME_FMT("OnGoBackground name: %s queueId: %" PRIu64, name_.c_str(), uniqueId_);
        listenerClazz->OnGoBackground();
    }
    std::unique_lock<std::mutex> lock(mutex_);
    ClearLocked(lock);
    waitReqCon_.notify_all();
    SetProducerCacheCleanFlagLocked(false, lock);
    return GSERROR_OK;
}

GSError BufferQueue::CleanCache(bool cleanAll, uint32_t *bufSeqNum)
{
    sptr<IBufferConsumerListener> listener;
    IBufferConsumerListenerClazz *listenerClazz;
    {
        std::lock_guard<std::mutex> lockGuard(listenerMutex_);
        listener = listener_;
        listenerClazz = listenerClazz_;
    }
    if (cleanAll) {
        if (listener != nullptr) {
            SURFACE_TRACE_NAME_FMT("OnGoBackground name: %s queueId: %" PRIu64, name_.c_str(), uniqueId_);
            listener->OnGoBackground();
        } else if (listenerClazz != nullptr) {
            SURFACE_TRACE_NAME_FMT("OnGoBackground name: %s queueId: %" PRIu64, name_.c_str(), uniqueId_);
            listenerClazz->OnGoBackground();
        }
    } else {
        if (listener != nullptr) {
            SURFACE_TRACE_NAME_FMT("OnCleanCache name: %s queueId: %" PRIu64, name_.c_str(), uniqueId_);
            listener->OnCleanCache(bufSeqNum);
        } else if (listenerClazz != nullptr) {
            SURFACE_TRACE_NAME_FMT("OnCleanCache name: %s queueId: %" PRIu64, name_.c_str(), uniqueId_);
            listenerClazz->OnCleanCache(bufSeqNum);
        }
    }
    std::unique_lock<std::mutex> lock(mutex_);
    ClearLocked(lock);
    waitReqCon_.notify_all();
    return GSERROR_OK;
}

GSError BufferQueue::OnConsumerDied()
{
    std::unique_lock<std::mutex> lock(mutex_);
    ClearLocked(lock);
    waitReqCon_.notify_all();
    return GSERROR_OK;
}

GSError BufferQueue::IsSurfaceBufferInCache(uint32_t seqNum, bool &isInCache)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (bufferQueueCache_.find(seqNum) != bufferQueueCache_.end()) {
        isInCache = true;
    } else {
        isInCache = false;
    }
    return GSERROR_OK;
}

uint64_t BufferQueue::GetUniqueId() const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return uniqueId_;
}

GSError BufferQueue::SetTransform(GraphicTransformType transform)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (transform_ == transform) {
            return GSERROR_OK;
        }

        transform_ = transform;
    }
    sptr<IBufferConsumerListener> listener;
    IBufferConsumerListenerClazz *listenerClazz;
    {
        std::lock_guard<std::mutex> lockGuard(listenerMutex_);
        listener = listener_;
        listenerClazz = listenerClazz_;
    }
    if (listener != nullptr) {
        SURFACE_TRACE_NAME_FMT("OnTransformChange transform: %u", transform);
        listener->OnTransformChange();
    } else if (listenerClazz != nullptr) {
        SURFACE_TRACE_NAME_FMT("OnTransformChange transform: %u", transform);
        listenerClazz->OnTransformChange();
    }
    return GSERROR_OK;
}

GraphicTransformType BufferQueue::GetTransform() const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return transform_;
}

GSError BufferQueue::SetTransformHint(GraphicTransformType transformHint)
{
    std::unique_lock<std::mutex> lock(mutex_);
    transformHint_ = transformHint;
    return GSERROR_OK;
}

GraphicTransformType BufferQueue::GetTransformHint() const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return transformHint_;
}

GSError BufferQueue::SetSurfaceSourceType(OHSurfaceSource sourceType)
{
    std::unique_lock<std::mutex> lock(mutex_);
    sourceType_ = sourceType;
    return GSERROR_OK;
}

OHSurfaceSource BufferQueue::GetSurfaceSourceType() const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return sourceType_;
}

GSError BufferQueue::SetHdrWhitePointBrightness(float brightness)
{
    std::unique_lock<std::mutex> lock(mutex_);
    hdrWhitePointBrightness_ = brightness;
    return GSERROR_OK;
}

GSError BufferQueue::SetSdrWhitePointBrightness(float brightness)
{
    std::unique_lock<std::mutex> lock(mutex_);
    sdrWhitePointBrightness_ = brightness;
    return GSERROR_OK;
}

float BufferQueue::GetHdrWhitePointBrightness() const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return hdrWhitePointBrightness_;
}

float BufferQueue::GetSdrWhitePointBrightness() const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return sdrWhitePointBrightness_;
}

GSError BufferQueue::SetSurfaceAppFrameworkType(std::string appFrameworkType)
{
    if (appFrameworkType.empty()) {
        return GSERROR_NO_ENTRY;
    }
    if (appFrameworkType.size() > MAXIMUM_LENGTH_OF_APP_FRAMEWORK) {
        return GSERROR_OUT_OF_RANGE;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    appFrameworkType_ = appFrameworkType;
    return GSERROR_OK;
}

std::string BufferQueue::GetSurfaceAppFrameworkType() const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return appFrameworkType_;
}

GSError BufferQueue::SetBufferHold(bool hold)
{
    std::unique_lock<std::mutex> lock(mutex_);
    isBufferHold_ = hold;
    return GSERROR_OK;
}

GSError BufferQueue::SetBufferName(const std::string &bufferName)
{
    std::unique_lock<std::mutex> lock(mutex_);
    bufferName_ = bufferName;
    return GSERROR_OK;
}

GSError BufferQueue::SetScalingMode(uint32_t sequence, ScalingMode scalingMode)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
        return GSERROR_NO_ENTRY;
    }
    bufferQueueCache_[sequence].buffer->SetSurfaceBufferScalingMode(scalingMode);
    return GSERROR_OK;
}

GSError BufferQueue::SetScalingMode(ScalingMode scalingMode)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    for (auto it = bufferQueueCache_.begin(); it != bufferQueueCache_.end(); it++) {
        it->second.buffer->SetSurfaceBufferScalingMode(scalingMode);
    }
    scalingMode_ = scalingMode;
    return GSERROR_OK;
}

GSError BufferQueue::GetScalingMode(uint32_t sequence, ScalingMode &scalingMode)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
        return GSERROR_NO_ENTRY;
    }
    scalingMode = bufferQueueCache_.at(sequence).buffer->GetSurfaceBufferScalingMode();
    return GSERROR_OK;
}

GSError BufferQueue::SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (metaData.size() == 0) {
        BLOGW("metaData size is 0, uniqueId: %{public}" PRIu64 ".", uniqueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
        return GSERROR_NO_ENTRY;
    }
    bufferQueueCache_[sequence].metaData.clear();
    bufferQueueCache_[sequence].metaData = metaData;
    bufferQueueCache_[sequence].hdrMetaDataType = HDRMetaDataType::HDR_META_DATA;
    return GSERROR_OK;
}

GSError BufferQueue::SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key,
                                    const std::vector<uint8_t> &metaData)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (key < GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X ||
        key > GraphicHDRMetadataKey::GRAPHIC_MATAKEY_HDR_VIVID) {
        BLOGW("key is %{public}d, uniqueId: %{public}" PRIu64 ".", key, uniqueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (metaData.size() == 0) {
        BLOGW("metaData size is 0, uniqueId: %{public}" PRIu64 ".", uniqueId_);
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
        return GSERROR_NO_ENTRY;
    }
    bufferQueueCache_[sequence].metaDataSet.clear();
    bufferQueueCache_[sequence].key = key;
    bufferQueueCache_[sequence].metaDataSet = metaData;
    bufferQueueCache_[sequence].hdrMetaDataType = HDRMetaDataType::HDR_META_DATA_SET;
    return GSERROR_OK;
}

GSError BufferQueue::QueryMetaDataType(uint32_t sequence, HDRMetaDataType &type)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
        return GSERROR_NO_ENTRY;
    }
    type = bufferQueueCache_.at(sequence).hdrMetaDataType;
    return GSERROR_OK;
}

GSError BufferQueue::GetMetaData(uint32_t sequence, std::vector<GraphicHDRMetaData> &metaData)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
        return GSERROR_NO_ENTRY;
    }
    metaData.clear();
    metaData = bufferQueueCache_.at(sequence).metaData;
    return GSERROR_OK;
}

GSError BufferQueue::GetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey &key,
                                    std::vector<uint8_t> &metaData)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
        return GSERROR_NO_ENTRY;
    }
    metaData.clear();
    key = bufferQueueCache_.at(sequence).key;
    metaData = bufferQueueCache_.at(sequence).metaDataSet;
    return GSERROR_OK;
}

GSError BufferQueue::SetTunnelHandle(const sptr<SurfaceTunnelHandle> &handle)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    bool tunnelHandleChange = false;
    if (tunnelHandle_ == nullptr) {
        if (handle == nullptr) {
            BLOGW("tunnel handle is nullptr, uniqueId: %{public}" PRIu64 ".", uniqueId_);
            return GSERROR_INVALID_ARGUMENTS;
        }
        tunnelHandleChange = true;
    } else {
        tunnelHandleChange = tunnelHandle_->Different(handle);
    }
    if (!tunnelHandleChange) {
        BLOGW("same tunnel handle, uniqueId: %{public}" PRIu64 ".", uniqueId_);
        return GSERROR_NO_ENTRY;
    }
    tunnelHandle_ = handle;
    sptr<IBufferConsumerListener> listener;
    IBufferConsumerListenerClazz *listenerClazz;
    {
        std::lock_guard<std::mutex> lockGuard(listenerMutex_);
        listener = listener_;
        listenerClazz = listenerClazz_;
    }
    if (listener != nullptr) {
        SURFACE_TRACE_NAME("OnTunnelHandleChange");
        listener->OnTunnelHandleChange();
    } else if (listenerClazz != nullptr) {
        SURFACE_TRACE_NAME("OnTunnelHandleChange");
        listenerClazz->OnTunnelHandleChange();
    } else {
        return SURFACE_ERROR_CONSUMER_UNREGISTER_LISTENER;
    }
    return GSERROR_OK;
}

sptr<SurfaceTunnelHandle> BufferQueue::GetTunnelHandle()
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return tunnelHandle_;
}

GSError BufferQueue::SetPresentTimestamp(uint32_t sequence, const GraphicPresentTimestamp &timestamp)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
        return GSERROR_NO_ENTRY;
    }
    bufferQueueCache_[sequence].presentTimestamp = timestamp;
    return GSERROR_OK;
}

GSError BufferQueue::GetPresentTimestamp(uint32_t sequence, GraphicPresentTimestampType type, int64_t &time)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (bufferQueueCache_.find(sequence) == bufferQueueCache_.end()) {
        return GSERROR_NO_ENTRY;
    }
    if (type != bufferQueueCache_.at(sequence).presentTimestamp.type) {
        BLOGE("seq: %{public}u, PresentTimestampType [%{public}d] is not supported, the supported type is [%{public}d],"
            "uniqueId: %{public}" PRIu64 ".", sequence, type,
            bufferQueueCache_.at(sequence).presentTimestamp.type, uniqueId_);
        return GSERROR_NO_ENTRY;
    }
    switch (type) {
        case GraphicPresentTimestampType::GRAPHIC_DISPLAY_PTS_DELAY: {
            time = bufferQueueCache_.at(sequence).presentTimestamp.time;
            return GSERROR_OK;
        }
        case GraphicPresentTimestampType::GRAPHIC_DISPLAY_PTS_TIMESTAMP: {
            time = bufferQueueCache_.at(sequence).presentTimestamp.time - bufferQueueCache_.at(sequence).timestamp;
            return GSERROR_OK;
        }
        default: {
            BLOGE("seq: %{public}u, unsupported type: %{public}d, uniqueId: %{public}" PRIu64 ".",
                sequence, type, uniqueId_);
            return GSERROR_TYPE_ERROR;
        }
    }
}

void BufferQueue::SetSurfaceBufferGlobalAlphaUnlocked(sptr<SurfaceBuffer> buffer)
{
    std::lock_guard<std::mutex> lockGuard(globalAlphaMutex_);
    if (globalAlpha_ < FORCE_GLOBAL_ALPHA_MIN || globalAlpha_ > FORCE_GLOBAL_ALPHA_MAX) {
        BLOGE("Invalid global alpha value: %{public}d, uniqueId: %{public}" PRIu64 ".", globalAlpha_, uniqueId_);
        return;
    }
    using namespace HDI::Display::Graphic::Common;
    V2_0::BufferHandleAttrKey key = V2_0::BufferHandleAttrKey::ATTRKEY_FORCE_GLOBAL_ALPHA;
    std::vector<uint8_t> values;
    auto ret = MetadataHelper::ConvertMetadataToVec(globalAlpha_, values);
    if (ret != GSERROR_OK) {
        BLOGE("Convert global alpha value failed, ret: %{public}d, value: %{public}d, uniqueId: %{public}" PRIu64 ".",
            ret, globalAlpha_, uniqueId_);
        return;
    }
    buffer->SetMetadata(key, values);
}

GSError BufferQueue::SetGlobalAlpha(int32_t alpha)
{
    std::lock_guard<std::mutex> lockGuard(globalAlphaMutex_);
    globalAlpha_ = alpha;
    return GSERROR_OK;
}

GSError BufferQueue::GetGlobalAlpha(int32_t &alpha)
{
    std::lock_guard<std::mutex> lockGuard(globalAlphaMutex_);
    alpha = globalAlpha_;
    return GSERROR_OK;
}

void BufferQueue::DumpMetadata(std::string &result, BufferElement element)
{
    HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType colorSpaceType;
    MetadataHelper::GetColorSpaceType(element.buffer, colorSpaceType);
    HDI::Display::Graphic::Common::V1_0::CM_HDR_Metadata_Type hdrMetadataType =
        HDI::Display::Graphic::Common::V1_0::CM_METADATA_NONE;
    std::vector<uint8_t> dataStatic;
    std::vector<uint8_t> dataDynamic;
    MetadataHelper::GetHDRDynamicMetadata(element.buffer, dataDynamic);
    MetadataHelper::GetHDRStaticMetadata(element.buffer, dataStatic);
    MetadataHelper::GetHDRMetadataType(element.buffer, hdrMetadataType);
    result += std::to_string(colorSpaceType) + ", ";
    result += " [staticMetadata: ";
    for (auto x : dataStatic) {
        result += std::to_string(x);
        result += " ";
    }
    result += " ],[dynamicMetadata: ";
    for (auto x : dataDynamic) {
        result += std::to_string(x);
        result += " ";
    }
    result += " ],[metadataType: ";
    result += std::to_string(hdrMetadataType) + "],";
}

void BufferQueue::DumpCache(std::string &result)
{
    for (auto it = bufferQueueCache_.begin(); it != bufferQueueCache_.end(); it++) {
        BufferElement element = it->second;
        if (BufferStateStrs.find(element.state) != BufferStateStrs.end()) {
            result += "        sequence = " + std::to_string(it->first) +
                ", state = " + std::to_string(element.state) +
                ", timestamp = " + std::to_string(element.timestamp);
        }
        for (decltype(element.damages.size()) i = 0; i < element.damages.size(); i++) {
            result += ", damagesRect = [" + std::to_string(i) + "] = [" +
            std::to_string(element.damages[i].x) + ", " +
            std::to_string(element.damages[i].y) + ", " +
            std::to_string(element.damages[i].w) + ", " +
            std::to_string(element.damages[i].h) + "],";
        }
        result += " config = [" + std::to_string(element.config.width) + "x" +
            std::to_string(element.config.height) + ", " +
            std::to_string(element.config.strideAlignment) + ", " +
            std::to_string(element.config.format) +", " +
            std::to_string(element.config.usage) + ", " +
            std::to_string(element.config.timeout) + ", " +
            std::to_string(element.config.colorGamut) + ", " +
            std::to_string(element.config.transform) + "],";
        DumpMetadata(result, element);
        result += " scalingMode = " + std::to_string(element.buffer->GetSurfaceBufferScalingMode()) + ",";
        result += " HDR = " + std::to_string(element.hdrMetaDataType) + ", ";

        double bufferMemSize = 0;
        if (element.buffer != nullptr) {
            result += " bufferWith = " + std::to_string(element.buffer->GetWidth()) +
                    ", bufferHeight = " + std::to_string(element.buffer->GetHeight());
            bufferMemSize = static_cast<double>(element.buffer->GetSize()) / BUFFER_MEMSIZE_RATE;
        }

        std::ostringstream ss;
        ss.precision(BUFFER_MEMSIZE_FORMAT);
        ss.setf(std::ios::fixed);
        ss << bufferMemSize;
        std::string str = ss.str();
        result += ", bufferMemSize = " + str + "(KiB).\n";
    }
}

void BufferQueue::Dump(std::string &result)
{
    std::unique_lock<std::mutex> lock(mutex_);
    std::ostringstream ss;
    ss.precision(BUFFER_MEMSIZE_FORMAT);
    ss.setf(std::ios::fixed);
    static double allSurfacesMemSize = 0;
    uint64_t totalBufferListSize = 0;
    double memSizeInKB = 0;

    isAllocatingBufferCon_.wait(lock, [this]() { return !isAllocatingBuffer_; });
    for (auto it = bufferQueueCache_.begin(); it != bufferQueueCache_.end(); it++) {
        BufferElement element = it->second;
        if (element.buffer != nullptr) {
            totalBufferListSize += element.buffer->GetSize();
        }
    }
    memSizeInKB = static_cast<double>(totalBufferListSize) / BUFFER_MEMSIZE_RATE;

    allSurfacesMemSize += memSizeInKB;
    uint32_t resultLen = result.size();
    std::string dumpEndFlag = "dumpend";
    if (resultLen > dumpEndFlag.size() && resultLen > 1) {
        std::string dumpEndIn(result, resultLen - dumpEndFlag.size(), resultLen - 1);
        if (dumpEndIn == dumpEndFlag) {
            ss << allSurfacesMemSize;
            std::string dumpEndStr = ss.str();
            result.erase(resultLen - dumpEndFlag.size(), resultLen - 1);
            result += dumpEndStr + " KiB.\n";
            allSurfacesMemSize = 0;
            return;
        }
    }

    ss.str("");
    ss << memSizeInKB;
    std::string str = ss.str();
    result.append("\nBufferQueue:\n");
    result += "      default-size = [" + std::to_string(defaultWidth_) + "x" + std::to_string(defaultHeight_) + "]" +
        ", FIFO = " + std::to_string(bufferQueueSize_) +
        ", name = " + name_ +
        ", uniqueId = " + std::to_string(uniqueId_) +
        ", usedBufferListLen = " + std::to_string(GetUsedSize()) +
        ", freeBufferListLen = " + std::to_string(freeList_.size()) +
        ", dirtyBufferListLen = " + std::to_string(dirtyList_.size()) +
        ", totalBuffersMemSize = " + str + "(KiB)" +
        ", hdrWhitePointBrightness = " + std::to_string(hdrWhitePointBrightness_) +
        ", sdrWhitePointBrightness = " + std::to_string(sdrWhitePointBrightness_) +
        ", lockLastFlushedBuffer seq = " + std::to_string(acquireLastFlushedBufSequence_) + "\n";

    result.append("      bufferQueueCache:\n");
    DumpCache(result);
}

bool BufferQueue::GetStatusLocked() const
{
    return isValidStatus_;
}

bool BufferQueue::GetStatus() const
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return GetStatusLocked();
}

void BufferQueue::SetStatus(bool status)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    isValidStatus_ = status;
    waitReqCon_.notify_all();
}

uint32_t BufferQueue::GetAvailableBufferCount()
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return static_cast<uint32_t>(dirtyList_.size());
}

void BufferQueue::SetConnectedPid(int32_t connectedPid)
{
    connectedPid_ = connectedPid;
}

/**
 * @brief Optimize the original FlushBuffer to reduce segmentation locking.
 */
GSError BufferQueue::FlushBufferImprovedLocked(uint32_t sequence, sptr<BufferExtraData> &bedata,
    const sptr<SyncFence> &fence, const BufferFlushConfigWithDamages &config, std::unique_lock<std::mutex> &lock)
{
    if (!GetStatusLocked()) {
        SURFACE_TRACE_NAME_FMT("status: %d", GetStatusLocked());
        BLOGN_FAILURE_RET(GSERROR_NO_CONSUMER);
    }
    // check param
    auto sret = CheckFlushConfig(config);
    if (sret != GSERROR_OK) {
        BLOGE("CheckFlushConfig ret: %{public}d, uniqueId: %{public}" PRIu64 ".", sret, uniqueId_);
        return sret;
    }

    sret = CheckBufferQueueCacheLocked(sequence);
    if (sret != GSERROR_OK) {
        return sret;
    }

    {
        std::lock_guard<std::mutex> lockGuard(listenerMutex_);
        if (listener_ == nullptr && listenerClazz_ == nullptr) {
            BLOGE("listener is nullptr, uniqueId: %{public}" PRIu64 ".", uniqueId_);
            return SURFACE_ERROR_CONSUMER_UNREGISTER_LISTENER;
        }
    }
    sret = DoFlushBufferLocked(sequence, bedata, fence, config, lock);
    if (sret != GSERROR_OK) {
        return sret;
    }
    return sret;
}

GSError BufferQueue::RequestAndDetachBuffer(const BufferRequestConfig& config, sptr<BufferExtraData>& bedata,
    struct IBufferProducer::RequestBufferReturnValue& retval)
{
    SURFACE_TRACE_NAME_FMT("RequestAndDetachBuffer queueId: %" PRIu64, uniqueId_);
    std::unique_lock<std::mutex> lock(mutex_);
    auto ret = RequestBufferLocked(config, bedata, retval, lock);
    if (ret != GSERROR_OK) {
        return ret;
    }
    return DetachBufferFromQueueLocked(retval.sequence, InvokerType::PRODUCER_INVOKER, lock);
}

GSError BufferQueue::AttachAndFlushBuffer(sptr<SurfaceBuffer>& buffer, sptr<BufferExtraData>& bedata,
    const sptr<SyncFence>& fence, BufferFlushConfigWithDamages& config, bool needMap)
{
    SURFACE_TRACE_NAME_FMT("AttachAndFlushBuffer queueId: %" PRIu64 " sequence: %u", uniqueId_, buffer->GetSeqNum());
    GSError ret;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        ret = AttachBufferToQueueLocked(buffer, InvokerType::PRODUCER_INVOKER, needMap);
        if (ret != GSERROR_OK) {
            return ret;
        }
        uint32_t sequence = buffer->GetSeqNum();
        ret = FlushBufferImprovedLocked(sequence, bedata, fence, config, lock);
        if (ret != GSERROR_OK) {
            for (auto it = dirtyList_.begin(); it != dirtyList_.end(); it++) {
                if (*it == sequence) {
                    dirtyList_.erase(it);
                    break;
                }
            }
            bufferQueueCache_.erase(sequence);
            return ret;
        }
    }
    CallConsumerListener();
    return ret;
}

GSError BufferQueue::GetLastFlushedDesiredPresentTimeStamp(int64_t &lastFlushedDesiredPresentTimeStamp)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    lastFlushedDesiredPresentTimeStamp = lastFlushedDesiredPresentTimeStamp_;
    return GSERROR_OK;
}

GSError BufferQueue::GetBufferSupportFastCompose(bool &bufferSupportFastCompose)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    bufferSupportFastCompose = bufferSupportFastCompose_;
    return GSERROR_OK;
}

GSError BufferQueue::GetBufferCacheConfig(const sptr<SurfaceBuffer>& buffer, BufferRequestConfig& config)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    auto iter = bufferQueueCache_.find(buffer->GetSeqNum());
    if (iter == bufferQueueCache_.end()) {
        return GSERROR_BUFFER_NOT_INCACHE;
    }
    config = iter->second.config;
    return GSERROR_OK;
}
}; // namespace OHOS
