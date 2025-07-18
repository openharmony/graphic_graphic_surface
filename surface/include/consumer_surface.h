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

#ifndef FRAMEWORKS_SURFACE_INCLUDE_CONSUMER_SURFACE_H
#define FRAMEWORKS_SURFACE_INCLUDE_CONSUMER_SURFACE_H

/**
 * @file consumer_surface.h
 * @brief consumer of surface.
 * Through this class, the data produced by the producer can be obtained for data consumption.\n
 */
#include <iconsumer_surface.h>
#include <map>
#include <string>
#include "buffer_queue.h"
#include "buffer_queue_consumer.h"
#include "buffer_queue_producer.h"

namespace OHOS {
class ConsumerSurface : public IConsumerSurface {
public:
    ConsumerSurface(const std::string &name);
    virtual ~ConsumerSurface();
    /**
     * @brief Initialization function.
     * 
     * @return {@link GSERROR_OK} 0 - Success.
     */
    GSError Init();
    /**
     * @brief Determine if it is the consumer.
     * 
     * @return true - is consumer.
     * @return false - is not consumer.
     */
    bool IsConsumer() const override;
    /**
     * @brief Get the Producer from the surface.
     * 
     * @return sptr<IBufferProducer> The object of producer.
     */
    sptr<IBufferProducer> GetProducer() const override;
    /**
     * @brief Acquire buffer for data consumed.
     * When the fenceFd is used up, you need to close it.
     * 
     * @param buffer [out] The buffer for data consumed.
     * @param fence [out] fence fd for asynchronous waiting mechanism.
     * @param timestamp [out] The timestamp of the produced data.
     * @param damage [out] The dirty buffer area set by the producer.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_BUFFER} 40601000 - no buffer.
     * 
     * @see ReleaseBuffer
     */
    GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, int32_t &fence,
                          int64_t &timestamp, Rect &damage) override;
    /**
     * @brief Release buffer for data production.
     * 
     * @param buffer [in] Consumed data buffer.
     * @param fence [in] fence fd for asynchronous waiting mechanism.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link SURFACE_ERROR_BUFFER_NOT_INCACHE} 41210000 - Buffer not in cache.
     * {@link SURFACE_ERROR_BUFFER_STATE_INVALID} 41207000 - Buffer state invalid.
     */
    GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, int32_t fence) override;
    /**
     * @brief Acquire buffer for data consumed.
     * 
     * @param buffer [out] The buffer for data consumed.
     * @param fence [out] fence fd for asynchronous waiting mechanism.
     * @param timestamp [out] The timestamp of the produced data.
     * @param damage [out] The dirty buffer area set by the producer.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_BUFFER} 40601000 - no buffer.
     * 
     * @see ReleaseBuffer
     */
    GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                          int64_t &timestamp, Rect &damage) override;
    /**
     * @brief Acquire buffer for data consumed.
     * 
     * @param buffer [out] The buffer for data consumed.
     * @param fence [out] fence fd for asynchronous waiting mechanism.
     * @param timestamp [out] The timestamp of the produced data.
     * @param damage [out] The dirty buffer area set by the producer.
     * @param isLppMode [in] Normal buffer or LPP buffer.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_BUFFER} 40601000 - no buffer.
     * 
     * @see ReleaseBuffer
     */
    GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                          int64_t &timestamp, std::vector<Rect> &damages, bool isLppMode = false) override;
    /**
     * @brief Acquire buffer for data consumed.
     * 
     * @param returnValue [out] Acquire buffer return value.
     * @param expectPresentTimestamp [in] Expect present timestamp.
     * @param isUsingAutoTimestamp [in] Is using auto timestamp.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_BUFFER} 40601000 - no buffer.
     * {@link GSERROR_NO_BUFFER_READY} 40605000 - No buffer timestamp meets the target time.
     */
    GSError AcquireBuffer(AcquireBufferReturnValue &returnValue, int64_t expectPresentTimestamp,
                          bool isUsingAutoTimestamp) override;
    /**
     * @brief Release buffer for data production.
     * 
     * @param buffer [in] Consumed data buffer.
     * @param fence [in] fence fd for asynchronous waiting mechanism.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link SURFACE_ERROR_BUFFER_NOT_INCACHE} 41210000 - Buffer not in cache.
     * {@link SURFACE_ERROR_BUFFER_STATE_INVALID} 41207000 - Buffer state invalid.
     */
    GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence) override;
    /**
     * @brief Attach the buffer to the surface.
     * 
     * @param buffer [in] Indicates the pointer to a SurfaceBuffer instance.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_CONSUMER} 41202000 - no consumer.
     * {@link SURFACE_ERROR_BUFFER_STATE_INVALID} 41207000 - Buffer state invalid.
     * {@link SURFACE_ERROR_CONSUMER_UNREGISTER_LISTENER} 41212000 - no register listener.
     * {@link GSERROR_OUT_OF_RANGE} 40603000 - out of range.
     */
    GSError AttachBuffer(sptr<SurfaceBuffer>& buffer) override;
    /**
     * @brief Detach the buffer from the surface.
     * 
     * @param buffer [in] Indicates the pointer to a SurfaceBuffer instance.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_ENTRY} 40602000 - Buffer state invalid or buffer not in cache.
     */
    GSError DetachBuffer(sptr<SurfaceBuffer>& buffer) override;
    /**
     * @brief Determine whether a buffer is ready for use
     * 
     * @return true There has available buffer.
     * @return false There has not available buffer.
     */
    bool QueryIfBufferAvailable() override;
    /**
     * @brief Get the Queue Size from the surface.
     * 
     * @return uint32_t Queue size of the surface.
     */
    uint32_t GetQueueSize() override;
    /**
     * @brief Set the Queue Size for the surface.
     * 
     * @param queueSize [in] The buffer queue size.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetQueueSize(uint32_t queueSize) override;
    /**
     * @brief Get the Name of the surface.
     * 
     * @return const std::string& The name of The surface.
     */
    const std::string& GetName() override;
    /**
     * @brief Set the Default Width And Height for the surface.
     * 
     * @param width [in] The default width for the surface.
     * @param height [in] The default height for the surface.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetDefaultWidthAndHeight(int32_t width, int32_t height) override;
    /**
     * @brief Get the Default Width from the surface.
     * 
     * @return int32_t The default width of the surface.
     */
    int32_t GetDefaultWidth() override;
    /**
     * @brief Get the Default Height from the surface.
     * 
     * @return int32_t The default height of the surface.
     */
    int32_t GetDefaultHeight() override;
    /**
     * @brief Set the Default Usage for the surface.
     * 
     * @param usage [in] the Default Usage of the surface.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetDefaultUsage(uint64_t usage) override;
    /**
     * @brief Get the Default Usage from the surface.
     * 
     * @return uint64_t the Default Usage of the surface.
     */
    uint64_t GetDefaultUsage() override;
    /**
     * @brief Set the User Data for the surface.
     * 
     * @param key [in] Indicates the key of the user data.
     * @param val [in] Indicates the val of the user data.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_OUT_OF_RANGE} 40603000 - Size out of range.
     * {@link GSERROR_API_FAILED} 50001000 - Set value repeat.
     */
    GSError SetUserData(const std::string &key, const std::string &val) override;
    /**
     * @brief Get the User Data of the surface.
     * 
     * @param key [in] Indicates the key of the user data.
     * @return std::string The val of the user data.
     */
    std::string GetUserData(const std::string &key) override;
    /**
     * @brief Register consumer listener function.
     * 
     * @param listener The callback of consumer function.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError RegisterConsumerListener(sptr<IBufferConsumerListener>& listener) override;
    /**
     * @brief Register consumer listener function.
     * 
     * @param listener The callback of consumer function.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError RegisterConsumerListener(IBufferConsumerListenerClazz *listener) override;
    /**
     * @brief Register release listener function.
     * 
     * @param listener The callback of release function.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError RegisterReleaseListener(OnReleaseFunc func) override;
    /**
     * @brief Unregister release listener function.
     * 
     * @return {@link GSERROR_OK} 0 - Success.
     */
    GSError UnRegisterReleaseListener() override
    {
        return GSERROR_OK;
    }
    /**
     * @brief Register delete buffer listener function.
     * 
     * @param func The callback of delete buffer function.
     * @param isForUniRedraw Is for uni redraw.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError RegisterDeleteBufferListener(OnDeleteBufferFunc func, bool isForUniRedraw = false) override;
    /**
     * @brief Unregister consumer listener function.
     * 
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError UnregisterConsumerListener() override;
    /**
     * @brief Get the Unique Id of the surface.
     * 
     * @return uint64_t The Unique Id of the surface.
     */
    uint64_t GetUniqueId() const override;
    /**
     * @brief Dump info of the surface.
     * 
     * @param result The info of the surface.
     */
    void Dump(std::string &result) const override;
    /**
     * @brief Dump current frame layer info of the surface.
     * 
     */
    void DumpCurrentFrameLayer() const override;
    /**
     * @brief Clean the surface buffer cache and inform consumer.
     * This interface will empty all caches of the current process.
     *
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError GoBackground() override;
    /**
     * @brief Set the Transform type for the surface.
     * 
     * @param transform The Transform type.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetTransform(GraphicTransformType transform) override;
    /**
     * @brief Get the Transform type from the surface.
     * 
     * @return GraphicTransformType The Transform type.
     */
    GraphicTransformType GetTransform() const override;
    /**
     * @brief Set the Scaling Mode for the surface buffer.
     * 
     * @param sequence [in] The number of surface buffer.
     * @param scalingMode [in] Scaling mode.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetScalingMode(uint32_t sequence, ScalingMode scalingMode) override;
    /**
     * @brief Get the Scaling Mode of the surface buffer.
     * 
     * @param sequence [in] The number of surface buffer.
     * @param scalingMode [out] Scaling mode.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError GetScalingMode(uint32_t sequence, ScalingMode &scalingMode) override;
    /**
     * @brief Set the Meta Data for the surface buffer.
     * 
     * @param sequence [in] The number of surface buffer.
     * @param metaData [in]  Meta data info.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData) override;
    /**
     * @brief Set the Meta Data for the surface buffer
     * 
     * @param sequence [in] The number of surface buffer.
     * @param key [in] The key of meta data.
     * @param metaData [in] Meta data info.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key, const std::vector<uint8_t> &metaData) override;
    /**
     * @brief Get the meta data type of the surface buffer.
     * 
     * @param sequence [in] The number of surface buffer.
     * @param type [out] The meta data type.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError QueryMetaDataType(uint32_t sequence, HDRMetaDataType &type) const override;
    /**
     * @brief Get the Meta Data of the surface buffer
     * 
     * @param sequence [in] The number of surface buffer.
     * @param metaData [out] Meta data info.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError GetMetaData(uint32_t sequence, std::vector<GraphicHDRMetaData> &metaData) const override;
    /**
     * @brief Get the Meta Data of the surface buffer
     * 
     * @param sequence [in] The number of surface buffer.
     * @param key [in] The key of meta data.
     * @param metaData [out] Meta data info.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError GetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey &key,
                           std::vector<uint8_t> &metaData) const override;
    /**
     * @brief Set the Tunnel Handle for the surface.
     * 
     * @param handle [in] Tunnel handle.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetTunnelHandle(const GraphicExtDataHandle *handle) override;
    /**
     * @brief Get the Tunnel Handle of the surface.
     * 
     * @return sptr<SurfaceTunnelHandle> Tunnel handle.
     */
    sptr<SurfaceTunnelHandle> GetTunnelHandle() const override;
    /**
     * @brief Set the Present Timestamp for the surface buffer.
     * 
     * @param sequence [in] The number of surface buffer.
     * @param timestamp [in] The present time info.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_ENTRY} 40602000 - Buffer not in cache.
     */
    GSError SetPresentTimestamp(uint32_t sequence, const GraphicPresentTimestamp &timestamp) override;
    /**
     * @brief Attach the buffer to the surface.
     * 
     * @param buffer [in] Indicates the pointer to a SurfaceBuffer instance.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_CONSUMER} 41202000 - no consumer.
     * {@link SURFACE_ERROR_BUFFER_STATE_INVALID} 41207000 - Buffer state invalid.
     * {@link SURFACE_ERROR_CONSUMER_UNREGISTER_LISTENER} 41212000 - no register listener.
     * {@link GSERROR_OUT_OF_RANGE} 40603000 - out of range.
     */
    GSError AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut) override;
    /**
     * @brief Register surface remote object.
     * 
     * @param client [in] The remote object.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError RegisterSurfaceDelegator(sptr<IRemoteObject> client) override;
    /**
     * @brief Register release listener function.
     * 
     * @param func [in] The callback of release function.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError RegisterReleaseListener(OnReleaseFuncWithFence func) override
    {
        return GSERROR_NOT_SUPPORT;
    }
    /**
     * @brief Register user data change listener callback.
     * 
     * @param funcName [in] The callback function name.
     * @param func [in] The callback function.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError RegisterUserDataChangeListener(const std::string &funcName, OnUserDataChangeFunc func) override;
    /**
     * @brief Unregister user data change listener callback.
     * 
     * @param funcName [in] The callback function name.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError UnRegisterUserDataChangeListener(const std::string &funcName) override;
    /**
     * @brief Clear user data change listener callback.
     * 
     * @return {@link GSERROR_OK} 0 - Success.
     */
    GSError ClearUserDataChangeListener() override;
    /**
     * @brief Set whether the requested buffer is in HEBC mode.
     * 
     * @param on [in] The switch of HEBC mode.
     */
    void ConsumerRequestCpuAccess(bool on) override;
    /**
     * @brief Attach buffer to the surface.
     * 
     * @param buffer [in] Indicates the pointer to a SurfaceBuffer instance.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Inner error.
     * {@link SURFACE_ERROR_BUFFER_IS_INCACHE} 41208000 - Buffer is in cache.
     * {@link SURFACE_ERROR_BUFFER_QUEUE_FULL} 41209000 - Buffer queue is full.
     */
    GSError AttachBufferToQueue(sptr<SurfaceBuffer> buffer) override;
    /**
     * @brief Detach buffer from the surface.
     * if isReserveSlot is true, a slot in the bufferqueue will be kept
     * empty until attachbuffer is used to fill the slot.
     * if isReserveSlot is true, it must used with AttachBufferToQueue together.
     * 
     * @param buffer [in] Indicates the pointer to a SurfaceBuffer instance.
     * @param isReserveSlot [in] Is need reserve slot or not.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Inner error.
     * {@link SURFACE_ERROR_BUFFER_NOT_INCACHE} 41210000 - Buffer is not in cache.
     * {@link SURFACE_ERROR_BUFFER_STATE_INVALID} 41207000 - Buffer state is invalid.
     */
    GSError DetachBufferFromQueue(sptr<SurfaceBuffer> buffer, bool isReserveSlot = false) override;
    /**
     * @brief Get the Transform Hint from the surface
     * 
     * @return GraphicTransformType The type of transform hint.
     */
    GraphicTransformType GetTransformHint() const override;
    /**
     * @brief Set the Transform Hint of the surface.
     * 
     * @param transformHint [in] The type of transform hint.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetTransformHint(GraphicTransformType transformHint) override;
    /**
     * @brief Get the buffer hold state.
     * 
     * @return true The buffer hold is on.
     * @return false The buffer hold is off.
     */
    inline bool IsBufferHold() override
    {
        if (consumer_ == nullptr) {
            return false;
        }
        return consumer_->IsBufferHold();
    }
    /**
     * @brief Set the Buffer Hold for the surface.
     * 
     * @param hold [in] Indicates the switch to bool instance.
     */
    void SetBufferHold(bool hold) override;
    /**
     * @brief Set the Scaling Mode for the surface buffer.
     * 
     * @param sequence [in] The number of surface buffer.
     * @param scalingMode [in] Scaling mode.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetScalingMode(ScalingMode scalingMode) override;
    /**
     * @brief Set the Surface Source Type for the surface.
     * 
     * @param sourceType [in] The source type.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetSurfaceSourceType(OHSurfaceSource sourceType) override;
    /**
     * @brief Get the Surface Source Type of the surface.
     * 
     * @return OHSurfaceSource The source type.
     */
    OHSurfaceSource GetSurfaceSourceType() const override;
    /**
     * @brief Set the Surface App Framework Type for the surface.
     * 
     * @param appFrameworkType [in] The app frame work type.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetSurfaceAppFrameworkType(std::string appFrameworkType) override;
    /**
     * @brief Get the Surface App Framework Type of the surface.
     * 
     * @return std::string The app frame work type.
     */
    std::string GetSurfaceAppFrameworkType() const override;
    /**
     * @brief Get the Hdr White Point Brightness from the surface.
     * 
     * @return float The brightness value for the surface.
     */
    float GetHdrWhitePointBrightness() const override;
    /**
     * @brief Get the Sdr White Point Brightness from the surface.
     * 
     * @return float The brightness value for the surface.
     */
    float GetSdrWhitePointBrightness() const override;
    /**
     * @brief Get the Surface Buffer Transform Type from the surface.
     * 
     * @param buffer [in] Indicates the pointer to a SurfaceBuffer instance.
     * @param transformType [out] The Transform type.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError GetSurfaceBufferTransformType(sptr<SurfaceBuffer> buffer, GraphicTransformType *transformType) override;
    /**
     * @brief Check whether the surface buffer in cache.
     * 
     * @param seqNum [in] The number of the surface buffer.
     * @param isInCache [out] Is buffer in cache result.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Internal error.
     */
    GSError IsSurfaceBufferInCache(uint32_t seqNum, bool &isInCache) override;
    /**
     * @brief Get the Global Alpha form the surface.
     * 
     * @param alpha [out] The Global Alpha of the surface.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Internal error.
     */
    GSError GetGlobalAlpha(int32_t &alpha) override;
    /**
     * @brief Get the Available Buffer Count from the surface.
     * 
     * @return uint32_t The Available Buffer Count of the surface.
     */
    uint32_t GetAvailableBufferCount() const override;
    /**
     * @brief Get the Last Flushed Desired Present Time Stamp from the surface.
     * 
     * @param lastFlushedDesiredPresentTimeStamp [out] The Last Flushed Desired Present Time Stamp
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Internal error.
     */
    GSError GetLastFlushedDesiredPresentTimeStamp(int64_t &lastFlushedDesiredPresentTimeStamp) const override;
    /**
     * @brief Get the Front Desired Present Time Stamp from the surface
     * 
     * @param desiredPresentTimeStamp [out] The Front Desired Present Time Stamp.
     * @param isAutoTimeStamp [out] Is auto time stamp.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Internal error.
     */
    GSError GetFrontDesiredPresentTimeStamp(int64_t &desiredPresentTimeStamp, bool &isAutoTimeStamp) const override;
    /**
     * @brief Get the Buffer Support Fast Compose from the surface.
     * 
     * @param bufferSupportFastCompose [out] The Buffer Support Fast Compose.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Internal error.
     */
    GSError GetBufferSupportFastCompose(bool &bufferSupportFastCompose) override;
    /**
     * @brief Get the Buffer Cache Config from the surface buffer.
     * 
     * @param buffer [in] Indicates the pointer to a SurfaceBuffer instance.
     * @param config [out] The config info of the surface buffer.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Internal error.
     */
    GSError GetBufferCacheConfig(const sptr<SurfaceBuffer>& buffer, BufferRequestConfig& config) override;
    /**
     * @brief Get the Cycle Buffers Number from the surface.
     * 
     * @param cycleBuffersNumber [out] The Cycle Buffers Number of the surface.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Inner error.
     */
    GSError GetCycleBuffersNumber(uint32_t& cycleBuffersNumber) override;
    /**
     * @brief Set the Cycle Buffers Number for the surface.
     * 
     * @param cycleBuffersNumber [in] The Cycle Buffers Number of the surface.
     * @return {@link GSERROR_NOT_SUPPORT} 50102000 - Not support.
     */
    GSError SetCycleBuffersNumber(uint32_t cycleBuffersNumber) override
    {
        (void)cycleBuffersNumber;
        return GSERROR_NOT_SUPPORT;
    }
    /**
     * @brief Get the Frame Gravity form the surface.
     * 
     * @param frameGravity [out] The frame gravity value.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Inner error.
     */
    GSError GetFrameGravity(int32_t &frameGravity) override;
    /**
     * @brief Get the Fixed Rotation form the surface.
     * 
     * @param fixedRotation [out] The frame rotation value.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Inner error.
     */
    GSError GetFixedRotation(int32_t &fixedRotation) override;
    /**
     * @brief Get the Last Consume Time from the surface.
     * 
     * @param lastConsumeTime [out] The Last Consume Time.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Inner error.
     */
    GSError GetLastConsumeTime(int64_t &lastConsumeTime) const override;
    /**
     * @brief Set the Max Queue Size for the surface.
     * 
     * @param queueSize [in] the Max Queue Size.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Inner error.
     */
    GSError SetMaxQueueSize(uint32_t queueSize) override;
    /**
     * @brief Get the Max Queue Size form the surface.
     * 
     * @param queueSize [out] the Max Queue Size.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Inner error.
     */
    GSError GetMaxQueueSize(uint32_t &queueSize) const override;
    /**
     * @brief Acquire buffer for data consumed.
     * 
     * @param returnValue [out] Acquire buffer return value Contains the acquired buffer information including
     *                          buffer, fence, damages, and other timestamp.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_NO_BUFFER} 40601000 - no buffer.
     */
    GSError AcquireBuffer(AcquireBufferReturnValue& returnValue) override;
    /**
     * @brief Release buffer for data production.
     * 
     * @param sequence [in] Consumed data buffer sequence.
     * @param fence [in] fence fd for asynchronous waiting mechanism.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link SURFACE_ERROR_BUFFER_NOT_INCACHE} 41210000 - Buffer not in cache.
     * {@link SURFACE_ERROR_BUFFER_STATE_INVALID} 41207000 - Buffer state invalid.
     */
    GSError ReleaseBuffer(uint32_t sequence, const sptr<SyncFence>& fence) override;
    /**
     * @brief Sets whether the bufferqueue is used by the game
     * @param isActiveGame [in] Flag indicating whether the queue is used by game:
     *             - true: Optimize for game usage
     *             - false: Normal operation mode
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Inner error.
     */
    GSError SetIsActiveGame(bool isActiveGame) override;
    /**
     * @brief Set the playback source for Lpp video
     *
     * @param isShbSource [in] sensorhub sends video source
     * @param isRsSource [in] render_service sends video source
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetLppDrawSource(bool isShbSource, bool isRsSource) override;
    /**
     * @brief Set the drop mode for the surface.
     *
     * @param enableDrop [in] the drop mode
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKNOWN} 50002000 - Inner error.
     */
    GSError SetDropBufferMode(bool enableDrop) override;
    /**
     * @brief Get the alpha type for the surface.
     *
     * @param alphaType [in] the alpha typ
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKNOWN} 50002000 - Inner error.
     */
    GSError GetAlphaType(GraphicAlphaType &alphaType) override;
private:
    std::map<std::string, std::string> userData_;
    sptr<BufferQueueProducer> producer_ = nullptr;
    sptr<BufferQueueConsumer> consumer_ = nullptr;
    std::string name_ = "not init";
    std::map<std::string, OnUserDataChangeFunc> onUserDataChange_;
    std::mutex lockMutex_;
    uint64_t uniqueId_ = 0;
    std::atomic<bool> hasRegistercallBackForRT_ = false;
    std::atomic<bool> hasRegistercallBackForRedraw_ = false;
    std::atomic<bool> isFirstBuffer_ = true;
    std::atomic<bool> supportFastCompose_ = false;
};
} // namespace OHOS

#endif // FRAMEWORKS_SURFACE_INCLUDE_CONSUMER_SURFACE_H
