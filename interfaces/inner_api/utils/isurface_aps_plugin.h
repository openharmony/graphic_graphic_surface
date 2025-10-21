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

#ifndef INCLUDE_ISURFACE_APS_PLUGIN_H
#define INCLUDE_ISURFACE_APS_PLUGIN_H

#include <dlfcn.h>
#include <refbase.h>
#include <mutex>
#include "surface.h"

namespace OHOS {

class ISurfaceApsPlugin : public RefBase {
private:
    using LoadApsFunc = void (*)(sptr<OHOS::ISurfaceApsPlugin> &instance);
    static inline std::mutex instanceMutex_;
    static inline void *handle_ = nullptr;
    static inline sptr<ISurfaceApsPlugin> instance_ = nullptr;
    static inline std::once_flag initInstanceFlag;

public:
    static sptr<ISurfaceApsPlugin> &LoadPlugin()
    {
        std::call_once(initInstanceFlag, []() {
            handle_ = dlopen("libaps_client.z.so", RTLD_NOW);
            if (handle_ == nullptr) {
                return;
            }
            LoadApsFunc loadApsFunc = reinterpret_cast<LoadApsFunc>(dlsym(handle_, "LoadApsFromSurface"));
            if (loadApsFunc != nullptr) {
                loadApsFunc(instance_);
            }
            if (instance_ == nullptr) {
                dlclose(handle_);
                handle_ = nullptr;
            }
        });
        return instance_;
    }

    ISurfaceApsPlugin() = default;

    ~ISurfaceApsPlugin() = default;

    virtual uint32_t InitSurface(const std::string &surfaceName)
    {
        (void)surfaceName;
        return 0;
    }

    virtual void OnFlushBuffer(const std::string &surfaceName, uint32_t count)
    {
        (void)surfaceName;
        (void)count;
    }

    virtual float GetApsSdrRatio(const std::string &pkgName)
    {
        (void)pkgName;
        return 1.0f;
    }
};
}  // namespace OHOS

#endif