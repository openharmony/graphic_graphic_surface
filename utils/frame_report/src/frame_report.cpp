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

static constexpr OHOS::HiviewDFX::HiLogLabel LOG_LABEL = { LOG_CORE, 0xD005830, "FrameReport" };
#define LOGF(...) (void)OHOS::HiviewDFX::HiLog::Fatal(LOG_LABEL, __VA_ARGS__)
#define LOGE(...) (void)OHOS::HiviewDFX::HiLog::Error(LOG_LABEL, __VA_ARGS__)
#define LOGW(...) (void)OHOS::HiviewDFX::HiLog::Warn(LOG_LABEL, __VA_ARGS__)
#define LOGI(...) (void)OHOS::HiviewDFX::HiLog::Info(LOG_LABEL, __VA_ARGS__)
#define LOGD(...) (void)OHOS::HiviewDFX::HiLog::Debug(LOG_LABEL, __VA_ARGS__)

#if (defined(__aarch64__) || defined(__x86_64__))
    const std::string GAME_ACCELERATE_SCHEDULE_SO_PATH = "/system/lib64/libgame_acc_sched_client.z.so";
#else
    const std::string GAME_ACCELERATE_SCHEDULE_SO_PATH = "/system/lib/libgame_acc_sched_client.z.so";
#endif
const std::string GAME_ACCELERATE_SCHEDULE_NOTIFYFRAMEINFO = "GAS_NotifyFrameInfo";

const std::string GAME_SF_KEY = "Surface";
constexpr int32_t REPORT_BUFFER_SIZE = 256;
constexpr int32_t THOUSAND_COUNT = 1000;
constexpr int32_t SKIP_HINT_STATUS = 0;
constexpr int32_t MAX_CACHE_COUNT = 5;

constexpr int32_t FR_GAME_BACKGROUND = 0;
constexpr int32_t FR_GAME_FOREGROUND = 1;
constexpr int32_t FR_GAME_SCHED = 2;

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
            if (!IsGameScene(pid)) {
                LOGW("FrameReport::SetGameScene Local Cache Did Not Contains The Value pid = %{public}d "
                     "state = 0", pid);
                return;
            }
            DeletePidInfo(pid);
        }
        break;
        case FR_GAME_FOREGROUND: {
            LoadLibrary();
            if (pid > FR_DEFAULT_PID) {
                LimitingCacheSize();
                AddPidInfo(pid);
            }
        }
        break;
        case FR_GAME_SCHED: {
            if (IsGameScene(pid)) {
                activelyPid_ = pid;
            }
        }
        break;
        default: {
            LOGW("FrameReport::SetGameScene state error!");
        }
        break;
    }
}

bool FrameReport::HasGameScene()
{
    return !gameSceneMap_.empty();
}

bool FrameReport::IsGameScene(int32_t pid)
{
    if (gameSceneMap_.find(pid) != gameSceneMap_.end()) {
        return true;
    }
    return false;
}

bool FrameReport::IsActiveGameWithPid(int32_t pid)
{
    if (pid <= FR_DEFAULT_PID) {
        return false;
    }
    return pid == activelyPid_;
}

bool FrameReport::IsActiveGameWithUniqueId(uint64_t uniqueId)
{
    if (uniqueId <= FR_DEFAULT_UNIQUEID) {
        return false;
    }
    return uniqueId == activelyUniqueId_;
}

void FrameReport::SetLastSwapBufferTime(int64_t lastSwapBufferTime)
{
    lastSwapBufferTime_ = lastSwapBufferTime;
}

void FrameReport::SetDequeueBufferTime(const std::string& layerName, int64_t dequeueBufferTime)
{
    if (!IsReportBySurfaceName(layerName)) {
        return;
    }
    dequeueBufferTime_ = dequeueBufferTime;
}

void FrameReport::SetQueueBufferTime(uint64_t uniqueId, const std::string& layerName, int64_t queueBufferTime)
{
    if (!IsReportBySurfaceName(layerName)) {
        return;
    }
    queueBufferTime_ = queueBufferTime;
    activelyUniqueId_ = uniqueId;
}

void FrameReport::SetPendingBufferNum(const std::string& layerName, int32_t pendingBufferNum)
{
    if (!IsReportBySurfaceName(layerName)) {
        return;
    }
    pendingBufferNum_ = pendingBufferNum;
}

bool FrameReport::IsReportBySurfaceName(const std::string& layerName)
{
    return layerName.find(GAME_SF_KEY) != std::string::npos;
}

void FrameReport::LoadLibrary()
{
    if (!isGameSoLoaded_) {
        dlerror();
        gameSoHandle_ = dlopen(GAME_ACCELERATE_SCHEDULE_SO_PATH.c_str(), RTLD_LAZY);
        if (gameSoHandle_ == nullptr) {
            LOGE("FrameReport::LoadLibrary dlopen libgame_acc_sched_client.z.so failed! error = %{public}s", dlerror());
        } else {
            LOGI("FrameReport::LoadLibrary dlopen libgame_acc_sched_client.z.so success!");
            isGameSoLoaded_ = true;
        }
    }
}

void FrameReport::CloseLibrary()
{
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
    if (!isGameSoLoaded_) {
        return nullptr;
    }
    dlerror();
    void *funcSym = dlsym(gameSoHandle_, symName.c_str());
    if (funcSym == nullptr) {
        LOGE("FrameReport::LoadSymbol Get %{public}s symbol failed: %{public}s", symName.c_str(), dlerror());
        return nullptr;
    }
    return funcSym;
}

void FrameReport::LimitingCacheSize()
{
    if (gameSceneMap_.size() >= MAX_CACHE_COUNT) {
        int32_t tempPid = gameSceneMap_.begin()->first;
        DeletePidInfo(tempPid);
    }
}

void FrameReport::AddPidInfo(int32_t pid)
{
    gameSceneMap_[pid] = true;
}

void FrameReport::DeletePidInfo(int32_t pid)
{
    gameSceneMap_.erase(pid);
    if (pid == activelyPid_) {
        activelyPid_ = FR_DEFAULT_PID;
        activelyUniqueId_ = FR_DEFAULT_UNIQUEID;
    }
}

void FrameReport::Report(int32_t pid, const std::string& layerName)
{
    if (!IsReportBySurfaceName(layerName)) {
        return;
    }
    int64_t timeStamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    std::string bufferMsg = "";
    if (pid == activelyPid_) {
        char msg[REPORT_BUFFER_SIZE] = { 0 };
        int32_t ret = sprintf_s(msg, sizeof(msg),
                                "{\"dequeueBufferTime\":\"%d\",\"queueBufferTime\":\"%d\",\"pendingBufferNum\":\"%d\","
                                "\"swapBufferTime\":\"%d\", \"skipHint\":\"%d\"}",
                                static_cast<int32_t>(dequeueBufferTime_ / THOUSAND_COUNT),
                                static_cast<int32_t>(queueBufferTime_ / THOUSAND_COUNT),
                                pendingBufferNum_,
                                static_cast<int32_t>(lastSwapBufferTime_ / THOUSAND_COUNT),
                                SKIP_HINT_STATUS);
        if (ret == -1) {
            return;
        }
        bufferMsg = msg;
    }
    NotifyFrameInfo(pid, layerName, timeStamp, bufferMsg);
}

void FrameReport::NotifyFrameInfo(int32_t pid, const std::string& layerName, int64_t timeStamp,
                                  const std::string& bufferMsg)
{
    if (notifyFrameInfoFunc_ == nullptr) {
        notifyFrameInfoFunc_ = (NotifyFrameInfoFunc)LoadSymbol(GAME_ACCELERATE_SCHEDULE_NOTIFYFRAMEINFO);
    }
    if (notifyFrameInfoFunc_ != nullptr) {
        notifyFrameInfoFunc_(pid, layerName, timeStamp, bufferMsg);
    }
}
} // namespace Rosen
} // namespace OHOS