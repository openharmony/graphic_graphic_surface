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

#ifndef UTILS_INCLUDE_HWSCHED_REPORTER_H
#define UTILS_INCLUDE_HWSCHED_REPORTER_H

#include "scene_reporter.h"

#include <set>
#include <atomic>
#include <shared_mutex>

namespace OHOS {
namespace Rosen {

using ReportFrameInfoFunc = int(*)(int32_t, const std::string&, const std::string&, uint64_t);

class HwschedReporter : public SceneReporter {
public:
    HwschedReporter();
    ~HwschedReporter() override = default;

    void Activate(int32_t pid) override;
    void Deactivate() override;
    bool IsActive() const override;
    bool IsActiveWithPid(int32_t pid) const override;
    void Report(const std::string& layerName, uint64_t uniqueId, const std::string& bufferMsg) override;

private:
    void ReportFrameInfo(int32_t pid, const std::string& layerName, const std::string& bufferMsg, uint64_t uniqueId);

    std::set<int32_t> pidSet_;
    mutable std::atomic<int32_t> activePid_ = -1;
    mutable std::shared_mutex pidMutex_;
};

} // namespace Rosen
} // namespace OHOS

#endif // UTILS_INCLUDE_HWSCHED_REPORTER_H