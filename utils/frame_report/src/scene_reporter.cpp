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

#include "scene_reporter.h"

#include <dlfcn.h>
#include <cstdio>
#include <hilog/log.h>

namespace OHOS {
namespace Rosen {

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD005830

#undef LOG_TAG
#define LOG_TAG "SceneReporter"

#define LOGE(format, ...) HILOG_ERROR(LOG_CORE, format, ##__VA_ARGS__)
#define LOGI(format, ...) HILOG_INFO(LOG_CORE, format, ##__VA_ARGS__)

SceneReporter::SceneReporter(const std::string& soName, const std::string& symName)
{
    libraryInfo_.soName = soName;
    libraryInfo_.symName = symName;
}

SceneReporter::~SceneReporter()
{
    CloseLibrary();
}

void* SceneReporter::LoadSymbol(const std::string& symName, void* handle)
{
    dlerror();
    void *funcSym = dlsym(handle, symName.c_str());
    if (funcSym == nullptr) {
        LOGE("LoadSymbol Get %{public}s symbol failed: %{public}s", symName.c_str(), dlerror());
        return nullptr;
    }
    return funcSym;
}

void SceneReporter::LoadLibrary()
{
    std::unique_lock lock(mutex_);
    if (libraryInfo_.isLoaded) {
        return;
    }
    dlerror();
    void *soHandle = dlopen(libraryInfo_.soName.c_str(), RTLD_LAZY);
    if (soHandle == nullptr) {
        LOGE("LoadLibrary %{public}s dlopen failed! error = %{public}s",
            libraryInfo_.soName.c_str(), dlerror());
        return;
    }
    LOGI("LoadLibrary %{public}s dlopen success!", libraryInfo_.soName.c_str());
    void* funcSym = LoadSymbol(libraryInfo_.symName, soHandle);
    if (funcSym == nullptr) {
        if (dlclose(soHandle) != 0) {
            LOGE("LoadLibrary %{public}s dlclose failed", libraryInfo_.soName.c_str());
        } else {
            LOGI("LoadLibrary %{public}s dlclose success!", libraryInfo_.soName.c_str());
        }
        return;
    }

    libraryInfo_.isLoaded = true;
    libraryInfo_.soHandle = soHandle;
    libraryInfo_.notifyFunc = funcSym;
    LOGI("LoadLibrary dlsym success!");
}

void SceneReporter::CloseLibrary()
{
    std::unique_lock lock(mutex_);
    libraryInfo_.notifyFunc = nullptr;
    if (libraryInfo_.soHandle != nullptr) {
        if (dlclose(libraryInfo_.soHandle) != 0) {
            LOGE("CloseLibrary %{public}s close failed!", libraryInfo_.soName.c_str());
        } else {
            libraryInfo_.soHandle = nullptr;
            libraryInfo_.isLoaded = false;
            LOGI("CloseLibrary %{public}s close success!", libraryInfo_.soName.c_str());
        }
    }
}

} // namespace Rosen
} // namespace OHOS