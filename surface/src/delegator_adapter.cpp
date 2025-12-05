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
#include <cerrno>
#include <dlfcn.h>
#include "buffer_log.h"
#include "buffer_queue.h"
#include "delegator_adapter.h"
#include "file_ex.h"

namespace OHOS {

DelegatorAdapter& DelegatorAdapter::GetInstance()
{
    static DelegatorAdapter instance;
    return instance;
}

DelegatorAdapter::DelegatorAdapter()
{
    Init();
}

void DelegatorAdapter::Init()
{
    const std::string DELEGATOR_LIBD = "libdelegator.z.so";
    handle_ = dlopen(DELEGATOR_LIBD.c_str(), RTLD_LAZY);
    if (handle_ == nullptr) {
        BLOGE("cannot open convert lib : libdelegator dlerror : %{public}s", dlerror());
        return;
    }
    RegisterFunction();
}

void DelegatorAdapter::RegisterFunction()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (const auto& funcConfig : CALLBACK_FUNC_CONFIGS) {
        void* func = dlsym(handle_, funcConfig.name.c_str());
        if (func == nullptr) {
            BLOGE("cannot find symbol : %{public}s dlerror : %{public}s", funcConfig.name.c_str(), dlerror());
            return;
        }
        funcMap_[funcConfig.type] = func;
        BLOGI("func name %{public}s", funcConfig.name.c_str());
    }
}

DelegatorAdapter::~DelegatorAdapter()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (handle_ != nullptr) {
        if (dlclose(handle_) != 0) {
            BLOGE("cannot close dynamic library dlerror : %{public}s", dlerror());
        }
        handle_ = nullptr;
        funcMap_.clear();
        BLOGI("~DelegatorAdapter is called");
    }
}

}  // namespace OHOS