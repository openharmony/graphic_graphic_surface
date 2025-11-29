/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SURFACE_INCLUDE_SURFACE_BUFFER_IMPL_H
#define FRAMEWORKS_SURFACE_INCLUDE_SURFACE_BUFFER_IMPL_H

#include <buffer_extra_data.h>
#include <buffer_handle_parcel.h>
#include <buffer_handle_utils.h>
#include <surface_buffer.h>
#include "egl_data.h"
#include "native_buffer.h"
#include "stdint.h"
#include "sync_fence.h"

struct BufferWrapper {};

namespace OHOS {
class SurfaceBufferImpl : public SurfaceBuffer {
public:
    SurfaceBufferImpl();
    SurfaceBufferImpl(uint32_t seqNum);
    virtual ~SurfaceBufferImpl();

    GSError Alloc(const BufferRequestConfig& config, const sptr<SurfaceBuffer>& previousBuffer = nullptr) override;
    GSError Map() override;
    GSError Unmap() override;
    GSError FlushCache() override;
    GSError InvalidateCache() override;

    BufferHandle *GetBufferHandle() const override;
    int32_t GetWidth() const override;
    int32_t GetHeight() const override;
    int32_t GetStride() const override;
    int32_t GetFormat() const override;
    uint64_t GetUsage() const override;
    uint64_t GetPhyAddr() const override;
    void* GetVirAddr() override;
    int32_t GetFileDescriptor() const override;
    uint32_t GetSize() const override;

    GraphicColorGamut GetSurfaceBufferColorGamut() const override;
    GraphicTransformType GetSurfaceBufferTransform() const override;
    void SetSurfaceBufferColorGamut(const GraphicColorGamut& colorGamut) override;
    void SetSurfaceBufferTransform(const GraphicTransformType& transform) override;

    int32_t GetSurfaceBufferWidth() const override;
    int32_t GetSurfaceBufferHeight() const override;
    void SetSurfaceBufferWidth(int32_t width) override;
    void SetSurfaceBufferHeight(int32_t width) override;

    uint32_t GetSeqNum() const override;

    void SetExtraData(sptr<BufferExtraData> bedata) override;
    sptr<BufferExtraData> GetExtraData() const override;

    void SetBufferHandle(BufferHandle *handle) override;
    GSError WriteToMessageParcel(MessageParcel &parcel) override;
    GSError ReadFromMessageParcel(MessageParcel &parcel,
        std::function<int(MessageParcel &parcel,
            std::function<int(Parcel &)>readFdDefaultFunc)>readSafeFdFunc = nullptr) override;

    OH_NativeBuffer* SurfaceBufferToNativeBuffer() override;

    static GSError CheckBufferConfig(int32_t width, int32_t height,
                                     int32_t format, uint64_t usage);

    // metadata
    GSError SetMetadata(uint32_t key, const std::vector<uint8_t>& value, bool enableCache = true) override;
    GSError GetMetadata(uint32_t key, std::vector<uint8_t>& value) override;
    GSError ListMetadataKeys(std::vector<uint32_t>& keys) override;
    GSError EraseMetadataKey(uint32_t key) override;

    void SetCropMetadata(const Rect& crop) override;
    bool GetCropMetadata(Rect& crop) override;

    GSError WriteBufferRequestConfig(MessageParcel &parcel) override;
    GSError ReadBufferRequestConfig(MessageParcel &parcel) override;
    BufferRequestConfig GetBufferRequestConfig() const override;
    void SetBufferRequestConfig(const BufferRequestConfig &config) override;
    void SetConsumerAttachBufferFlag(bool value) override;
    bool GetConsumerAttachBufferFlag() override;
    GSError GetPlanesInfo(void **planesInfo) override;
    void SetSurfaceBufferScalingMode(const ScalingMode &scalingMode) override;
    ScalingMode GetSurfaceBufferScalingMode() const override;
    void SetBufferDeletedFlag(BufferDeletedFlag bufferDeletedFlag) override;
    BufferDeletedFlag GetBufferDeletedFlag() const override;
    void ClearBufferDeletedFlag(BufferDeletedFlag bufferDeletedFlag) override;
    bool IsBufferDeleted() const override;

    GSError TryReclaim() override;
    GSError TryResumeIfNeeded() override;
    bool IsReclaimed() override;
    void SetAndMergeSyncFence(const sptr<SyncFence>& syncFence) override;
    sptr<SyncFence> GetSyncFence() const override;
    uint64_t GetBufferId() override;
    uint64_t GetFlushedTimestamp() const override;
    void SetFlushTimestamp(uint64_t timestamp) override;
private:
    void FreeBufferHandleLocked();
    bool MetaDataCachedLocked(const uint32_t key, const std::vector<uint8_t>& value);
    GSError GetImageLayout(void *layout);
    static void InitMemMgrMembers();
    static uint32_t GenerateSequenceNumber(uint32_t& seqNum);

    BufferHandle *handle_ = nullptr;
    uint32_t sequenceNumber_ = UINT32_MAX;
    uint64_t bufferId_ = UINT64_MAX;
    sptr<BufferExtraData> bedata_ = nullptr;
    sptr<EglData> eglData_ = nullptr;
    GraphicColorGamut surfaceBufferColorGamut_ = GraphicColorGamut::GRAPHIC_COLOR_GAMUT_SRGB;
    GraphicTransformType transform_ = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    ScalingMode scalingMode_ = ScalingMode::SCALING_MODE_SCALE_TO_WINDOW;
    int32_t surfaceBufferWidth_ = 0;
    int32_t surfaceBufferHeight_ = 0;
    mutable std::mutex mutex_;
    OH_NativeBuffer_Planes planesInfo_ = {0, {}};
    BufferRequestConfig bufferRequestConfig_ = {0, 0, 0, 0, 0, 0};
    bool isConsumerAttachBufferFlag_ = false;
    std::map<uint32_t, std::vector<uint8_t>> metaDataCache_;
    Rect crop_ = {0, 0, 0, 0};
    std::atomic<uint32_t> bufferDeletedFlag_ = 0;
    std::atomic<bool> isReclaimed_ = false;
    std::atomic<bool> isSeqNumExist_ = false;
    using MemMgrFunctionPtr = int32_t(*)(int32_t, int32_t);
    static inline void *libMemMgrClientHandle_ = nullptr;
    static inline MemMgrFunctionPtr reclaimFunc_ = nullptr;
    static inline MemMgrFunctionPtr resumeFunc_ = nullptr;
    static inline int32_t ownPid_ = -1;
    static inline std::atomic<bool> initMemMgrSucceed_ = false;
    sptr<SyncFence> syncFence_ = nullptr;
    std::atomic<uint64_t> lastFlushedTime_ = 0;
};
} // namespace OHOS

#endif // FRAMEWORKS_SURFACE_INCLUDE_SURFACE_BUFFER_IMPL_H
