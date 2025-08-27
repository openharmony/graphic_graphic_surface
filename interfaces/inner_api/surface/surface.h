/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

    virtual GSError GetProducerInitInfo(ProducerInitInfo &info)
    {
        (void)info;
        return GSERROR_NOT_SUPPORT;
    }
    virtual bool IsConsumer() const = 0;
    virtual sptr<IBufferProducer> GetProducer() const = 0;

    virtual GSError RequestBuffer(sptr<SurfaceBuffer>& buffer,
                                  int32_t &fence, BufferRequestConfig &config)
    {
        (void)buffer;
        (void)fence;
        (void)config;
        return GSERROR_NOT_SUPPORT;
    }

    virtual GSError RequestBuffers(std::vector<sptr<SurfaceBuffer>> &buffers,
        std::vector<sptr<SyncFence>> &fences, BufferRequestConfig &config)
    {
        (void)buffers;
        (void)fences;
        (void)config;
        return GSERROR_NOT_SUPPORT;
    }

    virtual GSError CancelBuffer(sptr<SurfaceBuffer>& buffer)
    {
        (void)buffer;
        return GSERROR_NOT_SUPPORT;
    }

    virtual GSError FlushBuffer(sptr<SurfaceBuffer>& buffer,
                                int32_t fence, BufferFlushConfig &config)
    {
        (void)buffer;
        (void)fence;
        (void)config;
        return GSERROR_NOT_SUPPORT;
    }

    virtual GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, int32_t &fence,
                                  int64_t &timestamp, Rect &damage)
    {
        (void)buffer;
        (void)fence;
        (void)timestamp;
        (void)damage;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, int32_t fence)
    {
        (void)buffer;
        (void)fence;
        return GSERROR_NOT_SUPPORT;
    }

    virtual GSError RequestBuffer(sptr<SurfaceBuffer>& buffer,
                                  sptr<SyncFence>& fence, BufferRequestConfig &config)
    {
        (void)buffer;
        (void)fence;
        (void)config;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError FlushBuffer(sptr<SurfaceBuffer>& buffer,
                                const sptr<SyncFence>& fence, BufferFlushConfig &config)
    {
        (void)buffer;
        (void)fence;
        (void)config;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                  int64_t &timestamp, Rect &damage)
    {
        (void)buffer;
        (void)fence;
        (void)timestamp;
        (void)damage;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError SetLppShareFd(int fd, bool state)
    {
        (void)fd;
        (void)state;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence)
    {
        (void)buffer;
        (void)fence;
        return GSERROR_NOT_SUPPORT;
    }

    virtual GSError AttachBuffer(sptr<SurfaceBuffer>& buffer) = 0;

    virtual GSError DetachBuffer(sptr<SurfaceBuffer>& buffer) = 0;

    virtual uint32_t GetQueueSize() = 0;
    virtual GSError SetQueueSize(uint32_t queueSize) = 0;

    virtual GSError SetDefaultWidthAndHeight(int32_t width, int32_t height)
    {
        (void)width;
        (void)height;
        return GSERROR_NOT_SUPPORT;
    }
    virtual int32_t GetDefaultWidth() = 0;
    virtual int32_t GetDefaultHeight() = 0;

    virtual GSError SetDefaultUsage(uint64_t usage) = 0;
    virtual uint64_t GetDefaultUsage() = 0;

    virtual GSError SetUserData(const std::string &key, const std::string &val) = 0;
    virtual std::string GetUserData(const std::string &key) = 0;

    virtual const std::string& GetName() = 0;
    virtual uint64_t GetUniqueId() const = 0;

    virtual GSError RegisterConsumerListener(sptr<IBufferConsumerListener>& listener)
    {
        (void)listener;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError RegisterConsumerListener(IBufferConsumerListenerClazz *listener)
    {
        (void)listener;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError RegisterReleaseListener(OnReleaseFunc func)
    {
        (void)func;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError RegisterDeleteBufferListener(OnDeleteBufferFunc func, bool isForUniRedraw = false)
    {
        (void)func;
        (void)isForUniRedraw;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError UnregisterConsumerListener()
    {
        return GSERROR_NOT_SUPPORT;
    }

    // Call carefully. This interface will empty all caches of the current process
    virtual GSError CleanCache(bool cleanAll = false)
    {
        (void)cleanAll;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError GoBackground()
    {
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError SetTransform(GraphicTransformType transform)
    {
        (void)transform;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GraphicTransformType GetTransform() const
    {
        return GraphicTransformType::GRAPHIC_ROTATE_NONE;
    }
    virtual GSError Connect()
    {
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError Disconnect()
    {
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError SetScalingMode(uint32_t sequence, ScalingMode scalingMode)
    {
        (void)sequence;
        (void)scalingMode;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError GetScalingMode(uint32_t sequence, ScalingMode &scalingMode)
    {
        (void)sequence;
        (void)scalingMode;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData) = 0;
    virtual GSError SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key,
                                   const std::vector<uint8_t> &metaData) = 0;
    virtual GSError QueryMetaDataType(uint32_t sequence, HDRMetaDataType &type) const
    {
        (void)sequence;
        (void)type;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError GetMetaData(uint32_t sequence, std::vector<GraphicHDRMetaData> &metaData) const
    {
        (void)sequence;
        (void)metaData;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError GetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey &key,
                                   std::vector<uint8_t> &metaData) const
    {
        (void)sequence;
        (void)key;
        (void)metaData;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError SetTunnelHandle(const GraphicExtDataHandle *handle) = 0;
    virtual sptr<SurfaceTunnelHandle> GetTunnelHandle() const
    {
        return nullptr;
    }
    virtual GSError SetPresentTimestamp(uint32_t sequence, const GraphicPresentTimestamp &timestamp)
    {
        (void)sequence;
        (void)timestamp;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError GetPresentTimestamp(uint32_t sequence, GraphicPresentTimestampType type,
                                        int64_t &time) const
    {
        (void)sequence;
        (void)type;
        (void)time;
        return GSERROR_NOT_SUPPORT;
    }

    virtual void Dump(std::string &result) const = 0;

    virtual int32_t GetDefaultFormat()
    {
        return 0;
    }
    virtual GSError SetDefaultFormat(int32_t format)
    {
        (void)format;
        return GSERROR_NOT_SUPPORT;
    }
    virtual int32_t GetDefaultColorGamut()
    {
        return 0;
    }
    virtual GSError SetDefaultColorGamut(int32_t colorGamut)
    {
        (void)colorGamut;
        return GSERROR_NOT_SUPPORT;
    }

    virtual sptr<NativeSurface> GetNativeSurface()
    {
        return nullptr;
    }

    virtual bool QueryIfBufferAvailable()
    {
        return false;
    }
    virtual GSError FlushBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
                                BufferFlushConfigWithDamages &config, bool needLock = true)
    {
        (void)buffer;
        (void)fence;
        (void)config;
        (void)needLock;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError FlushBuffers(const std::vector<sptr<SurfaceBuffer>> &buffers,
        const std::vector<sptr<SyncFence>> &fences, const std::vector<BufferFlushConfigWithDamages> &configs)
    {
        (void)buffers;
        (void)fences;
        (void)configs;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError UnRegisterReleaseListener()
    {
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError UnRegisterReleaseListenerBackup()
    {
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError SetWptrNativeWindowToPSurface(void* nativeWindow)
    {
        (void)nativeWindow;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError GetLastFlushedBuffer(sptr<SurfaceBuffer>& buffer,
        sptr<SyncFence>& fence, float matrix[16], bool isUseNewMatrix = false)
    {
        (void)buffer;
        (void)fence;
        (void)matrix;
        (void)isUseNewMatrix;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut) = 0;
    virtual GSError RegisterSurfaceDelegator(sptr<IRemoteObject> client) = 0;
    virtual GSError RegisterReleaseListener(OnReleaseFuncWithFence func)
    {
        (void)func;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError RegisterReleaseListenerBackup(OnReleaseFuncWithFence func)
    {
        (void)func;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError RegisterUserDataChangeListener(const std::string &funcName, OnUserDataChangeFunc func)
    {
        (void)funcName;
        (void)func;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError UnRegisterUserDataChangeListener(const std::string &funcName)
    {
        (void)funcName;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError ClearUserDataChangeListener()
    {
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError AttachBufferToQueue(sptr<SurfaceBuffer> buffer) = 0;
    virtual GSError DetachBufferFromQueue(sptr<SurfaceBuffer> buffer, bool isReserveSlot = false) = 0;
    virtual GraphicTransformType GetTransformHint() const
    {
        return GraphicTransformType::GRAPHIC_ROTATE_NONE;
    }
    virtual GSError SetTransformHint(GraphicTransformType transformHint)
    {
        (void)transformHint;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError SetBufferName(const std::string &name)
    {
        (void)name;
        return GSERROR_NOT_SUPPORT;
    }

    virtual void SetRequestWidthAndHeight(int32_t width, int32_t height)
    {
        (void)width;
        (void)height;
    }
    virtual int32_t GetRequestWidth()
    {
        return 0;
    }
    virtual int32_t GetRequestHeight()
    {
        return 0;
    }

    virtual void SetBufferHold(bool hold)
    {
        (void)hold;
    }
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
    virtual GSError SetSurfaceSourceType(OHSurfaceSource sourceType)
    {
        (void)sourceType;
        return GSERROR_NOT_SUPPORT;
    }
    virtual OHSurfaceSource GetSurfaceSourceType() const
    {
        return OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
    }
    virtual GSError SetSurfaceAppFrameworkType(std::string appFrameworkType)
    {
        (void)appFrameworkType;
        return GSERROR_NOT_SUPPORT;
    }
    virtual std::string GetSurfaceAppFrameworkType() const
    {
        return std::string();
    }
    virtual GSError SetHdrWhitePointBrightness(float brightness)
    {
        (void)brightness;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError SetSdrWhitePointBrightness(float brightness)
    {
        (void)brightness;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError AcquireLastFlushedBuffer(sptr<SurfaceBuffer> &buffer, sptr<SyncFence> &fence,
        float matrix[16], uint32_t matrixSize, bool isUseNewMatrix)
    {
        (void)buffer;
        (void)fence;
        (void)matrix;
        (void)matrixSize;
        (void)isUseNewMatrix;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError ReleaseLastFlushedBuffer(sptr<SurfaceBuffer> buffer)
    {
        (void)buffer;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError SetGlobalAlpha(int32_t alpha)
    {
        (void)alpha;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError SetRequestBufferNoblockMode(bool noblock)
    {
        (void)noblock;
        return GSERROR_NOT_SUPPORT;
    }
    virtual bool IsInHebcList()
    {
        return false;
    }
    /**
     * @brief Merge RequestBuffer and DetachBufferFromQueue function to reduce once ipc.
     */
    virtual GSError RequestAndDetachBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                           BufferRequestConfig& config)
    {
        (void)buffer;
        (void)fence;
        (void)config;
        return GSERROR_NOT_SUPPORT;
    }
    
    /**
     * @brief Merge AttachBufferToQueue And FlushBuffer function to reduce once ipc.
     */
    virtual GSError AttachAndFlushBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
                                         BufferFlushConfig& config, bool needMap)
    {
        (void)buffer;
        (void)fence;
        (void)config;
        (void)needMap;
        return GSERROR_NOT_SUPPORT;
    }
    /**
     * @brief Avoidance plan, which can be deleted later.
     */
    virtual GSError GetBufferCacheConfig(const sptr<SurfaceBuffer>& buffer, BufferRequestConfig& config)
    {
        (void)buffer;
        (void)config;
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError GetCycleBuffersNumber(uint32_t& cycleBuffersNumber)
    {
        (void)cycleBuffersNumber;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    /**
     * @brief Set the Rotating Buffers Number object for hdi create layer max cache count.
     * @param cycleBuffersNumber scope : (0, 2 * maxQueueSize], and it should be actual number of cycle buffers.
     */
    virtual GSError SetCycleBuffersNumber(uint32_t cycleBuffersNumber)
    {
        (void)cycleBuffersNumber;
        return SURFACE_ERROR_NOT_SUPPORT;
    }

    virtual GSError GetFrameGravity(int32_t &frameGravity)
    {
        (void)frameGravity;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    /**
     * @brief Set the frame gravity.
     * @param frameGravity -1 for invalid setting. 0 to 15 is normal value.
     */
    virtual GSError SetFrameGravity(int32_t frameGravity)
    {
        (void)frameGravity;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError GetFixedRotation(int32_t &fixedRotation)
    {
        (void)fixedRotation;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    /**
     * @brief Set the fixed rotation value for whether the rotation is fixed.
     * @param fixedRotation -1 for invalid setting. 1 for fixed or 0 for otherwise.
     */
    virtual GSError SetFixedRotation(int32_t fixedRotation)
    {
        (void)fixedRotation;
        return SURFACE_ERROR_NOT_SUPPORT;
    }

    /**
    * @brief In the strictly disconnected state, the producer must call the ConnectStrictly() interface before request
    *        buffer. Unlike Connect(), ConnectStrictly() does not distinguish between process IDs (PIDs) and is
    *        suitable for stricter connection management scenarios.
    */
    virtual GSError ConnectStrictly()
    {
        return SURFACE_ERROR_NOT_SUPPORT;
    }

    /**
    * @brief After calling DisconnectStrictly(), the consumer (server) enter the strictly disconnected state.
    *        In this state, any attempt by the producer (client) to request buffer will fail and return the error code
    *        GSERROR_CONSUMER_DISCONNECTED.
    */
    virtual GSError DisconnectStrictly()
    {
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError PreAllocBuffers(const BufferRequestConfig &config, uint32_t allocBufferCount)
    {
        (void)config;
        (void)allocBufferCount;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    /**
    * @brief Request a buffer with lock.
    * @param config Indicates the buffer config to be requested.
    * @param region Indicates the info of the dirty region.
    * @param buffer Indicates the pointer to a <b>SurfaceBuffer</b> instance.
    * @return Returns the error code of the request of lock.
    */
    virtual GSError ProducerSurfaceLockBuffer(BufferRequestConfig &config, Region region, sptr<SurfaceBuffer>& buffer)
    {
        (void)config;
        (void)region;
        (void)buffer;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    /**
    * @brief Unlock a buffer with lock.
    * @return Returns the error code of the request of unlock.
    */
    virtual GSError ProducerSurfaceUnlockAndFlushBuffer()
    {
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError GetAlphaType(GraphicAlphaType &alphaType)
    {
        (void)alphaType;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError SetAlphaType(GraphicAlphaType alphaType)
    {
        (void)alphaType;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
    virtual GSError SetBufferReallocFlag(bool flag)
    {
        (void)flag;
        return GSERROR_NOT_SUPPORT;
    }
    /**
    * @brief Set a buffer type leak.
    * @param bufferTypeLeak Indicates the bufferTypeLeak to be set.
    * @return Returns the error code of the set of bufferTypeLeak.
    */
    virtual GSError SetBufferTypeLeak(const std::string &bufferTypeLeak)
    {
        (void)bufferTypeLeak;
        return GSERROR_NOT_SUPPORT;
    }
    /**
     * @brief Get the Available Buffer Count from the surface.
     * @param count [out] The Available Buffer Count of the surface.
     * @return {@link GSERROR_OK} 0 - Success.
     *         {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     *         {@link SURFACE_ERROR_NOT_SUPPORT} 50102000 - Not surport usage.
     */
    virtual GSError GetAvailableBufferCount(uint32_t &count)
    {
        (void)count;
        return SURFACE_ERROR_NOT_SUPPORT;
    }
protected:
    Surface() = default;
};
} // namespace OHOS

#endif // INTERFACES_INNERKITS_SURFACE_SURFACE_H
