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
    uint16_t scene_tag;
    uint8_t ui_frame_cnt;
    uint8_t vid_frame_cnt;
    uint8_t vid_fps;
    uint16_t vid_win_x;
    uint16_t vid_win_y;
    uint16_t vid_win_width;
    uint16_t vid_win_height;
    uint8_t vid_win_size;
    uint16_t vid_vdh_width;
    uint16_t vid_vdh_height;
    uint8_t scale_mode : 4;
    uint8_t dp_pix_fmt : 4;
    uint8_t colorimetry : 4;
    uint8_t hdr : 4;
    uint16_t hdr_luminance;
    uint16_t hdr_ratio;
    uint8_t reserved[4];
};
#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // TV_METADATA_PQ_H