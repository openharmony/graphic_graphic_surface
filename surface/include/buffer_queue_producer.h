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

#ifndef FRAMEWORKS_SURFACE_INCLUDE_BUFFER_QUEUE_PRODUCER_H
#define FRAMEWORKS_SURFACE_INCLUDE_BUFFER_QUEUE_PRODUCER_H

#include <vector>
#include <mutex>
#include <refbase.h>
#include <iremote_stub.h>
#include <message_parcel.h>
#include <message_option.h>

#include "surface_type.h"
#include <ibuffer_producer.h>

#include "buffer_queue.h"

namespace OHOS {
class BufferQueueProducer : public IRemoteStub<IBufferProducer> {
public:
    BufferQueueProducer(sptr<BufferQueue> bufferQueue);
    virtual ~BufferQueueProducer();

    virtual int OnRemoteRequest(uint32_t code, MessageParcel &arguments,
                                MessageParcel &reply, MessageOption &option) override;

    virtual GSError RequestBuffer(const BufferRequestConfig &config, sptr<BufferExtraData> &bedata,
                                  RequestBufferReturnValue &retval) override;
    GSError RequestBuffers(const BufferRequestConfig &config, std::vector<sptr<BufferExtraData>> &bedata,
        std::vector<RequestBufferReturnValue> &retvalues) override;

    GSError GetProducerInitInfo(ProducerInitInfo &info) override;

    GSError CancelBuffer(uint32_t sequence, sptr<BufferExtraData> bedata) override;

    GSError FlushBuffer(uint32_t sequence, sptr<BufferExtraData> bedata,
                        sptr<SyncFence> fence, BufferFlushConfigWithDamages &config) override;

    GSError FlushBuffers(const std::vector<uint32_t> &sequences,
        const std::vector<sptr<BufferExtraData>> &bedata,
        const std::vector<sptr<SyncFence>> &fences,
        const std::vector<BufferFlushConfigWithDamages> &configs) override;
    GSError GetLastFlushedBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
        float matrix[16], bool isUseNewMatrix) override;

    GSError AttachBuffer(sptr<SurfaceBuffer>& buffer) override;
    GSError AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut) override;

    GSError DetachBuffer(sptr<SurfaceBuffer>& buffer) override;

    uint32_t GetQueueSize() override;
    GSError SetQueueSize(uint32_t queueSize) override;

    GSError GetName(std::string &name) override;
    uint64_t GetUniqueId() override;
    GSError GetNameAndUniqueId(std::string& name, uint64_t& uniqueId) override;

    int32_t GetDefaultWidth() override;
    int32_t GetDefaultHeight() override;
    GSError SetDefaultUsage(uint64_t usage) override;
    uint64_t GetDefaultUsage() override;

    GSError CleanCache(bool cleanAll, uint32_t *bufSeqNum) override;
    GSError GoBackground() override;

    GSError RegisterReleaseListener(sptr<IProducerListener> listener) override;
    GSError RegisterReleaseListenerWithFence(sptr<IProducerListener> listener) override;
    GSError UnRegisterReleaseListener() override;
    GSError UnRegisterReleaseListenerWithFence() override;

    GSError SetTransform(GraphicTransformType transform) override;
    GSError GetTransform(GraphicTransformType &transform) override;

    GSError Connect() override;
    GSError Disconnect(uint32_t *bufSeqNum) override;

    GSError SetScalingMode(uint32_t sequence, ScalingMode scalingMode) override;
    GSError SetBufferHold(bool hold) override;
    GSError SetBufferName(const std::string &bufferName) override;
    GSError SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData) override;
    GSError SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key,
                           const std::vector<uint8_t> &metaData) override;
    GSError SetGlobalAlpha(int32_t alpha) override;
    GSError SetTunnelHandle(const GraphicExtDataHandle *handle) override;
    GSError GetPresentTimestamp(uint32_t sequence, GraphicPresentTimestampType type, int64_t &time) override;

    bool GetStatus() const;
    void SetStatus(bool status);

    sptr<NativeSurface> GetNativeSurface() override;

    void OnBufferProducerRemoteDied();
    GSError AttachBufferToQueue(sptr<SurfaceBuffer> buffer) override;
    GSError DetachBufferFromQueue(sptr<SurfaceBuffer> buffer) override;

    GSError SetTransformHint(GraphicTransformType transformHint) override;
    GSError GetTransformHint(GraphicTransformType &transformHint) override;
    GSError SetScalingMode(ScalingMode scalingMode) override;

    GSError SetSurfaceSourceType(OHSurfaceSource sourceType) override;
    GSError GetSurfaceSourceType(OHSurfaceSource &sourceType) override;

    GSError SetSurfaceAppFrameworkType(std::string appFrameworkType) override;
    GSError GetSurfaceAppFrameworkType(std::string &appFrameworkType) override;

    GSError SetHdrWhitePointBrightness(float brightness) override;
    GSError SetSdrWhitePointBrightness(float brightness) override;

    GSError AcquireLastFlushedBuffer(sptr<SurfaceBuffer> &buffer, sptr<SyncFence> &fence,
        float matrix[16], uint32_t matrixSize, bool isUseNewMatrix) override;
    GSError ReleaseLastFlushedBuffer(uint32_t sequence) override;
    GSError RequestAndDetachBuffer(const BufferRequestConfig& config, sptr<BufferExtraData>& bedata,
        RequestBufferReturnValue& retval) override;
    GSError AttachAndFlushBuffer(sptr<SurfaceBuffer>& buffer, sptr<BufferExtraData>& bedata,
        const sptr<SyncFence>& fence, BufferFlushConfigWithDamages& config, bool needMap) override;
    GSError GetCycleBuffersNumber(uint32_t& cycleBuffersNumber) override;
    GSError SetCycleBuffersNumber(uint32_t cycleBuffersNumber) override;

    GSError ConnectStrictly() override;
    GSError DisconnectStrictly() override;
private:
    GSError CheckConnectLocked();
    GSError SetTunnelHandle(const sptr<SurfaceTunnelHandle> &handle);
    bool HandleDeathRecipient(sptr<IRemoteObject> token);

    int32_t GetProducerInitInfoRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t RequestBufferRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t RequestBuffersRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t CancelBufferRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t FlushBufferRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t FlushBuffersRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t AttachBufferRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t DetachBufferRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t GetQueueSizeRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t SetQueueSizeRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t GetNameRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t GetDefaultWidthRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t GetDefaultHeightRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t SetDefaultUsageRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t GetDefaultUsageRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t GetUniqueIdRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t CleanCacheRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t RegisterReleaseListenerRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t RegisterReleaseListenerWithFenceRemote(MessageParcel &arguments, MessageParcel &reply,
                                                   MessageOption &option);
    int32_t UnRegisterReleaseListenerRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t UnRegisterReleaseListenerWithFenceRemote(MessageParcel &arguments, MessageParcel &reply,
                                                     MessageOption &option);
    int32_t SetTransformRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t GetNameAndUniqueIdRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t DisconnectRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t ConnectRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t SetScalingModeRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t SetMetaDataRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t SetMetaDataSetRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t SetTunnelHandleRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t GoBackgroundRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t GetPresentTimestampRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t GetLastFlushedBufferRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t GetTransformRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t AttachBufferToQueueRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t DetachBufferFromQueueRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t SetTransformHintRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t GetTransformHintRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t SetBufferHoldRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t SetBufferNameRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t SetSurfaceSourceTypeRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t GetSurfaceSourceTypeRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t SetSurfaceAppFrameworkTypeRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t GetSurfaceAppFrameworkTypeRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);

    int32_t SetScalingModeV2Remote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t SetHdrWhitePointBrightnessRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t SetSdrWhitePointBrightnessRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t AcquireLastFlushedBufferRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t ReleaseLastFlushedBufferRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t SetGlobalAlphaRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t RequestAndDetachBufferRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t AttachAndFlushBufferRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t GetRotatingBuffersNumberRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t SetRotatingBuffersNumberRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t DisconnectStrictlyRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);
    int32_t ConnectStrictlyRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option);

    void SetConnectedPid(int32_t connectedPid);
    int32_t AttachBufferToQueueReadBuffer(MessageParcel &arguments,
        MessageParcel &reply, MessageOption &option, sptr<SurfaceBuffer> &buffer);
    bool CheckIsAlive();

    static const std::map<uint32_t, std::function<int32_t(BufferQueueProducer *that, MessageParcel &arguments,
        MessageParcel &reply, MessageOption &option)>> memberFuncMap_;

    class ProducerSurfaceDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit ProducerSurfaceDeathRecipient(wptr<BufferQueueProducer> producer);
        virtual ~ProducerSurfaceDeathRecipient() = default;

        void OnRemoteDied(const wptr<IRemoteObject>& remoteObject) override;
    private:
        wptr<BufferQueueProducer> producer_;
        std::string name_ = "DeathRecipient";
    };
    sptr<ProducerSurfaceDeathRecipient> producerSurfaceDeathRecipient_ = nullptr;
    sptr<IRemoteObject> token_ = nullptr;

    int32_t connectedPid_ = 0;
    bool isDisconnectStrictly_ = false;
    sptr<BufferQueue> bufferQueue_ = nullptr;
    std::string name_ = "not init";
    std::mutex mutex_;
    uint64_t uniqueId_ = 0;
    static const uint32_t MAGIC_INIT = 0x16273849;
    uint32_t magicNum_ = MAGIC_INIT;
};
}; // namespace OHOS

#endif // FRAMEWORKS_SURFACE_INCLUDE_BUFFER_QUEUE_PRODUCER_H
