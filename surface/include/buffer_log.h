/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SURFACE_INCLUDE_BUFFER_LOG_H
#define FRAMEWORKS_SURFACE_INCLUDE_BUFFER_LOG_H

#include <hilog/log.h>

namespace OHOS {
namespace {
// The "0xD001401" is the domain ID for graphic module that alloted by the OS.
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD001401
#undef LOG_TAG
#define LOG_TAG "Bufferqueue"
}

#if (defined(__aarch64__) || defined(__x86_64__))
#define BPUBI64  "%{public}ld"
#define BPUB_SIZE "%{public}lu"
#define BPUBU64  "%{public}lu"
#else
#define BPUBI64  "%{public}lld"
#define BPUB_SIZE "%{public}u"
#define BPUBU64  "%{public}llu"
#endif

#define B_CPRINTF(func, fmt, ...) \
    func(LOG_CORE, "<%{public}s:%{public}d-%{public}s>: " fmt, \
        __FILE_NAME__, __LINE__, __func__, ##__VA_ARGS__)

#define BLOGD(fmt, ...) B_CPRINTF(HILOG_DEBUG, fmt, ##__VA_ARGS__)
#define BLOGI(fmt, ...) B_CPRINTF(HILOG_INFO, fmt, ##__VA_ARGS__)
#define BLOGW(fmt, ...) B_CPRINTF(HILOG_WARN, fmt, ##__VA_ARGS__)
#define BLOGE(fmt, ...) B_CPRINTF(HILOG_ERROR, fmt, ##__VA_ARGS__)

#define BLOGN_FAILURE_RET(ret)                                                         \
    do {                                                                               \
        BLOGE("Fail ret: %{public}d, uniqueId: %{public}" PRIu64 ".", ret, uniqueId_); \
        return ret;                                                                    \
    } while (0)

#define BLOGE_CHECK_AND_RETURN_RET(cond, ret, fmt, ...)  \
    do {                                                 \
        if (!(cond)) {                                   \
            BLOGE(fmt, ##__VA_ARGS__);                   \
            return ret;                                  \
        }                                                \
    } while (0)
} // namespace OHOS

#endif // FRAMEWORKS_SURFACE_INCLUDE_BUFFER_LOG_H
