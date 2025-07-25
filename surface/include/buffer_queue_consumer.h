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

#ifndef FRAMEWORKS_SURFACE_INCLUDE_BUFFER_QUEUE_CONSUMER_H
#define FRAMEWORKS_SURFACE_INCLUDE_BUFFER_QUEUE_CONSUMER_H

#include <refbase.h>

#include "surface_type.h"
#include "surface_buffer.h"
#include "buffer_queue.h"

namespace OHOS {
class SURFACE_HIDDEN BufferQueueConsumer : public RefBase {
public:
    BufferQueueConsumer(sptr<BufferQueue>& bufferQueue);
    virtual ~BufferQueueConsumer();

    GSError AcquireBuffer(sptr<SurfaceBuffer> &buffer, sptr<SyncFence> &fence,
                               int64_t &timestamp, std::vector<Rect> &damages, bool isLppMode = false);
    GSError AcquireBuffer(IConsumerSurface::AcquireBufferReturnValue &returnValue, int64_t expectPresentTimestamp,
                          bool isUsingAutoTimestamp);
    GSError AcquireBuffer(IConsumerSurface::AcquireBufferReturnValue &returnValue);
    GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence);

    GSError AttachBuffer(sptr<SurfaceBuffer>& buffer);
    GSError AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut);

    GSError DetachBuffer(sptr<SurfaceBuffer>& buffer);
    GSError RegisterSurfaceDelegator(sptr<IRemoteObject> client, sptr<Surface> cSurface);

    bool QueryIfBufferAvailable();

    GSError RegisterConsumerListener(sptr<IBufferConsumerListener>& listener);
    GSError RegisterConsumerListener(IBufferConsumerListenerClazz *listener);
    GSError RegisterReleaseListener(OnReleaseFunc func);
    GSError RegisterDeleteBufferListener(OnDeleteBufferFunc func, bool isForUniRedraw = false);
    GSError UnregisterConsumerListener();

    GSError SetDefaultWidthAndHeight(int32_t width, int32_t height);
    GSError SetDefaultUsage(uint64_t usage);
    void Dump(std::string &result) const;
    void DumpCurrentFrameLayer() const;
    GraphicTransformType GetTransform() const;
    GSError GetScalingMode(uint32_t sequence, ScalingMode &scalingMode) const;
    GSError QueryMetaDataType(uint32_t sequence, HDRMetaDataType &type) const;
    GSError GetMetaData(uint32_t sequence, std::vector<GraphicHDRMetaData> &metaData) const;
    GSError GetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey &key, std::vector<uint8_t> &metaData) const;
    GSError GetGlobalAlpha(int32_t &alpha);
    sptr<SurfaceTunnelHandle> GetTunnelHandle() const;
    GSError SetPresentTimestamp(uint32_t sequence, const GraphicPresentTimestamp &timestamp);

    bool GetStatus() const;
    void SetStatus(bool status);
    GSError OnConsumerDied();
    GSError GoBackground();
    void ConsumerRequestCpuAccess(bool on)
    {
        bufferQueue_->ConsumerRequestCpuAccess(on);
    }

    GSError AttachBufferToQueue(sptr<SurfaceBuffer> buffer);
    GSError DetachBufferFromQueue(sptr<SurfaceBuffer> buffer, bool isReserveSlot);
    void SetBufferHold(bool hold);
    inline bool IsBufferHold()
    {
        if (bufferQueue_ == nullptr) {
            return false;
        }
        return bufferQueue_->IsBufferHold();
    }
    float GetHdrWhitePointBrightness() const;
    float GetSdrWhitePointBrightness() const;
    GSError IsSurfaceBufferInCache(uint32_t seqNum, bool &isInCache);
    uint32_t GetAvailableBufferCount() const;
    GSError GetLastFlushedDesiredPresentTimeStamp(int64_t &lastFlushedDesiredPresentTimeStamp) const;
    GSError GetFrontDesiredPresentTimeStamp(int64_t &desiredPresentTimeStamp, bool &isAutoTimeStamp) const;
    GSError GetBufferSupportFastCompose(bool &bufferSupportFastCompose) const;
    GSError GetBufferCacheConfig(const sptr<SurfaceBuffer>& buffer, BufferRequestConfig& config);
    GSError GetCycleBuffersNumber(uint32_t& cycleBuffersNumber);
    GSError GetFrameGravity(int32_t &frameGravity);
    GSError GetFixedRotation(int32_t &fixedRotation);
    GSError GetLastConsumeTime(int64_t &lastConsumeTime) const;
    GSError SetMaxQueueSize(uint32_t queueSize);
    GSError GetMaxQueueSize(uint32_t &queueSize) const;
    GSError ReleaseBuffer(uint32_t sequence, const sptr<SyncFence> &fence);
    GSError SetIsActiveGame(bool isActiveGame);
    GSError SetLppDrawSource(bool isShbSource, bool isRsSource);
    GSError SetDropBufferMode(bool enableDrop);
    GSError GetAlphaType(GraphicAlphaType &alphaType);
    GSError SetIsPriorityAlloc(bool isPriorityAlloc);
private:
    sptr<BufferQueue> bufferQueue_ = nullptr;
    std::string name_ = "not init";
};
} // namespace OHOS

#endif // FRAMEWORKS_SURFACE_INCLUDE_BUFFER_QUEUE_CONSUMER_H
