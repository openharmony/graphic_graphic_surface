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

#ifndef FRAMEWORKS_SURFACE_INCLUDE_PRODUCER_SURFACE_H
#define FRAMEWORKS_SURFACE_INCLUDE_PRODUCER_SURFACE_H

#include <atomic>
#include <map>
#include <string>
#include <vector>

#include <surface.h>
#include <ibuffer_producer.h>

#include "buffer_queue.h"
#include "buffer_queue_consumer.h"
#include "surface_buffer.h"
#include "producer_surface_delegator.h"

struct NativeWindow;
namespace OHOS {
class ProducerSurface : public Surface {
public:
    ProducerSurface(sptr<IBufferProducer>& producer);
    virtual ~ProducerSurface();

    // thread unsafe
    GSError Init();

    bool IsConsumer() const override;
    sptr<IBufferProducer> GetProducer() const override;
    GSError RequestBuffer(sptr<SurfaceBuffer>& buffer,
                          int32_t &fence, BufferRequestConfig &config) override;

    GSError RequestBuffers(std::vector<sptr<SurfaceBuffer>> &buffers,
        std::vector<sptr<SyncFence>> &fences, BufferRequestConfig &config) override;

    GSError CancelBuffer(sptr<SurfaceBuffer>& buffer) override;

    GSError FlushBuffer(sptr<SurfaceBuffer>& buffer,
                        int32_t fence, BufferFlushConfig &config) override;

    GSError FlushBuffers(const std::vector<sptr<SurfaceBuffer>> &buffers,
        const std::vector<sptr<SyncFence>> &fences, const std::vector<BufferFlushConfigWithDamages> &config) override;

    GSError RequestBuffer(sptr<SurfaceBuffer>& buffer,
                          sptr<SyncFence>& fence, BufferRequestConfig &config) override;
    GSError FlushBuffer(sptr<SurfaceBuffer>& buffer,
                        const sptr<SyncFence>& fence, BufferFlushConfig &config) override;
    GSError GetLastFlushedBuffer(sptr<SurfaceBuffer>& buffer,
        sptr<SyncFence>& fence, float matrix[16], bool isUseNewMatrix = false) override;
    GSError FlushBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
                        BufferFlushConfigWithDamages &config) override;

    GSError AttachBuffer(sptr<SurfaceBuffer>& buffer) override;
    GSError AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut) override;

    GSError DetachBuffer(sptr<SurfaceBuffer>& buffer) override;

    uint32_t GetQueueSize() override;
    GSError SetQueueSize(uint32_t queueSize) override;

    const std::string& GetName() override;
    uint64_t GetUniqueId() const override;
    int32_t GetDefaultWidth() override;
    int32_t GetDefaultHeight() override;
    GSError SetDefaultUsage(uint64_t usage) override;
    uint64_t GetDefaultUsage() override;
    void SetBufferHold(bool hold) override;

    GSError SetUserData(const std::string &key, const std::string &val) override;
    std::string GetUserData(const std::string &key) override;

    GSError RegisterReleaseListener(OnReleaseFunc func) override;
    GSError RegisterReleaseListener(OnReleaseFuncWithFence func) override;
    GSError RegisterReleaseListenerWithFence(OnReleaseFuncWithFence func) override;
    GSError UnRegisterReleaseListener() override;
    GSError UnRegisterReleaseListenerWithFence() override;

    void Dump(std::string &result) const override {};

    // Call carefully. This interface will empty all caches of the current process
    GSError CleanCache(bool cleanAll = false) override;
    GSError GoBackground() override;

    GSError SetTransform(GraphicTransformType transform) override;
    GraphicTransformType GetTransform() const override;

    GSError Connect() override;
    GSError Disconnect() override;
    GSError SetScalingMode(uint32_t sequence, ScalingMode scalingMode) override;
    GSError SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData) override;
    GSError SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key, const std::vector<uint8_t> &metaData) override;
    GSError SetTunnelHandle(const GraphicExtDataHandle *handle) override;
    GSError GetPresentTimestamp(uint32_t sequence, GraphicPresentTimestampType type, int64_t &time) const override;
    GSError SetWptrNativeWindowToPSurface(void* nativeWindow) override;
    virtual GSError RegisterSurfaceDelegator(sptr<IRemoteObject> client) override;
    GSError RegisterUserDataChangeListener(const std::string &funcName, OnUserDataChangeFunc func) override;
    GSError UnRegisterUserDataChangeListener(const std::string &funcName) override;
    GSError ClearUserDataChangeListener() override;
    GSError AttachBufferToQueue(sptr<SurfaceBuffer> buffer) override;
    GSError DetachBufferFromQueue(sptr<SurfaceBuffer> buffer) override;
    GraphicTransformType GetTransformHint() const override;
    GSError SetTransformHint(GraphicTransformType transformHint) override;
    GSError SetBufferName(const std::string &name) override;

    void SetRequestWidthAndHeight(int32_t width, int32_t height) override;
    int32_t GetRequestWidth() override;
    int32_t GetRequestHeight() override;
    GSError SetScalingMode(ScalingMode scalingMode) override;
    GSError SetSurfaceSourceType(OHSurfaceSource sourceType) override;
    OHSurfaceSource GetSurfaceSourceType() const override;
    GSError SetSurfaceAppFrameworkType(std::string appFrameworkType) override;
    std::string GetSurfaceAppFrameworkType() const override;

    void SetWindowConfig(const BufferRequestConfig& config) override;
    void SetWindowConfigWidthAndHeight(int32_t width, int32_t height) override;
    void SetWindowConfigStride(int32_t stride) override;
    void SetWindowConfigFormat(int32_t format) override;
    void SetWindowConfigUsage(uint64_t usage) override;
    void SetWindowConfigTimeout(int32_t timeout) override;
    void SetWindowConfigColorGamut(GraphicColorGamut colorGamut) override;
    void SetWindowConfigTransform(GraphicTransformType transform) override;
    BufferRequestConfig GetWindowConfig() override;
    GSError SetHdrWhitePointBrightness(float brightness) override;
    GSError SetSdrWhitePointBrightness(float brightness) override;
    GSError GetProducerInitInfo(ProducerInitInfo &info) override;
    GSError AcquireLastFlushedBuffer(sptr<SurfaceBuffer> &buffer, sptr<SyncFence> &fence,
        float matrix[16], uint32_t matrixSize, bool isUseNewMatrix) override;
    GSError ReleaseLastFlushedBuffer(sptr<SurfaceBuffer> buffer) override;
    GSError SetGlobalAlpha(int32_t alpha) override;
    virtual bool IsInHebcList() override
    {
        return initInfo_.isInHebcList;
    }
    GSError RequestAndDetachBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                   BufferRequestConfig& config) override;
    GSError AttachAndFlushBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
                                 BufferFlushConfig& config, bool needMap) override;
    GSError GetCycleBuffersNumber(uint32_t& cycleBuffersNumber) override;
    GSError SetCycleBuffersNumber(uint32_t cycleBuffersNumber) override;
    /**
    * @brief In the strictly disconnected state, the producer must call the ConnectStrictly() interface before request
    *        buffer. Unlike Connect(), ConnectStrictly() does not distinguish between process IDs (PIDs) and is
    *        suitable for stricter connection management scenarios.
    */
    GSError ConnectStrictly() override;
    /**
    * @brief After calling DisconnectStrictly(), the consumer (server) enter the strictly disconnected state.
    *        In this state, any attempt by the producer (client) to request buffer will fail and return the error code
    *        GSERROR_CONSUMER_DISCONNECTED.
    */
    GSError DisconnectStrictly() override;
private:
    bool IsRemote();
    void CleanAllLocked(uint32_t *bufSeqNum);
    GSError AddCacheLocked(sptr<BufferExtraData> &bedataimpl,
        IBufferProducer::RequestBufferReturnValue &retval, BufferRequestConfig &config);
    GSError SetMetadataValue(sptr<SurfaceBuffer>& buffer);
    GSError CleanCacheLocked(bool cleanAll);
    void SetBufferConfigLocked(sptr<BufferExtraData>& bedataimpl,
        IBufferProducer::RequestBufferReturnValue& retval, BufferRequestConfig& config);
    void DeleteCacheBufferLocked(sptr<BufferExtraData>& bedataimpl,
        IBufferProducer::RequestBufferReturnValue& retval, BufferRequestConfig& config);
    GSError UpdateCacheLocked(sptr<BufferExtraData>& bedataimpl,
        IBufferProducer::RequestBufferReturnValue& retval, BufferRequestConfig& config);
    void ReleasePreCacheBuffer(int bufferCacheSize);

    mutable std::mutex mutex_;
    std::atomic_bool inited_ = false;
    std::map<int32_t, sptr<SurfaceBuffer>> bufferProducerCache_;
    std::map<std::string, std::string> userData_;
    sptr<IBufferProducer> producer_ = nullptr;
    std::string name_ = "not init";
    uint64_t queueId_ = 0;
    bool isDisconnected_ = true;
    sptr<IProducerListener> listener_;
    sptr<IProducerListener> listenerWithFence_;
    std::mutex listenerMutex_;
    wptr<NativeWindow> wpNativeWindow_ = nullptr;
    wptr<ProducerSurfaceDelegator> wpPSurfaceDelegator_ = nullptr;
    std::mutex delegatorMutex_;
    std::map<std::string, OnUserDataChangeFunc> onUserDataChange_;
    std::mutex lockMutex_;
    std::string bufferName_ = "";
    int32_t requestWidth_ = 0;
    int32_t requestHeight_ = 0;
    GraphicTransformType lastSetTransformHint_ = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    BufferRequestConfig windowConfig_ = {0};
    ProducerInitInfo initInfo_ = {0};
    sptr<SurfaceBuffer> preCacheBuffer_ = nullptr;
};
} // namespace OHOS

#endif // FRAMEWORKS_SURFACE_INCLUDE_PRODUCER_SURFACE_H
