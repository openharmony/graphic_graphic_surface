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

#ifndef INTERFACES_INNERKITS_SURFACE_SURFACE_BUFFER_H
#define INTERFACES_INNERKITS_SURFACE_SURFACE_BUFFER_H

#include <functional>

#include <memory>
#include <refbase.h>

#include "buffer_handle_utils.h"
#include "surface_type.h"
#include "egl_data.h"
#include "buffer_extra_data.h"
#include "native_buffer.h"

struct BufferWrapper;

namespace OHOS {
class IProducerListener;
class MessageParcel;
class Parcel;
class SyncFence;

using ProducerInitInfo = struct {
    uint64_t uniqueId;
    int32_t width;
    int32_t height;
    std::string name;
    std::string appName;
    bool isInHebcList;
    std::string bufferName;
    uint64_t producerId;
    sptr<IProducerListener> propertyListener; // register callback in ctor
    int32_t transformHint;
};
class SurfaceBuffer : public RefBase {
public:
    virtual BufferHandle *GetBufferHandle() const = 0;
    virtual int32_t GetWidth() const = 0;
    virtual int32_t GetHeight() const = 0;
    virtual int32_t GetStride() const = 0;
    virtual int32_t GetFormat() const = 0;
    virtual uint64_t GetUsage() const = 0;
    virtual uint64_t GetPhyAddr() const = 0;
    virtual void* GetVirAddr() = 0;
    virtual int32_t GetFileDescriptor() const = 0;
    virtual uint32_t GetSize() const = 0;

    virtual GraphicColorGamut GetSurfaceBufferColorGamut() const = 0;
    virtual GraphicTransformType GetSurfaceBufferTransform() const = 0;
    virtual void SetSurfaceBufferColorGamut(const GraphicColorGamut& colorGamut) = 0;
    virtual void SetSurfaceBufferTransform(const GraphicTransformType& transform) = 0;

    virtual int32_t GetSurfaceBufferWidth() const = 0;
    virtual int32_t GetSurfaceBufferHeight() const = 0;
    virtual void SetSurfaceBufferWidth(int32_t width) = 0;
    virtual void SetSurfaceBufferHeight(int32_t width) = 0;

    virtual uint32_t GetSeqNum() const = 0;

    virtual void SetExtraData(sptr<BufferExtraData> bedata) = 0;
    virtual sptr<BufferExtraData> GetExtraData() const = 0;
    virtual GSError WriteToMessageParcel(MessageParcel &parcel) = 0;
    /*
     * @Description: ReadFromMessageParcel
     * @param parcel: A MessageParcel object
     * @param readSafeFdFunc：Optional parameter, caller can use this callback function to implement
     *                        their own way of obtaining Fd from parcel
     * @return  Returns GSERROR_OK if SetMetadata is successful; returns GErrorCode otherwise.
     */
    virtual GSError ReadFromMessageParcel(MessageParcel &parcel,
        std::function<int(MessageParcel &parcel,
            std::function<int(Parcel &)>readFdDefaultFunc)>readSafeFdFunc = nullptr) = 0;
    virtual void SetBufferHandle(BufferHandle *handle) = 0;

    /**
     * @brief Allocates a surface buffer based on the specified configuration.
     *
     * This method allocates a new buffer according to the parameters in the given BufferRequestConfig.
     * If a previous buffer (`previousBuffer`) is provided, the implementation may attempt to reuse or reallocate
     * from it to optimize memory usage or performance. If `previousBuffer` is null,
     * a new buffer is allocated from scratch.
     * @param config          Buffer configuration including size, format, usage, timeout, color gamut, etc.
     * @param previousBuffer  Optional previous buffer to be reused or reallocated. If nullptr, no reuse is attempted.
     * @return Returns:
     * - GSERROR_OK on successful allocation or reallocation;
     * - GSERROR_INVALID_ARGUMENTS if the input config is invalid;
     * - GSERROR_INTERNAL if the internal display buffer is not initialized;
     * - GSERROR_HDI_ERROR for lower-level allocation or registration failures.
     */
    virtual GSError Alloc(const BufferRequestConfig &config, const sptr<SurfaceBuffer>& previousBuffer = nullptr) = 0;
    virtual GSError Map() = 0;
    virtual GSError Unmap() = 0;
    virtual GSError FlushCache() = 0;
    virtual GSError InvalidateCache() = 0;

    // metadata
    /*
     * @Description: SetMetadata
     * @param key：Metadata key
     * @param value：Metadata value
     * @param enableCache：If true(default true), enable metaData cache optimization,
     *                     when the same metaData is repeatedly set, the instruction count can be reduced.
     *                     Prohibit setting different enableCache values for the same surfacebuffer object
     * @return  Returns GSERROR_OK if SetMetadata is successful; returns GErrorCode otherwise.
     */
    virtual GSError SetMetadata(uint32_t key, const std::vector<uint8_t>& value, bool enableCache = true) = 0;
    virtual GSError GetMetadata(uint32_t key, std::vector<uint8_t>& value) = 0;
    virtual GSError ListMetadataKeys(std::vector<uint32_t>& keys) = 0;
    virtual GSError EraseMetadataKey(uint32_t key) = 0;

    virtual void SetCropMetadata(const Rect& crop) = 0;
    virtual bool GetCropMetadata(Rect& crop) = 0;

    static SurfaceBuffer* NativeBufferToSurfaceBuffer(OH_NativeBuffer* buffer)
    {
        return reinterpret_cast<SurfaceBuffer *>(buffer);
    };

    static const SurfaceBuffer* NativeBufferToSurfaceBuffer(OH_NativeBuffer const* buffer)
    {
        return reinterpret_cast<SurfaceBuffer const*>(buffer);
    };

    virtual OH_NativeBuffer* SurfaceBufferToNativeBuffer() = 0;

    static sptr<SurfaceBuffer> Create();
    static bool CheckSeqNumExist(uint32_t sequence);

    virtual GSError WriteBufferRequestConfig(MessageParcel &parcel)
    {
        (void)parcel;
        return GSERROR_OK;
    };
    virtual GSError ReadBufferRequestConfig(MessageParcel &parcel)
    {
        (void)parcel;
        return GSERROR_OK;
    };
    virtual BufferRequestConfig GetBufferRequestConfig() const = 0;
    virtual void SetBufferRequestConfig(const BufferRequestConfig &config)
    {
        (void)config;
    };
    virtual void SetConsumerAttachBufferFlag(bool value)
    {
        (void)value;
    };
    virtual bool GetConsumerAttachBufferFlag()
    {
        return false;
    };
    virtual GSError GetPlanesInfo(void **planesInfo)
    {
        (void)planesInfo;
        return GSERROR_OK;
    };
    virtual void SetSurfaceBufferScalingMode(const ScalingMode &scalingMode)
    {
        (void) scalingMode;
    }
    virtual ScalingMode GetSurfaceBufferScalingMode() const
    {
        return SCALING_MODE_SCALE_TO_WINDOW;
    }
    virtual void SetBufferDeleteFromCacheFlag(const bool &flag)
    {
        (void) flag;
    }
    virtual bool GetBufferDeleteFromCacheFlag() const
    {
        return false;
    }
    virtual GSError TryReclaim()
    {
        return GSERROR_NOT_SUPPORT;
    }
    virtual GSError TryResumeIfNeeded()
    {
        return GSERROR_NOT_SUPPORT;
    }
    virtual bool IsReclaimed()
    {
        return false;
    }
    virtual void SetAndMergeSyncFence(const sptr<SyncFence>& syncFence) = 0;
    virtual sptr<SyncFence> GetSyncFence() const = 0;
    virtual uint64_t GetBufferId()
    {
        return 0;
    }
protected:
    SurfaceBuffer() {}
    SurfaceBuffer(const SurfaceBuffer&) = delete;
    SurfaceBuffer& operator=(const SurfaceBuffer&) = delete;
    virtual ~SurfaceBuffer() {}
};

struct SurfaceProperty {
    GraphicTransformType transformHint = GraphicTransformType::GRAPHIC_ROTATE_NONE;
};

using OnReleaseFunc = std::function<GSError(sptr<SurfaceBuffer> &)>;
using OnDeleteBufferFunc = std::function<void(int32_t)>;
using OnReleaseFuncWithFence = std::function<GSError(const sptr<SurfaceBuffer>&, const sptr<SyncFence>&)>;
using OnUserDataChangeFunc = std::function<void(const std::string& key, const std::string& value)>;
using OnPropertyChangeFunc = std::function<GSError(const SurfaceProperty&)>;
} // namespace OHOS

#endif // INTERFACES_INNERKITS_SURFACE_SURFACE_BUFFER_H
