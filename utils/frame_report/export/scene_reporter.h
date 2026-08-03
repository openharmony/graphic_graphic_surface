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

#ifndef UTILS_INCLUDE_SCENE_REPORTER_H
#define UTILS_INCLUDE_SCENE_REPORTER_H

#include <cstdint>
#include <string>
#include <shared_mutex>
#include <atomic>

namespace OHOS {
namespace Rosen {

constexpr int32_t FR_SCENE_NONE = 0;
constexpr int32_t FR_SCENE_GAME = 1;
constexpr int32_t FR_SCENE_HWSCHED = 2;
constexpr int32_t FR_SCENE_BACKGROUND = 3;
constexpr int32_t FR_SCENE_FOREGROUND = 4;

struct LibraryInfo {
    void* soHandle = nullptr;
    void* notifyFunc = nullptr;
    bool isLoaded = false;
    std::string soName;
    std::string symName;
};

class SceneReporter {
public:
    SceneReporter(const std::string& soName, const std::string& symName);
    virtual ~SceneReporter();

    void LoadLibrary();
    void CloseLibrary();

    virtual void Activate(int32_t pid) = 0;
    virtual void Deactivate() = 0;
    virtual bool IsActive() const = 0;
    virtual bool IsActiveWithPid(int32_t pid) const = 0;
    virtual void Report(const std::string &layerName, uint64_t uniqueId, const std::string &bufferMsg) = 0;

    int32_t GetSceneType() const { return sceneType_.load(); }

protected:
    std::atomic<int32_t> sceneType_ = FR_SCENE_NONE;
    LibraryInfo libraryInfo_;
    mutable std::shared_mutex mutex_;

private:
    void* LoadSymbol(const std::string& symName, void* handle);
};

} // namespace Rosen
} // namespace OHOS

#endif // UTILS_INCLUDE_SCENE_REPORTER_H