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

#ifndef UTILS_HEBC_WHITE_LIST_H
#define UTILS_HEBC_WHITE_LIST_H

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
    [[nodiscard]] bool Check(const std::string& appName) noexcept;
    void GetApplicationName(std::string& name) noexcept;
    bool Init() noexcept;

private:
    HebcWhiteList() = default;
    [[nodiscard]] bool ParseJson(std::string const &json) noexcept;
    void AcquireConfig(const std::string& filePath, std::string& jsonStr) noexcept;
    std::string GetConfigAbsolutePath() noexcept;
    void ReadFile(std::string const &file, size_t maxSize, std::string& buffer) noexcept;
    std::atomic_bool inited_ = false;
    std::vector<std::string> hebcList_;
    std::string appName_;
};

} // namespace OHOS


#endif // UTILS_HEBC_WHITE_LIST_H