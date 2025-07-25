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

#include "surface_buffer_impl.h"

#include <dlfcn.h>
#include <mutex>

#include <message_parcel.h>
#include <parameters.h>
#include <securec.h>
#include <sys/mman.h>
#include "buffer_log.h"
#include "buffer_extra_data_impl.h"
#include "surface_trace.h"
#include "v1_1/buffer_handle_meta_key_type.h"
#include "v1_2/display_buffer_type.h"
#include "v1_3/include/idisplay_buffer.h"

namespace OHOS {
namespace {
using IDisplayBufferSptr = std::shared_ptr<OHOS::HDI::Display::Buffer::V1_3::IDisplayBuffer>;
static IDisplayBufferSptr g_displayBuffer;
static std::mutex g_displayBufferMutex;
static std::mutex g_seqNumMutex;
static constexpr uint32_t MAX_SEQUENCE_NUM = 0xFFFF;
static std::bitset<MAX_SEQUENCE_NUM> g_seqBitset(0);
class DisplayBufferDiedRecipient : public OHOS::IRemoteObject::DeathRecipient {
public:
    DisplayBufferDiedRecipient() = default;
    virtual ~DisplayBufferDiedRecipient() = default;
    void OnRemoteDied(const OHOS::wptr<OHOS::IRemoteObject>& remote) override
    {
        std::lock_guard<std::mutex> bufferLock(g_displayBufferMutex);
        g_displayBuffer = nullptr;
        BLOGD("IDisplayBuffer died and g_displayBuffer is nullptr");
    };
};

IDisplayBufferSptr GetDisplayBuffer()
{
    std::lock_guard<std::mutex> bufferLock(g_displayBufferMutex);
    if (g_displayBuffer != nullptr) {
        return g_displayBuffer;
    }
    return nullptr;
}

IDisplayBufferSptr GetOrResetDisplayBuffer()
{
    std::lock_guard<std::mutex> bufferLock(g_displayBufferMutex);
    if (g_displayBuffer != nullptr) {
        return g_displayBuffer;
    }

    g_displayBuffer.reset(OHOS::HDI::Display::Buffer::V1_3::IDisplayBuffer::Get());
    if (g_displayBuffer == nullptr) {
        BLOGE("IDisplayBuffer::Get return nullptr.");
        return nullptr;
    }
    sptr<IRemoteObject::DeathRecipient> recipient = new DisplayBufferDiedRecipient();
    g_displayBuffer->AddDeathRecipient(recipient);
    return g_displayBuffer;
}

constexpr int32_t INVALID_ARGUMENT = -1;
constexpr uint64_t INVALID_PHYADDR = 0;
constexpr uint32_t INVALID_SIZE = 0;
constexpr uint64_t INVALID_USAGE = std::numeric_limits<std::uint64_t>::max();
const std::string MEMMGR_SO = "libmemmgrclient.z.so";
}

sptr<SurfaceBuffer> SurfaceBuffer::Create()
{
    sptr<SurfaceBuffer> surfaceBufferImpl = new SurfaceBufferImpl();
    return surfaceBufferImpl;
}

SurfaceBufferImpl::SurfaceBufferImpl()
{
    {
        g_seqNumMutex.lock();

        static uint32_t sequence_number_ = 0;
        // 0xFFFF is pid mask. 16 is pid offset.
        sequenceNumber_ = (static_cast<uint32_t>(getpid()) & 0xFFFF) << 16;
        // 0xFFFF is seqnum mask.
        sequence_number_++;
        sequenceNumber_ |= (GenerateSequenceNumber(sequence_number_) & MAX_SEQUENCE_NUM);
    
        InitMemMgrMembers();

        g_seqNumMutex.unlock();
    }
    metaDataCache_.clear();
    bedata_ = new BufferExtraDataImpl;

    BLOGD("SurfaceBufferImpl ctor, seq: %{public}u", sequenceNumber_);
}

uint32_t SurfaceBufferImpl::GenerateSequenceNumber(uint32_t& seqNum)
{
    bool isLoop = false;
    for (; seqNum <= MAX_SEQUENCE_NUM; ++seqNum) {
        if (seqNum == MAX_SEQUENCE_NUM) {
            if (isLoop) {
                BLOGE("SurfaceBufferImpl GenerateSequenceNumber failed, no idle seq");
                break;
            } else {
                seqNum = 0;
                isLoop = true;
            }
        }

        if (!g_seqBitset.test(seqNum)) {
            g_seqBitset.set(seqNum);
            break;
        }
    }

    return seqNum;
}

void SurfaceBufferImpl::InitMemMgrMembers()
{
    if (initMemMgrSucceed_.load()) {
        return;
    }
    libMemMgrClientHandle_ = dlopen(MEMMGR_SO.c_str(), RTLD_NOW);
    if (!libMemMgrClientHandle_) {
        BLOGE("dlopen libmemmgrclient failed, error:%{public}s", dlerror());
        return;
    }
    void *reclaim = dlsym(libMemMgrClientHandle_, "reclaim");
    if (!reclaim) {
        BLOGE("dlsym reclaim failed, error:%{public}s", dlerror());
        dlclose(libMemMgrClientHandle_);
        libMemMgrClientHandle_ = nullptr;
        return;
    }
    reclaimFunc_ = reinterpret_cast<MemMgrFunctionPtr>(reclaim);
    void *resume = dlsym(libMemMgrClientHandle_, "resume");
    if (!resume) {
        BLOGE("dlsym resume failed, error:%{public}s", dlerror());
        dlclose(libMemMgrClientHandle_);
        libMemMgrClientHandle_ = nullptr;
        return;
    }
    resumeFunc_ = reinterpret_cast<MemMgrFunctionPtr>(resume);
    ownPid_ = getpid();
    if (ownPid_ < 0) {
        BLOGE("ownPid_(%{public}d) is invaid", ownPid_);
        dlclose(libMemMgrClientHandle_);
        libMemMgrClientHandle_ = nullptr;
        return;
    }
    initMemMgrSucceed_ = true;
    BLOGI("InitMemMgrMembers %{public}s", initMemMgrSucceed_.load() ? "succeed" : "failed");
}

SurfaceBufferImpl::SurfaceBufferImpl(uint32_t seqNum)
{
    metaDataCache_.clear();
    g_seqNumMutex.lock();
    sequenceNumber_ = seqNum;
    g_seqBitset.set(sequenceNumber_ & MAX_SEQUENCE_NUM);
    g_seqNumMutex.unlock();
    bedata_ = new BufferExtraDataImpl;
    BLOGD("SurfaceBufferImpl ctor, seq: %{public}u", sequenceNumber_);
}

SurfaceBufferImpl::~SurfaceBufferImpl()
{
    BLOGD("~SurfaceBufferImpl dtor, seq: %{public}u", sequenceNumber_);
    g_seqNumMutex.lock();
    g_seqBitset.reset(sequenceNumber_ & MAX_SEQUENCE_NUM);
    g_seqNumMutex.unlock();
    FreeBufferHandleLocked();
}

bool SurfaceBufferImpl::MetaDataCachedLocked(const uint32_t key, const std::vector<uint8_t>& value)
{
    auto iter = metaDataCache_.find(key);
    if (iter != metaDataCache_.end() && (*iter).second == value) {
        return true;
    }
    return false;
}

GSError SurfaceBufferImpl::Alloc(const BufferRequestConfig& config, const sptr<SurfaceBuffer>& previousBuffer)
{
    IDisplayBufferSptr displayBuffer = GetOrResetDisplayBuffer();
    if (displayBuffer == nullptr) {
        return GSERROR_INTERNAL;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    FreeBufferHandleLocked();

    GSError ret = CheckBufferConfig(config.width, config.height, config.format, config.usage);
    if (ret != GSERROR_OK) {
        return GSERROR_INVALID_ARGUMENTS;
    }

    OHOS::HDI::Display::Buffer::V1_0::AllocInfo info = {config.width, config.height, config.usage, config.format};
    static bool debugHebcDisabled =
        std::atoi((system::GetParameter("persist.graphic.debug_hebc.disabled", "0")).c_str()) != 0;
    if (debugHebcDisabled) {
        info.usage |= (BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA);
    }
    int32_t dRet = 0;
    if (previousBuffer != nullptr && previousBuffer->GetBufferHandle() != nullptr) {
        SURFACE_TRACE_NAME_FMT("Realloc buffer");
        dRet = displayBuffer->ReAllocMem(info, *(previousBuffer->GetBufferHandle()), handle_);
        BLOGI("Realloc buffer, %{public}d", dRet);
    } else {
        SURFACE_TRACE_NAME_FMT("Alloc buffer");
        dRet = displayBuffer->AllocMem(info, handle_);
    }
    if (dRet == GRAPHIC_DISPLAY_SUCCESS && handle_ != nullptr) {
        dRet = displayBuffer->RegisterBuffer(*handle_);
        if (dRet != GRAPHIC_DISPLAY_SUCCESS && dRet != GRAPHIC_DISPLAY_NOT_SUPPORT) {
            BLOGE("AllocMem RegisterBuffer Failed with %{public}d", dRet);
            return GSERROR_HDI_ERROR;
        }
        surfaceBufferColorGamut_ = static_cast<GraphicColorGamut>(config.colorGamut);
        transform_ = static_cast<GraphicTransformType>(config.transform);
        surfaceBufferWidth_ = config.width;
        surfaceBufferHeight_ = config.height;
        bufferRequestConfig_ = config;
        return GSERROR_OK;
    }
    BLOGW("Alloc Failed with %{public}d, seq: %{public}u", dRet, sequenceNumber_);
    return GSERROR_HDI_ERROR;
}

GSError SurfaceBufferImpl::Map()
{
    IDisplayBufferSptr displayBuffer = GetOrResetDisplayBuffer();
    if (displayBuffer == nullptr) {
        return GSERROR_INTERNAL;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return GSERROR_INVALID_OPERATING;
    } else if (handle_->virAddr != nullptr) {
        return GSERROR_OK;
    }
    if (handle_->usage & BUFFER_USAGE_PROTECTED) {
        return GSERROR_OK;
    }

    void* virAddr = displayBuffer->Mmap(*handle_);
    if (virAddr == nullptr || virAddr == MAP_FAILED) {
        return GSERROR_HDI_ERROR;
    }
    return GSERROR_OK;
}

GSError SurfaceBufferImpl::Unmap()
{
    IDisplayBufferSptr displayBuffer = GetOrResetDisplayBuffer();
    if (displayBuffer == nullptr) {
        return GSERROR_INTERNAL;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return GSERROR_INVALID_OPERATING;
    } else if (handle_->virAddr == nullptr) {
        BLOGW("handle has been unmaped, seq: %{public}u", sequenceNumber_);
        return GSERROR_OK;
    }
    auto dRet = displayBuffer->Unmap(*handle_);
    if (dRet == GRAPHIC_DISPLAY_SUCCESS) {
        handle_->virAddr = nullptr;
        return GSERROR_OK;
    }
    BLOGW("Unmap Failed with %{public}d, seq: %{public}u", dRet, sequenceNumber_);
    return GSERROR_HDI_ERROR;
}

GSError SurfaceBufferImpl::FlushCache()
{
    IDisplayBufferSptr displayBuffer = GetOrResetDisplayBuffer();
    if (displayBuffer == nullptr) {
        return GSERROR_INTERNAL;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return GSERROR_INVALID_OPERATING;
    }
    auto dRet = displayBuffer->FlushCache(*handle_);
    if (dRet == GRAPHIC_DISPLAY_SUCCESS) {
        return GSERROR_OK;
    }
    BLOGW("FlushCache Failed with %{public}d, seq: %{public}u", dRet, sequenceNumber_);
    return GSERROR_HDI_ERROR;
}

GSError SurfaceBufferImpl::GetImageLayout(void* layout)
{
    IDisplayBufferSptr displayBuffer = GetOrResetDisplayBuffer();
    if (displayBuffer == nullptr) {
        return GSERROR_INTERNAL;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return GSERROR_INVALID_OPERATING;
    } else if (planesInfo_.planeCount != 0) {
        return GSERROR_OK;
    }

    auto dRet = displayBuffer->GetImageLayout(*handle_,
        *(static_cast<OHOS::HDI::Display::Buffer::V1_2::ImageLayout*>(layout)));
    if (dRet == GRAPHIC_DISPLAY_SUCCESS) {
        return GSERROR_OK;
    }
    BLOGW("GetImageLayout Failed with %{public}d, seq: %{public}u", dRet, sequenceNumber_);
    return GSERROR_HDI_ERROR;
}

GSError SurfaceBufferImpl::InvalidateCache()
{
    IDisplayBufferSptr displayBuffer = GetOrResetDisplayBuffer();
    if (displayBuffer == nullptr) {
        return GSERROR_INTERNAL;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return GSERROR_INVALID_OPERATING;
    }

    auto dRet = displayBuffer->InvalidateCache(*handle_);
    if (dRet == GRAPHIC_DISPLAY_SUCCESS) {
        return GSERROR_OK;
    }
    BLOGW("InvalidateCache Failed with %{public}d, seq: %{public}u", dRet, sequenceNumber_);
    return GSERROR_HDI_ERROR;
}

void SurfaceBufferImpl::FreeBufferHandleLocked()
{
    metaDataCache_.clear();
    if (handle_) {
        IDisplayBufferSptr displayBuffer = GetDisplayBuffer();
        if (displayBuffer == nullptr) {
            FreeBufferHandle(handle_);
            handle_ = nullptr;
            return;
        }
        if (handle_->virAddr != nullptr) {
            displayBuffer->Unmap(*handle_);
            handle_->virAddr = nullptr;
        }
        displayBuffer->FreeMem(*handle_);
        handle_ = nullptr;
    }
}

// return BufferHandle* is dangerous, need to refactor
BufferHandle* SurfaceBufferImpl::GetBufferHandle() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return handle_;
}

void SurfaceBufferImpl::SetSurfaceBufferColorGamut(const GraphicColorGamut& colorGamut)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (surfaceBufferColorGamut_ != colorGamut) {
        surfaceBufferColorGamut_ = colorGamut;
    }
}

GraphicColorGamut SurfaceBufferImpl::GetSurfaceBufferColorGamut() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return surfaceBufferColorGamut_;
}

void SurfaceBufferImpl::SetSurfaceBufferTransform(const GraphicTransformType& transform)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (transform_ != transform) {
        transform_ = transform;
    }
}

GraphicTransformType SurfaceBufferImpl::GetSurfaceBufferTransform() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return transform_;
}

int32_t SurfaceBufferImpl::GetSurfaceBufferWidth() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return surfaceBufferWidth_;
}

int32_t SurfaceBufferImpl::GetSurfaceBufferHeight() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return surfaceBufferHeight_;
}

void SurfaceBufferImpl::SetSurfaceBufferWidth(int32_t width)
{
    std::lock_guard<std::mutex> lock(mutex_);
    surfaceBufferWidth_ = width;
}

void SurfaceBufferImpl::SetSurfaceBufferHeight(int32_t height)
{
    std::lock_guard<std::mutex> lock(mutex_);
    surfaceBufferHeight_ = height;
}

int32_t SurfaceBufferImpl::GetWidth() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return INVALID_ARGUMENT;
    }
    return handle_->width;
}

int32_t SurfaceBufferImpl::GetHeight() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return INVALID_ARGUMENT;
    }
    return handle_->height;
}

int32_t SurfaceBufferImpl::GetStride() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return INVALID_ARGUMENT;
    }
    return handle_->stride;
}

int32_t SurfaceBufferImpl::GetFormat() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return INVALID_ARGUMENT;
    }
    return handle_->format;
}

uint64_t SurfaceBufferImpl::GetUsage() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return INVALID_USAGE;
    }
    return handle_->usage;
}

uint64_t SurfaceBufferImpl::GetPhyAddr() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return INVALID_PHYADDR;
    }
    return handle_->phyAddr;
}

void* SurfaceBufferImpl::GetVirAddr()
{
    GSError ret = this->Map();
    if (ret != GSERROR_OK) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return nullptr;
    }
    return handle_->virAddr;
}

int32_t SurfaceBufferImpl::GetFileDescriptor() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return INVALID_ARGUMENT;
    }
    return handle_->fd;
}

uint32_t SurfaceBufferImpl::GetSize() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return INVALID_SIZE;
    }
    return handle_->size;
}

GSError SurfaceBufferImpl::GetPlanesInfo(void** planesInfo)
{
    if (planesInfo == nullptr) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    OHOS::HDI::Display::Buffer::V1_2::ImageLayout layout;
    GSError ret = GetImageLayout(&layout);
    if (ret != GSERROR_OK) {
        BLOGW("GetImageLayout failed, ret:%d, seq: %{public}u", ret, sequenceNumber_);
        return ret;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (planesInfo_.planeCount != 0) {
        *planesInfo = static_cast<void*>(&planesInfo_);
        return GSERROR_OK;
    }
    planesInfo_.planeCount = layout.planes.size();
    for (uint32_t i = 0; i < planesInfo_.planeCount && i < 4; i++) { // 4: max plane count
        planesInfo_.planes[i].offset = layout.planes[i].offset;
        planesInfo_.planes[i].rowStride = layout.planes[i].hStride;
        planesInfo_.planes[i].columnStride = layout.planes[i].vStride;
    }

    *planesInfo = static_cast<void*>(&planesInfo_);
    return GSERROR_OK;
}

void SurfaceBufferImpl::SetExtraData(sptr<BufferExtraData> bedata)
{
    std::lock_guard<std::mutex> lock(mutex_);
    bedata_ = bedata;
}

sptr<BufferExtraData> SurfaceBufferImpl::GetExtraData() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return bedata_;
}

void SurfaceBufferImpl::SetBufferHandle(BufferHandle* handle)
{
    if (handle == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == handle) {
        return;
    }
    FreeBufferHandleLocked();

    handle_ = handle;
    IDisplayBufferSptr displayBuffer = GetDisplayBuffer();
    if (displayBuffer == nullptr) {
        return;
    }
    auto dRet = displayBuffer->RegisterBuffer(*handle_);
    if (dRet != GRAPHIC_DISPLAY_SUCCESS && dRet != GRAPHIC_DISPLAY_NOT_SUPPORT) {
        BLOGE("SetBufferHandle RegisterBuffer Failed with %{public}d", dRet);
        return;
    }
}

GSError SurfaceBufferImpl::WriteBufferRequestConfig(MessageParcel& parcel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!parcel.WriteInt32(bufferRequestConfig_.width) || !parcel.WriteInt32(bufferRequestConfig_.height) ||
        !parcel.WriteInt32(bufferRequestConfig_.strideAlignment) || !parcel.WriteInt32(bufferRequestConfig_.format) ||
        !parcel.WriteUint64(bufferRequestConfig_.usage) || !parcel.WriteInt32(bufferRequestConfig_.timeout) ||
        !parcel.WriteUint32(static_cast<uint32_t>(bufferRequestConfig_.colorGamut)) ||
        !parcel.WriteUint32(static_cast<uint32_t>(bufferRequestConfig_.transform)) ||
        !parcel.WriteInt32(scalingMode_)) {
        BLOGE("parcel write fail, seq: %{public}u.", sequenceNumber_);
        return SURFACE_ERROR_UNKOWN;
    }
    return GSERROR_OK;
}

GSError SurfaceBufferImpl::WriteBufferProperty(MessageParcel& parcel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!parcel.WriteInt32(bufferRequestConfig_.width) || !parcel.WriteInt32(bufferRequestConfig_.height) ||
        !parcel.WriteInt32(bufferRequestConfig_.strideAlignment) || !parcel.WriteInt32(bufferRequestConfig_.format) ||
        !parcel.WriteUint64(bufferRequestConfig_.usage) || !parcel.WriteInt32(bufferRequestConfig_.timeout) ||
        !parcel.WriteUint32(static_cast<uint32_t>(bufferRequestConfig_.colorGamut)) ||
        !parcel.WriteUint32(static_cast<uint32_t>(bufferRequestConfig_.transform)) ||
        !parcel.WriteInt32(scalingMode_) || !parcel.WriteInt32(transform_)) {
        BLOGE("parcel write fail, seq: %{public}u.", sequenceNumber_);
        return SURFACE_ERROR_UNKOWN;
    }
    return GSERROR_OK;
}

GSError SurfaceBufferImpl::WriteToMessageParcel(MessageParcel& parcel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return GSERROR_NOT_INIT;
    }
    bool ret = WriteBufferHandle(parcel, *handle_);
    if (ret == false) {
        return GSERROR_API_FAILED;
    }

    return GSERROR_OK;
}

GSError SurfaceBufferImpl::ReadBufferRequestConfig(MessageParcel& parcel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    uint32_t colorGamut = 0;
    uint32_t transform = 0;
    int32_t scalingMode = {};
    if (!parcel.ReadInt32(bufferRequestConfig_.width) || !parcel.ReadInt32(bufferRequestConfig_.height) ||
        !parcel.ReadInt32(bufferRequestConfig_.strideAlignment) || !parcel.ReadInt32(bufferRequestConfig_.format) ||
        !parcel.ReadUint64(bufferRequestConfig_.usage) || !parcel.ReadInt32(bufferRequestConfig_.timeout) ||
        !parcel.ReadUint32(colorGamut) || !parcel.ReadUint32(transform) || !parcel.ReadInt32(scalingMode)) {
        BLOGE("parcel read fail, seq: %{public}u.", sequenceNumber_);
        return GSERROR_API_FAILED;
    }
    surfaceBufferColorGamut_ = static_cast<GraphicColorGamut>(colorGamut);
    transform_ = static_cast<GraphicTransformType>(transform);
    surfaceBufferWidth_ = bufferRequestConfig_.width;
    surfaceBufferHeight_ = bufferRequestConfig_.height;
    scalingMode_ = static_cast<ScalingMode>(scalingMode);
    bufferRequestConfig_.colorGamut = static_cast<GraphicColorGamut>(colorGamut);
    bufferRequestConfig_.transform = static_cast<GraphicTransformType>(transform);
    return GSERROR_OK;
}

GSError SurfaceBufferImpl::ReadBufferProperty(MessageParcel& parcel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    uint32_t colorGamut = 0;
    uint32_t transform = 0;
    int32_t scalingMode = {};
    uint32_t configTransform = 0;
    if (!parcel.ReadInt32(bufferRequestConfig_.width) || !parcel.ReadInt32(bufferRequestConfig_.height) ||
        !parcel.ReadInt32(bufferRequestConfig_.strideAlignment) || !parcel.ReadInt32(bufferRequestConfig_.format) ||
        !parcel.ReadUint64(bufferRequestConfig_.usage) || !parcel.ReadInt32(bufferRequestConfig_.timeout) ||
        !parcel.ReadUint32(colorGamut) || !parcel.ReadUint32(configTransform) || !parcel.ReadInt32(scalingMode) ||
        !parcel.ReadUint32(transform)) {
        BLOGE("parcel read fail, seq: %{public}u.", sequenceNumber_);
        return GSERROR_API_FAILED;
    }
    surfaceBufferColorGamut_ = static_cast<GraphicColorGamut>(colorGamut);
    transform_ = static_cast<GraphicTransformType>(transform);
    surfaceBufferWidth_ = bufferRequestConfig_.width;
    surfaceBufferHeight_ = bufferRequestConfig_.height;
    scalingMode_ = static_cast<ScalingMode>(scalingMode);
    bufferRequestConfig_.colorGamut = static_cast<GraphicColorGamut>(colorGamut);
    bufferRequestConfig_.transform = static_cast<GraphicTransformType>(configTransform);
    return GSERROR_OK;
}

GSError SurfaceBufferImpl::ReadFromBufferInfo(const RSBufferInfo &bufferInfo)
{
    bufferRequestConfig_ = bufferInfo.bufferRequestConfig;
    surfaceBufferWidth_ = bufferInfo.surfaceBufferWidth;
    surfaceBufferHeight_ = bufferInfo.surfaceBufferHeight;
    scalingMode_ = bufferInfo.scalingMode;
    transform_ = bufferInfo.transform;
    surfaceBufferColorGamut_ = bufferInfo.surfaceBufferColorGamut;
    return GSERROR_OK;
}

GSError SurfaceBufferImpl::ReadFromMessageParcel(MessageParcel &parcel,
    std::function<int(MessageParcel &parcel, std::function<int(Parcel &)>readFdDefaultFunc)>readSafeFdFunc)
{
    auto handle = ReadBufferHandle(parcel, readSafeFdFunc);
    SetBufferHandle(handle);
    return handle ? GSERROR_OK : GSERROR_API_FAILED;
}

// return OH_NativeBuffer* is dangerous, need to refactor
OH_NativeBuffer* SurfaceBufferImpl::SurfaceBufferToNativeBuffer()
{
    return reinterpret_cast<OH_NativeBuffer *>(this);
}

uint32_t SurfaceBufferImpl::GetSeqNum() const
{
    return sequenceNumber_;
}

GSError SurfaceBufferImpl::CheckBufferConfig(int32_t width, int32_t height,
                                             int32_t format, uint64_t usage)
{
    if (width <= 0 || height <= 0) {
        BLOGE("width %{public}d height %{public}d", width, height);
        return GSERROR_INVALID_ARGUMENTS;
    }

    if (format < 0 || format > GRAPHIC_PIXEL_FMT_BUTT) {
        BLOGE("format is %{public}d", format);
        return GSERROR_INVALID_ARGUMENTS;
    }

    return GSERROR_OK;
}

GSError SurfaceBufferImpl::SetMetadata(uint32_t key, const std::vector<uint8_t>& value, bool enableCache)
{
    if (key == 0 || key >= HDI::Display::Graphic::Common::V1_1::ATTRKEY_END) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    IDisplayBufferSptr displayBuffer = GetOrResetDisplayBuffer();
    if (displayBuffer == nullptr) {
        return GSERROR_INTERNAL;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return GSERROR_NOT_INIT;
    }

    if (enableCache && MetaDataCachedLocked(key, value)) {
        return GSERROR_OK;
    }

    auto dRet = displayBuffer->SetMetadata(*handle_, key, value);
    if (dRet == GRAPHIC_DISPLAY_SUCCESS) {
        // cache metaData
        if (enableCache) {
            metaDataCache_[key] = value;
        }
        return GSERROR_OK;
    }
    BLOGE("SetMetadata Failed with %{public}d", dRet);
    return GSERROR_HDI_ERROR;
}

GSError SurfaceBufferImpl::GetMetadata(uint32_t key, std::vector<uint8_t>& value)
{
    if (key == 0 || key >= HDI::Display::Graphic::Common::V1_1::ATTRKEY_END) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    IDisplayBufferSptr displayBuffer = GetOrResetDisplayBuffer();
    if (displayBuffer == nullptr) {
        return GSERROR_INTERNAL;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return GSERROR_NOT_INIT;
    }
    auto dRet = displayBuffer->GetMetadata(*handle_, key, value);
    if (dRet == GRAPHIC_DISPLAY_SUCCESS) {
        return GSERROR_OK;
    }
    return GSERROR_HDI_ERROR;
}

GSError SurfaceBufferImpl::ListMetadataKeys(std::vector<uint32_t>& keys)
{
    IDisplayBufferSptr displayBuffer = GetOrResetDisplayBuffer();
    if (displayBuffer == nullptr) {
        return GSERROR_INTERNAL;
    }
    keys.clear();
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return GSERROR_NOT_INIT;
    }
    auto dRet = displayBuffer->ListMetadataKeys(*handle_, keys);
    if (dRet == GRAPHIC_DISPLAY_SUCCESS) {
        return GSERROR_OK;
    }
    BLOGE("ListMetadataKeys Failed with %{public}d", dRet);
    return GSERROR_HDI_ERROR;
}

GSError SurfaceBufferImpl::EraseMetadataKey(uint32_t key)
{
    if (key == 0 || key >= HDI::Display::Graphic::Common::V1_1::ATTRKEY_END) {
        return GSERROR_INVALID_ARGUMENTS;
    }
    IDisplayBufferSptr displayBuffer = GetOrResetDisplayBuffer();
    if (displayBuffer == nullptr) {
        return GSERROR_INTERNAL;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == nullptr) {
        return GSERROR_NOT_INIT;
    }
    auto dRet = displayBuffer->EraseMetadataKey(*handle_, key);
    if (dRet == GRAPHIC_DISPLAY_SUCCESS) {
        metaDataCache_.erase(key);
        return GSERROR_OK;
    }
    BLOGE("EraseMetadataKey Failed with %{public}d", dRet);
    return GSERROR_HDI_ERROR;
}

void SurfaceBufferImpl::SetCropMetadata(const Rect& crop)
{
    std::lock_guard<std::mutex> lock(mutex_);
    crop_ = crop;
}

bool SurfaceBufferImpl::GetCropMetadata(Rect& crop)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (crop_.w <= 0 || crop_.h <= 0) {
        return false;
    }
    crop = crop_;
    return true;
}

BufferRequestConfig SurfaceBufferImpl::GetBufferRequestConfig() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return bufferRequestConfig_;
}

void SurfaceBufferImpl::SetBufferRequestConfig(const BufferRequestConfig& config)
{
    std::lock_guard<std::mutex> lock(mutex_);
    bufferRequestConfig_ = config;
}

void SurfaceBufferImpl::SetConsumerAttachBufferFlag(bool value)
{
    std::lock_guard<std::mutex> lock(mutex_);
    isConsumerAttachBufferFlag_ = value;
}

bool SurfaceBufferImpl::GetConsumerAttachBufferFlag()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return isConsumerAttachBufferFlag_;
}

void SurfaceBufferImpl::SetSurfaceBufferScalingMode(const ScalingMode &scalingMode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    scalingMode_ = scalingMode;
}

ScalingMode SurfaceBufferImpl::GetSurfaceBufferScalingMode() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return scalingMode_;
}

void SurfaceBufferImpl::SetBufferDeleteFromCacheFlag(const bool &flag)
{
    isBufferDeleteFromCache = flag;
}

bool SurfaceBufferImpl::GetBufferDeleteFromCacheFlag() const
{
    return isBufferDeleteFromCache;
}

GSError SurfaceBufferImpl::TryReclaim()
{
    if (!initMemMgrSucceed_.load()) {
        BLOGE("init memmgr members failed");
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (isReclaimed_.load()) {
        return GSERROR_INVALID_OPERATING;
    }

    int32_t fd = -1;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (handle_ == nullptr) {
            return GSERROR_INVALID_ARGUMENTS;
        }
        fd = handle_->fd;
    }
    if (fd < 0) {
        return GSERROR_INVALID_ARGUMENTS;
    }

    int32_t ret = reclaimFunc_(ownPid_, fd);
    isReclaimed_ = (ret == 0 ? true : false);
    return (ret == 0 ? GSERROR_OK : GSERROR_API_FAILED);
}

GSError SurfaceBufferImpl::TryResumeIfNeeded()
{
    if (!initMemMgrSucceed_.load()) {
        BLOGE("init memmgr members failed");
        return GSERROR_INVALID_ARGUMENTS;
    }
    if (!isReclaimed_.load()) {
        return GSERROR_INVALID_OPERATING;
    }

    int32_t fd = -1;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (handle_ == nullptr) {
            return GSERROR_INVALID_ARGUMENTS;
        }
        fd = handle_->fd;
    }
    if (fd < 0) {
        return GSERROR_INVALID_ARGUMENTS;
    }

    int32_t ret = resumeFunc_(ownPid_, fd);
    isReclaimed_ = (ret == 0 ? false : true);
    return (ret == 0 ? GSERROR_OK : GSERROR_API_FAILED);
}

bool SurfaceBufferImpl::IsReclaimed()
{
    return isReclaimed_.load();
}

void SurfaceBufferImpl::SetAndMergeSyncFence(const sptr<SyncFence>& syncFence)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (syncFence == nullptr) {
        return;
    }
    if (syncFence_ != nullptr && syncFence_->IsValid()) {
        syncFence_ = SyncFence::MergeFence("SurfaceBufferSyncFence", syncFence_, syncFence);
    } else {
        syncFence_ = syncFence;
    }
}

sptr<SyncFence> SurfaceBufferImpl::GetSyncFence() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return syncFence_;
}
} // namespace OHOS
