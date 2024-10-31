/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "surfaceutils_fuzzer.h"

#include <securec.h>

#include "data_generate.h"
#include "iconsumer_surface.h"
#include "surface.h"
#include "surface_utils.h"

using namespace g_fuzzCommon;
namespace OHOS {
    namespace {
        constexpr uint32_t MATRIX_SIZE = 16;
    }
    bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
    {
        if (data == nullptr) {
            return false;
        }

        // initialize
        g_data = data;
        g_size = size;
        g_pos = 0;

        // get data
        uint64_t uniqueId1 = GetData<uint64_t>();
        uint64_t uniqueId2 = GetData<uint64_t>();
        uint64_t uniqueId3 = GetData<uint64_t>();

        // test
        sptr<OHOS::IConsumerSurface> cSurface = OHOS::IConsumerSurface::Create();
        sptr<OHOS::IBufferProducer> producer = cSurface->GetProducer();
        sptr<OHOS::Surface> pSurface = Surface::CreateSurfaceAsProducer(producer);
        SurfaceUtils* utils = SurfaceUtils::GetInstance();
        sptr<OHOS::Surface> surface = utils->GetSurface(uniqueId1);
        utils->Add(uniqueId1, surface);
        utils->Add(uniqueId2, pSurface);
        utils->Remove(uniqueId3);
        GraphicTransformType transformType = GetData<GraphicTransformType>();
        float matrix[MATRIX_SIZE];
        for (int i = 0; i < MATRIX_SIZE; i++) {
            matrix[i] = GetData<float>();
        }
        uint32_t matrixSize = MATRIX_SIZE;
        Rect crop = GetData<Rect>();
        sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
        utils->ComputeTransformMatrix(matrix, matrixSize, buffer, transformType, crop);
        utils->GetNativeWindow(uniqueId1);
        return true;
    }
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}

