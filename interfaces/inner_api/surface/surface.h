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

#ifndef INTERFACES_INNERKITS_SURFACE_SURFACE_H
#define INTERFACES_INNERKITS_SURFACE_SURFACE_H

#include <refbase.h>

#include "ibuffer_consumer_listener.h"
#include "ibuffer_producer.h"
#include "surface_buffer.h"
#include "surface_type.h"
#include "surface_tunnel_handle.h"

namespace OHOS {
class Surface : public RefBase {
public:
    static sptr<Surface> CreateSurfaceAsConsumer(std::string name = "noname");
    static sptr<Surface> CreateSurfaceAsProducer(sptr<IBufferProducer>& producer);

    virtual ~Surface() = default;

    virtual GSError GetProducerInitInfo(ProducerInitInfo &info) = 0;
    virtual bool IsConsumer() const = 0;
    virtual sptr<IBufferProducer> GetProducer() const = 0;

    virtual GSError RequestBuffer(sptr<SurfaceBuffer>& buffer,
                                  int32_t &fence, BufferRequestConfig &config) = 0;

    virtual GSError RequestBuffers(std::vector<sptr<SurfaceBuffer>> &buffers,
        std::vector<sptr<SyncFence>> &fences, BufferRequestConfig &config) = 0;

    virtual GSError CancelBuffer(sptr<SurfaceBuffer>& buffer) = 0;

    virtual GSError FlushBuffer(sptr<SurfaceBuffer>& buffer,
                                int32_t fence, BufferFlushConfig &config) = 0;

    virtual GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, int32_t &fence,
                                  int64_t &timestamp, Rect &damage) = 0;
    virtual GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, int32_t fence) = 0;

    virtual GSError RequestBuffer(sptr<SurfaceBuffer>& buffer,
                                  sptr<SyncFence>& fence, BufferRequestConfig &config) = 0;
    virtual GSError FlushBuffer(sptr<SurfaceBuffer>& buffer,
                                const sptr<SyncFence>& fence, BufferFlushConfig &config) = 0;
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
    virtual GSError RegisterReleaseListener(OnReleaseFunc func) = 0;
    virtual GSError RegisterDeleteBufferListener(OnDeleteBufferFunc func, bool isForUniRedraw = false) = 0;
    virtual GSError UnregisterConsumerListener() = 0;

    // Call carefully. This interface will empty all caches of the current process
    virtual GSError CleanCache(bool cleanAll = false) = 0;
    virtual GSError GoBackground() = 0;

    virtual GSError SetTransform(GraphicTransformType transform) = 0;
    virtual GraphicTransformType GetTransform() const = 0;

    virtual GSError Connect() = 0;
    virtual GSError Disconnect() = 0;
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
    virtual GSError GetPresentTimestamp(uint32_t sequence, GraphicPresentTimestampType type,
                                        int64_t &time) const = 0;

    virtual void Dump(std::string &result) const = 0;

    virtual int32_t GetDefaultFormat() = 0;
    virtual GSError SetDefaultFormat(int32_t format) = 0;
    virtual int32_t GetDefaultColorGamut() = 0;
    virtual GSError SetDefaultColorGamut(int32_t colorGamut) = 0;

    virtual sptr<NativeSurface> GetNativeSurface() = 0;

    virtual bool QueryIfBufferAvailable() = 0;
    virtual GSError FlushBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
                                BufferFlushConfigWithDamages &config) = 0;
    virtual GSError FlushBuffers(const std::vector<sptr<SurfaceBuffer>> &buffers,
        const std::vector<sptr<SyncFence>> &fences, const std::vector<BufferFlushConfigWithDamages> &configs) = 0;
    virtual GSError UnRegisterReleaseListener() = 0;
    virtual GSError SetWptrNativeWindowToPSurface(void* nativeWindow) = 0;
    virtual GSError GetLastFlushedBuffer(sptr<SurfaceBuffer>& buffer,
        sptr<SyncFence>& fence, float matrix[16], bool isUseNewMatrix = false) = 0;
    virtual GSError AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut) = 0;
    virtual GSError RegisterSurfaceDelegator(sptr<IRemoteObject> client) = 0;
    virtual GSError RegisterReleaseListener(OnReleaseFuncWithFence func) = 0;
    virtual GSError RegisterUserDataChangeListener(const std::string &funcName, OnUserDataChangeFunc func) = 0;
    virtual GSError UnRegisterUserDataChangeListener(const std::string &funcName) = 0;
    virtual GSError ClearUserDataChangeListener() = 0;

    virtual GSError AttachBufferToQueue(sptr<SurfaceBuffer> buffer) = 0;
    virtual GSError DetachBufferFromQueue(sptr<SurfaceBuffer> buffer) = 0;

    virtual GraphicTransformType GetTransformHint() const = 0;
    virtual GSError SetTransformHint(GraphicTransformType transformHint) = 0;

    virtual void SetRequestWidthAndHeight(int32_t width, int32_t height) = 0;
    virtual int32_t GetRequestWidth() = 0;
    virtual int32_t GetRequestHeight() = 0;

    virtual void SetBufferHold(bool hold) = 0;
    virtual void SetWindowConfig(const BufferRequestConfig& config)
    {
        (void)config;
    }
    virtual void SetWindowConfigWidthAndHeight(int32_t width, int32_t height)
    {
        (void)width;
        (void)height;
    }
    virtual void SetWindowConfigStride(int32_t stride)
    {
        (void)stride;
    }
    virtual void SetWindowConfigFormat(int32_t format)
    {
        (void)format;
    }
    virtual void SetWindowConfigUsage(uint64_t usage)
    {
        (void)usage;
    }
    virtual void SetWindowConfigTimeout(int32_t timeout)
    {
        (void)timeout;
    }
    virtual void SetWindowConfigColorGamut(GraphicColorGamut colorGamut)
    {
        (void)colorGamut;
    }
    virtual void SetWindowConfigTransform(GraphicTransformType transform)
    {
        (void)transform;
    }
    virtual BufferRequestConfig GetWindowConfig()
    {
        BufferRequestConfig config;
        return config;
    }
    virtual GSError SetScalingMode(ScalingMode scalingMode) = 0;
    virtual GSError SetSurfaceSourceType(OHSurfaceSource sourceType) = 0;
    virtual OHSurfaceSource GetSurfaceSourceType() const = 0;
    virtual GSError SetSurfaceAppFrameworkType(std::string appFrameworkType) = 0;
    virtual std::string GetSurfaceAppFrameworkType() const = 0;
    virtual GSError SetHdrWhitePointBrightness(float brightness) = 0;
    virtual GSError SetSdrWhitePointBrightness(float brightness) = 0;
    virtual GSError AcquireLastFlushedBuffer(sptr<SurfaceBuffer> &buffer, sptr<SyncFence> &fence,
        float matrix[16], uint32_t matrixSize, bool isUseNewMatrix)
    {
        (void)buffer;
        (void)fence;
        (void)matrix;
        (void)matrixSize;
        (void)isUseNewMatrix;
        return SURFACE_ERROR_NOT_SUPPORT;
    };
    virtual GSError ReleaseLastFlushedBuffer(sptr<SurfaceBuffer> buffer)
    {
        (void)buffer;
        return SURFACE_ERROR_NOT_SUPPORT;
    };
    virtual GSError SetGlobalAlpha(int32_t alpha) = 0;
    virtual bool IsInHebcList()
    {
        return false;
    }
protected:
    Surface() = default;
};
} // namespace OHOS

#endif // INTERFACES_INNERKITS_SURFACE_SURFACE_H
