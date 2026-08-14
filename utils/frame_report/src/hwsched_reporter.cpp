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

#include "hwsched_reporter.h"

#include <cstdio>
#include <hilog/log.h>

namespace OHOS {
namespace Rosen {

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD005830

#undef LOG_TAG
#define LOG_TAG "HwschedReporter"

#define LOGE(format, ...) HILOG_ERROR(LOG_CORE, format, ##__VA_ARGS__)
#define LOGW(format, ...) HILOG_WARN(LOG_CORE, format, ##__VA_ARGS__)
#define LOGI(format, ...) HILOG_INFO(LOG_CORE, format, ##__VA_ARGS__)

constexpr int32_t FR_DEFAULT_PID = 0;
const std::string HWSCHED_CLIENT_SO_PATH = "libhwsched_client.z.so";
const std::string HWSCHED_CLIENT_REPORT_FUNC = "ReportFrameInfo";

HwschedReporter::HwschedReporter() : SceneReporter(HWSCHED_CLIENT_SO_PATH, HWSCHED_CLIENT_REPORT_FUNC)
{
    sceneType_.store(FR_SCENE_HWSCHED);
}

void HwschedReporter::Activate(int32_t pid)
{
    if (pid <= FR_DEFAULT_PID) {
        LOGW("HwschedReporter::Activate invalid pid=%{public}d", pid);
        return;
    }
    std::unique_lock lock(pidMutex_);
    pidSet_.insert(pid);
}

void HwschedReporter::Deactivate()
{
    std::unique_lock lock(pidMutex_);
    pidSet_.clear();
    activePid_.store(-1);
    CloseLibrary();
}

bool HwschedReporter::IsActiveWithPid(int32_t pid) const
{
    if (pid <= FR_DEFAULT_PID) {
        return false;
    }

    if (activePid_.load() == pid) {
        return true;
    }

    std::shared_lock lock(pidMutex_);
    if (pidSet_.find(pid) != pidSet_.end()) {
        activePid_.store(pid);
        return true;
    }
    return false;
}

bool HwschedReporter::IsActive() const
{
    std::shared_lock lock(pidMutex_);
    return !pidSet_.empty();
}

void HwschedReporter::Report(const std::string& layerName, uint64_t uniqueId, const std::string& bufferMsg)
{
    if (bufferMsg.empty()) {
        return;
    }
    int32_t pid = activePid_.load();
    if (pid <= FR_DEFAULT_PID) {
        LOGW("HwschedReporter::Report invalid pid=%{public}d", pid);
        return;
    }
    ReportFrameInfo(activePid_.load(), layerName, bufferMsg, uniqueId);
}

void HwschedReporter::ReportFrameInfo(int32_t pid, const std::string& layerName,
    const std::string& bufferMsg, uint64_t uniqueId)
{
    std::shared_lock lock(mutex_);
    if (libraryInfo_.notifyFunc == nullptr) {
        return;
    }

    ReportFrameInfoFunc reportFrameFunc = reinterpret_cast<ReportFrameInfoFunc>(libraryInfo_.notifyFunc);
    int result = reportFrameFunc(pid, layerName, bufferMsg, uniqueId);
    lock.unlock();
    if (result != 0) {
        LOGW("HwschedReporter::ReportFrameInfo failed, result=%{public}d", result);
        Deactivate();
    }
}

} // namespace Rosen
} // namespace OHOS