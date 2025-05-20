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

#ifndef FRAMEWORKS_SURFACE_INCLUDE_BUFFER_CLIENT_PRODUCER_H
#define FRAMEWORKS_SURFACE_INCLUDE_BUFFER_CLIENT_PRODUCER_H

#include <map>
#include <vector>
#include <mutex>

#include <iremote_proxy.h>
#include <iremote_object.h>

#include <ibuffer_producer.h>

#include "surface_buffer_impl.h"

namespace OHOS {
class BufferClientProducer : public IRemoteProxy<IBufferProducer> {
public:
    BufferClientProducer(const sptr<IRemoteObject>& impl);
    virtual ~BufferClientProducer();

    GSError GetProducerInitInfo(ProducerInitInfo &info) override;

    GSError RequestBuffer(const BufferRequestConfig &config, sptr<BufferExtraData> &bedata,
                          RequestBufferReturnValue &retval) override;
    GSError RequestBuffers(const BufferRequestConfig &config, std::vector<sptr<BufferExtraData>> &bedata,
        std::vector<RequestBufferReturnValue> &retvalues) override;

    GSError CancelBuffer(uint32_t sequence, sptr<BufferExtraData> bedata) override;

    GSError FlushBuffer(uint32_t sequence, sptr<BufferExtraData> bedata,
                        sptr<SyncFence> fence, BufferFlushConfigWithDamages &config) override;

    GSError FlushBuffers(const std::vector<uint32_t> &sequences,
        const std::vector<sptr<BufferExtraData>> &bedata,
        const std::vector<sptr<SyncFence>> &fences,
        const std::vector<BufferFlushConfigWithDamages> &configs) override;

    GSError GetLastFlushedBuffer(sptr<SurfaceBuffer>& buffer,
        sptr<SyncFence>& fence, float matrix[16], bool isUseNewMatrix) override;
    uint32_t GetQueueSize() override;
    GSError SetQueueSize(uint32_t queueSize) override;

    GSError GetName(std::string &name) override;
    uint64_t GetUniqueId() override;
    GSError GetNameAndUniqueId(std::string& name, uint64_t& uniqueId) override;

    int32_t GetDefaultWidth() override;
    int32_t GetDefaultHeight() override;
    GSError SetDefaultUsage(uint64_t usage) override;
    uint64_t GetDefaultUsage() override;
    GSError SetTransform(GraphicTransformType transform) override;

    GSError AttachBuffer(sptr<SurfaceBuffer>& buffer) override;
    GSError AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut) override;
    GSError DetachBuffer(sptr<SurfaceBuffer>& buffer) override;
    GSError RegisterReleaseListener(sptr<IProducerListener> listener) override;
    GSError RegisterReleaseListenerBackup(sptr<IProducerListener> listener) override;
    GSError UnRegisterReleaseListener() override;
    GSError UnRegisterReleaseListenerBackup() override;
    GSError UnRegisterPropertyListener(uint64_t producerId) override;
    GSError RegisterPropertyListener(sptr<IProducerListener> listener, uint64_t producerId) override;

    // Call carefully. This interface will empty all caches of the current process
    GSError CleanCache(bool cleanAll, uint32_t *bufSeqNum) override;
    GSError Connect() override;
    GSError Disconnect(uint32_t *bufSeqNum) override;
    GSError GoBackground() override;

    GSError SetScalingMode(uint32_t sequence, ScalingMode scalingMode) override;
    GSError SetBufferHold(bool hold) override;
    GSError SetBufferName(const std::string &bufferName) override;

    GSError SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData) override;
    GSError SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key,
                           const std::vector<uint8_t> &metaData) override;
    GSError SetTunnelHandle(const GraphicExtDataHandle *handle) override;
    GSError GetPresentTimestamp(uint32_t sequence, GraphicPresentTimestampType type, int64_t &time) override;

    sptr<NativeSurface> GetNativeSurface() override;

    GSError GetTransform(GraphicTransformType &transform) override;
    GSError AttachBufferToQueue(sptr<SurfaceBuffer> buffer) override;
    GSError DetachBufferFromQueue(sptr<SurfaceBuffer> buffer) override;

    GSError GetTransformHint(GraphicTransformType &transformHint) override;
    GSError SetTransformHint(GraphicTransformType transformHint, uint64_t fromId) override;
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
    GSError SetGlobalAlpha(int32_t alpha) override;
    GSError SetRequestBufferNoblockMode(bool noblock) override;
    GSError RequestAndDetachBuffer(const BufferRequestConfig& config, sptr<BufferExtraData>& bedata,
        RequestBufferReturnValue& retval) override;
    GSError AttachAndFlushBuffer(sptr<SurfaceBuffer>& buffer, sptr<BufferExtraData>& bedata,
        const sptr<SyncFence>& fence, BufferFlushConfigWithDamages& config, bool needMap) override;
    GSError GetCycleBuffersNumber(uint32_t& cycleBuffersNumber) override;
    GSError SetCycleBuffersNumber(uint32_t cycleBuffersNumber) override;
    GSError SetFrameGravity(int32_t frameGravity) override;
    GSError SetFixedRotation(int32_t fixedRotation) override;
    GSError ConnectStrictly() override;
    GSError DisconnectStrictly() override;
    GSError PreAllocBuffers(const BufferRequestConfig &config, uint32_t allocBufferCount) override;
private:
    GSError MessageVariables(MessageParcel &arg);
    GSError SendRequest(uint32_t command, MessageParcel &arg, MessageParcel &reply, MessageOption &opt);
    GSError CheckRetval(MessageParcel &reply);
    GSError GetLastFlushedBufferCommon(sptr<SurfaceBuffer>& buffer,
        sptr<SyncFence>& fence, float matrix[16], uint32_t matrixSize, bool isUseNewMatrix, uint32_t command);
    GSError RequestBufferCommon(const BufferRequestConfig &config, sptr<BufferExtraData> &bedata,
        RequestBufferReturnValue &retval, uint32_t command);

    static inline BrokerDelegator<BufferClientProducer> delegator_;
    static inline const std::string DEFAULT_NAME = "not init";
    std::string name_ = DEFAULT_NAME;
    uint64_t uniqueId_ = 0;
    std::mutex mutex_;
    sptr<IBufferProducerToken> token_;
    GraphicTransformType lastSetTransformType_ = GraphicTransformType::GRAPHIC_ROTATE_BUTT;
};
}; // namespace OHOS

#endif // FRAMEWORKS_SURFACE_INCLUDE_BUFFER_CLIENT_PRODUCER_H
