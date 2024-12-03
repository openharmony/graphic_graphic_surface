/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <vector>

#include "surface.h"
#include "sync_fence.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class BaseSurface : public Surface {
public:
    bool IsConsumer() const override
    {
        return false;
    }
    sptr<IBufferProducer> GetProducer() const override
    {
        return nullptr;
    }
    GSError AttachBuffer(sptr<SurfaceBuffer> &buffer) override
    {
        (void)buffer;
        return GSERROR_NOT_SUPPORT;
    }
    GSError DetachBuffer(sptr<SurfaceBuffer> &buffer) override
    {
        (void)buffer;
        return GSERROR_NOT_SUPPORT;
    }
    uint32_t GetQueueSize() override
    {
        return 0;
    }
    GSError SetQueueSize(uint32_t queueSize) override
    {
        (void)queueSize;
        return GSERROR_NOT_SUPPORT;
    }
    int32_t GetDefaultWidth() override
    {
        return 0;
    }
    int32_t GetDefaultHeight() override
    {
        return 0;
    }
    GSError SetDefaultUsage(uint64_t usage) override
    {
        (void)usage;
        return GSERROR_NOT_SUPPORT;
    }
    uint64_t GetDefaultUsage() override
    {
        return 0;
    }
    GSError SetUserData(const std::string &key, const std::string &val) override
    {
        (void)key;
        (void)val;
        return GSERROR_NOT_SUPPORT;
    }
    std::string GetUserData(const std::string &key) override
    {
        (void)key;
        return std::string("");
    }
    const std::string &GetName() override
    {
        return name_;
    }
    uint64_t GetUniqueId() const override
    {
        return 0;
    }
    GSError GoBackground() override
    {
        return GSERROR_NOT_SUPPORT;
    }
    GSError SetTransform(GraphicTransformType transform) override
    {
        (void)transform;
        return GSERROR_NOT_SUPPORT;
    }
    GraphicTransformType GetTransform() const override
    {
        return GRAPHIC_ROTATE_NONE;
    }
    GSError SetScalingMode(uint32_t sequence, ScalingMode scalingMode) override
    {
        (void)sequence;
        (void)scalingMode;
        return GSERROR_NOT_SUPPORT;
    }
    GSError SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData) override
    {
        (void)sequence;
        (void)metaData;
        return GSERROR_NOT_SUPPORT;
    }
    GSError SetMetaDataSet(
        uint32_t sequence, GraphicHDRMetadataKey key, const std::vector<uint8_t> &metaData) override
    {
        (void)sequence;
        (void)key;
        (void)metaData;
        return GSERROR_NOT_SUPPORT;
    }
    GSError SetTunnelHandle(const GraphicExtDataHandle *handle) override
    {
        (void)handle;
        return GSERROR_NOT_SUPPORT;
    }
    void Dump(std::string &result) const override
    {
        (void)result;
    }
    GSError AttachBuffer(sptr<SurfaceBuffer> &buffer, int32_t timeOut) override
    {
        (void)buffer;
        (void)timeOut;
        return GSERROR_NOT_SUPPORT;
    }
    GSError RegisterSurfaceDelegator(sptr<IRemoteObject> client) override
    {
        (void)client;
        return GSERROR_NOT_SUPPORT;
    }
    GSError RegisterReleaseListener(OnReleaseFuncWithFence func) override
    {
        (void)func;
        return GSERROR_NOT_SUPPORT;
    }
    GSError RegisterUserDataChangeListener(const std::string &funcName, OnUserDataChangeFunc func) override
    {
        (void)funcName;
        (void)func;
        return GSERROR_NOT_SUPPORT;
    }
    GSError UnRegisterUserDataChangeListener(const std::string &funcName) override
    {
        (void)funcName;
        return GSERROR_NOT_SUPPORT;
    }
    GSError ClearUserDataChangeListener() override
    {
        return GSERROR_NOT_SUPPORT;
    }
    GSError AttachBufferToQueue(sptr<SurfaceBuffer> buffer) override
    {
        (void)buffer;
        return GSERROR_NOT_SUPPORT;
    }
    GSError DetachBufferFromQueue(sptr<SurfaceBuffer> buffer) override
    {
        (void)buffer;
        return GSERROR_NOT_SUPPORT;
    }
    GraphicTransformType GetTransformHint() const override
    {
        return GRAPHIC_ROTATE_NONE;
    }
    GSError SetTransformHint(GraphicTransformType transformHint) override
    {
        (void)transformHint;
        return GSERROR_NOT_SUPPORT;
    }
    void SetBufferHold(bool hold) override
    {
        (void)hold;
    }
    GSError SetScalingMode(ScalingMode scalingMode) override
    {
        (void)scalingMode;
        return GSERROR_NOT_SUPPORT;
    }
    GSError SetSurfaceSourceType(OHSurfaceSource sourceType) override
    {
        (void)sourceType;
        return GSERROR_NOT_SUPPORT;
    }
    OHSurfaceSource GetSurfaceSourceType() const override
    {
        return OH_SURFACE_SOURCE_DEFAULT;
    }
    GSError SetSurfaceAppFrameworkType(std::string appFrameworkType) override
    {
        (void)appFrameworkType;
        return GSERROR_NOT_SUPPORT;
    }
    std::string GetSurfaceAppFrameworkType() const override
    {
        return std::string("");
    }

private:
    const std::string name_ = "";
};

class SurfaceTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    static inline sptr<BaseSurface> surface = new BaseSurface();
    static inline ProducerInitInfo info;
    static inline sptr<SurfaceBuffer> buffer = nullptr;
    static inline int32_t fence = 0;
    static inline sptr<SyncFence> fencePtr = nullptr;
    static inline BufferRequestConfig bufferRequestConfig;
    static inline std::vector<sptr<SurfaceBuffer>> buffers = {};
    static inline std::vector<sptr<SyncFence>> fences = {};
    static inline BufferFlushConfig bufferFlushConfig;
    static inline int64_t timestamp = 0;
    static inline Rect damage;
    static inline int32_t width = 0;
    static inline int32_t height = 0;
    static inline sptr<IBufferConsumerListener> listener = nullptr;
    static inline OnReleaseFunc onReleaseFunc = nullptr;
    static inline OnDeleteBufferFunc onDeleteBufferFunc = nullptr;
    static inline uint32_t sequence = 0;
    static inline ScalingMode scalingMode;
    static inline HDRMetaDataType hdrMetaDataType;
    static inline std::vector<GraphicHDRMetaData> graphicHDRMetaDatas = {};
    static inline GraphicHDRMetadataKey key;
    static inline std::vector<uint8_t> metaDatas = {};
    static inline GraphicPresentTimestamp graphicPresentTimestamp;
    static inline GraphicPresentTimestampType graphicPresentTimestampType;
    static inline int64_t time = 0;
    static inline int32_t format = 0;
    static inline int32_t colorGamut = 0;
    static inline BufferFlushConfigWithDamages bufferFlushConfigWithDamages;
    static inline std::vector<BufferFlushConfigWithDamages> bufferFlushConfigWithDamagesVector = {};
    static inline float matrix[16];
    static inline int32_t stride = 0;
    static inline uint64_t usage = 0;
    static inline int32_t timeout = 0;
    static inline GraphicColorGamut graphicColorGamut;
    static inline GraphicTransformType transform;
    static inline float brightness = 0.0f;
    static inline uint32_t matrixSize = 0;
    static inline int32_t alpha = 0;
};

/*
* Function: SurfaceTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test functions from Surface
*/
HWTEST_F(SurfaceTest, SurfaceTest001, Function | MediumTest | Level2)
{
    EXPECT_EQ(surface->GetProducerInitInfo(info), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->RequestBuffer(buffer, fence, bufferRequestConfig), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->RequestBuffers(buffers, fences, bufferRequestConfig), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->CancelBuffer(buffer), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->FlushBuffer(buffer, fence, bufferFlushConfig), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->AcquireBuffer(buffer, fence, timestamp, damage), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->ReleaseBuffer(buffer, fence), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->RequestBuffer(buffer, fencePtr, bufferRequestConfig), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->FlushBuffer(buffer, fencePtr, bufferFlushConfig), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->AcquireBuffer(buffer, fencePtr, timestamp, damage), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->ReleaseBuffer(buffer, fencePtr), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->SetDefaultWidthAndHeight(width, height), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->RegisterConsumerListener(listener), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->Surface::RegisterReleaseListener(onReleaseFunc), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->RegisterDeleteBufferListener(onDeleteBufferFunc, false), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->UnregisterConsumerListener(), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->CleanCache(false), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->Connect(), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->Disconnect(), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->GetScalingMode(sequence, scalingMode), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->QueryMetaDataType(sequence, hdrMetaDataType), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->GetMetaData(sequence, graphicHDRMetaDatas), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->GetMetaDataSet(sequence, key, metaDatas), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->GetTunnelHandle(), nullptr);
    EXPECT_EQ(surface->SetPresentTimestamp(sequence, graphicPresentTimestamp), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->GetPresentTimestamp(sequence, graphicPresentTimestampType, time), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->GetDefaultFormat(), 0);
    EXPECT_EQ(surface->SetDefaultFormat(format), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->GetDefaultColorGamut(), 0);
    EXPECT_EQ(surface->SetDefaultColorGamut(colorGamut), GSERROR_NOT_SUPPORT);
}

/*
* Function: SurfaceTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test functions from Surface
*/
HWTEST_F(SurfaceTest, SurfaceTest002, Function | MediumTest | Level2)
{
    EXPECT_EQ(surface->GetNativeSurface(), nullptr);
    EXPECT_EQ(surface->QueryIfBufferAvailable(), false);
    EXPECT_EQ(surface->FlushBuffer(buffer, fencePtr, bufferFlushConfigWithDamages), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->FlushBuffers(buffers, fences, bufferFlushConfigWithDamagesVector), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->UnRegisterReleaseListener(), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->SetWptrNativeWindowToPSurface(nullptr), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->GetLastFlushedBuffer(buffer, fencePtr, matrix, false), GSERROR_NOT_SUPPORT);
    surface->SetRequestWidthAndHeight(width, height);
    EXPECT_EQ(surface->GetRequestWidth(), 0);
    EXPECT_EQ(surface->GetRequestHeight(), 0);
    surface->SetWindowConfig(bufferRequestConfig);
    surface->SetWindowConfigWidthAndHeight(width, height);
    surface->SetWindowConfigStride(stride);
    surface->SetWindowConfigFormat(format);
    surface->SetWindowConfigUsage(usage);
    surface->SetWindowConfigTimeout(timeout);
    surface->SetWindowConfigColorGamut(graphicColorGamut);
    surface->SetWindowConfigTransform(transform);
    surface->GetWindowConfig();
    EXPECT_EQ(surface->SetHdrWhitePointBrightness(brightness), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->SetSdrWhitePointBrightness(brightness), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->AcquireLastFlushedBuffer(buffer, fencePtr, matrix, matrixSize, false), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->ReleaseLastFlushedBuffer(buffer), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->SetGlobalAlpha(alpha), GSERROR_NOT_SUPPORT);
    EXPECT_EQ(surface->IsInHebcList(), false);
}
} // namespace OHOS::Rosen