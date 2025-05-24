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

/**
 * @addtogroup NativeFence
 * @{
 *
 * @brief Provides the native fence capability
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeFence
 * @since 20
 * @version 1.0
 */

/**
 * @file native_fence.h
 *
 * @brief Defines the functions for using native fence.
 *
 * @kit ArkGraphics2D
 * @library libnative_fence.so
 * @syscap SystemCapability.Graphic.Graphic2D.NativeFence
 * @since 20
 * @version 1.0
 */

#ifndef NDK_INCLUDE_NATIVE_FENCE_H_
#define NDK_INCLUDE_NATIVE_FENCE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Checks if the fenceFd is valid.
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeFence
 * @param fenceFd Indicates a file descriptor handle, which is used for timing synchronization.
 * @return Returns true if the fenceFd is valid.
 *         Returns false if the fenceFd is a negative integer.
 * @since 20
 * @version 1.0
 */
bool OH_NativeFence_IsValid(int fenceFd);

/**
 * @brief Waits for a fence signal. The maximum waiting time is determined by the timeout parameter.
 * The incoming fenceFd needs to be closed by the user themselves.
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeFence
 * @param fenceFd Indicates a file descriptor handle, which is used for timing synchronization.
 * @param timeout Indicates the timeout duration.
 * The unit is milliseconds, -1 represents permanent waiting, 0 represents immediate return.
 * @return Returns true if the fence signaled.
 *         Returns false in the following cases:
 *         1.if the fenceFd is a negative integer.
 *         2.no event occurred within the specified timeout period.
 *         3.the underlying poll interface call failed.
 *         4.the timeout value is 0.
 *         5.failed to duplicate the file descriptor.
 * @since 20
 * @version 1.0
 */
bool OH_NativeFence_Wait(int fenceFd, uint32_t timeout);

/**
 * @brief Waits forever for a fence signal.
 * The incoming fenceFd needs to be closed by the user themselves.
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeFence
 * @param fenceFd Indicates a file descriptor handle, which is used for timing synchronization.
 * @return Returns true if the fence signaled.
 *         Returns false in the following cases:
 *         1.if the fenceFd is a negative integer.
 *         2.no incidents have occurred, permanent waiting.
 *         3.failed to duplicate the file descriptor.
 * @since 20
 * @version 1.0
 */
bool OH_NativeFence_WaitForever(int fenceFd);

/**
 * @brief Close the fenceFd.
 *
 * @syscap SystemCapability.Graphic.Graphic2D.NativeFence
 * @param fenceFd Indicates a file descriptor handle, which is used for timing synchronization.
 * This value is a non negative integer.
 * @since 20
 * @version 1.0
 */
void OH_NativeFence_Close(int fenceFd);
#ifdef __cplusplus
}
#endif
/** @} */
#endif