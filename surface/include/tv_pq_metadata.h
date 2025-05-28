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

#ifndef TV_METADATA_PQ_H
#define TV_METADATA_PQ_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct TvVideoWindow {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    uint8_t size;
};

// tv metadata format for transmit
#pragma pack(push, 1)
struct TvPQMetadata {
    uint16_t sceneTag;
    uint8_t uiFrameCnt;
    uint8_t vidFrameCnt;
    uint8_t vidFps;
    uint16_t vidWinX;
    uint16_t vidWinY;
    uint16_t vidWinWidth;
    uint16_t vidWinHeight;
    uint8_t vidWinSize;
    uint16_t vidVdhWidth;
    uint16_t vidVdhHeight;
    uint8_t scaleMode : 4;
    uint8_t dpPixFmt : 4;
    uint8_t colorimetry : 4;
    uint8_t hdr : 4;
    uint16_t hdrLuminance;
    uint16_t hdrRatio;
    uint8_t reserved[4];
};
#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // TV_METADATA_PQ_H