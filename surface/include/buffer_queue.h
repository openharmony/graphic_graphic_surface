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

#ifndef FRAMEWORKS_SURFACE_INCLUDE_BUFFER_QUEUE_H
#define FRAMEWORKS_SURFACE_INCLUDE_BUFFER_QUEUE_H

#include <map>
#include <list>
#include <vector>
#include <mutex>

#include <ibuffer_consumer_listener.h>
#include <ibuffer_producer.h>
#include "iconsumer_surface.h"
#include "surface_type.h"
#include <surface_tunnel_handle.h>
#include "surface_buffer.h"
#include "consumer_surface_delegator.h"

namespace OHOS {
enum BufferState {
    BUFFER_STATE_RELEASED,
    BUFFER_STATE_REQUESTED,
    BUFFER_STATE_FLUSHED,
    BUFFER_STATE_ACQUIRED,
    BUFFER_STATE_ATTACHED,
};

enum InvokerType {
    PRODUCER_INVOKER,
    CONSUMER_INVOKER,
};

using BufferElement = struct BufferElement {
    sptr<SurfaceBuffer> buffer;
    BufferState state;
    bool isDeleting;

    BufferRequestConfig config;
    sptr<SyncFence> fence;
    int64_t timestamp;
    std::vector<Rect> damages;
    HDRMetaDataType hdrMetaDataType = HDRMetaDataType::HDR_NOT_USED;
    std::vector<GraphicHDRMetaData> metaData;
    GraphicHDRMetadataKey key;
    std::vector<uint8_t> metaDataSet;
    GraphicPresentTimestamp presentTimestamp = {GRAPHIC_DISPLAY_PTS_UNSUPPORTED, 0};
    /**
     * The desired time to present the buffer in nanoseconds.
     * The buffer should wait until desiredPresentTimestamp is reached before being consumed and displayed.
     * If multiple buffers reach desiredPresentTimestamp, the earlier buffer should be dropped.
     */
    int64_t desiredPresentTimestamp;
    /**
     * The desiredPresentTimestamp is automatically generated by the system, isAutoTimestamp is true.
     * The desiredPresentTimestamp is manually set by the producer, isAutoTimestamp is false.
     */
    bool isAutoTimestamp;
};

using BufferAndFence = std::pair<sptr<SurfaceBuffer>, sptr<SyncFence>>;

class BufferQueue : public RefBase {
public:
    BufferQueue(const std::string &name);
    virtual ~BufferQueue();

    GSError GetProducerInitInfo(ProducerInitInfo &info);

    GSError RequestBuffer(const BufferRequestConfig &config, sptr<BufferExtraData> &bedata,
                          struct IBufferProducer::RequestBufferReturnValue &retval);

    GSError ReuseBuffer(const BufferRequestConfig &config, sptr<BufferExtraData> &bedata,
                        struct IBufferProducer::RequestBufferReturnValue &retval, std::unique_lock<std::mutex> &lock);

    GSError CancelBuffer(uint32_t sequence, sptr<BufferExtraData> bedata);

    GSError FlushBuffer(uint32_t sequence, sptr<BufferExtraData> bedata,
                        sptr<SyncFence> fence, const BufferFlushConfigWithDamages &config);

    GSError DoFlushBuffer(uint32_t sequence, sptr<BufferExtraData> bedata,
        sptr<SyncFence> fence, const BufferFlushConfigWithDamages &config);

    GSError GetLastFlushedBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
        float matrix[16], uint32_t matrixSize, bool isUseNewMatrix, bool needRecordSequence = false);

    GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                          int64_t &timestamp, std::vector<Rect> &damages);
    GSError AcquireBuffer(IConsumerSurface::AcquireBufferReturnValue &returnValue, int64_t expectPresentTimestamp,
                          bool isUsingAutoTimestamp);
    GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence);

    GSError AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut);

    GSError DetachBuffer(sptr<SurfaceBuffer>& buffer);

    GSError RegisterSurfaceDelegator(sptr<IRemoteObject> client, sptr<Surface> cSurface);

    bool QueryIfBufferAvailable();

    uint32_t GetQueueSize();
    GSError SetQueueSize(uint32_t queueSize);

    GSError GetName(std::string &name);

    GSError RegisterConsumerListener(sptr<IBufferConsumerListener>& listener);
    GSError RegisterConsumerListener(IBufferConsumerListenerClazz *listener);
    GSError RegisterReleaseListener(OnReleaseFunc func);
    GSError RegisterProducerReleaseListener(sptr<IProducerListener> listener);
    GSError RegisterProducerReleaseListenerWithFence(sptr<IProducerListener> listener);
    GSError UnRegisterProducerReleaseListener();
    GSError UnRegisterProducerReleaseListenerWithFence();
    GSError RegisterDeleteBufferListener(OnDeleteBufferFunc func, bool isForUniRedraw = false);
    GSError UnregisterConsumerListener();

    GSError SetDefaultWidthAndHeight(int32_t width, int32_t height);
    int32_t GetDefaultWidth();
    int32_t GetDefaultHeight();
    GSError SetDefaultUsage(uint64_t usage);
    uint64_t GetDefaultUsage();

    GSError CleanCache(bool cleanAll, uint32_t *bufSeqNum);
    GSError GoBackground();
    GSError OnConsumerDied();

    uint64_t GetUniqueId() const;

    void Dump(std::string &result);

    GSError SetTransform(GraphicTransformType transform);
    GraphicTransformType GetTransform() const;

    GSError SetBufferHold(bool hold);
    inline bool IsBufferHold()
    {
        return isBufferHold_;
    }
    GSError SetScalingMode(uint32_t sequence, ScalingMode scalingMode);
    GSError GetScalingMode(uint32_t sequence, ScalingMode &scalingMode);
    GSError SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData);
    GSError SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key,
                           const std::vector<uint8_t> &metaData);
    GSError QueryMetaDataType(uint32_t sequence, HDRMetaDataType &type);
    GSError GetMetaData(uint32_t sequence, std::vector<GraphicHDRMetaData> &metaData);
    GSError GetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey &key,
                           std::vector<uint8_t> &metaData);
    GSError SetTunnelHandle(const sptr<SurfaceTunnelHandle> &handle);
    sptr<SurfaceTunnelHandle> GetTunnelHandle();
    GSError SetPresentTimestamp(uint32_t sequence, const GraphicPresentTimestamp &timestamp);
    GSError GetPresentTimestamp(uint32_t sequence, GraphicPresentTimestampType type, int64_t &time);

    bool GetStatus() const;
    void SetStatus(bool status);

    void SetBatchHandle(bool batch);

    GSError SetProducerCacheCleanFlag(bool flag);
    inline void ConsumerRequestCpuAccess(bool on)
    {
        isCpuAccessable_ = on;
    }

    GSError AttachBufferToQueue(sptr<SurfaceBuffer> buffer, InvokerType invokerType);
    GSError DetachBufferFromQueue(sptr<SurfaceBuffer> buffer, InvokerType invokerType);

    GSError SetTransformHint(GraphicTransformType transformHint);
    GraphicTransformType GetTransformHint() const;
    GSError SetScalingMode(ScalingMode scalingMode);

    GSError SetSurfaceSourceType(OHSurfaceSource sourceType);
    OHSurfaceSource GetSurfaceSourceType() const;

    GSError SetSurfaceAppFrameworkType(std::string appFrameworkType);
    std::string GetSurfaceAppFrameworkType() const;

    GSError SetHdrWhitePointBrightness(float brightness);
    GSError SetSdrWhitePointBrightness(float brightness);
    float GetHdrWhitePointBrightness() const;
    float GetSdrWhitePointBrightness() const;

    GSError IsSurfaceBufferInCache(uint32_t seqNum, bool &isInCache);

    GSError AcquireLastFlushedBuffer(sptr<SurfaceBuffer> &buffer, sptr<SyncFence> &fence,
        float matrix[16], uint32_t matrixSize, bool isUseNewMatrix);
    GSError ReleaseLastFlushedBuffer(uint32_t sequence);
    GSError SetGlobalAlpha(int32_t alpha);
    GSError GetGlobalAlpha(int32_t &alpha);
    uint32_t GetAvailableBufferCount();

    void SetConnectedPid(int32_t connectedPid);
    GSError RequestAndDetachBuffer(const BufferRequestConfig& config, sptr<BufferExtraData>& bedata,
        struct IBufferProducer::RequestBufferReturnValue& retval);
    GSError AttachAndFlushBuffer(sptr<SurfaceBuffer>& buffer, sptr<BufferExtraData>& bedata,
        const sptr<SyncFence>& fence, BufferFlushConfigWithDamages& config, bool needMap);

private:
    GSError AllocBuffer(sptr<SurfaceBuffer>& buffer, const BufferRequestConfig &config,
        std::unique_lock<std::mutex> &lock);
    void DeleteBufferInCache(uint32_t sequence, std::unique_lock<std::mutex> &lock);

    uint32_t GetUsedSize();
    void DeleteBuffersLocked(int32_t count, std::unique_lock<std::mutex> &lock);

    GSError PopFromFreeListLocked(sptr<SurfaceBuffer>& buffer, const BufferRequestConfig &config);
    GSError PopFromDirtyListLocked(sptr<SurfaceBuffer>& buffer);

    GSError CheckRequestConfig(const BufferRequestConfig &config);
    GSError CheckFlushConfig(const BufferFlushConfigWithDamages &config);
    void DumpCache(std::string &result);
    void DumpMetadata(std::string &result, BufferElement element);
    void ClearLocked(std::unique_lock<std::mutex> &lock);
    bool CheckProducerCacheListLocked();
    GSError SetProducerCacheCleanFlagLocked(bool flag, std::unique_lock<std::mutex> &lock);
    GSError AttachBufferUpdateStatus(std::unique_lock<std::mutex> &lock, uint32_t sequence, int32_t timeOut);
    void AttachBufferUpdateBufferInfo(sptr<SurfaceBuffer>& buffer, bool needMap);
    void ListenerBufferReleasedCb(sptr<SurfaceBuffer> &buffer, const sptr<SyncFence> &fence);
    void OnBufferDeleteCbForHardwareThreadLocked(const sptr<SurfaceBuffer> &buffer) const;
    GSError CheckBufferQueueCache(uint32_t sequence);
    GSError ReallocBufferLocked(const BufferRequestConfig &config,
        struct IBufferProducer::RequestBufferReturnValue &retval, std::unique_lock<std::mutex> &lock);
    void SetSurfaceBufferHebcMetaLocked(sptr<SurfaceBuffer> buffer);
    GSError RequestBufferCheckStatus();
    GSError DelegatorQueueBuffer(uint32_t sequence, sptr<SyncFence> fence);
    bool WaitForCondition();
    void RequestBufferDebugInfoLocked();
    bool GetStatusLocked() const;
    void CallConsumerListener();
    void SetSurfaceBufferGlobalAlphaUnlocked(sptr<SurfaceBuffer> buffer);
    void LogAndTraceAllBufferInBufferQueueCache();
    bool IsPresentTimestampReady(int64_t desiredPresentTimestamp, int64_t expectPresentTimestamp);
    void SetDesiredPresentTimestampAndUiTimestamp(uint32_t sequence, int64_t desiredPresentTimestamp,
                                                  uint64_t uiTimestamp);
    void DropFirstDirtyBuffer(BufferElement &frontBufferElement, BufferElement &secondBufferElement,
                              int64_t &frontDesiredPresentTimestamp, bool &frontIsAutoTimestamp,
                              std::vector<BufferAndFence> &dropBuffers);
    void ReleaseDropBuffers(std::vector<BufferAndFence> &dropBuffers);
    void OnBufferDeleteForRS(uint32_t sequence);
    void DeleteBufferInCacheNoWaitForAllocatingState(uint32_t sequence);
    void AddDeletingBuffersLocked(std::vector<uint32_t> &deletingBuffers);
    GSError DetachBufferFromQueueLocked(uint32_t sequence, InvokerType invokerType, std::unique_lock<std::mutex> &lock);
    GSError AttachBufferToQueueLocked(sptr<SurfaceBuffer> buffer, InvokerType invokerType, bool needMap);
    GSError FlushBufferImprovedLocked(uint32_t sequence, sptr<BufferExtraData> &bedata,
        const sptr<SyncFence> &fence, const BufferFlushConfigWithDamages &config, std::unique_lock<std::mutex> &lock);
    GSError CheckBufferQueueCacheLocked(uint32_t sequence);
    GSError DoFlushBufferLocked(uint32_t sequence, sptr<BufferExtraData> bedata,
        sptr<SyncFence> fence, const BufferFlushConfigWithDamages &config, std::unique_lock<std::mutex> &lock);
    GSError RequestBufferLocked(const BufferRequestConfig &config, sptr<BufferExtraData> &bedata,
        struct IBufferProducer::RequestBufferReturnValue &retval, std::unique_lock<std::mutex> &lock);
    GSError CancelBufferLocked(uint32_t sequence, sptr<BufferExtraData> bedata);

    int32_t defaultWidth_ = 0;
    int32_t defaultHeight_ = 0;
    uint64_t defaultUsage_ = 0;
    uint32_t bufferQueueSize_ = SURFACE_DEFAULT_QUEUE_SIZE;
    ScalingMode scalingMode_ = ScalingMode::SCALING_MODE_SCALE_TO_WINDOW;
    GraphicTransformType transform_ = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    GraphicTransformType lastFlushedTransform_ = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    std::string name_;
    std::list<uint32_t> freeList_;
    std::list<uint32_t> dirtyList_;
    std::list<uint32_t> deletingList_;
    std::list<uint32_t> producerCacheList_;
    std::map<uint32_t, BufferElement> bufferQueueCache_;
    sptr<IBufferConsumerListener> listener_ = nullptr;
    IBufferConsumerListenerClazz *listenerClazz_ = nullptr;
    mutable std::mutex mutex_;
    std::mutex listenerMutex_;
    std::mutex producerListenerMutex_;
    const uint64_t uniqueId_;
    OnReleaseFunc onBufferRelease_ = nullptr;
    std::mutex onBufferReleaseMutex_;
    sptr<IProducerListener> producerListener_ = nullptr;
    sptr<IProducerListener> producerListenerWithFence_ = nullptr;
    OnDeleteBufferFunc onBufferDeleteForRSMainThread_;
    OnDeleteBufferFunc onBufferDeleteForRSHardwareThread_;
    std::condition_variable waitReqCon_;
    std::condition_variable waitAttachCon_;
    sptr<SurfaceTunnelHandle> tunnelHandle_ = nullptr;
    bool isValidStatus_ = true;
    bool producerCacheClean_ = false;
    const bool isLocalRender_;
    uint32_t lastFlusedSequence_ = 0;
    sptr<SyncFence> lastFlusedFence_;
    wptr<ConsumerSurfaceDelegator> wpCSurfaceDelegator_;
    bool isCpuAccessable_ = false;
    GraphicTransformType transformHint_ = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    bool isBufferHold_ = false;
    bool isBatch_ = false;
    OHSurfaceSource sourceType_ = OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
    std::string appFrameworkType_ = "";
    float hdrWhitePointBrightness_ = 0.0;
    float sdrWhitePointBrightness_ = 0.0;
    uint32_t acquireLastFlushedBufSequence_;
    int32_t globalAlpha_ = -1;
    std::mutex globalAlphaMutex_;
    std::string requestBufferStateStr_;
    std::string acquireBufferStateStr_;
    int32_t connectedPid_ = 0;
    bool isAllocatingBuffer_ = false;
    std::condition_variable isAllocatingBufferCon_;
};
}; // namespace OHOS

#endif // FRAMEWORKS_SURFACE_INCLUDE_BUFFER_QUEUE_H
