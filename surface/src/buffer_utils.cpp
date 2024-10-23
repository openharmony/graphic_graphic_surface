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

#include "buffer_log.h"
#include "surface_buffer_impl.h"

#include <securec.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <sys/time.h>

namespace OHOS {
void ReadFileDescriptor(MessageParcel &parcel, int32_t &fd)
{
    fd = parcel.ReadInt32();
    if (fd < 0) {
        return;
    }

    fd = parcel.ReadFileDescriptor();
}

void WriteFileDescriptor(MessageParcel &parcel, int32_t fd)
{
    if (fd >= 0 && fcntl(fd, F_GETFL) == -1 && errno == EBADF) {
        fd = -1;
    }

    parcel.WriteInt32(fd);

    if (fd < 0) {
        return;
    }

    parcel.WriteFileDescriptor(fd);
    close(fd);
}

void ReadRequestConfig(MessageParcel &parcel, BufferRequestConfig &config)
{
    config.width = parcel.ReadInt32();
    config.height = parcel.ReadInt32();
    config.strideAlignment = parcel.ReadInt32();
    config.format = parcel.ReadInt32();
    config.usage = parcel.ReadUint64();
    config.timeout = parcel.ReadInt32();
    config.colorGamut = static_cast<GraphicColorGamut>(parcel.ReadInt32());
    config.transform = static_cast<GraphicTransformType>(parcel.ReadInt32());
}

void WriteRequestConfig(MessageParcel &parcel, BufferRequestConfig const & config)
{
    parcel.WriteInt32(config.width);
    parcel.WriteInt32(config.height);
    parcel.WriteInt32(config.strideAlignment);
    parcel.WriteInt32(config.format);
    parcel.WriteUint64(config.usage);
    parcel.WriteInt32(config.timeout);
    parcel.WriteInt32(static_cast<int32_t>(config.colorGamut));
    parcel.WriteInt32(static_cast<int32_t>(config.transform));
}

void ReadFlushConfig(MessageParcel &parcel, BufferFlushConfigWithDamages &config)
{
    uint32_t size = parcel.ReadUint32();
    if (size == 0) {
        BLOGE("ReadFlushConfig size is 0");
        return;
    }
    if (size > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("ReadFlushConfig size more than limit, size: %{public}u", size);
        return;
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
}

void WriteFlushConfig(MessageParcel &parcel, BufferFlushConfigWithDamages const & config)
{
    uint32_t size = config.damages.size();
    if (size > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("WriteFlushConfig size more than limit, size: %{public}u", size);
        return;
    }
    parcel.WriteUint32(size);
    for (const auto& rect : config.damages) {
        parcel.WriteInt32(rect.x);
        parcel.WriteInt32(rect.y);
        parcel.WriteInt32(rect.w);
        parcel.WriteInt32(rect.h);
    }
    parcel.WriteInt64(config.timestamp);
    parcel.WriteInt64(config.desiredPresentTimestamp);
}

GSError ReadSurfaceBufferImpl(MessageParcel &parcel,
                              uint32_t &sequence, sptr<SurfaceBuffer>& buffer)
{
    GSError ret = GSERROR_OK;
    sequence = parcel.ReadUint32();
    if (parcel.ReadBool()) {
        buffer = new SurfaceBufferImpl(sequence);
        ret = buffer->ReadFromMessageParcel(parcel);
    }
    return ret;
}

void WriteSurfaceBufferImpl(MessageParcel &parcel,
    uint32_t sequence, const sptr<SurfaceBuffer> &buffer)
{
    parcel.WriteUint32(sequence);
    parcel.WriteBool(buffer != nullptr);
    if (buffer == nullptr) {
        return;
    }
    buffer->WriteToMessageParcel(parcel);
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
        infos.push_back(info);
    }
}

void WriteVerifyAllocInfo(MessageParcel &parcel, const std::vector<BufferVerifyAllocInfo> &infos)
{
    uint32_t size = infos.size();
    if (size > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("WriteVerifyAllocInfo size more than limit, size: %{public}u", size);
        return;
    }
    parcel.WriteUint32(size);
    for (const auto &info : infos) {
        parcel.WriteUint32(info.width);
        parcel.WriteUint32(info.height);
        parcel.WriteUint64(info.usage);
        parcel.WriteInt32(info.format);
    }
}

void ReadHDRMetaData(MessageParcel &parcel, std::vector<GraphicHDRMetaData> &metaData)
{
    uint32_t size = parcel.ReadUint32();
    if (size > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("ReadHDRMetaData size more than limit, size: %{public}u", size);
        return;
    }
    metaData.clear();
    GraphicHDRMetaData data;
    for (uint32_t index = 0; index < size; index++) {
        data.key = static_cast<GraphicHDRMetadataKey>(parcel.ReadUint32());
        data.value = parcel.ReadFloat();
        metaData.push_back(data);
    }
}

void WriteHDRMetaData(MessageParcel &parcel, const std::vector<GraphicHDRMetaData> &metaData)
{
    uint32_t size = metaData.size();
    if (size > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("WriteHDRMetaData size more than limit, size: %{public}u", size);
        return;
    }
    parcel.WriteUint32(size);
    for (const auto &data : metaData) {
        parcel.WriteUint32(static_cast<uint32_t>(data.key));
        parcel.WriteFloat(data.value);
    }
}

void ReadHDRMetaDataSet(MessageParcel &parcel, std::vector<uint8_t> &metaData)
{
    uint32_t size = parcel.ReadUint32();
    if (size > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("ReadHDRMetaDataSet size more than limit, size: %{public}u", size);
        return;
    }
    metaData.clear();
    for (uint32_t index = 0; index < size; index++) {
        uint8_t data = parcel.ReadUint8();
        metaData.push_back(data);
    }
}

void WriteHDRMetaDataSet(MessageParcel &parcel, const std::vector<uint8_t> &metaData)
{
    uint32_t size = metaData.size();
    if (size > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("WriteHDRMetaDataSet size more than limit, size: %{public}u", size);
        return;
    }
    parcel.WriteUint32(size);
    for (const auto &data : metaData) {
        parcel.WriteUint8(data);
    }
}

void ReadExtDataHandle(MessageParcel &parcel, sptr<SurfaceTunnelHandle> &handle)
{
    if (handle == nullptr) {
        BLOGE("handle is null");
        return;
    }
    uint32_t reserveInts = parcel.ReadUint32();
    if (reserveInts > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("ReadExtDataHandle size more than limit, size: %{public}u", reserveInts);
        return;
    }
    GraphicExtDataHandle *tunnelHandle = AllocExtDataHandle(reserveInts);
    if (tunnelHandle == nullptr) {
        BLOGE("AllocExtDataHandle failed");
        return;
    }
    ReadFileDescriptor(parcel, tunnelHandle->fd);
    for (uint32_t index = 0; index < reserveInts; index++) {
        tunnelHandle->reserve[index] = parcel.ReadInt32();
    }
    if (handle->SetHandle(tunnelHandle) != GSERROR_OK) {
        BLOGE("SetHandle failed");
        FreeExtDataHandle(tunnelHandle);
        return;
    }
    FreeExtDataHandle(tunnelHandle);
}

void WriteExtDataHandle(MessageParcel &parcel, const GraphicExtDataHandle *handle)
{
    if (handle == nullptr) {
        BLOGE("handle is null");
        return;
    }
    uint32_t reserveInts = handle->reserveInts;
    if (reserveInts > SURFACE_PARCEL_SIZE_LIMIT) {
        BLOGE("WriteExtDataHandle size more than limit, size: %{public}u", reserveInts);
        return;
    }
    parcel.WriteUint32(reserveInts);
    WriteFileDescriptor(parcel, handle->fd);
    for (uint32_t index = 0; index < handle->reserveInts; index++) {
        parcel.WriteInt32(handle->reserve[index]);
    }
}

void CloneBuffer(uint8_t* dest, const uint8_t* src, size_t totalSize)
{
    size_t block_size = 1024 * 1024; // 1 MB block size
    size_t num_blocks = totalSize / block_size;
    size_t last_block_size = totalSize % block_size;

    // Obtain the number of parallelizable threads.
    size_t num_threads = std::thread::hardware_concurrency();
    num_threads = num_threads > 0 ? num_threads : 1;

    size_t blocks_per_thread = num_blocks / num_threads;
    size_t remaining_blocks = num_blocks % num_threads;

    // Lambda function to copy a block of memory
    auto copy_block = [&](uint8_t* current_dest, const uint8_t* current_src, size_t size) {
        auto ret = memcpy_s(current_dest, size, current_src, size);
        if (ret != 0) {
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
        size_t length_to_copy = blocks_to_copy * block_size;

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
