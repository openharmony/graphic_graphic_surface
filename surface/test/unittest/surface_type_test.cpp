/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "surface_type.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

class SurfaceTypeTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
};

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicLayerAlpha operator==
*/
HWTEST_F(SurfaceTypeTest, GraphicLayerAlphaEqualTest, Function | SmallTest | Level1)
{
    GraphicLayerAlpha alpha1 = {
        .enGlobalAlpha = true,
        .enPixelAlpha = false,
        .alpha0 = 100,
        .alpha1 = 150,
        .gAlpha = 200
    };

    GraphicLayerAlpha alpha2 = {
        .enGlobalAlpha = true,
        .enPixelAlpha = false,
        .alpha0 = 100,
        .alpha1 = 150,
        .gAlpha = 200
    };

    EXPECT_TRUE(alpha1 == alpha2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicLayerAlpha operator!=
*/
HWTEST_F(SurfaceTypeTest, GraphicLayerAlphaNotEqualTest, Function | SmallTest | Level1)
{
    GraphicLayerAlpha alpha1 = {
        .enGlobalAlpha = true,
        .enPixelAlpha = false,
        .alpha0 = 100,
        .alpha1 = 150,
        .gAlpha = 200
    };

    GraphicLayerAlpha alpha2 = {
        .enGlobalAlpha = false,
        .enPixelAlpha = true,
        .alpha0 = 101,
        .alpha1 = 151,
        .gAlpha = 201
    };

    EXPECT_FALSE(alpha1 == alpha2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicLayerAlpha enGlobalAlpha difference
*/
HWTEST_F(SurfaceTypeTest, GraphicLayerAlphaEnGlobalAlphaTest, Function | SmallTest | Level1)
{
    GraphicLayerAlpha alpha1 = {
        .enGlobalAlpha = true,
        .enPixelAlpha = false,
        .alpha0 = 100,
        .alpha1 = 150,
        .gAlpha = 200
    };

    GraphicLayerAlpha alpha2 = {
        .enGlobalAlpha = false,
        .enPixelAlpha = false,
        .alpha0 = 100,
        .alpha1 = 150,
        .gAlpha = 200
    };

    EXPECT_FALSE(alpha1 == alpha2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicLayerAlpha enPixelAlpha difference
*/
HWTEST_F(SurfaceTypeTest, GraphicLayerAlphaEnPixelAlphaTest, Function | SmallTest | Level1)
{
    GraphicLayerAlpha alpha1 = {
        .enGlobalAlpha = true,
        .enPixelAlpha = true,
        .alpha0 = 100,
        .alpha1 = 150,
        .gAlpha = 200
    };

    GraphicLayerAlpha alpha2 = {
        .enGlobalAlpha = true,
        .enPixelAlpha = false,
        .alpha0 = 100,
        .alpha1 = 150,
        .gAlpha = 200
    };

    EXPECT_FALSE(alpha1 == alpha2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicLayerAlpha alpha0 difference
*/
HWTEST_F(SurfaceTypeTest, GraphicLayerAlphaAlpha0Test, Function | SmallTest | Level1)
{
    GraphicLayerAlpha alpha1 = {
        .enGlobalAlpha = true,
        .enPixelAlpha = true,
        .alpha0 = 100,
        .alpha1 = 150,
        .gAlpha = 200
    };

    GraphicLayerAlpha alpha2 = {
        .enGlobalAlpha = true,
        .enPixelAlpha = true,
        .alpha0 = 101,
        .alpha1 = 150,
        .gAlpha = 200
    };

    EXPECT_FALSE(alpha1 == alpha2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicLayerAlpha alpha1 difference
*/
HWTEST_F(SurfaceTypeTest, GraphicLayerAlphaAlpha1Test, Function | SmallTest | Level1)
{
    GraphicLayerAlpha alpha1 = {
        .enGlobalAlpha = true,
        .enPixelAlpha = true,
        .alpha0 = 100,
        .alpha1 = 150,
        .gAlpha = 200
    };

    GraphicLayerAlpha alpha2 = {
        .enGlobalAlpha = true,
        .enPixelAlpha = true,
        .alpha0 = 100,
        .alpha1 = 151,
        .gAlpha = 200
    };

    EXPECT_FALSE(alpha1 == alpha2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicLayerAlpha gAlpha difference
*/
HWTEST_F(SurfaceTypeTest, GraphicLayerAlphaGAlphaTest, Function | SmallTest | Level1)
{
    GraphicLayerAlpha alpha1 = {
        .enGlobalAlpha = true,
        .enPixelAlpha = true,
        .alpha0 = 100,
        .alpha1 = 150,
        .gAlpha = 200
    };

    GraphicLayerAlpha alpha2 = {
        .enGlobalAlpha = true,
        .enPixelAlpha = true,
        .alpha0 = 100,
        .alpha1 = 150,
        .gAlpha = 201
    };

    EXPECT_FALSE(alpha1 == alpha2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicLayerAlpha boundary values
*/
HWTEST_F(SurfaceTypeTest, GraphicLayerAlphaBoundaryTest, Function | SmallTest | Level1)
{
    GraphicLayerAlpha alpha1 = {
        .enGlobalAlpha = true,
        .enPixelAlpha = true,
        .alpha0 = 0,
        .alpha1 = 255,
        .gAlpha = 128
    };

    GraphicLayerAlpha alpha2 = {
        .enGlobalAlpha = true,
        .enPixelAlpha = true,
        .alpha0 = 0,
        .alpha1 = 255,
        .gAlpha = 128
    };

    EXPECT_TRUE(alpha1 == alpha2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicPresentTimestamp operator==
*/
HWTEST_F(SurfaceTypeTest, GraphicPresentTimestampEqualTest, Function | SmallTest | Level1)
{
    GraphicPresentTimestamp timestamp1 = {
        .type = GRAPHIC_DISPLAY_PTS_TIMESTAMP,
        .time = 1234567890
    };

    GraphicPresentTimestamp timestamp2 = {
        .type = GRAPHIC_DISPLAY_PTS_TIMESTAMP,
        .time = 1234567890
    };

    EXPECT_TRUE(timestamp1 == timestamp2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicPresentTimestamp operator!=
*/
HWTEST_F(SurfaceTypeTest, GraphicPresentTimestampNotEqualTest, Function | SmallTest | Level1)
{
    GraphicPresentTimestamp timestamp1 = {
        .type = GRAPHIC_DISPLAY_PTS_TIMESTAMP,
        .time = 1234567890
    };

    GraphicPresentTimestamp timestamp2 = {
        .type = GRAPHIC_DISPLAY_PTS_DELAY,
        .time = 9876543210
    };

    EXPECT_FALSE(timestamp1 == timestamp2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicPresentTimestamp type difference
*/
HWTEST_F(SurfaceTypeTest, GraphicPresentTimestampTypeTest, Function | SmallTest | Level1)
{
    GraphicPresentTimestamp timestamp1 = {
        .type = GRAPHIC_DISPLAY_PTS_TIMESTAMP,
        .time = 1234567890
    };

    GraphicPresentTimestamp timestamp2 = {
        .type = GRAPHIC_DISPLAY_PTS_DELAY,
        .time = 1234567890
    };

    EXPECT_FALSE(timestamp1 == timestamp2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicPresentTimestamp time difference
*/
HWTEST_F(SurfaceTypeTest, GraphicPresentTimestampTimeTest, Function | SmallTest | Level1)
{
    GraphicPresentTimestamp timestamp1 = {
        .type = GRAPHIC_DISPLAY_PTS_TIMESTAMP,
        .time = 1234567890
    };

    GraphicPresentTimestamp timestamp2 = {
        .type = GRAPHIC_DISPLAY_PTS_TIMESTAMP,
        .time = 1234567891
    };

    EXPECT_FALSE(timestamp1 == timestamp2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicPresentTimestamp UNSUPPORTED type
*/
HWTEST_F(SurfaceTypeTest, GraphicPresentTimestampUnsupportedTest, Function | SmallTest | Level1)
{
    GraphicPresentTimestamp timestamp1 = {
        .type = GRAPHIC_DISPLAY_PTS_UNSUPPORTED,
        .time = 0
    };

    GraphicPresentTimestamp timestamp2 = {
        .type = GRAPHIC_DISPLAY_PTS_UNSUPPORTED,
        .time = 0
    };

    EXPECT_TRUE(timestamp1 == timestamp2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicPresentTimestamp negative time
*/
HWTEST_F(SurfaceTypeTest, GraphicPresentTimestampNegativeTimeTest, Function | SmallTest | Level1)
{
    GraphicPresentTimestamp timestamp1 = {
        .type = GRAPHIC_DISPLAY_PTS_TIMESTAMP,
        .time = -12345
    };

    GraphicPresentTimestamp timestamp2 = {
        .type = GRAPHIC_DISPLAY_PTS_TIMESTAMP,
        .time = -12345
    };

    EXPECT_TRUE(timestamp1 == timestamp2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicPresentTimestamp max time value
*/
HWTEST_F(SurfaceTypeTest, GraphicPresentTimestampMaxTimeTest, Function | SmallTest | Level1)
{
    GraphicPresentTimestamp timestamp1 = {
        .type = GRAPHIC_DISPLAY_PTS_TIMESTAMP,
        .time = 9223372036854775807LL
    };

    GraphicPresentTimestamp timestamp2 = {
        .type = GRAPHIC_DISPLAY_PTS_TIMESTAMP,
        .time = 9223372036854775807LL
    };

    EXPECT_TRUE(timestamp1 == timestamp2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaDataSet operator==
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataSetEqualTest, Function | SmallTest | Level1)
{
    std::vector<uint8_t> data1 = {1, 2, 3, 4, 5};
    std::vector<uint8_t> data2 = {1, 2, 3, 4, 5};

    GraphicHDRMetaDataSet metaSet1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .metaData = data1
    };

    GraphicHDRMetaDataSet metaSet2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .metaData = data2
    };

    EXPECT_TRUE(metaSet1 == metaSet2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaDataSet operator!=
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataSetNotEqualTest, Function | SmallTest | Level1)
{
    std::vector<uint8_t> data1 = {1, 2, 3, 4, 5};
    std::vector<uint8_t> data2 = {5, 4, 3, 2, 1};

    GraphicHDRMetaDataSet metaSet1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .metaData = data1
    };

    GraphicHDRMetaDataSet metaSet2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_GREEN_PRIMARY_X,
        .metaData = data2
    };

    EXPECT_FALSE(metaSet1 == metaSet2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaDataSet key difference
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataSetKeyTest, Function | SmallTest | Level1)
{
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};

    GraphicHDRMetaDataSet metaSet1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .metaData = data
    };

    GraphicHDRMetaDataSet metaSet2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_GREEN_PRIMARY_X,
        .metaData = data
    };

    EXPECT_FALSE(metaSet1 == metaSet2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaDataSet metaData difference
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataSetMetaDataTest, Function | SmallTest | Level1)
{
    std::vector<uint8_t> data1 = {1, 2, 3, 4, 5};
    std::vector<uint8_t> data2 = {1, 2, 3, 4, 6};

    GraphicHDRMetaDataSet metaSet1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .metaData = data1
    };

    GraphicHDRMetaDataSet metaSet2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .metaData = data2
    };

    EXPECT_FALSE(metaSet1 == metaSet2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaDataSet empty metaData
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataSetEmptyTest, Function | SmallTest | Level1)
{
    std::vector<uint8_t> data;

    GraphicHDRMetaDataSet metaSet1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .metaData = data
    };

    GraphicHDRMetaDataSet metaSet2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .metaData = data
    };

    EXPECT_TRUE(metaSet1 == metaSet2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaDataSet size difference
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataSetSizeTest, Function | SmallTest | Level1)
{
    std::vector<uint8_t> data1 = {1, 2, 3};
    std::vector<uint8_t> data2 = {1, 2, 3, 4};

    GraphicHDRMetaDataSet metaSet1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .metaData = data1
    };

    GraphicHDRMetaDataSet metaSet2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .metaData = data2
    };

    EXPECT_FALSE(metaSet1 == metaSet2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaDataSet all keys
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataSetAllKeysTest, Function | SmallTest | Level1)
{
    std::vector<uint8_t> data = {1, 2, 3, 4};

    GraphicHDRMetaDataSet metaSet1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_X,
        .metaData = data
    };

    GraphicHDRMetaDataSet metaSet2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_RED_PRIMARY_Y,
        .metaData = data
    };

    GraphicHDRMetaDataSet metaSet3 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_GREEN_PRIMARY_X,
        .metaData = data
    };

    GraphicHDRMetaDataSet metaSet4 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_GREEN_PRIMARY_Y,
        .metaData = data
    };

    GraphicHDRMetaDataSet metaSet5 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_BLUE_PRIMARY_X,
        .metaData = data
    };

    GraphicHDRMetaDataSet metaSet6 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_BLUE_PRIMARY_Y,
        .metaData = data
    };

    GraphicHDRMetaDataSet metaSet7 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_WHITE_PRIMARY_X,
        .metaData = data
    };

    GraphicHDRMetaDataSet metaSet8 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_WHITE_PRIMARY_Y,
        .metaData = data
    };

    GraphicHDRMetaDataSet metaSet9 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .metaData = data
    };

    GraphicHDRMetaDataSet metaSet10 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MIN_LUMINANCE,
        .metaData = data
    };

    GraphicHDRMetaDataSet metaSet11 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_CONTENT_LIGHT_LEVEL,
        .metaData = data
    };

    GraphicHDRMetaDataSet metaSet12 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_FRAME_AVERAGE_LIGHT_LEVEL,
        .metaData = data
    };

    GraphicHDRMetaDataSet metaSet13 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_HDR10_PLUS,
        .metaData = data
    };

    GraphicHDRMetaDataSet metaSet14 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_HDR_VIVID,
        .metaData = data
    };

    EXPECT_FALSE(metaSet1 == metaSet2);
    EXPECT_FALSE(metaSet2 == metaSet3);
    EXPECT_FALSE(metaSet3 == metaSet4);
    EXPECT_FALSE(metaSet4 == metaSet5);
    EXPECT_FALSE(metaSet5 == metaSet6);
    EXPECT_FALSE(metaSet6 == metaSet7);
    EXPECT_FALSE(metaSet7 == metaSet8);
    EXPECT_FALSE(metaSet8 == metaSet9);
    EXPECT_FALSE(metaSet9 == metaSet10);
    EXPECT_FALSE(metaSet10 == metaSet11);
    EXPECT_FALSE(metaSet11 == metaSet12);
    EXPECT_FALSE(metaSet12 == metaSet13);
    EXPECT_FALSE(metaSet13 == metaSet14);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaData operator==
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataEqualTest, Function | SmallTest | Level1)
{
    GraphicHDRMetaData metaData1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = 1000.0f
    };

    GraphicHDRMetaData metaData2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = 1000.0f
    };

    EXPECT_TRUE(metaData1 == metaData2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaData operator!=
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataNotEqualTest, Function | SmallTest | Level1)
{
    GraphicHDRMetaData metaData1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = 1000.0f
    };

    GraphicHDRMetaData metaData2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MIN_LUMINANCE,
        .value = 0.1f
    };

    EXPECT_FALSE(metaData1 == metaData2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaData key difference
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataKeyTest, Function | SmallTest | Level1)
{
    GraphicHDRMetaData metaData1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = 1000.0f
    };

    GraphicHDRMetaData metaData2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MIN_LUMINANCE,
        .value = 1000.0f
    };

    EXPECT_FALSE(metaData1 == metaData2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaData value difference
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataValueTest, Function | SmallTest | Level1)
{
    GraphicHDRMetaData metaData1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = 1000.0f
    };

    GraphicHDRMetaData metaData2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = 1000.1f
    };

    EXPECT_FALSE(metaData1 == metaData2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaData small difference within epsilon
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataEpsilonTest, Function | SmallTest | Level1)
{
    GraphicHDRMetaData metaData1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = 1000.0f
    };

    GraphicHDRMetaData metaData2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = 1000.0000001f
    };

    EXPECT_TRUE(metaData1 == metaData2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaData just outside epsilon
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataEpsilonFailTest, Function | SmallTest | Level1)
{
    GraphicHDRMetaData metaData1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = 0.0f
    };

    GraphicHDRMetaData metaData2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = 1.0e-5f
    };

    EXPECT_FALSE(metaData1 == metaData2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaData negative values
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataNegativeTest, Function | SmallTest | Level1)
{
    GraphicHDRMetaData metaData1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = -100.0f
    };

    GraphicHDRMetaData metaData2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = -100.0f
    };

    EXPECT_TRUE(metaData1 == metaData2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaData zero value
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataZeroTest, Function | SmallTest | Level1)
{
    GraphicHDRMetaData metaData1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = 0.0f
    };

    GraphicHDRMetaData metaData2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = 0.0f
    };

    EXPECT_TRUE(metaData1 == metaData2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaData very large values
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataLargeTest, Function | SmallTest | Level1)
{
    GraphicHDRMetaData metaData1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = 1000000.0f
    };

    GraphicHDRMetaData metaData2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MAX_LUMINANCE,
        .value = 1000000.0f
    };

    EXPECT_TRUE(metaData1 == metaData2);
}

/*
* Function: SurfaceTypeTest
* Type: Function
* Rank: Important(2)
* EnvConditions: N/A
* CaseDescription: test GraphicHDRMetaData very small positive values
*/
HWTEST_F(SurfaceTypeTest, GraphicHDRMetaDataSmallTest, Function | SmallTest | Level1)
{
    GraphicHDRMetaData metaData1 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MIN_LUMINANCE,
        .value = 0.0001f
    };

    GraphicHDRMetaData metaData2 = {
        .key = GraphicHDRMetadataKey::GRAPHIC_MATAKEY_MIN_LUMINANCE,
        .value = 0.0001f
    };

    EXPECT_TRUE(metaData1 == metaData2);
}