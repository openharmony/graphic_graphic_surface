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

#ifndef INTERFACES_INNERKITS_SURFACE_ICONSUMER_SURFACE_H
#define INTERFACES_INNERKITS_SURFACE_ICONSUMER_SURFACE_H

#include <refbase.h>

#include "ibuffer_consumer_listener.h"
#include "ibuffer_producer.h"
#include "surface.h"
#include "surface_buffer.h"
#include "surface_type.h"
#include "surface_tunnel_handle.h"

namespace OHOS {
class IConsumerSurface : public Surface {
public:
    using AcquireBufferReturnValue = struct {
        sptr<SurfaceBuffer> buffer;
        sptr<SyncFence> fence;
        int64_t timestamp;
        std::vector<Rect> damages;
        int64_t desiredPresentTimestamp;
        int64_t requestTimeNs;
        int64_t flushTimeNs;
        bool isAutoTimestamp;
    };

    static sptr<IConsumerSurface> Create(std::string name = "noname");

    virtual ~IConsumerSurface() = default;

    virtual bool IsConsumer() const
    {
        return true;
    }
    virtual sptr<IBufferProducer> GetProducer() const
    {
        return nullptr;
    }
    virtual GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, int32_t &fence,
                                  int64_t &timestamp, Rect &damage)
    {
        (void)buffer;
        (void)fence;
        (void)timestamp;
        (void)damage;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, int32_t fence)
    {
        (void)buffer;
        (void)fence;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                  int64_t &timestamp, Rect &damage)
    {
        (void)buffer;
        (void)fence;
        (void)timestamp;
        (void)damage;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence)
    {
        (void)buffer;
        (void)fence;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError AttachBuffer(sptr<SurfaceBuffer>& buffer)
    {
        (void)buffer;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError DetachBuffer(sptr<SurfaceBuffer>& buffer)
    {
        (void)buffer;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual uint32_t GetQueueSize()
    {
        return 0;
    }
    virtual GSError SetQueueSize(uint32_t queueSize)
    {
        (void)queueSize;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError SetDefaultWidthAndHeight(int32_t width, int32_t height)
    {
        (void)width;
        (void)height;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual int32_t GetDefaultWidth()
    {
        return 0;
    }
    virtual int32_t GetDefaultHeight()
    {
        return 0;
    }
    virtual GSError SetDefaultUsage(uint64_t usage)
    {
        (void)usage;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual uint64_t GetDefaultUsage()
    {
        return 0;
    }
    virtual GSError SetUserData(const std::string &key, const std::string &val)
    {
        (void)key;
        (void)val;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual std::string GetUserData(const std::string &key)
    {
        (void)key;
        return "";
    }
    virtual const std::string& GetName() = 0;
    virtual uint64_t GetUniqueId() const
    {
        return 0;
    }
    virtual GSError RegisterConsumerListener(sptr<IBufferConsumerListener>& listener)
    {
        (void)listener;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError RegisterConsumerListener(IBufferConsumerListenerClazz *listener)
    {
        (void)listener;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError UnregisterConsumerListener()
    {
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError GoBackground()
    {
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError SetTransform(GraphicTransformType transform)
    {
        (void)transform;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GraphicTransformType GetTransform() const
    {
        return GraphicTransformType::GRAPHIC_ROTATE_NONE;
    }
    virtual GSError SetScalingMode(uint32_t sequence, ScalingMode scalingMode)
    {
        (void)sequence;
        (void)scalingMode;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError GetScalingMode(uint32_t sequence, ScalingMode &scalingMode)
    {
        (void)sequence;
        (void)scalingMode;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData)
    {
        (void)sequence;
        (void)metaData;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key,
                                   const std::vector<uint8_t> &metaData)
    {
        (void)sequence;
        (void)key;
        (void)metaData;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError QueryMetaDataType(uint32_t sequence, HDRMetaDataType &type) const
    {
        (void)sequence;
        (void)type;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError GetMetaData(uint32_t sequence, std::vector<GraphicHDRMetaData> &metaData) const
    {
        (void)sequence;
        (void)metaData;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError GetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey &key,
                                   std::vector<uint8_t> &metaData) const
    {
        (void)sequence;
        (void)key;
        (void)metaData;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError SetTunnelHandle(const GraphicExtDataHandle *handle)
    {
        (void)handle;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual sptr<SurfaceTunnelHandle> GetTunnelHandle() const
    {
        return nullptr;
    }
    virtual GSError SetPresentTimestamp(uint32_t sequence, const GraphicPresentTimestamp &timestamp)
    {
        (void)sequence;
        (void)timestamp;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual void Dump(std::string &result) const
    {
        (void)result;
        return;
    }
    virtual void DumpCurrentFrameLayer() const
    {
        return;
    }
    virtual GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                  int64_t &timestamp, std::vector<Rect> &damages, bool isLppMode = false)
    {
        (void)buffer;
        (void)fence;
        (void)timestamp;
        (void)damages;
        (void)isLppMode;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError AcquireBuffer(AcquireBufferReturnValue &returnValue, int64_t expectPresentTimestamp,
                                  bool isUsingAutoTimestamp)
    {
        (void)returnValue;
        (void)expectPresentTimestamp;
        (void)isUsingAutoTimestamp;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut)
    {
        (void)buffer;
        (void)timeOut;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError RegisterSurfaceDelegator(sptr<IRemoteObject> client)
    {
        (void)client;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual void ConsumerRequestCpuAccess(bool on)
    {
        (void)on;
    }
    virtual GSError AttachBufferToQueue(sptr<SurfaceBuffer> buffer)
    {
        (void)buffer;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError DetachBufferFromQueue(sptr<SurfaceBuffer> buffer, bool isReserveSlot = false)
    {
        (void)buffer;
        (void)isReserveSlot;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual bool IsBufferHold()
    {
        return false;
    }
    virtual GSError SetScalingMode(ScalingMode scalingMode)
    {
        (void)scalingMode;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual float GetHdrWhitePointBrightness() const
    {
        return 0;
    }
    virtual float GetSdrWhitePointBrightness() const
    {
        return 0;
    }
    virtual GSError GetSurfaceBufferTransformType(sptr<SurfaceBuffer> buffer, GraphicTransformType *transformType)
    {
        (void)buffer;
        (void)transformType;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError IsSurfaceBufferInCache(uint32_t seqNum, bool &isInCache)
    {
        (void)seqNum;
        (void)isInCache;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError GetGlobalAlpha(int32_t &alpha)
    {
        (void)alpha;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual uint32_t GetAvailableBufferCount() const
    {
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError GetLastFlushedDesiredPresentTimeStamp(
        [[maybe_unused]] int64_t &lastFlushedDesiredPresentTimeStamp) const
    {
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError GetBufferSupportFastCompose([[maybe_unused]] bool &bufferSupportFastCompose)
    {
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError SetBufferName(const std::string &name)
    {
        (void)name;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError PreAllocBuffers(const BufferRequestConfig &config, uint32_t allocBufferCount)
    {
        (void)config;
        (void)allocBufferCount;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError GetLastConsumeTime(int64_t &lastConsumeTime) const
    {
        (void)lastConsumeTime;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError SetMaxQueueSize(uint32_t queueSize)
    {
        (void)queueSize;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError GetMaxQueueSize(uint32_t &queueSize) const
    {
        (void)queueSize;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError GetFrontDesiredPresentTimeStamp(
        int64_t &desiredPresentTimeStamp, bool &isAutoTimeStamp) const
    {
        (void)desiredPresentTimeStamp;
        (void)isAutoTimeStamp;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError AcquireBuffer(AcquireBufferReturnValue &returnValue)
    {
        (void)returnValue;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError ReleaseBuffer(uint32_t sequence, const sptr<SyncFence>& fence)
    {
        (void)sequence;
        (void)fence;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError SetIsActiveGame(bool isActiveGame)
    {
        (void)isActiveGame;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError SetLppDrawSource(bool isShbSource, bool isRsSource)
    {
        (void)isShbSource;
        (void)isRsSource;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError SetDropBufferMode(bool enableDrop)
    {
        (void)enableDrop;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError SetIsPriorityAlloc(bool isPriorityAlloc)
    {
        (void)isPriorityAlloc;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
protected:
    IConsumerSurface() = default;
};
} // namespace OHOS

#endif // INTERFACES_INNERKITS_SURFACE_SURFACE_H
