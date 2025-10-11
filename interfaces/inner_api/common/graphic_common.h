/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_COMMON_GRAPHIC_COMMON_H
#define INTERFACES_INNERKITS_COMMON_GRAPHIC_COMMON_H

#ifdef __cplusplus
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <ostream>
#include <unistd.h>

namespace OHOS {
#endif

#include "graphic_common_c.h"

#ifdef __cplusplus
inline const char *GSErrorBaseStr(GSError base)
{
    switch (base) {
        case GSERROR_OK:                    return "<200 ok>";
        case GSERROR_INVALID_ARGUMENTS:     return "<400 invalid arguments>";
        case GSERROR_NO_PERMISSION:         return "<403 no permission>";
        case GSERROR_CONNOT_CONNECT_SAMGR:  return "<404 connot connect to samgr>";
        case GSERROR_CONNOT_CONNECT_SERVER: return "<404 connot connect to server>";
        case GSERROR_CONNOT_CONNECT_WESTON: return "<404 connot connect to weston>";
        case GSERROR_NO_BUFFER:             return "<406 no buffer>";
        case GSERROR_NO_ENTRY:              return "<406 no entry>";
        case GSERROR_OUT_OF_RANGE:          return "<406 out of range>";
        case GSERROR_NO_SCREEN:             return "<406 no screen>";
        case GSERROR_NO_BUFFER_READY:       return "<406 no buffer ready>";
        case GSERROR_INVALID_OPERATING:     return "<412 invalid operating>";
        case GSERROR_NO_CONSUMER:           return "<412 no consumer>";
        case GSERROR_NOT_INIT:              return "<412 not init>";
        case GSERROR_TYPE_ERROR:            return "<412 type error>";
        case GSERROR_CONSUMER_IS_CONNECTED: return "<412 consumer is connected>";
        case GSERROR_BUFFER_STATE_INVALID:  return "<412 buffer state invalid>";
        case GSERROR_BUFFER_IS_INCACHE:     return "<412 buffer is incache>";
        case GSERROR_BUFFER_QUEUE_FULL:     return "<412 buffer queue full>";
        case GSERROR_BUFFER_NOT_INCACHE:    return "<412 buffer not in cache>";
        case GSERROR_CONSUMER_DISCONNECTED: return "<412 consumer disconnected>";
        case GSERROR_CONSUMER_UNREGISTER_LISTENER: return "<412 consumer unregister listener>";
        case GSERROR_API_FAILED:            return "<500 api call failed>";
        case GSERROR_INTERNAL:              return "<500 internal error>";
        case GSERROR_NO_MEM:                return "<500 no memory>";
        case GSERROR_PROXY_NOT_INCLUDE:     return "<500 proxy not include>";
        case GSERROR_SERVER_ERROR:          return "<500 server occur error>";
        case GSERROR_ANIMATION_RUNNING:     return "<500 animation is running>";
        case GSERROR_HDI_ERROR:             return "<500 hdi error>";
        case GSERROR_NOT_IMPLEMENT:         return "<501 not implement>";
        case GSERROR_NOT_SUPPORT:           return "<501 not support>";
        case GSERROR_BINDER:                return "<504 binder occur error>";
        case GSERROR_EGL_STATE_UNKOWN:      return "<600 egl state unknown>";
        case GSERROR_EGL_API_FAILED:        return "<600 egl api falied>";
        default:                            return nullptr;
    }
}

inline std::string LowErrorStrSpecial(GSError err)
{
    if (err == LOWERROR_INVALID) {
        // int to string (in 1000)
        char num[] = {
            static_cast<char>(err / 0x64 % 0xa), static_cast<char>(err / 0xa % 0xa), static_cast<char>(err % 0xa), 0
        };
        return std::string("with low error <") + num + ">";
    } else if (err == LOWERROR_FAILURE) {
        return "with low error <failure>";
    }
    return "";
}

#ifdef _WIN32
#define strerror_r(err, buf, len) strerror_s((buf), (len), (err))
#endif

inline std::string LowErrorStr(GSError lowerr)
{
    std::string lowError = LowErrorStrSpecial(lowerr);
    if (lowError == "" && lowerr != 0) {
        char buf[256] = {0}; // 256 mean buffer max length
        strerror_r(lowerr, buf, sizeof buf);
        lowError = std::string("with low error <") + buf + ">";
    }
    return lowError;
}

inline std::string GSErrorStr(GSError err)
{
    GSError diff = static_cast<GSError>(err % LOWERROR_MAX);
    const char *base = GSErrorBaseStr(static_cast<GSError>(err - diff));
    if (base == nullptr) {
        return "<GSError error index out of range>";
    }
    return std::string(base) + LowErrorStr(diff);
}

inline std::string SurfaceErrorStr(GSError err)
{
    return GSErrorStr(err);
}

inline std::ostream &operator <<(std::ostream &os, const GSError &err)
{
    os << GSErrorStr(err);
    return os;
}

inline bool operator ==(GSError a, GSError b)
{
    return static_cast<int32_t>(a) / LOWERROR_MAX == static_cast<int32_t>(b) / LOWERROR_MAX;
}

inline bool operator !=(GSError a, GSError b)
{
    return static_cast<int32_t>(a) / LOWERROR_MAX != static_cast<int32_t>(b) / LOWERROR_MAX;
}

using WMError = GSError;
using SurfaceError = GSError;
using VsyncError = GSError;
} // namespace OHOS
#endif // __cplusplus

#endif // INTERFACES_INNERKITS_COMMON_GRAPHIC_COMMON_H
