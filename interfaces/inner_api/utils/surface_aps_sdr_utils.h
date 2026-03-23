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
        auto [defaultShortSide, defaultLongSide] =
            std::make_pair(std::min(defaultWidth, defaultHeight), std::max(defaultWidth, defaultHeight));

        BLOGI("CalcWidthAndHeightBySdrRatio sdrRatio:%{public}f, width:%{public}d, height:%{public}d,"
            " defaultWidth:%{public}d, defaultHeight:%{public}d",
            sdrRatio, shortSide, longSide, defaultShortSide, defaultLongSide);

        bool isDefaultValid = (defaultShortSide > 0 && defaultLongSide > 0);
        bool isInputValid = (shortSide > 0 && longSide > 0);

        // Case 1: Default size is invalid (zero), use input size directly
        if (!isDefaultValid && isInputValid) {
            UpdateOriginInternalSize(shortSide, longSide);
            ApplySdrScale(width, height, sdrRatio);
            return;
        }

        // Case 2: Input size exceeds origin (folded state), update origin and bigOrigin (unfolded state), apply scaling
        // Note: bigOrigin tracks the expanded state for foldable device scenarios
        if (shortSide >= GetOriginInternalShortSide() || longSide >= GetOriginInternalLongSide()) {
            UpdateOriginInternalSize(shortSide, longSide);
            if (shortSide > GetOriginExternalShortSide() || longSide > GetOriginExternalLongSide()) {
                UpdateOriginExternalSize(shortSide, longSide);
            }
            ApplySdrScale(width, height, sdrRatio);
            return;
        }

        // Case 3: Calculate scaling based on ratio comparison
        if (isDefaultValid && (GetOriginInternalShortSide() != shortSide || GetOriginInternalLongSide() != longSide)) {
            float shortSideRatio = static_cast<float>(shortSide) / defaultShortSide;
            float longSideRatio = static_cast<float>(longSide) / defaultLongSide;

            bool needUseApsSdrRatio = (shortSideRatio - sdrRatio) > FLOAT_ZERO_TOLERANCE ||
                                      (longSideRatio - sdrRatio) > FLOAT_ZERO_TOLERANCE;

            if (needUseApsSdrRatio) {
                float minScaleFactor = std::min(sdrRatio / shortSideRatio, sdrRatio / longSideRatio);
                width = static_cast<int32_t>(width * minScaleFactor);
                height = static_cast<int32_t>(height * minScaleFactor);
                BLOGI("CalcWidthAndHeightBySdrRatio, rendering resolution adjustment result:"
                    " width:%{public}d, height:%{public}d, minScaleFactor:%{public}f",
                    width, height, minScaleFactor);
                return;
            }
            BLOGD("CalcWidthAndHeightBySdrRatio current scene not need to use sdrRatio");
            return;
        }

        ApplySdrScale(width, height, sdrRatio);
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

private:
    static void ApplySdrScale(int32_t& width, int32_t& height, float sdrRatio)
    {
        width = static_cast<int32_t>(width * sdrRatio);
        height = static_cast<int32_t>(height * sdrRatio);
    }

    static int32_t& GetOriginInternalShortSide()
    {
        static int32_t internalShortSide = 0;
        return internalShortSide;
    }

    static int32_t& GetOriginInternalLongSide()
    {
        static int32_t internalLongSide = 0;
        return internalLongSide;
    }

    static int32_t& GetOriginExternalShortSide()
    {
        static int32_t externalShortSide = 0;
        return externalShortSide;
    }

    static int32_t& GetOriginExternalLongSide()
    {
        static int32_t externalLongSide = 0;
        return externalLongSide;
    }

    static void UpdateOriginInternalSize(int32_t shortSide, int32_t longSide)
    {
        GetOriginInternalShortSide() = shortSide;
        GetOriginInternalLongSide() = longSide;
    }

    static void UpdateOriginExternalSize(int32_t shortSide, int32_t longSide)
    {
        GetOriginExternalShortSide() = shortSide;
        GetOriginExternalLongSide() = longSide;
    }
};
}  // namespace OHOS

#endif
