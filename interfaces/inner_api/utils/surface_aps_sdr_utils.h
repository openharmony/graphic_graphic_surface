/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef SURFACE_APS_SDR_UTILS
#define SURFACE_APS_SDR_UTILS

#include <algorithm>
#include "buffer_log.h"
#include "isurface_aps_plugin.h"

namespace OHOS {
constexpr float DEFAULT_SDR_RATIO = 1.0f;
constexpr float FLOAT_ZERO_TOLERANCE = 1e-6f;
static inline float SDR_RATIO = DEFAULT_SDR_RATIO;
class SurfaceApsSdrUtils {
public:
    static void CalcWidthAndHeightBySdrRatio(int32_t& width, int32_t& height, int32_t defaultWidth,
        int32_t defaultHeight, float sdrRatio)
    {
        // Use new parameters to ignore the size relationship between width and height.
        auto [shortSide, longSide] = std::make_pair(std::min(width, height), std::max(width, height));
        auto [shortDefaultSide, longDefaultSide] =
            std::make_pair(std::min(defaultWidth, defaultHeight), std::max(defaultWidth, defaultHeight));
        BLOGI("CalcWidthAndHeightBySdrRatio sdrRatio:%{public}f, width:%{public}d, height:%{public}d,"
            " defaultWidth:%{public}d, defaultHeight:%{public}d",
            sdrRatio, shortSide, longSide, shortDefaultSide, longDefaultSide);
        if (shortDefaultSide > 0 && longDefaultSide > 0) {
            float shortSideRatio = static_cast<float>(shortSide) / shortDefaultSide;
            float longSideRatio = static_cast<float>(longSide) / longDefaultSide;
            // If the ratio of any side to the defaultSide is greater than sdrRatio, sdrRatio needs to be used.
            bool needUseApsSdrRatio =
                (shortSideRatio - sdrRatio) > FLOAT_ZERO_TOLERANCE || (longSideRatio - sdrRatio) > FLOAT_ZERO_TOLERANCE;
            if (needUseApsSdrRatio) {
                float shortSideScaleFactor = sdrRatio / shortSideRatio;
                float longSideScaleFactor = sdrRatio / longSideRatio;
                // Select the min scale factor to ensure that the ratios on both sides are less or equal than sdrRatio.
                float minScaleFactor = std::min(shortSideScaleFactor, longSideScaleFactor);
                width = static_cast<int32_t>(width * minScaleFactor);
                height = static_cast<int32_t>(height * minScaleFactor);
                BLOGI("CalcWidthAndHeightBySdrRatio, rendering resolution adjustment result:"
                    " width:%{public}d, height:%{public}d, minScaleFactor:%{public}f",
                    width, height, minScaleFactor);
            }
            BLOGD("CalcWidthAndHeightBySdrRatio current scene not need to use sdrRatio");
        } else {
            width = static_cast<int32_t>(width * sdrRatio);
            height = static_cast<int32_t>(height * sdrRatio);
            BLOGI("CalcWidthAndHeightBySdrRatio defaultSize is invalid, use inputSize to calc:%{public}d * %{public}d",
                width, height);
        }
    }

    static void GetSdrRatio(const std::string &pkgName, float &sdrRatio)
    {
        if (std::fabs(sdrRatio - DEFAULT_SDR_RATIO) < FLOAT_ZERO_TOLERANCE) {
            auto &apsPlugin = OHOS::ISurfaceApsPlugin::LoadPlugin();
            if (apsPlugin != nullptr) {
                BLOGD("ProducerSurface::GetSdrRatio apsPlugin is not nullptr");
                sdrRatio = apsPlugin->GetApsSdrRatio(pkgName);
            }
        }
    }
};
}  // namespace OHOS

#endif
