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

#include "frame_report.h"

#include <dlfcn.h>
#include <cstdio>
#include <securec.h>
#include <unistd.h>
#include <hilog/log.h>

namespace OHOS {
namespace Rosen {

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD005830  // 设置domainID

#undef LOG_TAG
#define LOG_TAG "FrameReport"  // 设置tag

#define LOGF(format, ...) HILOG_FATAL(LOG_CORE, format, ##__VA_ARGS__)
#define LOGE(format, ...) HILOG_ERROR(LOG_CORE, format, ##__VA_ARGS__)
#define LOGW(format, ...) HILOG_WARN(LOG_CORE, format, ##__VA_ARGS__)
#define LOGI(format, ...) HILOG_INFO(LOG_CORE, format, ##__VA_ARGS__)
#define LOGD(format, ...) HILOG_DEBUG(LOG_CORE, format, ##__VA_ARGS__)

const std::string GAME_ACCELERATE_SCHEDULE_SO_PATH = "libgame_acc_sched_client.z.so";
const std::string GAME_ACCELERATE_SCHEDULE_NOTIFYFRAMEINFO = "GAS_NotifyFrameInfo";

const std::string GAME_SF_KEY = "Surface";
const std::string TYPE_COMMIT = "commit_time";
const std::string BUFFER_MSG = "";
constexpr int32_t REPORT_BUFFER_SIZE = 256;
constexpr int32_t THOUSAND_COUNT = 1000;
constexpr int32_t SKIP_HINT_STATUS = 0;

constexpr int32_t FR_GAME_BACKGROUND = 0;
constexpr int32_t FR_GAME_FOREGROUND = 1;
constexpr int32_t FR_GAME_SCHED = 2;
constexpr int32_t COMMIT_PID = -1;

FrameReport& FrameReport::GetInstance()
{
    static FrameReport instance;
    return instance;
}

FrameReport::FrameReport()
{
}

FrameReport::~FrameReport()
{
    CloseLibrary();
}

void FrameReport::SetGameScene(int32_t pid, int32_t state)
{
    LOGI("FrameReport::SetGameScene pid = %{public}d state = %{public}d ", pid, state);
    switch (state) {
        case FR_GAME_BACKGROUND: {
            if (IsActiveGameWithPid(pid)) {
                LOGI("FrameReport::SetGameScene Game Background Current Pid = %{public}d "
                     "state = 0", pid);
                DeletePidInfo();
            }
            break;
        }
        case FR_GAME_FOREGROUND: {
            LoadLibrary();
            break;
        }
        case FR_GAME_SCHED: {
            activelyPid_.store(pid);
            break;
        }
        default: {
            LOGW("FrameReport::SetGameScene state error!");
            break;
        }
    }
}

bool FrameReport::HasGameScene()
{
    return activelyPid_.load() != FR_DEFAULT_PID;
}

bool FrameReport::IsActiveGameWithPid(int32_t pid)
{
    if (pid <= FR_DEFAULT_PID) {
        return false;
    }
    return pid == activelyPid_.load();
}

bool FrameReport::IsActiveGameWithUniqueId(uint64_t uniqueId)
{
    if (uniqueId <= FR_DEFAULT_UNIQUEID) {
        return false;
    }
    return uniqueId == activelyUniqueId_.load();
}

void FrameReport::SetLastSwapBufferTime(int64_t lastSwapBufferTime)
{
    lastSwapBufferTime_.store(lastSwapBufferTime);
}

void FrameReport::SetDequeueBufferTime(const std::string& layerName, int64_t dequeueBufferTime)
{
    if (!IsReportBySurfaceName(layerName)) {
        return;
    }
    dequeueBufferTime_.store(dequeueBufferTime);
}

void FrameReport::SetQueueBufferTime(uint64_t uniqueId, const std::string& layerName, int64_t queueBufferTime)
{
    if (!IsReportBySurfaceName(layerName)) {
        return;
    }
    queueBufferTime_.store(queueBufferTime);
    activelyUniqueId_.store(uniqueId);
}

void FrameReport::SetPendingBufferNum(const std::string& layerName, int32_t pendingBufferNum)
{
    if (!IsReportBySurfaceName(layerName)) {
        return;
    }
    pendingBufferNum_.store(pendingBufferNum);
}

bool FrameReport::IsReportBySurfaceName(const std::string& layerName)
{
    return layerName.find(GAME_SF_KEY) != std::string::npos;
}

void FrameReport::LoadLibrary()
{
    std::unique_lock lock(mutex_);
    if (!isGameSoLoaded_) {
        dlerror();
        gameSoHandle_ = dlopen(GAME_ACCELERATE_SCHEDULE_SO_PATH.c_str(), RTLD_LAZY);
        if (gameSoHandle_ == nullptr) {
            LOGE("FrameReport::LoadLibrary dlopen libgame_acc_sched_client.z.so failed! error = %{public}s", dlerror());
            return;
        }
        LOGI("FrameReport::LoadLibrary dlopen libgame_acc_sched_client.z.so success!");
        notifyFrameInfoFunc_ = (NotifyFrameInfoFunc)LoadSymbol(GAME_ACCELERATE_SCHEDULE_NOTIFYFRAMEINFO);
        if (notifyFrameInfoFunc_ == nullptr) {
            return;
        }
        LOGI("FrameReport::LoadLibrary dlsym GAS_NotifyFrameInfo success!");
        isGameSoLoaded_ = true;
    }
}

void FrameReport::CloseLibrary()
{
    std::unique_lock lock(mutex_);
    notifyFrameInfoFunc_ = nullptr;
    if (gameSoHandle_ != nullptr) {
        if (dlclose(gameSoHandle_) != 0) {
            LOGE("FrameReport::CloseLibrary libgame_acc_sched_client.z.so close failed!");
        } else {
            gameSoHandle_ = nullptr;
            isGameSoLoaded_ = false;
            LOGI("FrameReport::CloseLibrary libgame_acc_sched_client.z.so close success!");
        }
    }
}

void* FrameReport::LoadSymbol(const std::string& symName)
{
    dlerror();
    void *funcSym = dlsym(gameSoHandle_, symName.c_str());
    if (funcSym == nullptr) {
        LOGE("FrameReport::LoadSymbol Get %{public}s symbol failed: %{public}s", symName.c_str(), dlerror());
        return nullptr;
    }
    return funcSym;
}

void FrameReport::DeletePidInfo()
{
    activelyPid_.store(FR_DEFAULT_PID);
    activelyUniqueId_.store(FR_DEFAULT_UNIQUEID);
}

void FrameReport::Report(const std::string& layerName)
{
    if (!IsReportBySurfaceName(layerName)) {
        return;
    }
    int64_t timeStamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    std::string bufferMsg = "";
    char msg[REPORT_BUFFER_SIZE] = { 0 };
    int32_t ret = sprintf_s(msg, sizeof(msg),
                            "{\"dequeueBufferTime\":\"%d\",\"queueBufferTime\":\"%d\",\"pendingBufferNum\":\"%d\","
                            "\"swapBufferTime\":\"%d\", \"skipHint\":\"%d\"}",
                            static_cast<int32_t>(dequeueBufferTime_.load() / THOUSAND_COUNT),
                            static_cast<int32_t>(queueBufferTime_.load() / THOUSAND_COUNT),
                            pendingBufferNum_.load(),
                            static_cast<int32_t>(lastSwapBufferTime_.load() / THOUSAND_COUNT),
                            SKIP_HINT_STATUS);
    if (ret == -1) {
        return;
    }
    bufferMsg = msg;
    NotifyFrameInfo(activelyPid_.load(), layerName, timeStamp, bufferMsg);
}

void FrameReport::ReportCommitTime(int64_t commitTime)
{
    NotifyFrameInfo(COMMIT_PID, TYPE_COMMIT, commitTime, BUFFER_MSG);
}

void FrameReport::NotifyFrameInfo(int32_t pid, const std::string& layerName, int64_t timeStamp,
                                  const std::string& bufferMsg)
{
    std::shared_lock lock(mutex_);
    if (notifyFrameInfoFunc_ == nullptr) {
        return;
    }
    bool result = notifyFrameInfoFunc_(pid, layerName, timeStamp, bufferMsg);
    if (!result) {
        LOGW("FrameReport::NotifyFrameInfo Call GAS_NotifyFrameInfo Func Error");
        DeletePidInfo();
    }
}
} // namespace Rosen
} // namespace OHOS