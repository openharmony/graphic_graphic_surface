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

/**
 * @addtogroup NativeWindow
 * @{
 *
 * @brief Provides the BufferHandle struct for native window.
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeWindow
 * @since 8
 * @version 1.0
 */

/**
 * @file buffer_handle.h
 *
 * @brief Defines the BufferHandle struct for native window.
 *
 * @kit ArkGraphics2D
 * @library libnative_window.so
 * @syscap SystemCapability.Graphic.Graphic2D.NativeWindow
 * @since 8
 * @version 1.0
 */

#ifndef INCLUDE_BUFFER_HANDLE_H
#define INCLUDE_BUFFER_HANDLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t fd;           /**< buffer fd, -1 if not supported */
    int32_t width;        /**< the width of memory */
    int32_t stride;       /**< the stride of memory */
    int32_t height;       /**< the height of memory */
    int32_t size;         /* < size of memory */
    int32_t format;       /**< the format of memory */
    uint64_t usage;        /**< the usage of memory */
    void *virAddr;        /**< Virtual address of memory  */
    uint64_t phyAddr;     /**< Physical address */
    uint32_t reserveFds;  /**< the number of reserved fd value */
    uint32_t reserveInts; /**< the number of reserved integer value */
    int32_t reserve[0];   /**< the data */
} BufferHandle;

#ifdef __cplusplus
}
#endif

/** @} */
#endif // INCLUDE_BUFFER_HANDLE_H
