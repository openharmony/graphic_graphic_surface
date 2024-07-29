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

#ifndef INTERFACES_INNERKITS_SURFACE_IBUFFER_PRODUCER_H
#define INTERFACES_INNERKITS_SURFACE_IBUFFER_PRODUCER_H

#include <string>
#include <vector>

#include "iremote_broker.h"

#include "buffer_extra_data.h"
#include "ibuffer_producer_listener.h"
#include "native_surface.h"
#include "surface_buffer.h"
#include "surface_type.h"
#include "external_window.h"

namespace OHOS {
class SyncFence;
class IBufferProducerToken : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"surf.IBufferProducerToken");

    IBufferProducerToken() = default;
    virtual ~IBufferProducerToken() noexcept = default;
};
class IBufferProducer : public IRemoteBroker {
public:
    struct RequestBufferReturnValue {
        uint32_t sequence;
        sptr<SurfaceBuffer> buffer;
        sptr<SyncFence> fence;
        std::vector<int32_t> deletingBuffers;
    };
    virtual GSError GetProducerInitInfo(ProducerInitInfo &info) = 0;

    virtual GSError RequestBuffer(const BufferRequestConfig &config, sptr<BufferExtraData> &bedata,
                                  RequestBufferReturnValue &retval) = 0;

    virtual GSError RequestBuffers(const BufferRequestConfig &config, std::vector<sptr<BufferExtraData>> &bedata,
        std::vector<RequestBufferReturnValue> &retvalues) = 0;

    virtual GSError CancelBuffer(uint32_t sequence, sptr<BufferExtraData> bedata) = 0;

    virtual GSError FlushBuffer(uint32_t sequence, sptr<BufferExtraData> bedata,
                                sptr<SyncFence> fence, BufferFlushConfigWithDamages &config) = 0;

    virtual GSError FlushBuffers(const std::vector<uint32_t> &sequences,
        const std::vector<sptr<BufferExtraData>> &bedata,
        const std::vector<sptr<SyncFence>> &fences,
        const std::vector<BufferFlushConfigWithDamages> &configs) = 0;

    virtual GSError AttachBuffer(sptr<SurfaceBuffer>& buffer) = 0;
    virtual GSError DetachBuffer(sptr<SurfaceBuffer>& buffer) = 0;

    virtual uint32_t GetQueueSize() = 0;
    virtual GSError SetQueueSize(uint32_t queueSize) = 0;

    virtual GSError GetName(std::string &name) = 0;
    virtual uint64_t GetUniqueId() = 0;
    virtual GSError GetNameAndUniqueId(std::string& name, uint64_t& uniqueId) = 0;

    virtual int32_t GetDefaultWidth() = 0;
    virtual int32_t GetDefaultHeight() = 0;
    virtual GSError SetDefaultUsage(uint64_t usage) = 0;
    virtual uint64_t GetDefaultUsage() = 0;

    virtual GSError CleanCache(bool cleanAll = false) = 0;
    virtual GSError GoBackground() = 0;

    virtual GSError RegisterReleaseListener(sptr<IProducerListener> listener) = 0;

    virtual GSError SetTransform(GraphicTransformType transform) = 0;

    virtual GSError IsSupportedAlloc(const std::vector<BufferVerifyAllocInfo> &infos,
                                     std::vector<bool> &supporteds) = 0;

    virtual GSError Connect() = 0;
    virtual GSError Disconnect() = 0;

    virtual GSError SetScalingMode(uint32_t sequence, ScalingMode scalingMode) = 0;
    virtual GSError SetBufferHold(bool hold) = 0;
    virtual GSError SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData) = 0;
    virtual GSError SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key,
                                   const std::vector<uint8_t> &metaData) = 0;
    virtual GSError SetTunnelHandle(const GraphicExtDataHandle *handle) = 0;
    virtual GSError GetPresentTimestamp(uint32_t sequence, GraphicPresentTimestampType type, int64_t &time) = 0;

    virtual sptr<NativeSurface> GetNativeSurface() = 0;
    virtual GSError UnRegisterReleaseListener() = 0;
    virtual GSError GetLastFlushedBuffer(sptr<SurfaceBuffer>& buffer,
        sptr<SyncFence>& fence, float matrix[16], bool isUseNewMatrix) = 0;
    virtual GSError AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut) = 0;

    virtual GSError GetTransform(GraphicTransformType &transform) = 0;

    virtual GSError AttachBufferToQueue(sptr<SurfaceBuffer> buffer) = 0;
    virtual GSError DetachBufferFromQueue(sptr<SurfaceBuffer> buffer) = 0;
    virtual GSError GetTransformHint(GraphicTransformType &transformHint) = 0;
    virtual GSError SetTransformHint(GraphicTransformType transformHint) = 0;
    virtual GSError SetScalingMode(ScalingMode scalingMode) = 0;

    virtual GSError SetSurfaceSourceType(OHSurfaceSource sourceType) = 0;
    virtual GSError GetSurfaceSourceType(OHSurfaceSource &sourceType) = 0;

    virtual GSError SetSurfaceAppFrameworkType(std::string appFrameworkType) = 0;
    virtual GSError GetSurfaceAppFrameworkType(std::string &appFrameworkType) = 0;

    virtual GSError SetHdrWhitePointBrightness(float brightness) = 0;
    virtual GSError SetSdrWhitePointBrightness(float brightness) = 0;
    virtual GSError AcquireLastFlushedBuffer(sptr<SurfaceBuffer> &buffer, sptr<SyncFence> &fence,
        float matrix[16], bool isUseNewMatrix) = 0;
    virtual GSError ReleaseLastFlushedBuffer(uint32_t sequence) = 0;
    DECLARE_INTERFACE_DESCRIPTOR(u"surf.IBufferProducer");

protected:
    enum {
        BUFFER_PRODUCER_REQUEST_BUFFER = 0,
        BUFFER_PRODUCER_CANCEL_BUFFER,
        BUFFER_PRODUCER_FLUSH_BUFFER,
        BUFFER_PRODUCER_GET_QUEUE_SIZE,
        BUFFER_PRODUCER_SET_QUEUE_SIZE,
        BUFFER_PRODUCER_GET_NAME,
        BUFFER_PRODUCER_GET_DEFAULT_WIDTH,
        BUFFER_PRODUCER_GET_DEFAULT_HEIGHT,
        BUFFER_PRODUCER_GET_DEFAULT_USAGE,
        BUFFER_PRODUCER_CLEAN_CACHE,
        BUFFER_PRODUCER_ATTACH_BUFFER,
        BUFFER_PRODUCER_DETACH_BUFFER,
        BUFFER_PRODUCER_REGISTER_RELEASE_LISTENER,
        BUFFER_PRODUCER_GET_UNIQUE_ID,
        BUFFER_PRODUCER_SET_TRANSFORM,
        BUFFER_PRODUCER_IS_SUPPORTED_ALLOC,
        BUFFER_PRODUCER_GET_NAMEANDUNIQUEDID,
        BUFFER_PRODUCER_DISCONNECT,
        BUFFER_PRODUCER_SET_SCALING_MODE,
        BUFFER_PRODUCER_SET_METADATA,
        BUFFER_PRODUCER_SET_METADATASET,
        BUFFER_PRODUCER_SET_TUNNEL_HANDLE,
        BUFFER_PRODUCER_GO_BACKGROUND,
        BUFFER_PRODUCER_GET_PRESENT_TIMESTAMP,
        BUFFER_PRODUCER_UNREGISTER_RELEASE_LISTENER,
        BUFFER_PRODUCER_GET_LAST_FLUSHED_BUFFER,
        BUFFER_PRODUCER_GET_TRANSFORM,
        BUFFER_PRODUCER_ATTACH_BUFFER_TO_QUEUE,
        BUFFER_PRODUCER_DETACH_BUFFER_FROM_QUEUE,
        BUFFER_PRODUCER_SET_DEFAULT_USAGE,
        BUFFER_PRODUCER_GET_TRANSFORMHINT,
        BUFFER_PRODUCER_SET_TRANSFORMHINT,
        BUFFER_PRODUCER_SET_BUFFER_HOLD,
        BUFFER_PRODUCER_SET_SOURCE_TYPE,
        BUFFER_PRODUCER_GET_SOURCE_TYPE,
        BUFFER_PRODUCER_SET_APP_FRAMEWORK_TYPE,
        BUFFER_PRODUCER_GET_APP_FRAMEWORK_TYPE,
        BUFFER_PRODUCER_SET_SCALING_MODEV2,
        BUFFER_PRODUCER_SET_HDRWHITEPOINTBRIGHTNESS,
        BUFFER_PRODUCER_SET_SDRWHITEPOINTBRIGHTNESS,
        BUFFER_PRODUCER_REQUEST_BUFFERS,
        BUFFER_PRODUCER_FLUSH_BUFFERS,
        BUFFER_PRODUCER_GET_INIT_INFO,
        BUFFER_PRODUCER_CONNECT,
        BUFFER_PRODUCER_ACQUIRE_LAST_FLUSHED_BUFFER,
        BUFFER_PRODUCER_RELEASE_LAST_FLUSHED_BUFFER,
    };
};
} // namespace OHOS

#endif // INTERFACES_INNERKITS_SURFACE_IBUFFER_PRODUCER_H
