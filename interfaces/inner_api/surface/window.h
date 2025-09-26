/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef NDK_INCLUDE_NATIVE_WINDOW_H_
#define NDK_INCLUDE_NATIVE_WINDOW_H_

#include "external_window.h"
#include "native_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MKMAGIC(a, b, c, d) (((a) << 24) + ((b) << 16) + ((c) << 8) + ((d) << 0))

enum NativeObjectMagic {
    NATIVE_OBJECT_MAGIC_WINDOW = MKMAGIC('W', 'I', 'N', 'D'),
    NATIVE_OBJECT_MAGIC_WINDOW_BUFFER = MKMAGIC('W', 'B', 'U', 'F'),
    NATIVE_OBJECT_MAGIC_WINDOW_INVALID = MKMAGIC('I', 'V', 'L', 'D'),
};

// pSurface type is OHOS::sptr<OHOS::Surface>*
OHNativeWindow* CreateNativeWindowFromSurface(void* pSurface);
void DestoryNativeWindow(OHNativeWindow* window);

// pSurfaceBuffer type is OHOS::sptr<OHOS::SurfaceBuffer>*
OHNativeWindowBuffer* CreateNativeWindowBufferFromSurfaceBuffer(void* pSurfaceBuffer);
OHNativeWindowBuffer* CreateNativeWindowBufferFromNativeBuffer(OH_NativeBuffer* nativeBuffer);
void DestroyNativeWindowBuffer(OHNativeWindowBuffer* buffer);

int32_t NativeWindowRequestBuffer(OHNativeWindow *window, OHNativeWindowBuffer **buffer, int *fenceFd);
int32_t NativeWindowFlushBuffer(OHNativeWindow *window, OHNativeWindowBuffer *buffer,
    int fenceFd, Region region);
int32_t GetLastFlushedBuffer(OHNativeWindow *window, OHNativeWindowBuffer **buffer,
    int *fenceFd, float matrix[16]);
int32_t NativeWindowCancelBuffer(OHNativeWindow *window, OHNativeWindowBuffer *buffer);

// The meaning and quantity of parameters vary according to the code type.
// For details, see the NativeWindowOperation comment.
int32_t NativeWindowHandleOpt(OHNativeWindow *window, int code, ...);
BufferHandle *GetBufferHandleFromNative(OHNativeWindowBuffer *buffer);

// NativeObject: NativeWindow, NativeWindowBuffer
int32_t NativeObjectReference(void *obj);
int32_t NativeObjectUnreference(void *obj);
int32_t GetNativeObjectMagic(void *obj);

int32_t NativeWindowSetScalingMode(OHNativeWindow *window, uint32_t sequence, OHScalingMode scalingMode);
int32_t NativeWindowSetMetaData(OHNativeWindow *window, uint32_t sequence, int32_t size,
                                const OHHDRMetaData *metaData);
int32_t NativeWindowSetMetaDataSet(OHNativeWindow *window, uint32_t sequence, OHHDRMetadataKey key,
                                   int32_t size, const uint8_t *metaData);
int32_t NativeWindowSetTunnelHandle(OHNativeWindow *window, const OHExtDataHandle *handle);
int32_t GetSurfaceId(OHNativeWindow *window, uint64_t *surfaceId);
int32_t CreateNativeWindowFromSurfaceId(uint64_t surfaceId, OHNativeWindow **window);
int32_t NativeWindowAttachBuffer(OHNativeWindow *window, OHNativeWindowBuffer *buffer);
int32_t NativeWindowDetachBuffer(OHNativeWindow *window, OHNativeWindowBuffer *buffer);
int32_t NativeWindowGetTransformHint(OHNativeWindow *window, OH_NativeBuffer_TransformType *transform);
int32_t NativeWindowSetTransformHint(OHNativeWindow *window, OH_NativeBuffer_TransformType transform);
int32_t NativeWindowGetDefaultWidthAndHeight(OHNativeWindow *window, int32_t *width, int32_t *height);
int32_t NativeWindowSetRequestWidthAndHeight(OHNativeWindow *window, int32_t width, int32_t height);
void NativeWindowSetBufferHold(OHNativeWindow *window);
int32_t NativeWindowWriteToParcel(OHNativeWindow *window, OHIPCParcel *parcel);
int32_t NativeWindowReadFromParcel(OHIPCParcel *parcel, OHNativeWindow **window);
int32_t GetLastFlushedBufferV2(OHNativeWindow *window, OHNativeWindowBuffer **buffer, int *fenceFd, float matrix[16]);
int32_t NativeWindowSetScalingModeV2(OHNativeWindow *window, OHScalingModeV2 scalingMode);
int32_t NativeWindowDisconnect(OHNativeWindow *window);
int32_t OH_NativeWindow_SetColorSpace(OHNativeWindow *window, OH_NativeBuffer_ColorSpace colorSpace);
int32_t OH_NativeWindow_GetColorSpace(OHNativeWindow *window, OH_NativeBuffer_ColorSpace *colorSpace);
int32_t OH_NativeWindow_SetMetadataValue(OHNativeWindow *window, OH_NativeBuffer_MetadataKey metadataKey,
    int32_t size, uint8_t *metadata);
int32_t OH_NativeWindow_GetMetadataValue(OHNativeWindow *window, OH_NativeBuffer_MetadataKey metadataKey,
    int32_t *size, uint8_t **metadata);
int32_t NativeWindowCleanCache(OHNativeWindow *window);
/**
 * @brief Request native window buffer with lock.
 * 1. The lock and unlock interfaces need to be used in pairs;
 * 2. The internal usage of the interface is set to CPU read/write
 * 3. The lock interface has requestBuffer logic inside, so there is no need to call the requestBuffer interface again;
 * 4. If fenceFd is not equal to -1 inside the lock interface after requestBuffer, it will wait;
 * 5. Repeated locking or unlocking will result in an illegal operation error code;
 * @param window Indicates the window.
 * @param region Indicates the info of the dirty region.
 * @param buffer Indicates the pointer to a <b>NativeWindowBuffer</b> instance.
 * @return Returns the error code of the request of lock.
 */
int32_t NativeWindowLockBuffer(OHNativeWindow *window, Region region, OHNativeWindowBuffer **buffer);
/**
 * @brief Unlock native window buffer with lock.
 * The lock and unlock interfaces need to be used in pairs;
 * @param window Indicates the window.
 * @return Returns the error code of the request of unlock.
 */
int32_t NativeWindowUnlockAndFlushBuffer(OHNativeWindow *window);
int32_t ConvertColorSpaceTypeToNativeBufferColorSpace(int32_t colorSpaceType, OH_NativeBuffer_ColorSpace* colorSpace);
int32_t NativeWindow_PreAllocBuffers(OHNativeWindow *window, uint32_t allocBufferCnt);
#ifdef __cplusplus
}
#endif

#endif