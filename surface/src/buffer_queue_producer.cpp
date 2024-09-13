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

#include "buffer_queue_producer.h"

#include <cinttypes>
#include <mutex>
#include <set>

#include "buffer_extra_data_impl.h"
#include "buffer_log.h"
#include "buffer_producer_listener.h"
#include "buffer_utils.h"
#include "frame_report.h"
#include "sync_fence.h"

#define BUFFER_PRODUCER_API_FUNC_PAIR(apiSequenceNum, func) \
    {apiSequenceNum, [](BufferQueueProducer *that, MessageParcel &arguments, MessageParcel &reply, \
        MessageOption &option){return that->func(arguments, reply, option);}} \

namespace OHOS {
namespace {
constexpr int32_t BUFFER_MATRIX_SIZE = 16;
} // namespace

BufferQueueProducer::BufferQueueProducer(sptr<BufferQueue> bufferQueue)
    : producerSurfaceDeathRecipient_(new ProducerSurfaceDeathRecipient(this))
{
    bufferQueue_ = std::move(bufferQueue);
    if (bufferQueue_ != nullptr) {
        bufferQueue_->GetName(name_);
        uniqueId_ = bufferQueue_->GetUniqueId();
    }
    memberFuncMap_ = {
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_INIT_INFO, GetProducerInitInfoRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_REQUEST_BUFFER, RequestBufferRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_REQUEST_BUFFERS, RequestBuffersRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_CANCEL_BUFFER, CancelBufferRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_FLUSH_BUFFER, FlushBufferRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_FLUSH_BUFFERS, FlushBuffersRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_ATTACH_BUFFER, AttachBufferRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_DETACH_BUFFER, DetachBufferRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_QUEUE_SIZE, GetQueueSizeRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_QUEUE_SIZE, SetQueueSizeRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_NAME, GetNameRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_DEFAULT_WIDTH, GetDefaultWidthRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_DEFAULT_HEIGHT, GetDefaultHeightRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_DEFAULT_USAGE, GetDefaultUsageRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_UNIQUE_ID, GetUniqueIdRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_CLEAN_CACHE, CleanCacheRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_REGISTER_RELEASE_LISTENER, RegisterReleaseListenerRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_TRANSFORM, SetTransformRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_IS_SUPPORTED_ALLOC, IsSupportedAllocRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_NAMEANDUNIQUEDID, GetNameAndUniqueIdRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_DISCONNECT, DisconnectRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_CONNECT, ConnectRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_SCALING_MODE, SetScalingModeRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_METADATA, SetMetaDataRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_METADATASET, SetMetaDataSetRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_TUNNEL_HANDLE, SetTunnelHandleRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GO_BACKGROUND, GoBackgroundRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_PRESENT_TIMESTAMP, GetPresentTimestampRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_UNREGISTER_RELEASE_LISTENER, UnRegisterReleaseListenerRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_LAST_FLUSHED_BUFFER, GetLastFlushedBufferRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_TRANSFORM, GetTransformRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_ATTACH_BUFFER_TO_QUEUE, AttachBufferToQueueRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_DETACH_BUFFER_FROM_QUEUE, DetachBufferFromQueueRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_DEFAULT_USAGE, SetDefaultUsageRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_TRANSFORMHINT, GetTransformHintRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_TRANSFORMHINT, SetTransformHintRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_BUFFER_HOLD, SetBufferHoldRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_SCALING_MODEV2, SetScalingModeV2Remote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_SOURCE_TYPE, SetSurfaceSourceTypeRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_SOURCE_TYPE, GetSurfaceSourceTypeRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_APP_FRAMEWORK_TYPE, SetSurfaceAppFrameworkTypeRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_APP_FRAMEWORK_TYPE, GetSurfaceAppFrameworkTypeRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(
            BUFFER_PRODUCER_SET_HDRWHITEPOINTBRIGHTNESS, SetHdrWhitePointBrightnessRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(
            BUFFER_PRODUCER_SET_SDRWHITEPOINTBRIGHTNESS, SetSdrWhitePointBrightnessRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_ACQUIRE_LAST_FLUSHED_BUFFER, AcquireLastFlushedBufferRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_RELEASE_LAST_FLUSHED_BUFFER, ReleaseLastFlushedBufferRemote),
        BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_GLOBALALPHA, SetGlobalAlphaRemote),
    };
}

BufferQueueProducer::~BufferQueueProducer()
{
    if (token_ && producerSurfaceDeathRecipient_) {
        token_->RemoveDeathRecipient(producerSurfaceDeathRecipient_);
        token_ = nullptr;
    }
}

GSError BufferQueueProducer::CheckConnectLocked()
{
    if (connectedPid_ == 0) {
        BLOGW("no connections, uniqueId: %{public}" PRIu64 ".", uniqueId_);
        return SURFACE_ERROR_CONSUMER_DISCONNECTED;
    }

    if (connectedPid_ != GetCallingPid()) {
        BLOGW("connected by: %{public}d, uniqueId: %{public}" PRIu64 ".", connectedPid_, uniqueId_);
        return SURFACE_ERROR_CONSUMER_IS_CONNECTED;
    }

    return GSERROR_OK;
}

int32_t BufferQueueProducer::OnRemoteRequest(uint32_t code, MessageParcel &arguments,
                                             MessageParcel &reply, MessageOption &option)
{
    auto it = memberFuncMap_.find(code);
    if (it == memberFuncMap_.end()) {
        BLOGE("cannot process %{public}u", code);
        return IPCObjectStub::OnRemoteRequest(code, arguments, reply, option);
    }

    if (it->second == nullptr) {
        BLOGE("memberFuncMap_[%{public}u] is nullptr", code);
        return ERR_NONE;
    }

    auto remoteDescriptor = arguments.ReadInterfaceToken();
    if (GetDescriptor() != remoteDescriptor) {
        return ERR_INVALID_STATE;
    }

    auto ret = it->second(this, arguments, reply, option);
    return ret;
}

int32_t BufferQueueProducer::RequestBufferRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    RequestBufferReturnValue retval;
    sptr<BufferExtraData> bedataimpl = new BufferExtraDataImpl;
    BufferRequestConfig config = {};
    int64_t startTimeNs = 0;
    int64_t endTimeNs = 0;
    bool isActiveGame = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        isActiveGame = Rosen::FrameReport::GetInstance().IsActiveGameWithPid(connectedPid_);
    }
    if (isActiveGame) {
        startTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    ReadRequestConfig(arguments, config);

    GSError sret = RequestBuffer(config, bedataimpl, retval);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (sret == GSERROR_OK &&
        (WriteSurfaceBufferImpl(reply, retval.sequence, retval.buffer) != GSERROR_OK ||
            bedataimpl->WriteToParcel(reply) != GSERROR_OK || !retval.fence->WriteToMessageParcel(reply) ||
            !reply.WriteUInt32Vector(retval.deletingBuffers))) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    if (isActiveGame) {
        endTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        Rosen::FrameReport::GetInstance().SetDequeueBufferTime(name_, (endTimeNs - startTimeNs));
    }

    return ERR_NONE;
}

int32_t BufferQueueProducer::RequestBuffersRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    std::vector<RequestBufferReturnValue> retvalues;
    std::vector<sptr<BufferExtraData>> bedataimpls;
    BufferRequestConfig config = {};
    uint32_t num = 0;

    arguments.ReadUint32(num);
    ReadRequestConfig(arguments, config);
    if (num == 0 || num > SURFACE_MAX_QUEUE_SIZE) {
        return ERR_NONE;
    }
    retvalues.resize(num);
    bedataimpls.reserve(num);

    for (uint32_t i = 0; i < num; ++i) {
        sptr<BufferExtraData> data = new BufferExtraDataImpl;
        bedataimpls.emplace_back(data);
    }
    GSError sret = RequestBuffers(config, bedataimpls, retvalues);
    num = static_cast<uint32_t>(retvalues.size());
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (sret == GSERROR_OK || sret == GSERROR_NO_BUFFER) {
        if (!reply.WriteUint32(num)) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        for (uint32_t i = 0; i < num; ++i) {
            if (WriteSurfaceBufferImpl(reply, retvalues[i].sequence, retvalues[i].buffer) != GSERROR_OK ||
                bedataimpls[i]->WriteToParcel(reply) != GSERROR_OK ||
                !retvalues[i].fence->WriteToMessageParcel(reply) ||
                !reply.WriteUInt32Vector(retvalues[i].deletingBuffers)) {
                return IPC_STUB_WRITE_PARCEL_ERR;
            }
        }
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetProducerInitInfoRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    ProducerInitInfo info;
    sptr<IRemoteObject> token = arguments.ReadRemoteObject();
    if (token == nullptr || !arguments.ReadString(info.appName)) {
        reply.WriteInt32(GSERROR_INVALID_ARGUMENTS);
        return GSERROR_BINDER;
    }
    (void)GetProducerInitInfo(info);
    if (!reply.WriteInt32(info.width) || !reply.WriteInt32(info.height) || !reply.WriteUint64(info.uniqueId) ||
        !reply.WriteString(info.name) || !reply.WriteBool(info.isInHebcList)) {
        return GSERROR_BINDER;
    }

    bool result = HandleDeathRecipient(token);
    if (!reply.WriteInt32(result ? GSERROR_OK : SURFACE_ERROR_UNKOWN)) {
        return GSERROR_BINDER;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::CancelBufferRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    uint32_t sequence;
    sptr<BufferExtraData> bedataimpl = new BufferExtraDataImpl;

    sequence = arguments.ReadUint32();
    if (bedataimpl->ReadFromParcel(arguments) != GSERROR_OK) {
        return ERR_INVALID_REPLY;
    }

    GSError sret = CancelBuffer(sequence, bedataimpl);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::FlushBufferRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    uint32_t sequence;
    BufferFlushConfigWithDamages config;
    sptr<BufferExtraData> bedataimpl = new BufferExtraDataImpl;
    int64_t startTimeNs = 0;
    int64_t endTimeNs = 0;
    bool isActiveGame = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        isActiveGame = Rosen::FrameReport::GetInstance().IsActiveGameWithPid(connectedPid_);
    }
    if (isActiveGame) {
        startTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    sequence = arguments.ReadUint32();
    if (bedataimpl->ReadFromParcel(arguments) != GSERROR_OK) {
        return ERR_INVALID_REPLY;
    }

    sptr<SyncFence> fence = SyncFence::ReadFromMessageParcel(arguments);
    if (ReadFlushConfig(arguments, config) != GSERROR_OK) {
        return ERR_INVALID_REPLY;
    }

    GSError sret = FlushBuffer(sequence, bedataimpl, fence, config);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    if (isActiveGame) {
        uint64_t uniqueId = GetUniqueId();
        endTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        Rosen::FrameReport::GetInstance().SetQueueBufferTime(uniqueId, name_, (endTimeNs - startTimeNs));
        Rosen::FrameReport::GetInstance().Report(name_);
    }

    return ERR_NONE;
}

int32_t BufferQueueProducer::FlushBuffersRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    std::vector<uint32_t> sequences;
    std::vector<BufferFlushConfigWithDamages> configs;
    std::vector<sptr<BufferExtraData>> bedataimpls;
    std::vector<sptr<SyncFence>> fences;
    arguments.ReadUInt32Vector(&sequences);
    if (sequences.size() == 0 || sequences.size() > SURFACE_MAX_QUEUE_SIZE) {
        return ERR_NONE;
    }
    for (size_t i = 0; i < sequences.size(); ++i) {
        sptr<BufferExtraData> bedataimpl = new BufferExtraDataImpl;
        if (bedataimpl->ReadFromParcel(arguments) != GSERROR_OK) {
            return ERR_INVALID_REPLY;
        }
        bedataimpls.emplace_back(bedataimpl);
        sptr<SyncFence> fence = SyncFence::ReadFromMessageParcel(arguments);
        fences.emplace_back(fence);
        BufferFlushConfigWithDamages config;
        if (ReadFlushConfig(arguments, config) != GSERROR_OK) {
            return ERR_INVALID_REPLY;
        }
        configs.emplace_back(config);
    }

    GSError sret = FlushBuffers(sequences, bedataimpls, fences, configs);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetLastFlushedBufferRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    float matrix[BUFFER_MATRIX_SIZE];
    bool isUseNewMatrix = arguments.ReadBool();
    GSError sret = GetLastFlushedBuffer(buffer, fence, matrix, isUseNewMatrix);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (sret == GSERROR_OK) {
        uint32_t sequence = buffer->GetSeqNum();
        if (WriteSurfaceBufferImpl(reply, sequence, buffer) != GSERROR_OK ||
            buffer->WriteBufferRequestConfig(reply) != GSERROR_OK) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        std::vector<float> writeMatrixVector(matrix, matrix + sizeof(matrix) / sizeof(float));
        if (!fence->WriteToMessageParcel(reply) || !reply.WriteFloatVector(writeMatrixVector)) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::AttachBufferToQueueRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    sptr<SurfaceBuffer> buffer = nullptr;
    uint32_t sequence;
    GSError ret = ReadSurfaceBufferImpl(arguments, sequence, buffer);
    if (ret != GSERROR_OK || buffer == nullptr) {
        if (!reply.WriteInt32(SURFACE_ERROR_UNKOWN)) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        return ERR_INVALID_DATA;
    }
    ret = buffer->ReadBufferRequestConfig(arguments);
    if (ret != GSERROR_OK) {
        if (!reply.WriteInt32(SURFACE_ERROR_UNKOWN)) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        return ERR_INVALID_DATA;
    }
    ret = AttachBufferToQueue(buffer);
    if (!reply.WriteInt32(ret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::DetachBufferFromQueueRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    sptr<SurfaceBuffer> buffer = nullptr;
    uint32_t sequence;
    GSError ret = ReadSurfaceBufferImpl(arguments, sequence, buffer);
    if (ret != GSERROR_OK || buffer == nullptr) {
        if (!reply.WriteInt32(SURFACE_ERROR_UNKOWN)) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        return ERR_INVALID_DATA;
    }
    ret = DetachBufferFromQueue(buffer);
    if (!reply.WriteInt32(ret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::AttachBufferRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    sptr<SurfaceBuffer> buffer;
    uint32_t sequence;
    int32_t timeOut;
    GSError ret = ReadSurfaceBufferImpl(arguments, sequence, buffer);
    if (ret != GSERROR_OK || buffer == nullptr) {
        if (!reply.WriteInt32(ret)) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        return ERR_INVALID_DATA;
    }
    timeOut = arguments.ReadInt32();

    ret = AttachBuffer(buffer, timeOut);
    if (!reply.WriteInt32(ret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::DetachBufferRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetQueueSizeRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    if (!reply.WriteInt32(GetQueueSize())) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetQueueSizeRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    uint32_t queueSize = arguments.ReadUint32();
    GSError sret = SetQueueSize(queueSize);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetNameRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    std::string name;
    auto sret = bufferQueue_->GetName(name);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (sret == GSERROR_OK && !reply.WriteString(name)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetNameAndUniqueIdRemote(MessageParcel &arguments, MessageParcel &reply,
                                                      MessageOption &option)
{
    std::string name = "not init";
    uint64_t uniqueId = 0;
    auto ret = GetNameAndUniqueId(name, uniqueId);
    if (!reply.WriteInt32(ret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (ret == GSERROR_OK && (!reply.WriteString(name) || !reply.WriteUint64(uniqueId))) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetDefaultWidthRemote(MessageParcel &arguments, MessageParcel &reply,
                                                   MessageOption &option)
{
    if (!reply.WriteInt32(GetDefaultWidth())) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetDefaultHeightRemote(MessageParcel &arguments, MessageParcel &reply,
                                                    MessageOption &option)
{
    if (!reply.WriteInt32(GetDefaultHeight())) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetDefaultUsageRemote(MessageParcel &arguments, MessageParcel &reply,
                                                   MessageOption &option)
{
    uint64_t usage = arguments.ReadUint64();
    GSError sret = SetDefaultUsage(usage);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetDefaultUsageRemote(MessageParcel &arguments, MessageParcel &reply,
                                                   MessageOption &option)
{
    if (!reply.WriteUint64(GetDefaultUsage())) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetUniqueIdRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    if (!reply.WriteUint64(GetUniqueId())) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::CleanCacheRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    bool cleanAll = arguments.ReadBool();
    if (!reply.WriteInt32(CleanCache(cleanAll))) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GoBackgroundRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    if (!reply.WriteInt32(GoBackground())) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::RegisterReleaseListenerRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    sptr<IRemoteObject> listenerObject = arguments.ReadRemoteObject();
    if (listenerObject == nullptr) {
        if (!reply.WriteInt32(GSERROR_INVALID_ARGUMENTS)) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        return ERR_INVALID_REPLY;
    }
    sptr<IProducerListener> listener = iface_cast<IProducerListener>(listenerObject);
    GSError sret = RegisterReleaseListener(listener);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::UnRegisterReleaseListenerRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    GSError sret = UnRegisterReleaseListener();
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetTransformRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    GraphicTransformType transform = static_cast<GraphicTransformType>(arguments.ReadUint32());
    GSError sret = SetTransform(transform);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::IsSupportedAllocRemote(MessageParcel &arguments, MessageParcel &reply,
                                                    MessageOption &option)
{
    std::vector<BufferVerifyAllocInfo> infos;
    ReadVerifyAllocInfo(arguments, infos);

    std::vector<bool> supporteds;
    GSError sret = IsSupportedAlloc(infos, supporteds);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (sret == GSERROR_OK && !reply.WriteBoolVector(supporteds)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t BufferQueueProducer::ConnectRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    GSError sret = Connect();
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::DisconnectRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    GSError sret = Disconnect();
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetScalingModeRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    uint32_t sequence = arguments.ReadUint32();
    ScalingMode scalingMode = static_cast<ScalingMode>(arguments.ReadInt32());
    GSError sret = SetScalingMode(sequence, scalingMode);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetScalingModeV2Remote(MessageParcel &arguments, MessageParcel &reply,
                                                    MessageOption &option)
{
    ScalingMode scalingMode = static_cast<ScalingMode>(arguments.ReadInt32());
    GSError sret = SetScalingMode(scalingMode);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetBufferHoldRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    bool hold = arguments.ReadBool();
    GSError sret = SetBufferHold(hold);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetMetaDataRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    uint32_t sequence = arguments.ReadUint32();
    std::vector<GraphicHDRMetaData> metaData;
    if (ReadHDRMetaData(arguments, metaData) != GSERROR_OK) {
        return GSERROR_BINDER;
    }
    GSError sret = SetMetaData(sequence, metaData);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetMetaDataSetRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    uint32_t sequence = arguments.ReadUint32();
    GraphicHDRMetadataKey key = static_cast<GraphicHDRMetadataKey>(arguments.ReadUint32());
    std::vector<uint8_t> metaData;
    if (ReadHDRMetaDataSet(arguments, metaData) != GSERROR_OK) {
        return GSERROR_BINDER;
    }
    GSError sret = SetMetaDataSet(sequence, key, metaData);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetTunnelHandleRemote(MessageParcel &arguments, MessageParcel &reply,
                                                   MessageOption &option)
{
    sptr<SurfaceTunnelHandle> handle = nullptr;
    if (arguments.ReadBool()) {
        handle = new SurfaceTunnelHandle();
        if (ReadExtDataHandle(arguments, handle) != GSERROR_OK) {
            return GSERROR_BINDER;
        }
    }
    GSError sret = SetTunnelHandle(handle);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetPresentTimestampRemote(MessageParcel &arguments, MessageParcel &reply,
                                                       MessageOption &option)
{
    uint32_t sequence = arguments.ReadUint32();
    GraphicPresentTimestampType type = static_cast<GraphicPresentTimestampType>(arguments.ReadUint32());
    int64_t time = 0;
    GSError sret = GetPresentTimestamp(sequence, type, time);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (sret == GSERROR_OK && !reply.WriteInt64(time)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetTransformRemote(
    MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_BUTT;
    auto ret = GetTransform(transform);
    if (ret != GSERROR_OK) {
        if (!reply.WriteInt32(static_cast<int32_t>(ret))) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        return ERR_INVALID_REPLY;
    }

    if (!reply.WriteInt32(GSERROR_OK) || !reply.WriteUint32(static_cast<uint32_t>(transform))) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t BufferQueueProducer::SetTransformHintRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    GraphicTransformType transformHint = static_cast<GraphicTransformType>(arguments.ReadUint32());
    GSError sret = SetTransformHint(transformHint);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetTransformHintRemote(
    MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    GraphicTransformType transformHint = GraphicTransformType::GRAPHIC_ROTATE_BUTT;
    auto ret = GetTransformHint(transformHint);
    if (ret != GSERROR_OK) {
        if (!reply.WriteInt32(static_cast<int32_t>(ret))) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        return ERR_INVALID_REPLY;
    }

    if (!reply.WriteInt32(GSERROR_OK) || !reply.WriteUint32(static_cast<uint32_t>(transformHint))) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t BufferQueueProducer::SetSurfaceSourceTypeRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    OHSurfaceSource sourceType = static_cast<OHSurfaceSource>(arguments.ReadUint32());
    GSError sret = SetSurfaceSourceType(sourceType);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetSurfaceSourceTypeRemote(
    MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    OHSurfaceSource sourceType = OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
    auto ret = GetSurfaceSourceType(sourceType);
    if (ret != GSERROR_OK) {
        if (!reply.WriteInt32(static_cast<int32_t>(ret))) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        return ERR_INVALID_REPLY;
    }

    if (!reply.WriteInt32(GSERROR_OK) || !reply.WriteUint32(static_cast<uint32_t>(sourceType))) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t BufferQueueProducer::SetSurfaceAppFrameworkTypeRemote(
    MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    std::string appFrameworkType = arguments.ReadString();
    GSError sret = SetSurfaceAppFrameworkType(appFrameworkType);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetSurfaceAppFrameworkTypeRemote(
    MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    std::string appFrameworkType = "";
    auto ret = GetSurfaceAppFrameworkType(appFrameworkType);
    if (ret != GSERROR_OK) {
        if (!reply.WriteInt32(static_cast<int32_t>(ret))) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        return ERR_INVALID_REPLY;
    }

    if (!reply.WriteInt32(GSERROR_OK) || !reply.WriteString(appFrameworkType)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t BufferQueueProducer::SetHdrWhitePointBrightnessRemote(
    MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    float brightness = arguments.ReadFloat();
    GSError sret = SetHdrWhitePointBrightness(brightness);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetSdrWhitePointBrightnessRemote(
    MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    float brightness = arguments.ReadFloat();
    GSError sret = SetSdrWhitePointBrightness(brightness);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::AcquireLastFlushedBufferRemote(
    MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    float matrix[BUFFER_MATRIX_SIZE];
    bool isUseNewMatrix = arguments.ReadBool();
    GSError sret = AcquireLastFlushedBuffer(buffer, fence, matrix, BUFFER_MATRIX_SIZE, isUseNewMatrix);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (sret == GSERROR_OK) {
        uint32_t sequence = buffer->GetSeqNum();
        if (WriteSurfaceBufferImpl(reply, sequence, buffer) != GSERROR_OK ||
            buffer->WriteBufferRequestConfig(reply) != GSERROR_OK) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        std::vector<float> writeMatrixVector(matrix, matrix + sizeof(matrix) / sizeof(float));
        if (!fence->WriteToMessageParcel(reply) || !reply.WriteFloatVector(writeMatrixVector)) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::ReleaseLastFlushedBufferRemote(
    MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    uint32_t sequence = arguments.ReadUint32();
    GSError sret = ReleaseLastFlushedBuffer(sequence);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetGlobalAlphaRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    int32_t alpha = arguments.ReadInt32();
    GSError sret = SetGlobalAlpha(alpha);
    if (!reply.WriteInt32(sret)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

GSError BufferQueueProducer::AcquireLastFlushedBuffer(sptr<SurfaceBuffer> &buffer,
    sptr<SyncFence> &fence, float matrix[16], uint32_t matrixSize, bool isUseNewMatrix)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->AcquireLastFlushedBuffer(buffer, fence, matrix, matrixSize, isUseNewMatrix);
}

GSError BufferQueueProducer::ReleaseLastFlushedBuffer(uint32_t sequence)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->ReleaseLastFlushedBuffer(sequence);
}

GSError BufferQueueProducer::RequestBuffer(const BufferRequestConfig &config, sptr<BufferExtraData> &bedata,
                                           RequestBufferReturnValue &retval)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }

    auto ret = Connect();
    if (ret != SURFACE_ERROR_OK) {
        return ret;
    }
    return bufferQueue_->RequestBuffer(config, bedata, retval);
}

GSError BufferQueueProducer::RequestBuffers(const BufferRequestConfig &config,
    std::vector<sptr<BufferExtraData>> &bedata, std::vector<RequestBufferReturnValue> &retvalues)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    auto ret = Connect();
    if (ret != SURFACE_ERROR_OK) {
        return ret;
    }
    bufferQueue_->SetBatchHandle(true);
    for (size_t i = 0; i < retvalues.size(); ++i) {
        ret = bufferQueue_->RequestBuffer(config, bedata[i], retvalues[i]);
        if (ret != GSERROR_OK) {
            retvalues.resize(i);
            break;
        }
    }
    bufferQueue_->SetBatchHandle(false);
    if (retvalues.size() == 0) {
        return ret;
    }
    return GSERROR_OK;
}

GSError BufferQueueProducer::GetProducerInitInfo(ProducerInitInfo &info)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->GetProducerInitInfo(info);
}

GSError BufferQueueProducer::CancelBuffer(uint32_t sequence, sptr<BufferExtraData> bedata)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->CancelBuffer(sequence, bedata);
}

GSError BufferQueueProducer::FlushBuffer(uint32_t sequence, sptr<BufferExtraData> bedata,
                                         sptr<SyncFence> fence, BufferFlushConfigWithDamages &config)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->FlushBuffer(sequence, bedata, fence, config);
}

GSError BufferQueueProducer::FlushBuffers(const std::vector<uint32_t> &sequences,
    const std::vector<sptr<BufferExtraData>> &bedata,
    const std::vector<sptr<SyncFence>> &fences,
    const std::vector<BufferFlushConfigWithDamages> &configs)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    GSError ret;
    for (size_t i = 0; i < sequences.size(); ++i) {
        ret = bufferQueue_->FlushBuffer(sequences[i], bedata[i], fences[i], configs[i]);
        if (ret != GSERROR_OK) {
            BLOGE("FlushBuffer failed: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, uniqueId_);
            return ret;
        }
    }
    return ret;
}

GSError BufferQueueProducer::GetLastFlushedBuffer(sptr<SurfaceBuffer>& buffer,
    sptr<SyncFence>& fence, float matrix[16], bool isUseNewMatrix)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->GetLastFlushedBuffer(buffer, fence, matrix, BUFFER_MATRIX_SIZE, isUseNewMatrix);
}

GSError BufferQueueProducer::AttachBufferToQueue(sptr<SurfaceBuffer> buffer)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->AttachBufferToQueue(buffer, InvokerType::PRODUCER_INVOKER);
}

GSError BufferQueueProducer::DetachBufferFromQueue(sptr<SurfaceBuffer> buffer)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->DetachBufferFromQueue(buffer, InvokerType::PRODUCER_INVOKER);
}

GSError BufferQueueProducer::AttachBuffer(sptr<SurfaceBuffer>& buffer)
{
    int32_t timeOut = 0;
    return AttachBuffer(buffer, timeOut);
}

GSError BufferQueueProducer::AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->AttachBuffer(buffer, timeOut);
}

GSError BufferQueueProducer::DetachBuffer(sptr<SurfaceBuffer>& buffer)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }

    return bufferQueue_->DetachBuffer(buffer);
}

uint32_t BufferQueueProducer::GetQueueSize()
{
    if (bufferQueue_ == nullptr) {
        return 0;
    }
    return bufferQueue_->GetQueueSize();
}

GSError BufferQueueProducer::SetQueueSize(uint32_t queueSize)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->SetQueueSize(queueSize);
}

GSError BufferQueueProducer::GetName(std::string &name)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->GetName(name);
}

int32_t BufferQueueProducer::GetDefaultWidth()
{
    if (bufferQueue_ == nullptr) {
        return 0;
    }
    return bufferQueue_->GetDefaultWidth();
}

int32_t BufferQueueProducer::GetDefaultHeight()
{
    if (bufferQueue_ == nullptr) {
        return 0;
    }
    return bufferQueue_->GetDefaultHeight();
}

GSError BufferQueueProducer::SetDefaultUsage(uint64_t usage)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->SetDefaultUsage(usage);
}

uint64_t BufferQueueProducer::GetDefaultUsage()
{
    if (bufferQueue_ == nullptr) {
        return 0;
    }
    return bufferQueue_->GetDefaultUsage();
}

uint64_t BufferQueueProducer::GetUniqueId()
{
    if (bufferQueue_ == nullptr) {
        return 0;
    }
    return bufferQueue_->GetUniqueId();
}

GSError BufferQueueProducer::CleanCache(bool cleanAll)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto ret = CheckConnectLocked();
        if (ret != GSERROR_OK) {
            return ret;
        }
    }

    return bufferQueue_->CleanCache(cleanAll);
}

GSError BufferQueueProducer::GoBackground()
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto ret = CheckConnectLocked();
        if (ret != GSERROR_OK) {
            return ret;
        }
    }
    return bufferQueue_->SetProducerCacheCleanFlag(true);
}

GSError BufferQueueProducer::RegisterReleaseListener(sptr<IProducerListener> listener)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->RegisterProducerReleaseListener(listener);
}

GSError BufferQueueProducer::UnRegisterReleaseListener()
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->UnRegisterProducerReleaseListener();
}

bool BufferQueueProducer::HandleDeathRecipient(sptr<IRemoteObject> token)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (token_ != nullptr) {
        token_->RemoveDeathRecipient(producerSurfaceDeathRecipient_);
    }
    token_ = token;
    return token_->AddDeathRecipient(producerSurfaceDeathRecipient_);
}

GSError BufferQueueProducer::SetTransform(GraphicTransformType transform)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->SetTransform(transform);
}

GSError BufferQueueProducer::GetTransform(GraphicTransformType &transform)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (bufferQueue_ == nullptr) {
        transform = GraphicTransformType::GRAPHIC_ROTATE_BUTT;
        return GSERROR_INVALID_ARGUMENTS;
    }
    transform = bufferQueue_->GetTransform();
    return GSERROR_OK;
}

GSError BufferQueueProducer::SetTransformHint(GraphicTransformType transformHint)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->SetTransformHint(transformHint);
}

GSError BufferQueueProducer::GetTransformHint(GraphicTransformType &transformHint)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (bufferQueue_ == nullptr) {
        transformHint = GraphicTransformType::GRAPHIC_ROTATE_BUTT;
        return GSERROR_INVALID_ARGUMENTS;
    }
    transformHint = bufferQueue_->GetTransformHint();
    return GSERROR_OK;
}

GSError BufferQueueProducer::SetSurfaceSourceType(OHSurfaceSource sourceType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->SetSurfaceSourceType(sourceType);
}

GSError BufferQueueProducer::GetSurfaceSourceType(OHSurfaceSource &sourceType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (bufferQueue_ == nullptr) {
        sourceType = OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT;
        return GSERROR_INVALID_ARGUMENTS;
    }
    sourceType = bufferQueue_->GetSurfaceSourceType();
    return GSERROR_OK;
}

GSError BufferQueueProducer::SetSurfaceAppFrameworkType(std::string appFrameworkType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->SetSurfaceAppFrameworkType(appFrameworkType);
}

GSError BufferQueueProducer::GetSurfaceAppFrameworkType(std::string &appFrameworkType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (bufferQueue_ == nullptr) {
        appFrameworkType = "";
        return GSERROR_INVALID_ARGUMENTS;
    }
    appFrameworkType = bufferQueue_->GetSurfaceAppFrameworkType();
    return GSERROR_OK;
}

GSError BufferQueueProducer::SetHdrWhitePointBrightness(float brightness)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->SetHdrWhitePointBrightness(brightness);
}

GSError BufferQueueProducer::SetSdrWhitePointBrightness(float brightness)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->SetSdrWhitePointBrightness(brightness);
}

GSError BufferQueueProducer::IsSupportedAlloc(const std::vector<BufferVerifyAllocInfo> &infos,
                                              std::vector<bool> &supporteds)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }

    return bufferQueue_->IsSupportedAlloc(infos, supporteds);
}

GSError BufferQueueProducer::GetNameAndUniqueId(std::string& name, uint64_t& uniqueId)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    uniqueId = GetUniqueId();
    return GetName(name);
}

GSError BufferQueueProducer::Connect()
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto callingPid = GetCallingPid();
    if (connectedPid_ != 0 && connectedPid_ != callingPid) {
        BLOGW("connected by: %{public}d, request by: %{public}d , uniqueId: %{public}" PRIu64 ".",
            connectedPid_, callingPid, uniqueId_);
        return SURFACE_ERROR_CONSUMER_IS_CONNECTED;
    }
    connectedPid_ = callingPid;
    return SURFACE_ERROR_OK;
}

GSError BufferQueueProducer::Disconnect()
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto ret = CheckConnectLocked();
        if (ret != GSERROR_OK) {
            return ret;
        }
        connectedPid_ = 0;
    }
    return bufferQueue_->CleanCache(false);
}

GSError BufferQueueProducer::SetScalingMode(uint32_t sequence, ScalingMode scalingMode)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->SetScalingMode(sequence, scalingMode);
}

GSError BufferQueueProducer::SetScalingMode(ScalingMode scalingMode)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->SetScalingMode(scalingMode);
}

GSError BufferQueueProducer::SetBufferHold(bool hold)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->SetBufferHold(hold);
}

GSError BufferQueueProducer::SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }

    return bufferQueue_->SetMetaData(sequence, metaData);
}

GSError BufferQueueProducer::SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key,
                                            const std::vector<uint8_t> &metaData)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }

    return bufferQueue_->SetMetaDataSet(sequence, key, metaData);
}

GSError BufferQueueProducer::SetTunnelHandle(const sptr<SurfaceTunnelHandle> &handle)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->SetTunnelHandle(handle);
}

GSError BufferQueueProducer::SetTunnelHandle(const GraphicExtDataHandle *handle)
{
    sptr<SurfaceTunnelHandle> tunnelHandle = new SurfaceTunnelHandle();
    if (tunnelHandle->SetHandle(handle) != GSERROR_OK) {
        return GSERROR_INVALID_OPERATING;
    }
    return bufferQueue_->SetTunnelHandle(tunnelHandle);
}

GSError BufferQueueProducer::GetPresentTimestamp(uint32_t sequence, GraphicPresentTimestampType type, int64_t &time)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->GetPresentTimestamp(sequence, type, time);
}

bool BufferQueueProducer::GetStatus() const
{
    if (bufferQueue_ == nullptr) {
        return false;
    }
    return bufferQueue_->GetStatus();
}

void BufferQueueProducer::SetStatus(bool status)
{
    if (bufferQueue_ == nullptr) {
        return;
    }
    bufferQueue_->SetStatus(status);
}

GSError BufferQueueProducer::SetGlobalAlpha(int32_t alpha)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->SetGlobalAlpha(alpha);
}

sptr<NativeSurface> BufferQueueProducer::GetNativeSurface()
{
    return nullptr;
}

void BufferQueueProducer::OnBufferProducerRemoteDied()
{
    if (bufferQueue_ == nullptr) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connectedPid_ == 0) {
            BLOGD("no connections, uniqueId: %{public}" PRIu64 ".", uniqueId_);
            return;
        }
        connectedPid_ = 0;
    }
    bufferQueue_->CleanCache(false);
}

BufferQueueProducer::ProducerSurfaceDeathRecipient::ProducerSurfaceDeathRecipient(
    wptr<BufferQueueProducer> producer) : producer_(producer)
{
}

void BufferQueueProducer::ProducerSurfaceDeathRecipient::OnRemoteDied(const wptr<IRemoteObject>& remoteObject)
{
    auto remoteToken = remoteObject.promote();
    if (remoteToken == nullptr) {
        BLOGW("can't promote remote object.");
        return;
    }

    auto producer = producer_.promote();
    if (producer == nullptr) {
        BLOGD("producer is nullptr");
        return;
    }

    if (producer->token_ != remoteToken) {
        BLOGD("token doesn't match, ignore it, uniqueId: %{public}" PRIu64 ".", producer->GetUniqueId());
        return;
    }
    BLOGD("remote object died, uniqueId: %{public}" PRIu64 ".", producer->GetUniqueId());
    producer->OnBufferProducerRemoteDied();
}
}; // namespace OHOS
