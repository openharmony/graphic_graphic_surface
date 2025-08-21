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

#include "buffer_utils.h"
#include <fcntl.h>
#include <unistd.h>
#include <parameters.h>

#include "buffer_log.h"
#include "surface_buffer_impl.h"

#include <securec.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <sys/time.h>

namespace OHOS {
namespace {
constexpr size_t BLOCK_SIZE = 1024 * 1024; // 1 MB block size
}

GSError WriteFileDescriptor(MessageParcel &parcel, int32_t fd)
{
    if (fd >= 0 && fcntl(fd, F_GETFL) == -1 && errno == EBADF) {
        fd = -1;
    }

    if (!parcel.WriteInt32(fd)) {
        return GSERROR_BINDER;
    }

    if (fd < 0) {
        return GSERROR_OK;
    }

    if (!parcel.WriteFileDescriptor(fd)) {
        return GSERROR_BINDER;
    }
    return GSERROR_OK;
}

bool GetBoolParameter(const std::string &name, const std::string &defaultValue)
{
    return system::GetParameter(name, defaultValue) != "0";
}

bool ReadRequestConfig(MessageParcel &parcel, BufferRequestConfig &config)
{
    bool parcelRead = !parcel.ReadInt32(config.width) || !parcel.ReadInt32(config.height) ||
                      !parcel.ReadInt32(config.strideAlignment) || !parcel.ReadInt32(config.format) ||
                      !parcel.ReadUint64(config.usage) || !parcel.ReadInt32(config.timeout);
    if (parcelRead) {
        BLOGE("parcel read fail.");
        return false;
    }

    config.colorGamut = static_cast<GraphicColorGamut>(parcel.ReadInt32());
    if (config.colorGamut < GRAPHIC_COLOR_GAMUT_INVALID || config.colorGamut > GRAPHIC_COLOR_GAMUT_DISPLAY_BT2020) {
        config.colorGamut = GRAPHIC_COLOR_GAMUT_INVALID;
    }
    config.transform = static_cast<GraphicTransformType>(parcel.ReadInt32());
    if (config.transform < GRAPHIC_ROTATE_NONE || config.transform > GRAPHIC_ROTATE_BUTT) {
        config.transform = GRAPHIC_ROTATE_BUTT;
    }
    return true;
}

GSError ReadFlushConfig(MessageParcel &parcel, BufferFlushConfigWithDamages &config)
{
    uint32_t size = parcel.ReadUint32();
    if (size == 0) {
        BLOGE("ReadFlushConfig size is 0");
        return GSERROR_BINDER;
    }
    if (size > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("ReadFlushConfig size more than limit, size: %{public}u", size);
        return GSERROR_BINDER;
    }
    config.damages.clear();
    config.damages.reserve(size);
    for (uint32_t i = 0; i < size; i++) {
        Rect rect = {
            .x = parcel.ReadInt32(),
            .y = parcel.ReadInt32(),
            .w = parcel.ReadInt32(),
            .h = parcel.ReadInt32(),
        };
        config.damages.emplace_back(rect);
    }
    config.timestamp = parcel.ReadInt64();
    config.desiredPresentTimestamp = parcel.ReadInt64();
    return GSERROR_OK;
}

GSError WriteFlushConfig(MessageParcel &parcel, BufferFlushConfigWithDamages const & config)
{
    uint32_t size = config.damages.size();
    if (size > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("WriteFlushConfig size more than limit, size: %{public}u", size);
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (!parcel.WriteUint32(size)) {
        return GSERROR_BINDER;
    }
    for (const auto& rect : config.damages) {
        if (!parcel.WriteInt32(rect.x) || !parcel.WriteInt32(rect.y) ||
            !parcel.WriteInt32(rect.w) || !parcel.WriteInt32(rect.h)) {
            return GSERROR_BINDER;
        }
    }
    if (!parcel.WriteInt64(config.timestamp)) {
        return GSERROR_BINDER;
    }

    if (!parcel.WriteInt64(config.desiredPresentTimestamp)) {
        return GSERROR_BINDER;
    }
    return GSERROR_OK;
}

GSError ReadSurfaceBufferImpl(MessageParcel &parcel, uint32_t &sequence, sptr<SurfaceBuffer> &buffer,
    std::function<int(MessageParcel &parcel, std::function<int(Parcel &)>readFdDefaultFunc)> readSafeFdFunc)
{
    GSError ret = GSERROR_OK;
    sequence = parcel.ReadUint32();
    if (parcel.ReadBool()) {
        buffer = new SurfaceBufferImpl(sequence);
        ret = buffer->ReadFromMessageParcel(parcel, readSafeFdFunc);
    }
    return ret;
}

GSError WriteSurfaceBufferImpl(MessageParcel &parcel,
    uint32_t sequence, const sptr<SurfaceBuffer> &buffer)
{
    if (!parcel.WriteUint32(sequence)) {
        return GSERROR_BINDER;
    }
    if (!parcel.WriteBool(buffer != nullptr)) {
        return GSERROR_BINDER;
    }
    if (buffer != nullptr) {
        return buffer->WriteToMessageParcel(parcel);
    }
    return GSERROR_OK;
}

void ReadVerifyAllocInfo(MessageParcel &parcel, std::vector<BufferVerifyAllocInfo> &infos)
{
    uint32_t size = parcel.ReadUint32();
    if (size > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("ReadVerifyAllocInfo size more than limit, size: %{public}u", size);
        return;
    }
    infos.clear();
    BufferVerifyAllocInfo info;
    for (uint32_t index = 0; index < size; index++) {
        info.width = parcel.ReadUint32();
        info.height = parcel.ReadUint32();
        info.usage = parcel.ReadUint64();
        info.format = static_cast<GraphicPixelFormat>(parcel.ReadInt32());
        infos.emplace_back(info);
    }
}

GSError WriteVerifyAllocInfo(MessageParcel &parcel, const std::vector<BufferVerifyAllocInfo> &infos)
{
    uint32_t size = infos.size();
    if (size > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("WriteVerifyAllocInfo size more than limit, size: %{public}u", size);
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (!parcel.WriteUint32(size)) {
        return GSERROR_BINDER;
    }
    for (const auto &info : infos) {
        if (!parcel.WriteUint32(info.width) || !parcel.WriteUint32(info.height) ||
            !parcel.WriteUint64(info.usage) || !parcel.WriteInt32(info.format)) {
            return GSERROR_BINDER;
        }
    }
    return GSERROR_OK;
}

GSError ReadHDRMetaData(MessageParcel &parcel, std::vector<GraphicHDRMetaData> &metaData)
{
    uint32_t size = 0;
    if (!parcel.ReadUint32(size)) {
        return GSERROR_BINDER;
    }
    if (size > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("ReadHDRMetaData size more than limit, size: %{public}u", size);
        return GSERROR_BINDER;
    }
    metaData.clear();
    GraphicHDRMetaData data;
    for (uint32_t index = 0; index < size; index++) {
        data.key = static_cast<GraphicHDRMetadataKey>(parcel.ReadUint32());
        if (!parcel.ReadFloat(data.value)) {
            return GSERROR_BINDER;
        }
        metaData.emplace_back(data);
    }
    return GSERROR_OK;
}

GSError WriteHDRMetaData(MessageParcel &parcel, const std::vector<GraphicHDRMetaData> &metaData)
{
    uint32_t size = metaData.size();
    if (size > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("WriteHDRMetaData size more than limit, size: %{public}u", size);
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (!parcel.WriteUint32(size)) {
        return GSERROR_BINDER;
    }
    for (const auto &data : metaData) {
        if (!parcel.WriteUint32(static_cast<uint32_t>(data.key)) || !parcel.WriteFloat(data.value)) {
            return GSERROR_BINDER;
        }
    }
    return GSERROR_OK;
}

GSError ReadHDRMetaDataSet(MessageParcel &parcel, std::vector<uint8_t> &metaData)
{
    uint32_t size = parcel.ReadUint32();
    if (size > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("ReadHDRMetaDataSet size more than limit, size: %{public}u", size);
        return GSERROR_BINDER;
    }
    metaData.clear();
    for (uint32_t index = 0; index < size; index++) {
        uint8_t data = parcel.ReadUint8();
        metaData.emplace_back(data);
    }
    return GSERROR_OK;
}

GSError WriteHDRMetaDataSet(MessageParcel &parcel, const std::vector<uint8_t> &metaData)
{
    uint32_t size = metaData.size();
    if (size > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("WriteHDRMetaDataSet size more than limit, size: %{public}u", size);
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (!parcel.WriteUint32(size)) {
        return GSERROR_BINDER;
    }
    for (const auto &data : metaData) {
        if (!parcel.WriteUint8(data)) {
            return GSERROR_BINDER;
        }
    }
    return GSERROR_OK;
}

GSError ReadExtDataHandle(MessageParcel &parcel, sptr<SurfaceTunnelHandle> &handle)
{
    if (handle == nullptr) {
        BLOGE("handle is null");
        return GSERROR_BINDER;
    }
    uint32_t reserveInts = 0;
    if (!parcel.ReadUint32(reserveInts)) {
        return GSERROR_BINDER;
    }
    if (reserveInts > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("ReadExtDataHandle size more than limit, size: %{public}u", reserveInts);
        return GSERROR_BINDER;
    }
    GraphicExtDataHandle *tunnelHandle = AllocExtDataHandle(reserveInts);
    if (tunnelHandle == nullptr) {
        BLOGE("AllocExtDataHandle failed");
        return GSERROR_BINDER;
    }
    ReadFileDescriptor(parcel, tunnelHandle->fd);
    for (uint32_t index = 0; index < reserveInts; index++) {
        if (!parcel.ReadInt32(tunnelHandle->reserve[index])) {
            FreeExtDataHandle(tunnelHandle);
            return GSERROR_BINDER;
        }
    }
    if (handle->SetHandle(tunnelHandle) != GSERROR_OK) {
        BLOGE("SetHandle failed");
        FreeExtDataHandle(tunnelHandle);
        return GSERROR_BINDER;
    }
    FreeExtDataHandle(tunnelHandle);
    return GSERROR_OK;
}

GSError WriteExtDataHandle(MessageParcel &parcel, const GraphicExtDataHandle *handle)
{
    if (handle == nullptr) {
        BLOGE("handle is null");
        return GSERROR_INVALID_ARGUMENTS;
    }
    uint32_t reserveInts = handle->reserveInts;
    if (reserveInts > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("WriteExtDataHandle size more than limit, size: %{public}u", reserveInts);
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (!parcel.WriteUint32(reserveInts)) {
        return GSERROR_BINDER;
    }
    GSError ret = WriteFileDescriptor(parcel, handle->fd);
    if (ret != GSERROR_OK) {
        return ret;
    }
    for (uint32_t index = 0; index < handle->reserveInts; index++) {
        if (!parcel.WriteInt32(handle->reserve[index])) {
            return GSERROR_BINDER;
        }
    }
    return GSERROR_OK;
}

void CloneBuffer(uint8_t* dest, const uint8_t* src, size_t totalSize)
{
    if (dest == nullptr || src == nullptr) {
        return;
    }
    size_t num_blocks = totalSize / BLOCK_SIZE;
    size_t last_block_size = totalSize % BLOCK_SIZE;

    // Obtain the number of parallelizable threads.
    size_t num_threads = std::thread::hardware_concurrency();
    num_threads = num_threads > 0 ? num_threads : 1;

    size_t blocks_per_thread = num_blocks / num_threads;
    size_t remaining_blocks = num_blocks % num_threads;

    // Lambda function to copy a block of memory
    auto copy_block = [&](uint8_t* current_dest, const uint8_t* current_src, size_t size) {
        auto ret = memcpy_s(current_dest, size, current_src, size);
        if (ret != EOK) {
            BLOGE("memcpy_s ret:%{public}d", static_cast<int>(ret));
        }
    };

    // Vector to store threads
    std::vector<std::thread> threads;
    uint8_t* current_dest = dest;
    const uint8_t* current_src = src;

    // Create threads and copy blocks of memory
    for (size_t i = 0; i < num_threads; ++i) {
        size_t blocks_to_copy = blocks_per_thread + (i < remaining_blocks ? 1 : 0);
        size_t length_to_copy = blocks_to_copy * BLOCK_SIZE;

        threads.emplace_back(copy_block, current_dest, current_src, length_to_copy);

        current_dest += length_to_copy;
        current_src += length_to_copy;
    }

    if (last_block_size > 0) {
        threads.emplace_back(copy_block, current_dest, current_src, last_block_size);
    }

    // Wait for all threads to finish
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }
}

void WriteToFile(std::string prefixPath, std::string pid, void* dest, size_t size, int32_t format, int32_t width,
    int32_t height, const std::string name)
{
    if (dest == nullptr) {
        BLOGE("dest is nulltr");
        return;
    }
    struct timeval now;
    gettimeofday(&now, nullptr);
    constexpr int secToUsec = 1000 * 1000;
    int64_t nowVal = (int64_t)now.tv_sec * secToUsec + (int64_t)now.tv_usec;

    std::stringstream ss;
    ss << prefixPath << pid << "_" << name << "_" << nowVal << "_" << format << "_"
        << width << "x" << height << ".raw";

    // Open the file for writing in binary mode
    std::ofstream rawDataFile(ss.str(), std::ofstream::binary);
    if (!rawDataFile.good()) {
        BLOGE("open failed: (%{public}d)%{public}s", errno, strerror(errno));
        free(dest);
        return;
    }

    // Write the data to the file
    rawDataFile.write(static_cast<const char *>(dest), size);
    rawDataFile.flush();
    rawDataFile.close();

    // Free the memory allocated for the data
    free(dest);
}

GSError DumpToFileAsync(pid_t pid, std::string name, sptr<SurfaceBuffer> &buffer)
{
    bool rsDumpFlag = access("/data/bq_dump", F_OK) == 0;
    bool appDumpFlag = access("/data/storage/el1/base/bq_dump", F_OK) == 0;
    if (!rsDumpFlag && !appDumpFlag) {
        return GSERROR_OK;
    }

    if (buffer == nullptr) {
        BLOGE("buffer is a nullptr.");
        return GSERROR_INVALID_ARGUMENTS;
    }

    size_t size = buffer->GetSize();
    if (size > 0) {
        uint8_t* src = static_cast<uint8_t*>(buffer->GetVirAddr());

        if (src == nullptr) {
            BLOGE("src is a nullptr.");
            return GSERROR_INVALID_ARGUMENTS;
        }

        uint8_t* dest = static_cast<uint8_t*>(malloc(size));
        if (dest != nullptr) {
            // Copy through multithreading
            CloneBuffer(dest, src, size);

            std::string prefixPath = "/data/bq_";
            if (appDumpFlag) {
                // Is app texture export
                prefixPath = "/data/storage/el1/base/bq_";
            }

            // create dump thread，async export file
            std::thread file_writer(WriteToFile, prefixPath, std::to_string(pid), dest, size, buffer->GetFormat(),
                buffer->GetWidth(), buffer->GetHeight(), name);
            file_writer.detach();
        } else {
            BLOGE("dest is a nullptr.");
            return GSERROR_INTERNAL;
        }
    } else {
        BLOGE("BufferDump buffer size(%{public}zu) error.", size);
        return GSERROR_INTERNAL;
    }

    return GSERROR_OK;
}
} // namespace OHOS
