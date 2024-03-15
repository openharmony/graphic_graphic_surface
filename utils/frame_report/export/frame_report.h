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

#ifndef UTILS_INCLUDE_FRAME_REPORT_H
#define UTILS_INCLUDE_FRAME_REPORT_H

#include <cstdint>
#include <string>
#include <unordered_map>

namespace OHOS {
namespace Rosen {

using NotifyFrameInfoFunc = void(*)(int32_t, const std::string&, int64_t, const std::string&);
constexpr int32_t FR_DEFAULT_PID = 0;
constexpr uint64_t FR_DEFAULT_UNIQUEID = 0;

class FrameReport {
public:
    static FrameReport& GetInstance();

    void SetGameScene(int32_t pid, int32_t state);
    bool HasGameScene();
    bool IsGameScene(int32_t pid);
    bool IsActiveGameWithPid(int32_t pid);
    bool IsActiveGameWithUniqueId(uint64_t uniqueId);
    void SetLastSwapBufferTime(int64_t lastSwapBufferTime);
    void SetDequeueBufferTime(const std::string& layerName, int64_t dequeueBufferTime);
    void SetQueueBufferTime(uint64_t uniqueId, const std::string& layerName, int64_t queueBufferTime);
    void SetPendingBufferNum(const std::string& layerName, int32_t pendingBufferNum);
    void Report(int32_t pid, const std::string& layerName);

private:
    FrameReport();
    ~FrameReport();

    bool IsReportBySurfaceName(const std::string& layerName);
    void LoadLibrary();
    void CloseLibrary();
    void* LoadSymbol(const std::string& symName);

    void LimitingCacheSize();
    void AddPidInfo(int32_t pid);
    void DeletePidInfo(int32_t pid);
    void NotifyFrameInfo(int32_t pid, const std::string& layerName, int64_t timeStamp, const std::string& bufferMsg);

    int32_t activelyPid_ = FR_DEFAULT_PID;
    int32_t pendingBufferNum_ = 0;
    uint64_t activelyUniqueId_ = FR_DEFAULT_UNIQUEID;
    int64_t lastSwapBufferTime_ = 0;
    int64_t dequeueBufferTime_ = 0;
    int64_t queueBufferTime_ = 0;
    bool isGameSoLoaded_ = false;
    void* gameSoHandle_ = nullptr;
    NotifyFrameInfoFunc notifyFrameInfoFunc_ = nullptr;

    std::unordered_map<int32_t, bool> gameSceneMap_;
};

} // namespace Rosen
} // namespace OHOS

#endif // UTILS_INCLUDE_FRAME_REPORT_H