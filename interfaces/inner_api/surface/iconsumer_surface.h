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
    };
    static sptr<IConsumerSurface> Create(std::string name = "noname");

    virtual ~IConsumerSurface() = default;

    virtual bool IsConsumer() const = 0;
    virtual sptr<IBufferProducer> GetProducer() const = 0;

    virtual GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, int32_t &fence,
                                  int64_t &timestamp, Rect &damage) = 0;
    virtual GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, int32_t fence) = 0;

    virtual GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                  int64_t &timestamp, Rect &damage) = 0;
    virtual GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence) = 0;

    virtual GSError AttachBuffer(sptr<SurfaceBuffer>& buffer) = 0;

    virtual GSError DetachBuffer(sptr<SurfaceBuffer>& buffer) = 0;

    virtual uint32_t GetQueueSize() = 0;
    virtual GSError SetQueueSize(uint32_t queueSize) = 0;

    virtual GSError SetDefaultWidthAndHeight(int32_t width, int32_t height) = 0;
    virtual int32_t GetDefaultWidth() = 0;
    virtual int32_t GetDefaultHeight() = 0;

    virtual GSError SetDefaultUsage(uint64_t usage) = 0;
    virtual uint64_t GetDefaultUsage() = 0;

    virtual GSError SetUserData(const std::string &key, const std::string &val) = 0;
    virtual std::string GetUserData(const std::string &key) = 0;

    virtual const std::string& GetName() = 0;
    virtual uint64_t GetUniqueId() const = 0;

    virtual GSError RegisterConsumerListener(sptr<IBufferConsumerListener>& listener) = 0;
    virtual GSError RegisterConsumerListener(IBufferConsumerListenerClazz *listener) = 0;
    virtual GSError UnregisterConsumerListener() = 0;

    virtual GSError GoBackground() = 0;

    virtual GSError SetTransform(GraphicTransformType transform) = 0;
    virtual GraphicTransformType GetTransform() const = 0;

    virtual GSError SetScalingMode(uint32_t sequence, ScalingMode scalingMode) = 0;
    virtual GSError GetScalingMode(uint32_t sequence, ScalingMode &scalingMode) = 0;
    virtual GSError SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData) = 0;
    virtual GSError SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key,
                                   const std::vector<uint8_t> &metaData) = 0;
    virtual GSError QueryMetaDataType(uint32_t sequence, HDRMetaDataType &type) const = 0;
    virtual GSError GetMetaData(uint32_t sequence, std::vector<GraphicHDRMetaData> &metaData) const = 0;
    virtual GSError GetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey &key,
                                   std::vector<uint8_t> &metaData) const = 0;
    virtual GSError SetTunnelHandle(const GraphicExtDataHandle *handle) = 0;
    virtual sptr<SurfaceTunnelHandle> GetTunnelHandle() const = 0;
    virtual GSError SetPresentTimestamp(uint32_t sequence, const GraphicPresentTimestamp &timestamp) = 0;
    virtual void Dump(std::string &result) const = 0;
    virtual void DumpCurrentFrameLayer() const = 0;

    virtual GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                  int64_t &timestamp, std::vector<Rect> &damages) = 0;
    virtual GSError AcquireBuffer(AcquireBufferReturnValue &returnValue, int64_t expectPresentTimestamp,
                                  bool isUsingAutoTimestamp)
    {
        (void)returnValue;
        (void)expectPresentTimestamp;
        (void)isUsingAutoTimestamp;
        return SURFACE_ERROR_NOT_SUPPORT;
    };
    virtual GSError AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut) = 0;
    virtual GSError RegisterSurfaceDelegator(sptr<IRemoteObject> client) = 0;
    virtual void ConsumerRequestCpuAccess(bool on) = 0;
    virtual GSError AttachBufferToQueue(sptr<SurfaceBuffer> buffer) = 0;
    virtual GSError DetachBufferFromQueue(sptr<SurfaceBuffer> buffer, bool isReserveSlot = false) = 0;
    virtual bool IsBufferHold() = 0;
    virtual GSError SetScalingMode(ScalingMode scalingMode) = 0;
    virtual float GetHdrWhitePointBrightness() const = 0;
    virtual float GetSdrWhitePointBrightness() const = 0;

    virtual GSError GetSurfaceBufferTransformType(sptr<SurfaceBuffer> buffer, GraphicTransformType *transformType) = 0;

    virtual GSError IsSurfaceBufferInCache(uint32_t seqNum, bool &isInCache) = 0;
    virtual GSError GetGlobalAlpha(int32_t &alpha)
    {
        (void)alpha;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual uint32_t GetAvailableBufferCount() const
    {
        return SURFACE_ERROR_NOT_SUPPORT;
    };
    virtual GSError GetLastFlushedDesiredPresentTimeStamp(
        [[maybe_unused]] int64_t &lastFlushedDesiredPresentTimeStamp) const
    {
        return SURFACE_ERROR_NOT_SUPPORT;
    };
    virtual GSError GetBufferSupportFastCompose([[maybe_unused]] bool &bufferSupportFastCompose)
    {
        return SURFACE_ERROR_NOT_SUPPORT;
    };
    virtual GSError SetBufferName(const std::string &name)
    {
        (void)name;
        return GSERROR_NOT_SUPPORT;
    }
protected:
    IConsumerSurface() = default;
};
} // namespace OHOS

#endif // INTERFACES_INNERKITS_SURFACE_SURFACE_H
