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

#ifndef OHOS_CONFIG_H
#define OHOS_CONFIG_H

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace OHOS {

class HebcWhiteList final {
public:
    static HebcWhiteList& GetInstance()
    {
        static HebcWhiteList inst;
        return inst;
    }

    HebcWhiteList(const HebcWhiteList&) = delete;
    HebcWhiteList& operator=(const HebcWhiteList&) = delete;
    [[nodiscard]] bool Check() noexcept;

private:
    HebcWhiteList() = default;
    void Init() noexcept;
    void GetApplicationName(std::string& name) noexcept;
    [[nodiscard]] bool ParseJson(std::string const &json) noexcept;
    [[nodiscard]] std::string AcquireConfig(const std::string& filePath) noexcept;
    std::string GetConfigAbsolutePath() noexcept;
    std::unique_ptr<char[]> ReadFile(std::string const &file, size_t &size, size_t maxSize) noexcept;
    std::atomic_bool checkResult_ = false;
    std::atomic_bool inited_ = false;
};

} // namespace OHOS


#endif // OHOS_CONFIG_H