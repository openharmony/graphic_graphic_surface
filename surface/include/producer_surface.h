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

#ifndef FRAMEWORKS_SURFACE_INCLUDE_PRODUCER_SURFACE_H
#define FRAMEWORKS_SURFACE_INCLUDE_PRODUCER_SURFACE_H

/**
 * @file producer_surface.h
 * @brief producer of surface.
 * Through this class, memory can be allocated from the BufferQueue for data production,
 * and the memory data can be delivered to the consumer for data consumption.\n
 */
#include <atomic>
#include <ibuffer_producer.h>
#include <map>
#include <string>
#include <surface.h>
#include <vector>
#include "buffer_queue.h"
#include "buffer_queue_consumer.h"
#include "producer_surface_delegator.h"
#include "surface_buffer.h"

struct NativeWindow;
namespace OHOS {
class ProducerSurface : public Surface {
public:
    ProducerSurface(sptr<IBufferProducer>& producer);
    virtual ~ProducerSurface();
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
     * @return sptr<IBufferProducer> - The object of producer.
     */
    sptr<IBufferProducer> GetProducer() const override;
    /**
     * @brief Request buffer for data production.
     * When the fenceFd is used up, you need to close it.
     * 
     * @param buffer [out] The buffer for data production.
     * @param fence [out] fence fd for asynchronous waiting mechanism.
     * @param config [in] The parameter type for requesting a buffer.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_CONSUMER} 41202000 - no consumer.
     * {@link GSERROR_NO_BUFFER} 40601000 - no buffer.
     * {@link} GSERROR_CONSUMER_IS_CONNECTED 41206000 - consumer is connected already.
     * 
     * @see FlushBuffer
     */
    GSError RequestBuffer(sptr<SurfaceBuffer>& buffer,
                          int32_t &fence, BufferRequestConfig &config) override;
    /**
     * @brief Request buffers for data production.
     * 
     * @param buffer [out] The buffers for data production.
     * @param fence [out] fence fds for asynchronous waiting mechanism.
     * @param config [in] The parameter type for requesting a buffer.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_CONSUMER} 41202000 - no consumer.
     * {@link GSERROR_NO_BUFFER} 40601000 - no buffer.
     * {@link} GSERROR_CONSUMER_IS_CONNECTED 41206000 - consumer is connected already.
     * 
     * @see FlushBuffers
     */
    GSError RequestBuffers(std::vector<sptr<SurfaceBuffer>> &buffers,
        std::vector<sptr<SyncFence>> &fences, BufferRequestConfig &config) override;
    /**
     * @brief Cancel the requested buffer.
     * Change buffer state from requested to released.
     * @param buffer [in] Indicates the pointer to a SurfaceBuffer instance.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Internal error.
     * {@link SURFACE_ERROR_BUFFER_NOT_INCACHE} 41210000 - Buffer not in cache.
     * {@link SURFACE_ERROR_BUFFER_STATE_INVALID} 41207000 - Buffer state invalid.
     */
    GSError CancelBuffer(sptr<SurfaceBuffer>& buffer) override;
    /**
     * @brief Flush buffer to data consumption.
     * 
     * @param buffer [in] Processed data buffer.
     * @param fence [in] fence fd for asynchronous waiting mechanism.
     * @param config [in] The parameter type for flushing a buffer.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_CONSUMER} 41202000 - no consumer.
     * {@link SURFACE_ERROR_BUFFER_NOT_INCACHE} 41210000 - Buffer not in cache.
     * {@link SURFACE_ERROR_BUFFER_STATE_INVALID} 41207000 - Buffer state invalid.
     * {@link SURFACE_ERROR_CONSUMER_UNREGISTER_LISTENER} 41212000 - no register listener.
     */
    GSError FlushBuffer(sptr<SurfaceBuffer>& buffer,
                        int32_t fence, BufferFlushConfig &config) override;
    /**
     * @brief Flush buffers to data consumption.
     * 
     * @param buffer [in] Processed data buffers.
     * @param fence [in] fence fds for asynchronous waiting mechanism.
     * @param config [in] The parameter type for flushing buffers.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_CONSUMER} 41202000 - no consumer.
     * {@link SURFACE_ERROR_BUFFER_NOT_INCACHE} 41210000 - Buffer not in cache.
     * {@link SURFACE_ERROR_BUFFER_STATE_INVALID} 41207000 - Buffer state invalid.
     * {@link SURFACE_ERROR_CONSUMER_UNREGISTER_LISTENER} 41212000 - no register listener.
     */
    GSError FlushBuffers(const std::vector<sptr<SurfaceBuffer>> &buffers,
        const std::vector<sptr<SyncFence>> &fences, const std::vector<BufferFlushConfigWithDamages> &config) override;
    /**
     * @brief Request buffer for data production.
     * 
     * @param buffer [out] The buffer for data production.
     * @param fence [out] fence fd for asynchronous waiting mechanism.
     * @param config [in] The parameter type for requesting a buffer.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_CONSUMER} 41202000 - no consumer.
     * {@link GSERROR_NO_BUFFER} 40601000 - no buffer.
     * {@link} GSERROR_CONSUMER_IS_CONNECTED 41206000 - consumer is connected already.
     * 
     * @see FlushBuffer
     */
    GSError RequestBuffer(sptr<SurfaceBuffer>& buffer,
                          sptr<SyncFence>& fence, BufferRequestConfig &config) override;
    /**
     * @brief Flush buffer to data consumption.
     * 
     * @param buffer [in] Processed data buffer.
     * @param fence [in] fence fd for asynchronous waiting mechanism.
     * @param config [in] The parameter type for flushing a buffer.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_CONSUMER} 41202000 - no consumer.
     * {@link SURFACE_ERROR_BUFFER_NOT_INCACHE} 41210000 - Buffer not in cache.
     * {@link SURFACE_ERROR_BUFFER_STATE_INVALID} 41207000 - Buffer state invalid.
     * {@link SURFACE_ERROR_CONSUMER_UNREGISTER_LISTENER} 41212000 - no register listener.
     */
    GSError FlushBuffer(sptr<SurfaceBuffer>& buffer,
                        const sptr<SyncFence>& fence, BufferFlushConfig &config) override;
    /**
     * @brief Get the Last Flushed Buffer object.
     * 
     * @param buffer [out] Indicates the pointer to a SurfaceBuffer instance.
     * @param fence [out] fence fd for asynchronous waiting mechanism.
     * @param matrix [out] Orientation matrix.
     * @param isUseNewMatrix [in] Is use new matrix switch.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link SURFACE_ERROR_BUFFER_STATE_INVALID} 41207000 - Buffer state invalid.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Internal error.
     * {@link SURFACE_ERROR_NOT_SUPPORT} 50102000 - Not surport usage.
     */
    GSError GetLastFlushedBuffer(sptr<SurfaceBuffer>& buffer,
        sptr<SyncFence>& fence, float matrix[16], bool isUseNewMatrix = false) override;
    /**
     * @brief Flush buffer to data consumption.
     * 
     * @param buffer [in] Processed data buffer.
     * @param fence [in] fence fd for asynchronous waiting mechanism.
     * @param config [in] The parameter type for flushing a buffer.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_CONSUMER} 41202000 - no consumer.
     * {@link SURFACE_ERROR_BUFFER_NOT_INCACHE} 41210000 - Buffer not in cache.
     * {@link SURFACE_ERROR_BUFFER_STATE_INVALID} 41207000 - Buffer state invalid.
     * {@link SURFACE_ERROR_CONSUMER_UNREGISTER_LISTENER} 41212000 - no register listener.
     */
    GSError FlushBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
                        BufferFlushConfigWithDamages &config, bool needLock = true) override;
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
     * @brief Attach the buffer to the surface.
     * 
     * @param buffer [in] Indicates the pointer to a SurfaceBuffer instance.
     * @param timeOut [in] Timeout interval.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_CONSUMER} 41202000 - no consumer.
     * {@link SURFACE_ERROR_BUFFER_STATE_INVALID} 41207000 - Buffer state invalid.
     * {@link SURFACE_ERROR_CONSUMER_UNREGISTER_LISTENER} 41212000 - no register listener.
     * {@link GSERROR_OUT_OF_RANGE} 40603000 - out of range.
     */
    GSError AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut) override;
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
     * @brief Get the Queue Size from the surface.
     * 
     * @return uint32_t The buffer queue size.
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
     * @brief Get the Unique Id of the surface.
     * 
     * @return uint64_t The Unique Id of the surface.
     */
    uint64_t GetUniqueId() const override;
    /**
     * @brief Get the Default Width of the surface.
     * 
     * @return int32_t The default width of the surface.
     */
    int32_t GetDefaultWidth() override;
    /**
     * @brief Get the Default height of the surface.
     * 
     * @return int32_t The default height of the surface.
     */
    int32_t GetDefaultHeight() override;
    /**
     * @brief Set the Default Usage of the surface.
     * 
     * @param usage [in] the Default Usage of the surface.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetDefaultUsage(uint64_t usage) override;
    /**
     * @brief Get the Default Usage of the surface.
     * 
     * @return uint64_t the Default Usage of the surface.
     */
    uint64_t GetDefaultUsage() override;
    /**
     * @brief Set the Buffer Hold for the surface.
     * 
     * @param hold [in] Indicates the switch to bool instance.
     */
    void SetBufferHold(bool hold) override;
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
     * @brief Register release listener function.
     * 
     * @param func [in] The callback of release function.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError RegisterReleaseListener(OnReleaseFunc func) override;
    /**
     * @brief Register release listener function.
     * 
     * @param func [in] The callback of release function.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError RegisterReleaseListener(OnReleaseFuncWithFence func) override;
    /**
     * @brief Register release listener function.
     * 
     * @param func [in] The callback of release function.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError RegisterReleaseListenerBackup(OnReleaseFuncWithFence func) override;
    /**
     * @brief Unregister release listener function.
     * 
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError UnRegisterReleaseListener() override;
    /**
     * @brief Unregister release listener function.
     * 
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError UnRegisterReleaseListenerBackup() override;
    /**
     * @brief Dump info of the surface.
     * 
     * @param result The info of the surface.
     */
    void Dump(std::string &result) const override {};
    /**
     * @brief Clean the surface buffer cache.
     * This interface will empty all caches of the current process.
     * 
     * @param cleanAll [in] Clean all buffer or reserve one buffer.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_CONSUMER_IS_CONNECTED} 41206000 - surface is connected by other client.
     */
    GSError CleanCache(bool cleanAll = false) override;
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
     * @brief The client establishes a connection to the server.
     * 
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_CONSUMER_IS_CONNECTED} 41206000 - surface is connected by other client.
     */
    GSError Connect() override;
    /**
     * @brief Terminate the client-server connection.
     * 
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link SURFACE_ERROR_CONSUMER_DISCONNECTED} 41211000 - surface is already disconnected.
     */
    GSError Disconnect() override;
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
     * @brief Set the Meta Data for the surface buffer
     * 
     * @param sequence [in] The number of surface buffer.
     * @param metaData [in] Meta data info.
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
     * @brief Set the Tunnel Handle for the surface.
     * 
     * @param handle [in] Tunnel handle.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetTunnelHandle(const GraphicExtDataHandle *handle) override;
    /**
     * @brief Get the Present Timestamp from the surface buffer.
     * 
     * @param sequence [in] The number of surface buffer.
     * @param type [in] Present timestamp type.
     * @param time [out] The present time info.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_ENTRY} 40602000 - Buffer not in cache.
     * {@link GSERROR_TYPE_ERROR 41204000} - The type is not support.
     */
    GSError GetPresentTimestamp(uint32_t sequence, GraphicPresentTimestampType type, int64_t &time) const override;
    /**
     * @brief Set the Wptr Native Window for the Surface.
     * 
     * @param nativeWindow [in] Indicates the pointer to a nativewindow instance.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetWptrNativeWindowToPSurface(void* nativeWindow) override;
    /**
     * @brief Register surface remote object.
     * 
     * @param client [in] The remote object.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    virtual GSError RegisterSurfaceDelegator(sptr<IRemoteObject> client) override;
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
     * @brief Set the Buffer Name.
     * 
     * @param name [in] The buffer name.
     * @return {@link GSERROR_OK} 0 - Success.
     */
    GSError SetBufferName(const std::string &name) override;
    /**
     * @brief Set the Request Width And Height for the surface.
     * 
     * @param width [in] The width of the surface.
     * @param height [in] The height of the surface.
     */
    void SetRequestWidthAndHeight(int32_t width, int32_t height) override;
    /**
     * @brief Get the Request Width of the surface.
     * 
     * @return int32_t The width of the surface.
     */
    int32_t GetRequestWidth() override;
    /**
     * @brief Get the Request Height of the surface.
     * 
     * @return int32_t The height of the surface.
     */
    int32_t GetRequestHeight() override;
    /**
     * @brief Set the Scaling Mode for the surface.
     * 
     * @param scalingMode [in] The scaling mode.
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
     * @brief Set the Window Config for the surface.
     * 
     * @param config [in] The buffer request config.
     */
    void SetWindowConfig(const BufferRequestConfig& config) override;
    /**
     * @brief Set the Window Config Width And Height for the surface.
     * 
     * @param width [in] The width of the surface.
     * @param height [in] The height of the surface.
     */
    void SetWindowConfigWidthAndHeight(int32_t width, int32_t height) override;
    /**
     * @brief Set the Window Config Stride for the surface.
     * 
     * @param stride [in] The stride type of the surface.
     */
    void SetWindowConfigStride(int32_t stride) override;
    /**
     * @brief Set the Window Config Format for the surface.
     * 
     * @param format [in] The format type of the surface.
     */
    void SetWindowConfigFormat(int32_t format) override;
    /**
     * @brief Set the Window Config Usage for the surface.
     * 
     * @param usage [in] The usage type of the surface.
     */
    void SetWindowConfigUsage(uint64_t usage) override;
    /**
     * @brief Set the Window Config Timeout for the surface.
     * 
     * @param timeout [in] The timeout of the surface.
     */
    void SetWindowConfigTimeout(int32_t timeout) override;
    /**
     * @brief Set the Window Config Color Gamut for the surface.
     * 
     * @param colorGamut [in] The colorGamut type of the surface.
     */
    void SetWindowConfigColorGamut(GraphicColorGamut colorGamut) override;
    /**
     * @brief Set the Window Config Transform for the surface.
     * 
     * @param transform [in] The transform type of the surface.
     */
    void SetWindowConfigTransform(GraphicTransformType transform) override;
    /**
     * @brief Get the Window Config from the surface.
     * 
     * @return BufferRequestConfig The config of the surface buffer.
     */
    BufferRequestConfig GetWindowConfig() override;
    /**
     * @brief Set the Hdr White Point Brightness for the surface.
     * 
     * @param brightness [in] The brightness value for the surface.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetHdrWhitePointBrightness(float brightness) override;
    /**
     * @brief Set the Sdr White Point Brightness for the surface.
     * 
     * @param brightness [in] The brightness value for the surface.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetSdrWhitePointBrightness(float brightness) override;
    /**
     * @brief Get the Producer Init Info from the surface.
     * 
     * @param info [out] The producer init information.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError GetProducerInitInfo(ProducerInitInfo &info) override;
    /**
     * @brief Acquire The last flushed buffer form the surface.
     * 
     * @param buffer [out] Indicates the pointer to a SurfaceBuffer instance.
     * @param fence [out] Fence fd for asynchronous waiting mechanism.
     * @param matrix [out] Orientation matrix.
     * @param matrixSize [in] The matrix array size.
     * @param isUseNewMatrix [in] Is use new matrix switch.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link SURFACE_ERROR_BUFFER_STATE_INVALID} 41207000 - Buffer state invalid.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Internal error.
     * {@link SURFACE_ERROR_NOT_SUPPORT} 50102000 - Not surport usage.
     */
    GSError AcquireLastFlushedBuffer(sptr<SurfaceBuffer> &buffer, sptr<SyncFence> &fence,
        float matrix[16], uint32_t matrixSize, bool isUseNewMatrix) override;
    /**
     * @brief Give back the last flushed buffer to the surface.
     * 
     * @param buffer [in] Indicates the pointer to a SurfaceBuffer instance.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link SURFACE_ERROR_BUFFER_STATE_INVALID} 41207000 - Buffer state invalid.
     */
    GSError ReleaseLastFlushedBuffer(sptr<SurfaceBuffer> buffer) override;
    /**
     * @brief Set the Global Alpha for the surface.
     * 
     * @param alpha [in] The Global Alpha of the surface.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetGlobalAlpha(int32_t alpha) override;
    /**
     * @brief Set the request buffer isblocked for the surface.
     * Blocking mode: Keep the original logic unchanged.
     * Non-blocking mode:
     * 1.If all buffers are requested and no buffer is in flushed state, immediately return GSERROR_NO_BUFFER.
     * 2.If all buffers are requested and there are buffers in flushed state, return the earliest flushed buffer
     * to the user through the RequestBuffer interface.
     *
     * @param noblock [in] Request buffer mode flag:
     * - false: Blocking request mode (default)
     * - true: Non-blocking request mode
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     */
    GSError SetRequestBufferNoblockMode(bool noblock = false) override;
    /**
     * @brief Check the surface is in hebc white list.
     * 
     * @return true Is in hebc white list.
     * @return false Not in the hebc white list.
     */
    virtual bool IsInHebcList() override
    {
        return initInfo_.isInHebcList;
    }
    /**
     * @brief Request and detach the buffer from the surface.
     * 
     * @param buffer [out] Indicates the pointer to a SurfaceBuffer instance.
     * @param fence [out] Fence fd for asynchronous waiting mechanism.
     * @param config [in] The parameter type for requesting the buffer.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link GSERROR_NO_CONSUMER} 41202000 - no consumer.
     * {@link GSERROR_NO_BUFFER} 40601000 - no buffer.
     * {@link} GSERROR_CONSUMER_IS_CONNECTED 41206000 - consumer is connected already.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Inner error.
     */
    GSError RequestAndDetachBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                   BufferRequestConfig& config) override;
    /**
     * @brief Attach and flush the buffer to the surface.
     * 
     * @param buffer [in] Indicates the pointer to a SurfaceBuffer instance.
     * @param fence [in] Fence fd for asynchronous waiting mechanism.
     * @param config [in] The parameter type for flushing the buffer.
     * @param needMap [in] Is need map.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Inner error.
     * {@link SURFACE_ERROR_BUFFER_IS_INCACHE} 41208000 - Buffer is in cache.
     * {@link SURFACE_ERROR_BUFFER_QUEUE_FULL} 41209000 - Buffer queue is full.
     * {@link GSERROR_NO_CONSUMER} 41202000 - no consumer.
     * {@link SURFACE_ERROR_CONSUMER_UNREGISTER_LISTENER} 41212000 - no register listener.
     */
    GSError AttachAndFlushBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
                                 BufferFlushConfig& config, bool needMap) override;
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
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Inner error.
     */
    GSError SetCycleBuffersNumber(uint32_t cycleBuffersNumber) override;
    /**
     * @brief Set the Frame Gravity for the surface.
     * 
     * @param frameGravity [in] The frame gravity value.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Inner error.
     */
    GSError SetFrameGravity(int32_t frameGravity) override;
    /**
     * @brief Set the Fixed Rotation for the surface.
     * 
     * @param fixedRotation [in] The frame rotation value.
     * @return {@link GSERROR_OK} 0 - Success.
     * {@link SURFACE_ERROR_UNKOWN} 50002000 - Inner error.
     */
    GSError SetFixedRotation(int32_t fixedRotation) override;
    /**
    * @brief The client establishes a connection to the server.
    * In the strictly disconnected state, the producer must call the ConnectStrictly() interface before request
    * buffer. Unlike Connect(), ConnectStrictly() does not distinguish between process IDs (PIDs) and is
    * suitable for stricter connection management scenarios.
    * @return {@link GSERROR_OK} 0 - Success.
    * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
    */
    GSError ConnectStrictly() override;
    /**
    * @brief Terminate the client-server connection.
    * After calling DisconnectStrictly(), the consumer (server) enter the strictly disconnected state.
    * In this state, any attempt by the producer (client) to request buffer will fail and return the error code
    * GSERROR_CONSUMER_DISCONNECTED.
    * @return {@link GSERROR_OK} 0 - Success.
    * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
    */
    GSError DisconnectStrictly() override;
    /**
    * @brief Advance buffer allocation.
    * 1.The interface needs to be used before first use of requestBuffer; otherwise, the buffer is already
    * allocated, causing the preAlloc interface to fail to optimize the buffer allocation time;
    * 2.The specifications of the SurfaceBuffer preAlloc cannot exceed the size of the bufferQueueCache;
    * 3.The interface is an asynchronous interface;
    * @param config [in] The parameter type for requesting the buffer.
    * @param allocBufferCount [in] The number of alloc buffer count.
    * @return {@link GSERROR_OK} 0 - Success.
    * {@link GSERROR_INVALID_ARGUMENTS} 40001000 - Param invalid.
    * {@link SURFACE_ERROR_BUFFER_QUEUE_FULL} 41209000 - Buffer queue is full.
    * {@link SURFACE_ERROR_OUT_OF_RANGE} 40603000 - out of range.
    */
    GSError PreAllocBuffers(const BufferRequestConfig &config, uint32_t allocBufferCount) override;  
    /**
     * @brief Request a buffer with lock.
     * 
     * @param config Indicates the buffer config to be requested.
     * @param region Indicates the info of the dirty region.
     * @param buffer Indicates the pointer to a <b>SurfaceBuffer</b> instance.
     * @return Returns the error code of the request of lock.
     * {@link GSERROR_INVALID_OPERATING} 41201000 - Operate invalid.
     */
    GSError ProducerSurfaceLockBuffer(BufferRequestConfig &config, Region region, sptr<SurfaceBuffer>& buffer) override;
    /**
     * @brief Unlock a buffer with lock.
     * 
     * @return Returns the error code of the request of unlock.
     * {@link GSERROR_INVALID_OPERATING} 41201000 - Operate invalid.
     */
    GSError ProducerSurfaceUnlockAndFlushBuffer() override;
    /**
     * @brief Set the fd of Lpp shared memory.
     * @param fd File descriptor.
     * @param state Link or Unlink.
     * @return Returns the error code of the request of unlock.
     * {@link GSERROR_INVALID_OPERATING} 41201000 - Operate invalid.
     */
    GSError SetLppShareFd(int fd, bool state) override;
private:
    GSError PropertyChangeCallback(const SurfaceProperty& property);
    GSError ResetPropertyListenerInner(uint64_t producerId);
    bool IsRemote();
    void CleanAllLocked(uint32_t *bufSeqNum);
    GSError AddCacheLocked(sptr<BufferExtraData> &bedataimpl,
        IBufferProducer::RequestBufferReturnValue &retval, BufferRequestConfig &config);
    GSError SetMetadataValue(sptr<SurfaceBuffer>& buffer);
    GSError CleanCacheLocked(bool cleanAll);
    void SetBufferConfigLocked(sptr<BufferExtraData>& bedataimpl,
        IBufferProducer::RequestBufferReturnValue& retval, BufferRequestConfig& config);
    void DeleteCacheBufferLocked(sptr<BufferExtraData>& bedataimpl,
        IBufferProducer::RequestBufferReturnValue& retval, BufferRequestConfig& config);
    GSError UpdateCacheLocked(sptr<BufferExtraData>& bedataimpl,
        IBufferProducer::RequestBufferReturnValue& retval, BufferRequestConfig& config);
    void ReleasePreCacheBuffer(int bufferCacheSize);

    GSError RequestBufferLocked(sptr<SurfaceBuffer>& buffer,
        sptr<SyncFence>& fence, BufferRequestConfig& config);
    GSError ProducerSurfaceCancelBufferLocked(sptr<SurfaceBuffer>& buffer);

    mutable std::mutex mutex_;
    std::atomic_bool inited_ = false;
    std::map<int32_t, sptr<SurfaceBuffer>> bufferProducerCache_;
    std::map<std::string, std::string> userData_;
    sptr<IBufferProducer> producer_ = nullptr;
    std::string name_ = "not init";
    uint64_t queueId_ = 0;
    bool isDisconnected_ = true;
    sptr<IProducerListener> listener_;
    sptr<IProducerListener> listenerBackup_;
    std::mutex listenerMutex_;
    wptr<NativeWindow> wpNativeWindow_ = nullptr;
    wptr<ProducerSurfaceDelegator> wpPSurfaceDelegator_ = nullptr;
    std::mutex delegatorMutex_;
    std::map<std::string, OnUserDataChangeFunc> onUserDataChange_;
    std::mutex lockMutex_;
    std::string bufferName_ = "";
    int32_t requestWidth_ = 0;
    int32_t requestHeight_ = 0;
    GraphicTransformType lastSetTransformHint_ = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    BufferRequestConfig windowConfig_ = {0};
    ProducerInitInfo initInfo_ = {0};
    sptr<SurfaceBuffer> preCacheBuffer_ = nullptr;
    sptr<SurfaceBuffer> mLockedBuffer_ = nullptr;
    Region region_ = {.rects = nullptr, .rectNumber = 0};
};
} // namespace OHOS

#endif // FRAMEWORKS_SURFACE_INCLUDE_PRODUCER_SURFACE_H
