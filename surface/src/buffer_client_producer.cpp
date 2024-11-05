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

#include "buffer_client_producer.h"

#include <cinttypes>

#include <iremote_stub.h>
#include "buffer_log.h"
#include "buffer_utils.h"
#include "sync_fence.h"
#include "message_option.h"
#include "securec.h"
#include "rs_frame_report_ext.h"

#define DEFINE_MESSAGE_VARIABLES(arg, ret, opt)                            \
    MessageOption opt;                                                     \
    MessageParcel arg;                                                     \
    MessageParcel ret;                                                     \
    do {                                                                   \
        GSError retCode = MessageVariables(arg);                           \
        if (retCode != GSERROR_OK) {                                       \
            return retCode;                                                \
        }                                                                  \
    } while (0)

#define SEND_REQUEST(COMMAND, arguments, reply, option)                    \
    do {                                                                   \
        GSError ret = SendRequest(COMMAND, arguments, reply, option);      \
        if (ret != GSERROR_OK) {                                           \
            return ret;                                                    \
        }                                                                  \
    } while (0)

namespace OHOS {
namespace {
    constexpr size_t MATRIX4_SIZE = 16;
}
BufferClientProducer::BufferClientProducer(const sptr<IRemoteObject>& impl)
    : IRemoteProxy<IBufferProducer>(impl)
{
}

BufferClientProducer::~BufferClientProducer()
{
}

GSError BufferClientProducer::MessageVariables(MessageParcel &arg)
{
    if (!(arg).WriteInterfaceToken(GetDescriptor())) {
        BLOGE("WriteInterfaceToken failed, uniqueId: %{public}" PRIu64 ".", uniqueId_);
        return GSERROR_BINDER;
    }
    return GSERROR_OK;
}

GSError BufferClientProducer::SendRequest(uint32_t command, MessageParcel &arg,
                                          MessageParcel &reply, MessageOption &opt)
{
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        BLOGE("Remote is nullptr!");
        return GSERROR_SERVER_ERROR;
    }
    int32_t ret = remote->SendRequest(command, arg, reply, opt);
    if (ret != ERR_NONE) {
        BLOGE("SendRequest ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, uniqueId_);
        return GSERROR_BINDER;
    }
    return GSERROR_OK;
}

GSError BufferClientProducer::CheckRetval(MessageParcel &reply)
{
    int32_t ret = reply.ReadInt32();
    if (ret != GSERROR_OK) {
        BLOGE("Remote ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, uniqueId_);
        return static_cast<GSError>(ret);
    }
    return GSERROR_OK;
}

GSError BufferClientProducer::RequestBuffer(const BufferRequestConfig &config, sptr<BufferExtraData> &bedata,
                                            RequestBufferReturnValue &retval)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    WriteRequestConfig(arguments, config);

    retval.isConnected = false;
    SEND_REQUEST(BUFFER_PRODUCER_REQUEST_BUFFER, arguments, reply, option);
    GSError ret = CheckRetval(reply);
    if (ret != GSERROR_OK) {
        reply.ReadBool(retval.isConnected);
        return ret;
    }

    ret = ReadSurfaceBufferImpl(reply, retval.sequence, retval.buffer);
    if (ret != GSERROR_OK) {
        return SURFACE_ERROR_UNKOWN;
    }
    if (retval.buffer != nullptr) {
        retval.buffer->SetBufferRequestConfig(config);
    }

    ret = bedata->ReadFromParcel(reply);
    if (ret != GSERROR_OK) {
        return SURFACE_ERROR_UNKOWN;
    }
    retval.fence = SyncFence::ReadFromMessageParcel(reply);
    reply.ReadUInt32Vector(&retval.deletingBuffers);

    return GSERROR_OK;
}

GSError BufferClientProducer::RequestBuffers(const BufferRequestConfig &config,
    std::vector<sptr<BufferExtraData>> &bedata, std::vector<RequestBufferReturnValue> &retvalues)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    uint32_t num = static_cast<uint32_t>(bedata.size());
    arguments.WriteUint32(num);
    WriteRequestConfig(arguments, config);
    retvalues[0].isConnected = false;
    SEND_REQUEST(BUFFER_PRODUCER_REQUEST_BUFFERS, arguments, reply, option);
    GSError ret = CheckRetval(reply);
    if (ret != GSERROR_OK && ret != GSERROR_NO_BUFFER) {
        reply.ReadBool(retvalues[0].isConnected);
        return ret;
    }

    num = reply.ReadUint32();
    if (num > SURFACE_MAX_QUEUE_SIZE || num == 0) {
        BLOGE("num is invalid, %{public}u, uniqueId: %{public}" PRIu64 ".", num, uniqueId_);
        return SURFACE_ERROR_UNKOWN;
    }

    ret = GSERROR_OK;
    retvalues.resize(num);
    for (size_t i = 0; i < num; ++i) {
        auto &retval = retvalues[i];
        ret = ReadSurfaceBufferImpl(reply, retval.sequence, retval.buffer);
        if (ret != GSERROR_OK) {
            return SURFACE_ERROR_UNKOWN;
        }
        if (retval.buffer != nullptr) {
            retval.buffer->SetBufferRequestConfig(config);
        }
        ret = bedata[i]->ReadFromParcel(reply);
        if (ret != GSERROR_OK) {
            return SURFACE_ERROR_UNKOWN;
        }
        retval.fence = SyncFence::ReadFromMessageParcel(reply);
        reply.ReadUInt32Vector(&retval.deletingBuffers);
    }
    return ret;
}

GSError BufferClientProducer::GetLastFlushedBufferCommon(sptr<SurfaceBuffer>& buffer,
    sptr<SyncFence>& fence, float matrix[16], uint32_t matrixSize, bool isUseNewMatrix, uint32_t command)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    arguments.WriteBool(isUseNewMatrix);
    SEND_REQUEST(command, arguments, reply, option);
    GSError ret = CheckRetval(reply);
    if (ret != GSERROR_OK) {
        return ret;
    }
    uint32_t sequence;
    ret = ReadSurfaceBufferImpl(reply, sequence, buffer);
    if (ret != GSERROR_OK) {
        return SURFACE_ERROR_UNKOWN;
    }
    ret = buffer->ReadBufferRequestConfig(reply);
    if (ret != GSERROR_OK) {
        return SURFACE_ERROR_UNKOWN;
    }

    fence = SyncFence::ReadFromMessageParcel(reply);
    std::vector<float> readMatrixVector;
    reply.ReadFloatVector(&readMatrixVector);
    if (memcpy_s(matrix, matrixSize * sizeof(float),
        readMatrixVector.data(), readMatrixVector.size() * sizeof(float)) != EOK) {
        BLOGE("memcpy_s fail, uniqueId: %{public}" PRIu64 ".", uniqueId_);
        return SURFACE_ERROR_UNKOWN;
    }
    return GSERROR_OK;
}

GSError BufferClientProducer::GetLastFlushedBuffer(sptr<SurfaceBuffer>& buffer,
    sptr<SyncFence>& fence, float matrix[16], bool isUseNewMatrix)
{
    return GetLastFlushedBufferCommon(buffer, fence,
        matrix, MATRIX4_SIZE, isUseNewMatrix, BUFFER_PRODUCER_GET_LAST_FLUSHED_BUFFER);
}

GSError BufferClientProducer::GetProducerInitInfo(ProducerInitInfo &info)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    token_ = new IRemoteStub<IBufferProducerToken>();
    arguments.WriteRemoteObject(token_->AsObject());
    SEND_REQUEST(BUFFER_PRODUCER_GET_INIT_INFO, arguments, reply, option);
    reply.ReadInt32(info.width);
    reply.ReadInt32(info.height);
    reply.ReadUint64(info.uniqueId);
    uniqueId_ = info.uniqueId;
    reply.ReadString(info.name);
    return CheckRetval(reply);
}

GSError BufferClientProducer::CancelBuffer(uint32_t sequence, sptr<BufferExtraData> bedata)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    arguments.WriteUint32(sequence);
    bedata->WriteToParcel(arguments);

    SEND_REQUEST(BUFFER_PRODUCER_CANCEL_BUFFER, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::FlushBuffer(uint32_t sequence, sptr<BufferExtraData> bedata,
                                          sptr<SyncFence> fence, BufferFlushConfigWithDamages &config)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    arguments.WriteUint32(sequence);
    bedata->WriteToParcel(arguments);
    fence->WriteToMessageParcel(arguments);
    WriteFlushConfig(arguments, config);

    SEND_REQUEST(BUFFER_PRODUCER_FLUSH_BUFFER, arguments, reply, option);
    GSError ret = CheckRetval(reply);
    if (ret != GSERROR_OK) {
        return ret;
    }

    if (OHOS::RsFrameReportExt::GetInstance().GetEnable()) {
        OHOS::RsFrameReportExt::GetInstance().HandleSwapBuffer();
    }
    return GSERROR_OK;
}

GSError BufferClientProducer::FlushBuffers(const std::vector<uint32_t> &sequences,
    const std::vector<sptr<BufferExtraData>> &bedata,
    const std::vector<sptr<SyncFence>> &fences,
    const std::vector<BufferFlushConfigWithDamages> &configs)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    if (sequences.size() <= 0 || sequences.size() > SURFACE_MAX_QUEUE_SIZE) {
        return SURFACE_ERROR_UNKOWN;
    }
    arguments.WriteUInt32Vector(sequences);
    for (uint32_t i = 0; i < sequences.size(); ++i) {
        bedata[i]->WriteToParcel(arguments);
        fences[i]->WriteToMessageParcel(arguments);
        WriteFlushConfig(arguments, configs[i]);
    }
    SEND_REQUEST(BUFFER_PRODUCER_FLUSH_BUFFERS, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::AttachBufferToQueue(sptr<SurfaceBuffer> buffer)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    uint32_t sequence = buffer->GetSeqNum();
    WriteSurfaceBufferImpl(arguments, sequence, buffer);
    GSError ret = buffer->WriteBufferRequestConfig(arguments);
    if (ret != GSERROR_OK) {
        BLOGE("WriteBufferRequestConfig ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, uniqueId_);
        return ret;
    }
    SEND_REQUEST(BUFFER_PRODUCER_ATTACH_BUFFER_TO_QUEUE, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::DetachBufferFromQueue(sptr<SurfaceBuffer> buffer)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    uint32_t sequence = buffer->GetSeqNum();
    WriteSurfaceBufferImpl(arguments, sequence, buffer);
    SEND_REQUEST(BUFFER_PRODUCER_DETACH_BUFFER_FROM_QUEUE, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::AttachBuffer(sptr<SurfaceBuffer>& buffer)
{
    return GSERROR_NOT_SUPPORT;
}

GSError BufferClientProducer::AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    uint32_t sequence = buffer->GetSeqNum();
    WriteSurfaceBufferImpl(arguments, sequence, buffer);
    arguments.WriteInt32(timeOut);
    SEND_REQUEST(BUFFER_PRODUCER_ATTACH_BUFFER, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::DetachBuffer(sptr<SurfaceBuffer>& buffer)
{
    return GSERROR_NOT_SUPPORT;
}

GSError BufferClientProducer::RegisterReleaseListener(sptr<IProducerListener> listener)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    arguments.WriteRemoteObject(listener->AsObject());

    SEND_REQUEST(BUFFER_PRODUCER_REGISTER_RELEASE_LISTENER, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::UnRegisterReleaseListener()
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    SEND_REQUEST(BUFFER_PRODUCER_UNREGISTER_RELEASE_LISTENER, arguments, reply, option);
    return CheckRetval(reply);
}

uint32_t BufferClientProducer::GetQueueSize()
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    SEND_REQUEST(BUFFER_PRODUCER_GET_QUEUE_SIZE, arguments, reply, option);

    return reply.ReadUint32();
}

GSError BufferClientProducer::SetQueueSize(uint32_t queueSize)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    arguments.WriteUint32(queueSize);

    SEND_REQUEST(BUFFER_PRODUCER_SET_QUEUE_SIZE, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::GetName(std::string &name)
{
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        if (name_ != "not init") {
            name = name_;
            return GSERROR_OK;
        }
    }
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    SEND_REQUEST(BUFFER_PRODUCER_GET_NAME, arguments, reply, option);
    GSError ret = CheckRetval(reply);
    if (ret != GSERROR_OK) {
        return ret;
    }
    if (reply.ReadString(name) == false) {
        BLOGE("reply.ReadString return false, uniqueId: %{public}" PRIu64 ".", uniqueId_);
        return GSERROR_BINDER;
    }
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        name_ = name;
    }
    return ret;
}

uint64_t BufferClientProducer::GetUniqueId()
{
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        if (uniqueId_ != 0) {
            return uniqueId_;
        }
    }
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    SEND_REQUEST(BUFFER_PRODUCER_GET_UNIQUE_ID, arguments, reply, option);
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        uniqueId_ = reply.ReadUint64();
        return uniqueId_;
    }
}

GSError BufferClientProducer::GetNameAndUniqueId(std::string& name, uint64_t& uniqueId)
{
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        if (uniqueId_ != 0 && name_ != "not init") {
            uniqueId = uniqueId_;
            name = name_;
            return GSERROR_OK;
        }
    }
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    SEND_REQUEST(BUFFER_PRODUCER_GET_NAMEANDUNIQUEDID, arguments, reply, option);
    GSError ret = CheckRetval(reply);
    if (ret != GSERROR_OK) {
        return ret;
    }
    if (reply.ReadString(name) == false) {
        BLOGE("reply.ReadString return false, uniqueId: %{public}" PRIu64 ".", uniqueId_);
        return GSERROR_BINDER;
    }

    uniqueId = reply.ReadUint64();
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        name_ = name;
        uniqueId_ = uniqueId;
    }
    return ret;
}

int32_t BufferClientProducer::GetDefaultWidth()
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    SEND_REQUEST(BUFFER_PRODUCER_GET_DEFAULT_WIDTH, arguments, reply, option);

    return reply.ReadInt32();
}

int32_t BufferClientProducer::GetDefaultHeight()
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    SEND_REQUEST(BUFFER_PRODUCER_GET_DEFAULT_HEIGHT, arguments, reply, option);

    return reply.ReadInt32();
}

GSError BufferClientProducer::SetDefaultUsage(uint64_t usage)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    arguments.WriteUint64(usage);

    SEND_REQUEST(BUFFER_PRODUCER_SET_DEFAULT_USAGE, arguments, reply, option);

    return CheckRetval(reply);
}

uint64_t BufferClientProducer::GetDefaultUsage()
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    SEND_REQUEST(BUFFER_PRODUCER_GET_DEFAULT_USAGE, arguments, reply, option);

    return reply.ReadUint64();
}

GSError BufferClientProducer::CleanCache(bool cleanAll)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    arguments.WriteBool(cleanAll);
    SEND_REQUEST(BUFFER_PRODUCER_CLEAN_CACHE, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::GoBackground()
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    SEND_REQUEST(BUFFER_PRODUCER_GO_BACKGROUND, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::SetTransform(GraphicTransformType transform)
{
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        if (lastSetTransformType_ == transform) {
            return GSERROR_OK;
        }
        lastSetTransformType_ = transform;
    }

    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    arguments.WriteUint32(static_cast<uint32_t>(transform));

    SEND_REQUEST(BUFFER_PRODUCER_SET_TRANSFORM, arguments, reply, option);
    GSError ret = CheckRetval(reply);
    if (ret != GSERROR_OK) {
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);
            lastSetTransformType_ = GraphicTransformType::GRAPHIC_ROTATE_BUTT;
        }
        return ret;
    }

    return GSERROR_OK;
}

GSError BufferClientProducer::IsSupportedAlloc(const std::vector<BufferVerifyAllocInfo> &infos,
                                               std::vector<bool> &supporteds)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    WriteVerifyAllocInfo(arguments, infos);

    SEND_REQUEST(BUFFER_PRODUCER_IS_SUPPORTED_ALLOC, arguments, reply, option);
    GSError ret = CheckRetval(reply);
    if (ret != GSERROR_OK) {
        return ret;
    }

    if (reply.ReadBoolVector(&supporteds) == false) {
        BLOGE("reply.ReadBoolVector return false, uniqueId: %{public}" PRIu64 ".", uniqueId_);
        return GSERROR_BINDER;
    }

    return static_cast<GSError>(ret);
}

GSError BufferClientProducer::Connect()
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    SEND_REQUEST(BUFFER_PRODUCER_CONNECT, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::Disconnect()
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    SEND_REQUEST(BUFFER_PRODUCER_DISCONNECT, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::SetScalingMode(uint32_t sequence, ScalingMode scalingMode)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    arguments.WriteUint32(sequence);
    arguments.WriteInt32(static_cast<int32_t>(scalingMode));
    SEND_REQUEST(BUFFER_PRODUCER_SET_SCALING_MODE, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::SetScalingMode(ScalingMode scalingMode)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    arguments.WriteInt32(static_cast<int32_t>(scalingMode));
    SEND_REQUEST(BUFFER_PRODUCER_SET_SCALING_MODEV2, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::SetBufferHold(bool hold)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    arguments.WriteBool(hold);
    SEND_REQUEST(BUFFER_PRODUCER_SET_BUFFER_HOLD, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    arguments.WriteUint32(sequence);
    WriteHDRMetaData(arguments, metaData);
    SEND_REQUEST(BUFFER_PRODUCER_SET_METADATA, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key,
                                             const std::vector<uint8_t> &metaData)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    arguments.WriteUint32(sequence);
    arguments.WriteUint32(static_cast<uint32_t>(key));
    WriteHDRMetaDataSet(arguments, metaData);
    SEND_REQUEST(BUFFER_PRODUCER_SET_METADATASET, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::SetTunnelHandle(const GraphicExtDataHandle *handle)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    if (handle == nullptr) {
        arguments.WriteBool(false);
    } else {
        arguments.WriteBool(true);
        WriteExtDataHandle(arguments, handle);
    }
    SEND_REQUEST(BUFFER_PRODUCER_SET_TUNNEL_HANDLE, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::GetPresentTimestamp(uint32_t sequence, GraphicPresentTimestampType type, int64_t &time)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    arguments.WriteUint32(sequence);
    arguments.WriteUint32(static_cast<uint32_t>(type));
    SEND_REQUEST(BUFFER_PRODUCER_GET_PRESENT_TIMESTAMP, arguments, reply, option);
    GSError ret = CheckRetval(reply);
    if (ret != GSERROR_OK) {
        return ret;
    }
    time = reply.ReadInt64();
    return static_cast<GSError>(ret);
}

sptr<NativeSurface> BufferClientProducer::GetNativeSurface()
{
    return nullptr;
}

GSError BufferClientProducer::GetTransform(GraphicTransformType &transform)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    SEND_REQUEST(BUFFER_PRODUCER_GET_TRANSFORM, arguments, reply, option);

    GSError ret = CheckRetval(reply);
    if (ret != GSERROR_OK) {
        return ret;
    }
    transform = static_cast<GraphicTransformType>(reply.ReadUint32());
    return GSERROR_OK;
}

GSError BufferClientProducer::GetTransformHint(GraphicTransformType &transformHint)
{
    return GSERROR_NOT_SUPPORT;
}

GSError BufferClientProducer::SetTransformHint(GraphicTransformType transformHint)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    arguments.WriteUint32(static_cast<uint32_t>(transformHint));

    SEND_REQUEST(BUFFER_PRODUCER_SET_TRANSFORMHINT, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::SetSurfaceSourceType(OHSurfaceSource sourceType)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    arguments.WriteUint32(static_cast<uint32_t>(sourceType));
    SEND_REQUEST(BUFFER_PRODUCER_SET_SOURCE_TYPE, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::GetSurfaceSourceType(OHSurfaceSource &sourceType)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    SEND_REQUEST(BUFFER_PRODUCER_GET_SOURCE_TYPE, arguments, reply, option);
    GSError ret = CheckRetval(reply);
    if (ret != GSERROR_OK) {
        return ret;
    }
    sourceType = static_cast<OHSurfaceSource>(reply.ReadUint32());
    return GSERROR_OK;
}

GSError BufferClientProducer::SetSurfaceAppFrameworkType(std::string appFrameworkType)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    arguments.WriteString(appFrameworkType);
    SEND_REQUEST(BUFFER_PRODUCER_SET_APP_FRAMEWORK_TYPE, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::GetSurfaceAppFrameworkType(std::string &appFrameworkType)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    SEND_REQUEST(BUFFER_PRODUCER_GET_APP_FRAMEWORK_TYPE, arguments, reply, option);
    GSError ret = CheckRetval(reply);
    if (ret != GSERROR_OK) {
        return ret;
    }
    appFrameworkType = static_cast<std::string>(reply.ReadString());
    return GSERROR_OK;
}

GSError BufferClientProducer::SetHdrWhitePointBrightness(float brightness)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    arguments.WriteFloat(brightness);

    SEND_REQUEST(BUFFER_PRODUCER_SET_HDRWHITEPOINTBRIGHTNESS, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::SetSdrWhitePointBrightness(float brightness)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);

    arguments.WriteFloat(brightness);

    SEND_REQUEST(BUFFER_PRODUCER_SET_SDRWHITEPOINTBRIGHTNESS, arguments, reply, option);
    return CheckRetval(reply);
}

GSError BufferClientProducer::AcquireLastFlushedBuffer(sptr<SurfaceBuffer> &buffer, sptr<SyncFence> &fence,
    float matrix[16], uint32_t matrixSize, bool isUseNewMatrix)
{
    return GetLastFlushedBufferCommon(buffer, fence,
        matrix, matrixSize, isUseNewMatrix, BUFFER_PRODUCER_ACQUIRE_LAST_FLUSHED_BUFFER);
}

GSError BufferClientProducer::ReleaseLastFlushedBuffer(uint32_t sequence)
{
    DEFINE_MESSAGE_VARIABLES(arguments, reply, option);
    arguments.WriteUint32(sequence);
    SEND_REQUEST(BUFFER_PRODUCER_RELEASE_LAST_FLUSHED_BUFFER, arguments, reply, option);
    return CheckRetval(reply);
}
}; // namespace OHOS
