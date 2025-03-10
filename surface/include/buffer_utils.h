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

#include "surface_buffer.h"

namespace OHOS {
void ReadFileDescriptor(MessageParcel &parcel, int32_t &fd);
GSError WriteFileDescriptor(MessageParcel &parcel, int32_t fd);

void ReadRequestConfig(MessageParcel &parcel, BufferRequestConfig &config);
GSError WriteRequestConfig(MessageParcel &parcel, const BufferRequestConfig  &config);

GSError ReadFlushConfig(MessageParcel &parcel, BufferFlushConfigWithDamages &config);
GSError WriteFlushConfig(MessageParcel &parcel, const BufferFlushConfigWithDamages &config);

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

GSError ReadSurfaceProperty(MessageParcel &parcel, SurfaceProperty& property);
GSError WriteSurfaceProperty(MessageParcel &parcel, const SurfaceProperty& property);

GSError DumpToFileAsync(pid_t pid, std::string name, sptr<SurfaceBuffer> &buffer);
GSError BufferUtilRegisterPropertyListener(sptr<IProducerListener> listener, uint64_t producerId, 
    std::map<uint64_t, sptr<IProducerListener>> propertyChangeListeners_);
GSError BufferUtilUnRegisterPropertyListener(uint64_t producerId, std::map<uint64_t,
 sptr<IProducerListener>>propertyChangeListeners_);
bool isBufferUtilPresentTimestampReady(int64_t desiredPresentTimestamp, int64_t expectPresentTimestamp);
GSError BufferUtilGetCycleBuffersNumber(uint32& cycleBuffersNumber, uint32_t rotatingBufferNumber_, uint32_t bufferQueueSize_);
} // namespace OHOS

#endif // FRAMEWORKS_SURFACE_INCLUDE_BUFFER_UTILS_H
