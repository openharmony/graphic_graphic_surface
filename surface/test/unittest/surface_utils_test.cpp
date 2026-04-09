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
#include <securec.h>
#include <gtest/gtest.h>
#include <surface.h>
#include <consumer_surface.h>
#include <surface_utils.h>
#include "buffer_consumer_listener.h"
#include "surface_buffer_impl.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
class SurfaceUtilsTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    static bool IsArrayEmpty(const float arr[16]);
    static inline sptr<IConsumerSurface> csurface1 = nullptr;
    static inline sptr<IBufferProducer> producer1 = nullptr;
    static inline sptr<Surface> psurface1 = nullptr;

    static inline sptr<IConsumerSurface> csurface2 = nullptr;
    static inline sptr<IBufferProducer> producer2 = nullptr;
    static inline sptr<Surface> psurface2 = nullptr;

    static inline SurfaceUtils *utils = nullptr;
    static constexpr const int32_t TRANSFORM_MATRIX_SIZE = 16;
};

void SurfaceUtilsTest::SetUpTestCase()
{
    csurface1 = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener1 = new BufferConsumerListener();
    csurface1->RegisterConsumerListener(listener1);
    producer1 = csurface1->GetProducer();
    psurface1 = Surface::CreateSurfaceAsProducer(producer1);

    csurface2 = IConsumerSurface::Create();
    sptr<IBufferConsumerListener> listener2 = new BufferConsumerListener();
    csurface2->RegisterConsumerListener(listener2);
    producer2 = csurface2->GetProducer();
    psurface2 = Surface::CreateSurfaceAsProducer(producer2);
}

void SurfaceUtilsTest::TearDownTestCase()
{
    csurface1 = nullptr;
    producer1 = nullptr;
    psurface1 = nullptr;

    csurface2 = nullptr;
    producer2 = nullptr;
    psurface2 = nullptr;
    utils = nullptr;
}

/*
* Function: GetInstance
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetInstance
*                  2. check ret
 */
HWTEST_F(SurfaceUtilsTest, GetInstance001, TestSize.Level0)
{
    utils = SurfaceUtils::GetInstance();
    ASSERT_NE(utils, nullptr);
}

/*
* Function: GetInstance
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call Add
*                  2. check ret
 */
HWTEST_F(SurfaceUtilsTest, Add001, TestSize.Level0)
{
    GSError ret = utils->Add(psurface1->GetUniqueId(), nullptr);
    ASSERT_EQ(ret, OHOS::GSERROR_INVALID_ARGUMENTS);
}

/*
* Function: GetInstance
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call Add 2 times
*                  2. check ret
 */
HWTEST_F(SurfaceUtilsTest, Add002, TestSize.Level0)
{
    GSError ret = utils->Add(psurface1->GetUniqueId(), psurface1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = utils->Add(psurface1->GetUniqueId(), psurface1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
* Function: GetSurface
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetSurface by abnormal uniqueId
*                  2. check ret
 */
HWTEST_F(SurfaceUtilsTest, GetSurface001, TestSize.Level0)
{
    sptr<Surface> surface = utils->GetSurface(0);
    ASSERT_EQ(surface, nullptr);
}

/*
* Function: GetSurface
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetSurface
*                  2. check ret
 */
HWTEST_F(SurfaceUtilsTest, GetSurface002, TestSize.Level0)
{
    sptr<Surface> surface1 = utils->GetSurface(psurface1->GetUniqueId());
    ASSERT_NE(surface1, nullptr);
}

/*
* Function: GetSurface
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call GetSurface
*                  2. call Add
*                  3. call GetSurface again
*                  4. check ret
 */
HWTEST_F(SurfaceUtilsTest, GetSurface003, TestSize.Level0)
{
    sptr<Surface> surface2 = utils->GetSurface(psurface2->GetUniqueId());
    ASSERT_NE(surface2, nullptr);

    GSError ret = utils->Add(psurface2->GetUniqueId(), psurface2);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    surface2 = utils->GetSurface(psurface2->GetUniqueId());
    ASSERT_NE(surface2, nullptr);
}

/*
* Function: Remove
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call Remove
*                  2. check ret
 */
HWTEST_F(SurfaceUtilsTest, Remove001, TestSize.Level0)
{
    GSError ret = utils->Remove(0);
    ASSERT_EQ(ret, GSERROR_INVALID_OPERATING);
}

/*
* Function: Remove
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call Remove 2 times
*                  2. check ret
 */
HWTEST_F(SurfaceUtilsTest, Remove002, TestSize.Level0)
{
    GSError ret = utils->Remove(psurface1->GetUniqueId());
    ASSERT_EQ(ret, OHOS::GSERROR_OK);

    ret = utils->Remove(psurface1->GetUniqueId());
    ASSERT_EQ(ret, SURFACE_ERROR_INVALID_OPERATING);
}

/*
* Function: ComputeTransformMatrix
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call ComputeTransformMatrix
*                  2. call ComputeTransformMatrixV2
 */
HWTEST_F(SurfaceUtilsTest, ComputeTransformMatrix001, TestSize.Level0)
{
    // Prepare params
    sptr<SurfaceBuffer> buffer = new SurfaceBufferImpl();
    buffer->SetSurfaceBufferWidth(1920);
    buffer->SetSurfaceBufferHeight(1920);
    float matrix[TRANSFORM_MATRIX_SIZE];
    Rect crop = {};
    crop.w = buffer->GetWidth();
    crop.h = buffer->GetHeight();
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_FLIP_H;
    float emptyMatrix[TRANSFORM_MATRIX_SIZE];
    //invalid parameter
    sptr<SurfaceBuffer> tmpBuffer = nullptr;
    utils->ComputeTransformMatrix(matrix, TRANSFORM_MATRIX_SIZE, tmpBuffer, transform, crop);
    ASSERT_TRUE(IsArrayEmpty(matrix));
    utils->ComputeTransformMatrixV2(matrix, TRANSFORM_MATRIX_SIZE, tmpBuffer, transform, crop);
    ASSERT_TRUE(IsArrayEmpty(matrix));

    // Compute matrix with normal crop
    utils->ComputeTransformMatrix(matrix, 16, buffer, transform, crop);
    ASSERT_NE(matrix, emptyMatrix);
    utils->ComputeTransformMatrixV2(matrix, 16, buffer, transform, crop);
    ASSERT_NE(matrix, emptyMatrix);
}

bool SurfaceUtilsTest::IsArrayEmpty(const float arr[TRANSFORM_MATRIX_SIZE])
{
    return std::all_of(arr, arr + TRANSFORM_MATRIX_SIZE, [](float value) { return value == 0.0f; });
}

/*
* Function: ComputeTransformMatrix
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: 1. call ComputeTransformMatrix with small crop
*                  2. call ComputeTransformMatrixV2 with small crop
 */
HWTEST_F(SurfaceUtilsTest, ComputeTransformMatrix002, TestSize.Level0)
{
    // Prepare params
    sptr<SurfaceBuffer> buffer = new SurfaceBufferImpl();
    buffer->SetSurfaceBufferWidth(1920);
    buffer->SetSurfaceBufferHeight(1920);
    float matrix[16];
    Rect crop = {};
    crop.w = 100;
    crop.h = 100;
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_FLIP_H;

    // Compute matrix with normal crop
    float emptyMatrix[16];
    utils->ComputeTransformMatrix(matrix, 16, buffer, transform, crop);
    ASSERT_NE(matrix, emptyMatrix);
    utils->ComputeTransformMatrixV2(matrix, 16, buffer, transform, crop);
    ASSERT_NE(matrix, emptyMatrix);
}

/*
 * Function: ComputeTransformMatrix
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call ComputeTransformMatrix with different transform types
 *                  2. call ComputeTransformMatrixV2 with different transform types
 */
HWTEST_F(SurfaceUtilsTest, ComputeTransformMatrix003, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = new SurfaceBufferImpl();
    buffer->SetSurfaceBufferWidth(1920);
    buffer->SetSurfaceBufferHeight(1080);
    float matrix[16];
    Rect crop = {};
    crop.w = buffer->GetWidth();
    crop.h = buffer->GetHeight();

    std::vector<GraphicTransformType> transforms = {
        GraphicTransformType::GRAPHIC_ROTATE_NONE,
        GraphicTransformType::GRAPHIC_ROTATE_90,
        GraphicTransformType::GRAPHIC_ROTATE_180,
        GraphicTransformType::GRAPHIC_ROTATE_270,
        GraphicTransformType::GRAPHIC_FLIP_V,
        GraphicTransformType::GRAPHIC_FLIP_H_ROT90,
        GraphicTransformType::GRAPHIC_FLIP_V_ROT90,
        GraphicTransformType::GRAPHIC_FLIP_H_ROT180,
        GraphicTransformType::GRAPHIC_FLIP_V_ROT180,
        GraphicTransformType::GRAPHIC_FLIP_H_ROT270,
        GraphicTransformType::GRAPHIC_FLIP_V_ROT270
    };

    for (auto transform : transforms) {
        utils->ComputeTransformMatrix(matrix, 16, buffer, transform, crop);
        ASSERT_FALSE(IsArrayEmpty(matrix));
        utils->ComputeTransformMatrixV2(matrix, 16, buffer, transform, crop);
        ASSERT_FALSE(IsArrayEmpty(matrix));
    }
}

/*
 * Function: ComputeTransformMatrix
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call ComputeTransformMatrix with zero crop
 *                  2. call ComputeTransformMatrixV2 with zero crop
 */
HWTEST_F(SurfaceUtilsTest, ComputeTransformMatrix004, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = new SurfaceBufferImpl();
    buffer->SetSurfaceBufferWidth(1920);
    buffer->SetSurfaceBufferHeight(1080);
    float matrix[16];
    Rect crop = {};
    crop.w = 0;
    crop.h = 0;
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_NONE;

    utils->ComputeTransformMatrix(matrix, 16, buffer, transform, crop);
    utils->ComputeTransformMatrixV2(matrix, 16, buffer, transform, crop);
}

/*
 * Function: ComputeTransformMatrix
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call ComputeTransformMatrix with invalid matrix size
 *                  2. call ComputeTransformMatrixV2 with invalid matrix size
 */
HWTEST_F(SurfaceUtilsTest, ComputeTransformMatrix005, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = new SurfaceBufferImpl();
    buffer->SetSurfaceBufferWidth(1920);
    buffer->SetSurfaceBufferHeight(1080);
    float matrix[16];
    Rect crop = {};
    crop.w = buffer->GetWidth();
    crop.h = buffer->GetHeight();
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_NONE;

    utils->ComputeTransformMatrix(matrix, 0, buffer, transform, crop);
    utils->ComputeTransformMatrixV2(matrix, 0, buffer, transform, crop);
}

/*
 * Function: ComputeBufferMatrix
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call ComputeBufferMatrix with normal parameters
 */
HWTEST_F(SurfaceUtilsTest, ComputeBufferMatrix001, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = new SurfaceBufferImpl();
    buffer->SetSurfaceBufferWidth(1920);
    buffer->SetSurfaceBufferHeight(1080);
    float matrix[16];
    Rect crop = {};
    crop.w = buffer->GetWidth();
    crop.h = buffer->GetHeight();
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_NONE;

    utils->ComputeBufferMatrix(matrix, 16, buffer, transform, crop);
    ASSERT_FALSE(IsArrayEmpty(matrix));
}

/*
 * Function: ComputeBufferMatrix
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call ComputeBufferMatrix with different transform types
 */
HWTEST_F(SurfaceUtilsTest, ComputeBufferMatrix002, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = new SurfaceBufferImpl();
    buffer->SetSurfaceBufferWidth(1920);
    buffer->SetSurfaceBufferHeight(1080);
    float matrix[16];
    Rect crop = {};
    crop.w = buffer->GetWidth();
    crop.h = buffer->GetHeight();

    std::vector<GraphicTransformType> transforms = {
        GraphicTransformType::GRAPHIC_ROTATE_90,
        GraphicTransformType::GRAPHIC_ROTATE_180,
        GraphicTransformType::GRAPHIC_ROTATE_270,
        GraphicTransformType::GRAPHIC_FLIP_H,
        GraphicTransformType::GRAPHIC_FLIP_V
    };

    for (auto transform : transforms) {
        utils->ComputeBufferMatrix(matrix, 16, buffer, transform, crop);
        ASSERT_FALSE(IsArrayEmpty(matrix));
    }
}

/*
 * Function: ComputeBufferMatrix
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call ComputeBufferMatrix with null buffer
 */
HWTEST_F(SurfaceUtilsTest, ComputeBufferMatrix003, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = nullptr;
    float matrix[16];
    Rect crop = {};
    crop.w = 1920;
    crop.h = 1080;
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_NONE;

    utils->ComputeBufferMatrix(matrix, 16, buffer, transform, crop);
}

/*
 * Function: AddNativeWindow
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call AddNativeWindow with null window
 *                  2. check ret
 */
HWTEST_F(SurfaceUtilsTest, AddNativeWindow001, TestSize.Level0)
{
    SurfaceError ret = utils->AddNativeWindow(psurface1->GetUniqueId(), nullptr);
    ASSERT_EQ(ret, SURFACE_ERROR_NULLPTR);
}

/*
 * Function: AddNativeWindow
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call AddNativeWindow
 *                  2. check ret
 */
HWTEST_F(SurfaceUtilsTest, AddNativeWindow002, TestSize.Level0)
{
    void* nativeWindow = reinterpret_cast<void*>(0x1234);
    SurfaceError ret = utils->AddNativeWindow(psurface1->GetUniqueId(), nativeWindow);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);
}

/*
 * Function: AddNativeWindow
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call AddNativeWindow 2 times
 *                  2. check ret
 */
HWTEST_F(SurfaceUtilsTest, AddNativeWindow003, TestSize.Level0)
{
    void* nativeWindow = reinterpret_cast<void*>(0x5678);
    SurfaceError ret = utils->AddNativeWindow(psurface2->GetUniqueId(), nativeWindow);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);

    ret = utils->AddNativeWindow(psurface2->GetUniqueId(), nativeWindow);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);
}

/*
 * Function: GetNativeWindow
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call GetNativeWindow with invalid uniqueId
 *                  2. check ret
 */
HWTEST_F(SurfaceUtilsTest, GetNativeWindow001, TestSize.Level0)
{
    void* nativeWindow = utils->GetNativeWindow(0);
    ASSERT_EQ(nativeWindow, nullptr);
}

/*
 * Function: RemoveNativeWindow
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call AddNativeWindow
 *                  2. call RemoveNativeWindow
 *                  3. check ret
 */
HWTEST_F(SurfaceUtilsTest, RemoveNativeWindow002, TestSize.Level0)
{
    void* nativeWindow = reinterpret_cast<void*>(0xDEF0);
    utils->AddNativeWindow(psurface2->GetUniqueId(), nativeWindow);

    SurfaceError ret = utils->RemoveNativeWindow(psurface2->GetUniqueId());
    ASSERT_EQ(ret, SURFACE_ERROR_OK);
}

/*
 * Function: Add
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call Add with invalid uniqueId
 *                  2. check ret
 */
HWTEST_F(SurfaceUtilsTest, Add003, TestSize.Level0)
{
    GSError ret = utils->Add(0, psurface1);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}

/*
 * Function: GetSurface
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call Add with invalid uniqueId
 *                  2. call GetSurface with invalid uniqueId
 *                  3. check ret
 */
HWTEST_F(SurfaceUtilsTest, GetSurface004, TestSize.Level0)
{
    utils->Add(0, psurface1);
    sptr<Surface> surface = utils->GetSurface(0);
    ASSERT_NE(surface, nullptr);
}

/*
 * Function: Remove
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. call Add with invalid uniqueId
 *                  2. call Remove with invalid uniqueId
 *                  3. check ret
 */
HWTEST_F(SurfaceUtilsTest, Remove003, TestSize.Level0)
{
    utils->Add(0, psurface1);
    GSError ret = utils->Remove(0);
    ASSERT_EQ(ret, OHOS::GSERROR_OK);
}
}