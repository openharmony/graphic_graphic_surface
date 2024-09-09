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

#ifndef FRAMEWORKS_SURFACE_INCLUDE_CONSUMER_SURFACE_H
#define FRAMEWORKS_SURFACE_INCLUDE_CONSUMER_SURFACE_H

#include <map>
#include <string>

#include <iconsumer_surface.h>

#include "buffer_queue.h"
#include "buffer_queue_producer.h"
#include "buffer_queue_consumer.h"

namespace OHOS {
class ConsumerSurface : public IConsumerSurface {
public:
    ConsumerSurface(const std::string &name, bool isShared = false);
    virtual ~ConsumerSurface();
    GSError Init();

    bool IsConsumer() const override;
    sptr<IBufferProducer> GetProducer() const override;

    GSError GetProducerInitInfo(ProducerInitInfo &info) override;

    SURFACE_HIDDEN GSError RequestBuffer(sptr<SurfaceBuffer>& buffer,
        int32_t &fence, BufferRequestConfig &config) override
    {
        return GSERROR_NOT_SUPPORT;
    }

    SURFACE_HIDDEN GSError RequestBuffers(std::vector<sptr<SurfaceBuffer>> &buffers,
        std::vector<sptr<SyncFence>> &fences, BufferRequestConfig &config) override
    {
        return GSERROR_NOT_SUPPORT;
    }

    SURFACE_HIDDEN GSError CancelBuffer(sptr<SurfaceBuffer>& buffer) override
    {
        return GSERROR_NOT_SUPPORT;
    }

    SURFACE_HIDDEN GSError FlushBuffer(sptr<SurfaceBuffer>& buffer, int32_t fence, BufferFlushConfig &config) override
    {
        return GSERROR_NOT_SUPPORT;
    }

    GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, int32_t &fence,
                          int64_t &timestamp, Rect &damage) override;

    GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, int32_t fence) override;

    SURFACE_HIDDEN GSError RequestBuffer(sptr<SurfaceBuffer>& buffer,
        sptr<SyncFence>& fence, BufferRequestConfig &config) override
    {
        return GSERROR_NOT_SUPPORT;
    }

    SURFACE_HIDDEN GSError FlushBuffer(sptr<SurfaceBuffer>& buffer,
        const sptr<SyncFence>& fence, BufferFlushConfig &config) override
    {
        return GSERROR_NOT_SUPPORT;
    }

    SURFACE_HIDDEN GSError FlushBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
        BufferFlushConfigWithDamages &config) override
    {
        return GSERROR_NOT_SUPPORT;
    }

    SURFACE_HIDDEN GSError FlushBuffers(const std::vector<sptr<SurfaceBuffer>> &buffers,
        const std::vector<sptr<SyncFence>> &fences, const std::vector<BufferFlushConfigWithDamages> &config) override
    {
        return GSERROR_NOT_SUPPORT;
    }

    SURFACE_HIDDEN GSError GetLastFlushedBuffer(sptr<SurfaceBuffer>& buffer,
        sptr<SyncFence>& fence, float matrix[16], bool isUseNewMatrix = false) override
    {
        return GSERROR_NOT_SUPPORT;
    }

    GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                          int64_t &timestamp, Rect &damage) override;
    GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                          int64_t &timestamp, std::vector<Rect> &damages) override;

    GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence) override;

    GSError AttachBuffer(sptr<SurfaceBuffer>& buffer) override;
    GSError DetachBuffer(sptr<SurfaceBuffer>& buffer) override;

    bool QueryIfBufferAvailable() override;

    uint32_t GetQueueSize() override;
    GSError SetQueueSize(uint32_t queueSize) override;

    const std::string& GetName() override;

    GSError SetDefaultWidthAndHeight(int32_t width, int32_t height) override;
    int32_t GetDefaultWidth() override;
    int32_t GetDefaultHeight() override;
    GSError SetDefaultUsage(uint64_t usage) override;
    uint64_t GetDefaultUsage() override;

    GSError SetUserData(const std::string &key, const std::string &val) override;
    std::string GetUserData(const std::string &key) override;

    GSError RegisterConsumerListener(sptr<IBufferConsumerListener>& listener) override;
    GSError RegisterConsumerListener(IBufferConsumerListenerClazz *listener) override;
    GSError RegisterReleaseListener(OnReleaseFunc func) override;
    SURFACE_HIDDEN GSError UnRegisterReleaseListener() override
    {
        return GSERROR_OK;
    }
    GSError RegisterDeleteBufferListener(OnDeleteBufferFunc func, bool isForUniRedraw = false) override;
    GSError UnregisterConsumerListener() override;

    uint64_t GetUniqueId() const override;

    void Dump(std::string &result) const override;

    SURFACE_HIDDEN GSError CleanCache(bool cleanAll = false) override
    {
        return GSERROR_NOT_SUPPORT;
    }

    GSError GoBackground() override;

    GSError SetTransform(GraphicTransformType transform) override;
    GraphicTransformType GetTransform() const override;

    SURFACE_HIDDEN GSError IsSupportedAlloc(const std::vector<BufferVerifyAllocInfo> &infos,
        std::vector<bool> &supporteds) override
    {
        return GSERROR_NOT_SUPPORT;
    }

    SURFACE_HIDDEN GSError Connect() override
    {
        return GSERROR_NOT_SUPPORT;
    }

    SURFACE_HIDDEN GSError Disconnect() override
    {
        return GSERROR_NOT_SUPPORT;
    }
    GSError SetScalingMode(uint32_t sequence, ScalingMode scalingMode) override;
    GSError GetScalingMode(uint32_t sequence, ScalingMode &scalingMode) override;
    GSError SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData) override;
    GSError SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key, const std::vector<uint8_t> &metaData) override;
    GSError QueryMetaDataType(uint32_t sequence, HDRMetaDataType &type) const override;
    GSError GetMetaData(uint32_t sequence, std::vector<GraphicHDRMetaData> &metaData) const override;
    GSError GetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey &key,
                           std::vector<uint8_t> &metaData) const override;
    GSError SetTunnelHandle(const GraphicExtDataHandle *handle) override;
    sptr<SurfaceTunnelHandle> GetTunnelHandle() const override;
    GSError SetPresentTimestamp(uint32_t sequence, const GraphicPresentTimestamp &timestamp) override;
    SURFACE_HIDDEN GSError GetPresentTimestamp(uint32_t sequence,
        GraphicPresentTimestampType type, int64_t &time) const override
    {
        return GSERROR_NOT_SUPPORT;
    }

    SURFACE_HIDDEN int32_t GetDefaultFormat() override
    {
        return 0;
    }
    SURFACE_HIDDEN GSError SetDefaultFormat(int32_t format) override
    {
        return GSERROR_NOT_SUPPORT;
    }
    SURFACE_HIDDEN int32_t GetDefaultColorGamut() override
    {
        return 0;
    }
    SURFACE_HIDDEN GSError SetDefaultColorGamut(int32_t colorGamut) override
    {
        return GSERROR_NOT_SUPPORT;
    }

    SURFACE_HIDDEN sptr<NativeSurface> GetNativeSurface() override
    {
        return nullptr;
    }
    SURFACE_HIDDEN GSError SetWptrNativeWindowToPSurface(void* nativeWindow) override
    {
        return GSERROR_NOT_SUPPORT;
    }
    GSError AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut) override;
    GSError RegisterSurfaceDelegator(sptr<IRemoteObject> client) override;
    SURFACE_HIDDEN GSError RegisterReleaseListener(OnReleaseFuncWithFence func) override
    {
        return GSERROR_NOT_SUPPORT;
    }
    GSError RegisterUserDataChangeListener(const std::string &funcName, OnUserDataChangeFunc func) override;
    GSError UnRegisterUserDataChangeListener(const std::string &funcName) override;
    GSError ClearUserDataChangeListener() override;
    void ConsumerRequestCpuAccess(bool on) override;
    GSError AttachBufferToQueue(sptr<SurfaceBuffer> buffer) override;
    GSError DetachBufferFromQueue(sptr<SurfaceBuffer> buffer) override;
    GraphicTransformType GetTransformHint() const override;
    GSError SetTransformHint(GraphicTransformType transformHint) override;
    inline bool IsBufferHold() override
    {
        if (consumer_ == nullptr) {
            return false;
        }
        return consumer_->IsBufferHold();
    }
    void SetBufferHold(bool hold) override;

    void SetRequestWidthAndHeight(int32_t width, int32_t height) override;
    int32_t GetRequestWidth() override;
    int32_t GetRequestHeight() override;
    GSError SetScalingMode(ScalingMode scalingMode) override;
    GSError SetSurfaceSourceType(OHSurfaceSource sourceType) override;
    OHSurfaceSource GetSurfaceSourceType() const override;
    GSError SetSurfaceAppFrameworkType(std::string appFrameworkType) override;
    std::string GetSurfaceAppFrameworkType() const override;

    void SetWindowConfig(const BufferRequestConfig& config) override;
    void SetWindowConfigWidth(int32_t width) override;
    void SetWindowConfigHeight(int32_t height) override;
    void SetWindowConfigStride(int32_t stride) override;
    void SetWindowConfigFormat(int32_t format) override;
    void SetWindowConfigUsage(uint64_t usage) override;
    void SetWindowConfigTimeout(int32_t timeout) override;
    void SetWindowConfigColorGamut(GraphicColorGamut colorGamut) override;
    void SetWindowConfigTransform(GraphicTransformType transform) override;
    BufferRequestConfig GetWindowConfig() override;
    GSError SetHdrWhitePointBrightness(float brightness) override;
    GSError SetSdrWhitePointBrightness(float brightness) override;
    float GetHdrWhitePointBrightness() const override;
    float GetSdrWhitePointBrightness() const override;

    GSError GetSurfaceBufferTransformType(sptr<SurfaceBuffer> buffer, GraphicTransformType *transformType) override;
    GSError IsSurfaceBufferInCache(uint32_t seqNum, bool &isInCache) override;
    GSError AcquireLastFlushedBuffer(sptr<SurfaceBuffer> &buffer, sptr<SyncFence> &fence,
        float matrix[16], uint32_t matrixSize, bool isUseNewMatrix) override;
    GSError ReleaseLastFlushedBuffer(sptr<SurfaceBuffer> buffer) override;
    GSError SetGlobalAlpha(int32_t alpha) override;
    GSError GetGlobalAlpha(int32_t &alpha) override;
private:
    std::map<std::string, std::string> userData_;
    sptr<BufferQueueProducer> producer_ = nullptr;
    sptr<BufferQueueConsumer> consumer_ = nullptr;
    std::string name_ = "not init";
    bool isShared_ = false;
    std::map<std::string, OnUserDataChangeFunc> onUserDataChange_;
    std::mutex lockMutex_;
    uint64_t uniqueId_ = 0;
};
} // namespace OHOS

#endif // FRAMEWORKS_SURFACE_INCLUDE_CONSUMER_SURFACE_H
