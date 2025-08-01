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

#include "buffer_queue_producer.h"

#include <cinttypes>
#include <mutex>
#include <set>
#include <csignal>

#include "buffer_extra_data_impl.h"
#include "buffer_log.h"
#include "buffer_producer_listener.h"
#include "buffer_utils.h"
#include "frame_report.h"
#include <parameter.h>
#include <parameters.h>
#include "sync_fence.h"

#define BUFFER_PRODUCER_API_FUNC_PAIR(apiSequenceNum, func) \
    {apiSequenceNum, [](BufferQueueProducer *that, MessageParcel &arguments, MessageParcel &reply, \
        MessageOption &option){return that->func(arguments, reply, option);}} \

namespace OHOS {
namespace {
constexpr int32_t BUFFER_MATRIX_SIZE = 16;
} // namespace

const std::map<uint32_t, std::function<int32_t(BufferQueueProducer *that, MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)>> BufferQueueProducer::memberFuncMap_ = {
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
    BUFFER_PRODUCER_API_FUNC_PAIR(
        BUFFER_PRODUCER_REGISTER_RELEASE_LISTENER_BACKUP, RegisterReleaseListenerBackupRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_TRANSFORM, SetTransformRemote),
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
    BUFFER_PRODUCER_API_FUNC_PAIR(
        BUFFER_PRODUCER_UNREGISTER_RELEASE_LISTENER_BACKUP, UnRegisterReleaseListenerBackupRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_LAST_FLUSHED_BUFFER, GetLastFlushedBufferRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_TRANSFORM, GetTransformRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_ATTACH_BUFFER_TO_QUEUE, AttachBufferToQueueRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_DETACH_BUFFER_FROM_QUEUE, DetachBufferFromQueueRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_DEFAULT_USAGE, SetDefaultUsageRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_TRANSFORMHINT, GetTransformHintRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_TRANSFORMHINT, SetTransformHintRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_BUFFER_HOLD, SetBufferHoldRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_BUFFER_NAME, SetBufferNameRemote),
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
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_REQUESTBUFFER_NOBLOCKMODE, SetRequestBufferNoblockModeRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_REQUEST_AND_DETACH_BUFFER, RequestAndDetachBufferRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_ATTACH_AND_FLUSH_BUFFER, AttachAndFlushBufferRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_GET_ROTATING_BUFFERS_NUMBER, GetRotatingBuffersNumberRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_ROTATING_BUFFERS_NUMBER, SetRotatingBuffersNumberRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_FRAME_GRAVITY, SetFrameGravityRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_FIXED_ROTATION, SetFixedRotationRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_DISCONNECT_STRICTLY, DisconnectStrictlyRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_CONNECT_STRICTLY, ConnectStrictlyRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_REGISTER_PROPERTY_LISTENER, RegisterPropertyListenerRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_UNREGISTER_PROPERTY_LISTENER, UnRegisterPropertyListenerRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_PRE_ALLOC_BUFFERS, PreAllocBuffersRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_LPP_FD, SetLppShareFdRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_BUFFER_REALLOC_FLAG, SetBufferReallocFlagRemote),
    BUFFER_PRODUCER_API_FUNC_PAIR(BUFFER_PRODUCER_SET_ALPHA_TYPE, SetAlphaTypeRemote),
};

BufferQueueProducer::BufferQueueProducer(sptr<BufferQueue> bufferQueue)
    : producerSurfaceDeathRecipient_(new ProducerSurfaceDeathRecipient(this))
{
    bufferQueue_ = std::move(bufferQueue);
    if (bufferQueue_ != nullptr) {
        bufferQueue_->GetName(name_);
        uniqueId_ = bufferQueue_->GetUniqueId();
    }
}

BufferQueueProducer::~BufferQueueProducer()
{
    (void)CheckIsAlive();
    magicNum_ = 0;
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
    if (!CheckIsAlive()) {
        return ERR_NULL_OBJECT;
    }
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
    int32_t connectedPid = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connectedPid = connectedPid_;
    }
    isActiveGame = Rosen::FrameReport::GetInstance().IsActiveGameWithPid(connectedPid);
    if (isActiveGame) {
        startTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    ReadRequestConfig(arguments, config);

    GSError sRet = RequestBuffer(config, bedataimpl, retval);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (sRet == GSERROR_OK &&
        (WriteSurfaceBufferImpl(reply, retval.sequence, retval.buffer) != GSERROR_OK ||
        (retval.buffer != nullptr && !reply.WriteUint64(retval.buffer->GetBufferRequestConfig().usage)) ||
        bedataimpl->WriteToParcel(reply) != GSERROR_OK || !retval.fence->WriteToMessageParcel(reply) ||
        !reply.WriteUInt32Vector(retval.deletingBuffers))) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    } else if (sRet != GSERROR_OK && !reply.WriteBool(retval.isConnected)) {
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
    GSError sRet = RequestBuffers(config, bedataimpls, retvalues);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (sRet == GSERROR_OK || sRet == GSERROR_NO_BUFFER) {
        num = static_cast<uint32_t>(retvalues.size());
        if (!reply.WriteUint32(num)) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        for (uint32_t i = 0; i < num; ++i) {
            if (WriteSurfaceBufferImpl(reply, retvalues[i].sequence, retvalues[i].buffer) != GSERROR_OK ||
                (retvalues[i].buffer != nullptr &&
                    !reply.WriteUint64(retvalues[i].buffer->GetBufferRequestConfig().usage)) ||
                bedataimpls[i]->WriteToParcel(reply) != GSERROR_OK ||
                !retvalues[i].fence->WriteToMessageParcel(reply) ||
                !reply.WriteUInt32Vector(retvalues[i].deletingBuffers)) {
                return IPC_STUB_WRITE_PARCEL_ERR;
            }
        }
    } else if (sRet != GSERROR_OK && !reply.WriteBool(retvalues[0].isConnected)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetProducerInitInfoRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    ProducerInitInfo info;
    sptr<IRemoteObject> token = arguments.ReadRemoteObject();
    if (token == nullptr || !arguments.ReadString(info.appName)) {
        return ERR_INVALID_DATA;
    }
    sptr<IRemoteObject> listenerObject = arguments.ReadRemoteObject();
    if (listenerObject == nullptr) {
        return ERR_INVALID_DATA;
    }
    (void)GetProducerInitInfo(info);
    if (!reply.WriteInt32(info.width) || !reply.WriteInt32(info.height) || !reply.WriteUint64(info.uniqueId) ||
        !reply.WriteString(info.name) || !reply.WriteBool(info.isInHebcList) || !reply.WriteString(info.bufferName) ||
        !reply.WriteUint64(info.producerId) || !reply.WriteInt32(info.transformHint)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    sptr<IProducerListener> listener = iface_cast<IProducerListener>(listenerObject);
    GSError sRet = RegisterPropertyListener(listener, info.producerId);
    bool result = HandleDeathRecipient(token);
    if (!reply.WriteInt32((result && sRet == GSERROR_OK) ? GSERROR_OK : SURFACE_ERROR_UNKOWN)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
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

    GSError sRet = CancelBuffer(sequence, bedataimpl);
    if (!reply.WriteInt32(sRet)) {
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
    int32_t connectedPid = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connectedPid = connectedPid_;
    }
    isActiveGame = Rosen::FrameReport::GetInstance().IsActiveGameWithPid(connectedPid);
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

    GSError sRet = FlushBuffer(sequence, bedataimpl, fence, config);
    if (!reply.WriteInt32(sRet)) {
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

    GSError sRet = FlushBuffers(sequences, bedataimpls, fences, configs);
    if (!reply.WriteInt32(sRet)) {
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
    GSError sRet = GetLastFlushedBuffer(buffer, fence, matrix, isUseNewMatrix);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (sRet == GSERROR_OK) {
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

int32_t BufferQueueProducer::AttachBufferToQueueReadBuffer(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option, sptr<SurfaceBuffer> &buffer)
{
    uint32_t sequence;
    GSError sRet = ReadSurfaceBufferImpl(arguments, sequence, buffer);
    if (sRet != GSERROR_OK || buffer == nullptr) {
        if (!reply.WriteInt32(SURFACE_ERROR_UNKOWN)) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        return ERR_INVALID_DATA;
    }
    sRet = buffer->ReadBufferRequestConfig(arguments);
    if (sRet != GSERROR_OK) {
        if (!reply.WriteInt32(SURFACE_ERROR_UNKOWN)) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        return ERR_INVALID_DATA;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::AttachBufferToQueueRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    sptr<SurfaceBuffer> buffer = nullptr;
    auto ret = AttachBufferToQueueReadBuffer(arguments, reply, option, buffer);
    if (ret != ERR_NONE) {
        return ret;
    }
    GSError sRet = AttachBufferToQueue(buffer);
    if (!reply.WriteInt32(sRet)) {
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
    GSError sRet = SetQueueSize(queueSize);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetNameRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    std::string name;
    auto sRet = bufferQueue_->GetName(name);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (sRet == GSERROR_OK && !reply.WriteString(name)) {
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
    GSError sRet = SetDefaultUsage(usage);
    if (!reply.WriteInt32(sRet)) {
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
    uint32_t bufSeqNum = 0;
    GSError result = CleanCache(cleanAll, &bufSeqNum);
    if (!reply.WriteInt32(result)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (result == GSERROR_OK && !reply.WriteUint32(bufSeqNum)) {
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
    GSError sRet = RegisterReleaseListener(listener);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::RegisterPropertyListenerRemote(MessageParcel &arguments,
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
    int64_t id = -1;
    if (!arguments.ReadInt64(id)) {
        return ERR_INVALID_REPLY;
    }
    auto pid = static_cast<pid_t>(id);
    GSError sRet = RegisterPropertyListener(listener, pid);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::UnRegisterPropertyListenerRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    int64_t pid = -1;
    if (!arguments.ReadInt64(pid)) {
        return ERR_INVALID_REPLY;
    }
    auto id = static_cast<pid_t>(pid);
    GSError sRet = UnRegisterPropertyListener(id);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetLppShareFdRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    int32_t fd = arguments.ReadFileDescriptor();
    if (fd < 0) {
        reply.WriteInt32(GSERROR_INVALID_ARGUMENTS);
        return ERR_INVALID_VALUE;
    }
    bool state = arguments.ReadBool();
    GSError sRet = SetLppShareFd(fd, state);
    if (!reply.WriteInt32(sRet)) {
        close(fd);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    close(fd);
    return ERR_NONE;
}

int32_t BufferQueueProducer::RegisterReleaseListenerBackupRemote(MessageParcel &arguments,
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
    GSError sRet = RegisterReleaseListenerBackup(listener);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::UnRegisterReleaseListenerRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    GSError sRet = UnRegisterReleaseListener();
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::UnRegisterReleaseListenerBackupRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    GSError sRet = UnRegisterReleaseListenerBackup();
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetTransformRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    GraphicTransformType transform = static_cast<GraphicTransformType>(arguments.ReadUint32());
    GSError sRet = SetTransform(transform);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::ConnectRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    GSError sRet = Connect();
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::DisconnectRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    uint32_t bufSeqNum = 0;
    GSError sRet = Disconnect(&bufSeqNum);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    if (sRet == GSERROR_OK && !reply.WriteUint32(bufSeqNum)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::ConnectStrictlyRemote(MessageParcel &arguments, MessageParcel &reply,
                                                   MessageOption &option)
{
    GSError sRet = ConnectStrictly();
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::DisconnectStrictlyRemote(MessageParcel &arguments, MessageParcel &reply,
                                                      MessageOption &option)
{
    GSError sRet = DisconnectStrictly();
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetScalingModeRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    uint32_t sequence = arguments.ReadUint32();
    ScalingMode scalingMode = static_cast<ScalingMode>(arguments.ReadInt32());
    GSError sRet = SetScalingMode(sequence, scalingMode);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetScalingModeV2Remote(MessageParcel &arguments, MessageParcel &reply,
                                                    MessageOption &option)
{
    ScalingMode scalingMode = static_cast<ScalingMode>(arguments.ReadInt32());
    GSError sRet = SetScalingMode(scalingMode);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetBufferHoldRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    bool hold = arguments.ReadBool();
    GSError sRet = SetBufferHold(hold);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetBufferReallocFlagRemote(MessageParcel &arguments, MessageParcel &reply,
                                                        MessageOption &option)
{
    bool flag = arguments.ReadBool();
    GSError sRet = SetBufferReallocFlag(flag);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetBufferNameRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    std::string bufferName = arguments.ReadString();
    GSError sRet = SetBufferName(bufferName);
    if (!reply.WriteInt32(sRet)) {
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
    GSError sRet = SetMetaData(sequence, metaData);
    if (!reply.WriteInt32(sRet)) {
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
    GSError sRet = SetMetaDataSet(sequence, key, metaData);
    if (!reply.WriteInt32(sRet)) {
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
    GSError sRet = SetTunnelHandle(handle);
    if (!reply.WriteInt32(sRet)) {
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
    GSError sRet = GetPresentTimestamp(sequence, type, time);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (sRet == GSERROR_OK && !reply.WriteInt64(time)) {
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
    uint32_t transformId = -1;
    if (!arguments.ReadUint32(transformId)) {
        return ERR_INVALID_REPLY;
    }
    GraphicTransformType transformHint = static_cast<GraphicTransformType>(transformId);
    uint64_t id = -1;
    if (!arguments.ReadUint64(id)) {
        return ERR_INVALID_REPLY;
    }
    GSError sRet = SetTransformHint(transformHint, id);
    if (!reply.WriteInt32(sRet)) {
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
    GSError sRet = SetSurfaceSourceType(sourceType);
    if (!reply.WriteInt32(sRet)) {
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
    GSError sRet = SetSurfaceAppFrameworkType(appFrameworkType);
    if (!reply.WriteInt32(sRet)) {
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
    GSError sRet = SetHdrWhitePointBrightness(brightness);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetSdrWhitePointBrightnessRemote(
    MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    float brightness = arguments.ReadFloat();
    GSError sRet = SetSdrWhitePointBrightness(brightness);
    if (!reply.WriteInt32(sRet)) {
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
    GSError sRet = AcquireLastFlushedBuffer(buffer, fence, matrix, BUFFER_MATRIX_SIZE, isUseNewMatrix);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (sRet == GSERROR_OK) {
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
    GSError sRet = ReleaseLastFlushedBuffer(sequence);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetGlobalAlphaRemote(MessageParcel &arguments, MessageParcel &reply, MessageOption &option)
{
    int32_t alpha = arguments.ReadInt32();
    GSError sRet = SetGlobalAlpha(alpha);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::SetRequestBufferNoblockModeRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    bool noblock = arguments.ReadBool();
    GSError sRet = SetRequestBufferNoblockMode(noblock);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::RequestAndDetachBufferRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    RequestBufferReturnValue retval;
    sptr<BufferExtraData> bedataimpl = new BufferExtraDataImpl;
    BufferRequestConfig config = {};
    ReadRequestConfig(arguments, config);

    GSError sRet = RequestAndDetachBuffer(config, bedataimpl, retval);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (sRet == GSERROR_OK &&
        (WriteSurfaceBufferImpl(reply, retval.sequence, retval.buffer) != GSERROR_OK ||
        (retval.buffer != nullptr && !reply.WriteUint64(retval.buffer->GetBufferRequestConfig().usage)) ||
        bedataimpl->WriteToParcel(reply) != GSERROR_OK || !retval.fence->WriteToMessageParcel(reply) ||
        !reply.WriteUInt32Vector(retval.deletingBuffers))) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    } else if (sRet != GSERROR_OK && !reply.WriteBool(retval.isConnected)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::AttachAndFlushBufferRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    sptr<SurfaceBuffer> buffer = nullptr;
    auto ret = AttachBufferToQueueReadBuffer(arguments, reply, option, buffer);
    if (ret != ERR_NONE) {
        return ret;
    }
    BufferFlushConfigWithDamages config;
    sptr<BufferExtraData> bedataimpl = new BufferExtraDataImpl;
    if (bedataimpl->ReadFromParcel(arguments) != GSERROR_OK) {
        return ERR_INVALID_REPLY;
    }

    sptr<SyncFence> fence = SyncFence::ReadFromMessageParcel(arguments);
    if (ReadFlushConfig(arguments, config) != GSERROR_OK) {
        return ERR_INVALID_REPLY;
    }
    bool needMap = arguments.ReadBool();
    GSError sRet = AttachAndFlushBuffer(buffer, bedataimpl, fence, config, needMap);
    if (!reply.WriteInt32(sRet)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t BufferQueueProducer::GetRotatingBuffersNumberRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    uint32_t cycleBuffersNumber = 0;
    auto ret = GetCycleBuffersNumber(cycleBuffersNumber);
    if (ret != GSERROR_OK) {
        if (!reply.WriteInt32(static_cast<int32_t>(ret))) {
            return IPC_STUB_WRITE_PARCEL_ERR;
        }
        return ERR_INVALID_REPLY;
    }

    if (!reply.WriteInt32(GSERROR_OK) || !reply.WriteUint32(cycleBuffersNumber)) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t BufferQueueProducer::SetRotatingBuffersNumberRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    uint32_t cycleBuffersNumber = arguments.ReadUint32();
    auto ret = SetCycleBuffersNumber(cycleBuffersNumber);
    if (!reply.WriteInt32(static_cast<int32_t>(ret))) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t BufferQueueProducer::SetFrameGravityRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    int32_t frameGravity = arguments.ReadInt32();
    auto ret = SetFrameGravity(frameGravity);
    if (!reply.WriteInt32(static_cast<int32_t>(ret))) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t BufferQueueProducer::SetFixedRotationRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    int32_t fixedRotation = arguments.ReadInt32();
    auto ret = SetFixedRotation(fixedRotation);
    if (!reply.WriteInt32(static_cast<int32_t>(ret))) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t BufferQueueProducer::SetAlphaTypeRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    GraphicAlphaType alphaType = static_cast<GraphicAlphaType>(arguments.ReadInt32());
    auto ret = SetAlphaType(alphaType);
    if (!reply.WriteInt32(static_cast<int32_t>(ret))) {
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

GSError BufferQueueProducer::RequestAndDetachBuffer(const BufferRequestConfig& config, sptr<BufferExtraData>& bedata,
    RequestBufferReturnValue& retval)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }

    retval.isConnected = false;
    auto ret = Connect();
    if (ret != SURFACE_ERROR_OK) {
        return ret;
    }
    retval.isConnected = true;
    return bufferQueue_->RequestAndDetachBuffer(config, bedata, retval);
}

GSError BufferQueueProducer::AttachAndFlushBuffer(sptr<SurfaceBuffer>& buffer, sptr<BufferExtraData>& bedata,
    const sptr<SyncFence>& fence, BufferFlushConfigWithDamages& config, bool needMap)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    if (isDisconnectStrictly_) {
        BLOGW("connected failed because buffer queue is disconnect strictly, uniqueId: %{public}" PRIu64 ".",
            uniqueId_);
        return GSERROR_CONSUMER_DISCONNECTED;
    }
    return bufferQueue_->AttachAndFlushBuffer(buffer, bedata, fence, config, needMap);
}

int32_t BufferQueueProducer::PreAllocBuffersRemote(MessageParcel &arguments,
    MessageParcel &reply, MessageOption &option)
{
    BufferRequestConfig config = {};
    ReadRequestConfig(arguments, config);
    uint32_t allocBufferCount = arguments.ReadUint32();
    GSError sRet = PreAllocBuffers(config, allocBufferCount);
    if (sRet != GSERROR_OK) {
        BLOGE("PreAllocBuffers failed, width: %{public}d, height: %{public}d, format: %{public}d, usage: \
            %{public}" PRIu64 ", allocBufferCount: %{public}u, sRet: %{public}d, uniqueId: %{public}" PRIu64 " ",
            config.width, config.height, config.format, config.usage, allocBufferCount, sRet, uniqueId_);
        return sRet;
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

    retval.isConnected = false;
    auto ret = Connect();
    if (ret != SURFACE_ERROR_OK) {
        return ret;
    }
    retval.isConnected = true;
    return bufferQueue_->RequestBuffer(config, bedata, retval);
}

GSError BufferQueueProducer::RequestBuffers(const BufferRequestConfig &config,
    std::vector<sptr<BufferExtraData>> &bedata, std::vector<RequestBufferReturnValue> &retvalues)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    retvalues[0].isConnected = false;
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
        retvalues.resize(1);
        retvalues[0].isConnected = true;
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
    if (isDisconnectStrictly_) {
        BLOGW("connected failed because buffer queue is disconnect strictly, uniqueId: %{public}" PRIu64 ".",
            uniqueId_);
        return GSERROR_CONSUMER_DISCONNECTED;
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
    if (isDisconnectStrictly_) {
        BLOGW("connected failed because buffer queue is disconnect strictly, uniqueId: %{public}" PRIu64 ".",
            uniqueId_);
        return GSERROR_CONSUMER_DISCONNECTED;
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
    return bufferQueue_->DetachBufferFromQueue(buffer, InvokerType::PRODUCER_INVOKER, false);
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

GSError BufferQueueProducer::CleanCache(bool cleanAll, uint32_t* bufSeqNum)
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

    return bufferQueue_->CleanCache(cleanAll, bufSeqNum);
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

GSError BufferQueueProducer::RegisterPropertyListener(sptr<IProducerListener> listener, uint64_t producerId)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->RegisterProducerPropertyListener(listener, producerId);
}

GSError BufferQueueProducer::UnRegisterPropertyListener(uint64_t producerId)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->UnRegisterProducerPropertyListener(producerId);
}

GSError BufferQueueProducer::RegisterReleaseListener(sptr<IProducerListener> listener)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->RegisterProducerReleaseListener(listener);
}

GSError BufferQueueProducer::RegisterReleaseListenerBackup(sptr<IProducerListener> listener)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->RegisterProducerReleaseListenerBackup(listener);
}

GSError BufferQueueProducer::UnRegisterReleaseListener()
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->UnRegisterProducerReleaseListener();
}

GSError BufferQueueProducer::UnRegisterReleaseListenerBackup()
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->UnRegisterProducerReleaseListenerBackup();
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

GSError BufferQueueProducer::SetTransformHint(GraphicTransformType transformHint, uint64_t fromId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->SetTransformHint(transformHint, fromId);
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
    if (isDisconnectStrictly_) {
        BLOGW("connected failed because buffer queue is disconnect strictly, uniqueId: %{public}" PRIu64 ".",
            uniqueId_);
        return GSERROR_CONSUMER_DISCONNECTED;
    }
    SetConnectedPidLocked(callingPid);
    return SURFACE_ERROR_OK;
}

GSError BufferQueueProducer::Disconnect(uint32_t* bufSeqNum)
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
        SetConnectedPidLocked(0);
    }
    return bufferQueue_->CleanCache(false, bufSeqNum);
}

GSError BufferQueueProducer::ConnectStrictly()
{
    std::lock_guard<std::mutex> lock(mutex_);
    isDisconnectStrictly_ = false;
    return SURFACE_ERROR_OK;
}

GSError BufferQueueProducer::DisconnectStrictly()
{
    std::lock_guard<std::mutex> lock(mutex_);
    isDisconnectStrictly_ = true;
    return SURFACE_ERROR_OK;
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

GSError BufferQueueProducer::SetBufferReallocFlag(bool flag)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->SetBufferReallocFlag(flag);
}

GSError BufferQueueProducer::SetBufferName(const std::string &bufferName)
{
    if (bufferQueue_ == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    return bufferQueue_->SetBufferName(bufferName);
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

GSError BufferQueueProducer::SetRequestBufferNoblockMode(bool noblock)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->SetRequestBufferNoblockMode(noblock);
}

GSError BufferQueueProducer::PreAllocBuffers(const BufferRequestConfig &config, uint32_t allocBufferCount)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->PreAllocBuffers(config, allocBufferCount);
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
        SetConnectedPidLocked(0);
    }
    bufferQueue_->CleanCache(false, nullptr);
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
        BLOGW("producer is nullptr");
        return;
    }

    if (producer->token_ != remoteToken) {
        BLOGW("token doesn't match, ignore it, uniqueId: %{public}" PRIu64 ".", producer->GetUniqueId());
        return;
    }
    BLOGD("remote object died, uniqueId: %{public}" PRIu64 ".", producer->GetUniqueId());
    producer->OnBufferProducerRemoteDied();
}

void BufferQueueProducer::SetConnectedPidLocked(int32_t connectedPid)
{
    connectedPid_ = connectedPid;
    if (bufferQueue_) {
        bufferQueue_->SetConnectedPidLocked(connectedPid_);
    }
}

bool BufferQueueProducer::CheckIsAlive()
{
    static const bool isBeta = system::GetParameter("const.logsystem.versiontype", "") == "beta";
    if (magicNum_ != MAGIC_INIT) {
        BLOGE("report to HiViewOcean magicNum %{public}d", magicNum_);
        if (isBeta) {
            raise(42);  // 42 : report to HiViewOcean
        }
        return false;
    }
    return true;
}

GSError BufferQueueProducer::GetCycleBuffersNumber(uint32_t& cycleBuffersNumber)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->GetCycleBuffersNumber(cycleBuffersNumber);
}

GSError BufferQueueProducer::SetCycleBuffersNumber(uint32_t cycleBuffersNumber)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->SetCycleBuffersNumber(cycleBuffersNumber);
}

GSError BufferQueueProducer::SetFrameGravity(int32_t frameGravity)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->SetFrameGravity(frameGravity);
}

GSError BufferQueueProducer::SetFixedRotation(int32_t fixedRotation)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->SetFixedRotation(fixedRotation);
}

GSError BufferQueueProducer::SetLppShareFd(int fd, bool state)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->SetLppShareFd(fd, state);
}

GSError BufferQueueProducer::SetAlphaType(GraphicAlphaType alphaType)
{
    if (bufferQueue_ == nullptr) {
        return SURFACE_ERROR_UNKOWN;
    }
    return bufferQueue_->SetAlphaType(alphaType);
}
}; // namespace OHOS
