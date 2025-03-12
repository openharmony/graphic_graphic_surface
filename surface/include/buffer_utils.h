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

#ifndef FRAMEWORKS_SURFACE_INCLUDE_BUFFER_UTILS_H
#define FRAMEWORKS_SURFACE_INCLUDE_BUFFER_UTILS_H

#include <errno.h>
#include <message_parcel.h>
#include "surface_type.h"
#include <surface_tunnel_handle.h>
#include <ibuffer_producer.h>
#include "surface_buffer.h"

namespace OHOS {
inline void ReadFileDescriptor(MessageParcel &parcel, int32_t &fd)
{
    fd = parcel.ReadInt32();
    if (fd < 0) {
        return;
    }

    fd = parcel.ReadFileDescriptor();
}
GSError WriteFileDescriptor(MessageParcel &parcel, int32_t fd);

void ReadRequestConfig(MessageParcel &parcel, BufferRequestConfig &config);
static inline GSError WriteRequestConfig(MessageParcel &parcel, BufferRequestConfig const & config)
{
    if (!parcel.WriteInt32(config.width) || !parcel.WriteInt32(config.height) ||
        !parcel.WriteInt32(config.strideAlignment) || !parcel.WriteInt32(config.format) ||
        !parcel.WriteUint64(config.usage) || !parcel.WriteInt32(config.timeout) ||
        !parcel.WriteInt32(static_cast<int32_t>(config.colorGamut)) ||
        !parcel.WriteInt32(static_cast<int32_t>(config.transform))) {
        return GSERROR_BINDER;
    }
    return GSERROR_OK;
}

GSError ReadFlushConfig(MessageParcel &parcel, BufferFlushConfigWithDamages &config);
GSError WriteFlushConfig(MessageParcel &parcel, const BufferFlushConfigWithDamages & config);

GSError ReadSurfaceBufferImpl(MessageParcel &parcel, uint32_t &sequence, sptr<SurfaceBuffer> &buffer,
    std::function<int(MessageParcel &parcel, std::function<int(Parcel &)>readFdDefaultFunc)> readSafeFdFunc = nullptr);
GSError WriteSurfaceBufferImpl(MessageParcel &parcel, uint32_t sequence, const sptr<SurfaceBuffer> &buffer);

void ReadVerifyAllocInfo(MessageParcel &parcel, std::vector<BufferVerifyAllocInfo> &infos);
GSError WriteVerifyAllocInfo(MessageParcel &parcel, const std::vector<BufferVerifyAllocInfo> &infos);

GSError ReadHDRMetaData(MessageParcel &parcel, std::vector<GraphicHDRMetaData> &metaData);
GSError WriteHDRMetaData(MessageParcel &parcel, const std::vector<GraphicHDRMetaData> &metaData);

GSError ReadHDRMetaDataSet(MessageParcel &parcel, std::vector<uint8_t> &metaData);
GSError WriteHDRMetaDataSet(MessageParcel &parcel, const std::vector<uint8_t> &metaData);

GSError ReadExtDataHandle(MessageParcel &parcel, sptr<SurfaceTunnelHandle> &handle);
GSError WriteExtDataHandle(MessageParcel &parcel, const GraphicExtDataHandle *handle);

GSError DumpToFileAsync(pid_t pid, std::string name, sptr<SurfaceBuffer> &buffer);

static inline GSError ReadSurfaceProperty(MessageParcel &parcel, SurfaceProperty& property)
{
    uint32_t val = parcel.ReadUint32();
    if (val > GraphicTransformType::GRAPHIC_ROTATE_BUTT) {
        return GSERROR_BINDER;
    }
    property.transformHint = static_cast<GraphicTransformType>(val);
    return GSERROR_OK;
}

static inline GSError WriteSurfaceProperty(MessageParcel &parcel, const SurfaceProperty& property)
{
    uint32_t tmp = static_cast<uint32_t>(property.transformHint);
    if (!parcel.WriteUint32(tmp)) {
        return GSERROR_BINDER;
    }
    return GSERROR_OK;
}

static inline GSError BufferUtilRegisterPropertyListener(sptr<IProducerListener> listener,
    uint64_t producerId, std::map<uint64_t, sptr<IProducerListener>> propertyChangeListeners)
{
    const size_t propertyChangeListenerMaxNum = 50;
    if (propertyChangeListeners.size() > propertyChangeListenerMaxNum) {
        return GSERROR_API_FAILED;
    }

    if (propertyChangeListeners.find(producerId) == propertyChangeListeners.end()) {
        propertyChangeListeners[producerId] = listener;
    }
    return GSERROR_OK;
}

static inline GSError BufferUtilUnRegisterPropertyListener(uint64_t producerId,
    std::map<uint64_t, sptr<IProducerListener>> propertyChangeListeners)
{
    propertyChangeListeners.erase(producerId);
    return GSERROR_OK;
}

static inline bool isBufferUtilPresentTimestampReady(int64_t desiredPresentTimestamp,
    int64_t expectPresentTimestamp)
{
    if (desiredPresentTimestamp <= expectPresentTimestamp) {
        return true;
    }
    uint32_t oneSecondTimestamp = 1e9;
    if (desiredPresentTimestamp - oneSecondTimestamp > expectPresentTimestamp) {
        return true;
    }
    return false;
}

static inline GSError BufferUtilGetCycleBuffersNumber(uint32_t& cycleBuffersNumber,
    uint32_t rotatingBufferNumber, uint32_t bufferQueueSize)
{
    if (rotatingBufferNumber == 0) {
        cycleBuffersNumber = bufferQueueSize;
    } else {
        cycleBuffersNumber = rotatingBufferNumber;
    }
    return GSERROR_OK;
}
} // namespace OHOS

#endif // FRAMEWORKS_SURFACE_INCLUDE_BUFFER_UTILS_H
