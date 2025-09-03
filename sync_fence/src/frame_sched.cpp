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

#include "frame_sched.h"

#include <dlfcn.h>
#include <cstdio>
#include <securec.h>
#include <unistd.h>
#include <hilog/log.h>

namespace OHOS {
namespace Rosen {
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD001400
#undef LOG_TAG
#define LOG_TAG "FrameSched"
#define B_CPRINTF(func, fmt, ...) \
    func(LOG_CORE, "%{public}s: " fmt, __func__, ##__VA_ARGS__)

#define LOGD(fmt, ...) B_CPRINTF(HILOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) B_CPRINTF(HILOG_INFO, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) B_CPRINTF(HILOG_WARN, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) B_CPRINTF(HILOG_ERROR, fmt, ##__VA_ARGS__)
#define LOGF(fmt, ...) B_CPRINTF(HILOG_FATAL, fmt, ##__VA_ARGS__)

const std::string FRAME_AWARE_SO_PATH = "libframe_ui_intf.z.so";

FrameSched& FrameSched::GetInstance()
{
    static FrameSched instance;
    return instance;
}

FrameSched::FrameSched()
{
    LoadLibrary();
}

FrameSched::~FrameSched()
{
    CloseLibrary();
}

bool FrameSched::LoadLibrary()
{
    if (!schedSoLoaded_) {
        schedHandle_ = dlopen(FRAME_AWARE_SO_PATH.c_str(), RTLD_LAZY);
        if (schedHandle_ == nullptr) {
            LOGE("dlopen libframe_ui_intf.so failed! error = %{public}s", dlerror());
            return false;
        }
        schedSoLoaded_ = true;
    }
    LOGI("load library success!");
    sendFenceIdFunc_ = (SendFenceIdFunc)LoadSymbol("SendFenceId");
    monitorGpuStartFunc_ = (MonitorGpuStartFunc)LoadSymbol("MonitorGpuStart");
    monitorGpuEndFunc_ = (MonitorGpuEndFunc)LoadSymbol("MonitorGpuEnd");
    isScbSceneFunc_ = (IsScbSceneFunc)LoadSymbol("IsScbScene");
    return true;
}

void FrameSched::CloseLibrary()
{
    if (schedHandle_ != nullptr) {
        if (dlclose(schedHandle_) != 0) {
            LOGE("libframe_ui_intf.so close failed!\n");
            return;
        }
    }
    schedHandle_ = nullptr;
    schedSoLoaded_ = false;
    sendFenceIdFunc_ = nullptr;
    monitorGpuStartFunc_ = nullptr;
    monitorGpuEndFunc_ = nullptr;
    isScbSceneFunc_ = nullptr;
    LOGI("libframe_ui_intf.so close success!\n");
}

void* FrameSched::LoadSymbol(const char* symName)
{
    if (!schedSoLoaded_) {
        LOGE("libframe_ui_intf.so not loaded.\n");
        return nullptr;
    }

    void *funcSym = dlsym(schedHandle_, symName);
    if (funcSym == nullptr) {
        LOGE("Get %{public}s symbol failed: %{public}s\n", symName, dlerror());
        return nullptr;
    }
    return funcSym;
}

void FrameSched::Init()
{
    if (initFunc_ == nullptr) {
        initFunc_ = (InitFunc)LoadSymbol("Init");
    }
    if (initFunc_ != nullptr) {
        initFunc_();
    } else {
        LOGE("load Init function failed.");
    }
}

void FrameSched::SendFenceId(uint32_t fenceId)
{
    if (schedSoLoaded_ && sendFenceIdFunc_ != nullptr) {
        sendFenceIdFunc_(fenceId);
    } else {
        LOGE("load SendFenceId function failed.");
    }
}

void FrameSched::MonitorGpuStart(uint32_t fenceId)
{
    if (schedSoLoaded_ && monitorGpuStartFunc_ != nullptr) {
        monitorGpuStartFunc_(fenceId);
    } else {
        LOGE("FrameSched:[MonitorStart]load MonitorStart function failed.");
    }
}

void FrameSched::MonitorGpuEnd()
{
    if (schedSoLoaded_ && monitorGpuEndFunc_ != nullptr) {
        monitorGpuEndFunc_();
    } else {
        LOGE("load MonitorEnd function failed.");
    }
}

bool FrameSched::IsScbScene()
{
    if (schedSoLoaded_ && isScbSceneFunc_ != nullptr) {
        return isScbSceneFunc_();
    } else {
        LOGE("load IsScbScene function failed.");
        return false;
    }
}

void FrameSched::SetFrameParam(int requestId, int load, int schedFrameNum, int value)
{
    if (setFrameParamFunc_ == nullptr) {
        setFrameParamFunc_ = (SetFrameParamFunc)LoadSymbol("SetFrameParam");
    }
    if (setFrameParamFunc_ != nullptr) {
        setFrameParamFunc_(requestId, load, schedFrameNum, value);
    } else {
        LOGE("load MonitorEnd function failed.");
    }
}
} // namespace Rosen
} // namespace OHOS