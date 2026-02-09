# AGENTS.md

This file provides guidance to Agents Code when working with code in this repository.

## Overview

This is the **graphic_graphic_surface** repository, part of the OpenHarmony graphics subsystem. It provides the Surface component for cross-process graphics buffer management, producer-consumer buffer queue implementation, and zero-copy shared memory transmission for OpenHarmony OS.

## Build System

This project uses the **GN (Generate Ninja)** build system, which is part of OpenHarmony's build infrastructure.

### Building

Build commands are typically run from the OpenHarmony root directory (not this repository root):

```bash
hb build graphic_surface -i # full build of graphic_surface
hb build graphic_surface -i --skip-download --build-target <target> # fast incremental build
```

### Testing

```bash
# Build all tests for graphic_surface
hb build graphic_surface -t
# Fast rebuild of specific target. Full path usually works, e.g. //foundation/graphic/graphic_surface/surface/test/unittest/buffer_queue_test
hb build graphic_surface -t --skip-download --build-target <target>
```

## Key Directories

**surface/** - Core Surface framework
- `include/` - Surface header files
- `src/` - Surface implementation
  - `buffer_queue.cpp` - Buffer queue management
  - `producer_surface.cpp` - Producer implementation
  - `consumer_surface.cpp` - Consumer implementation
  - `surface_buffer_impl.cpp` - Buffer implementation
  - `metadata_helper.cpp` - Metadata (colorspace, HDR) handling
- `test/` - Tests (unittest, fuzztest, systemtest)

**interfaces/inner_api/** - API definitions
- `surface/` - Core Surface APIs (NativeWindow, NativeBuffer)
- `buffer_handle/` - Buffer handle APIs
- `sync_fence/` - Sync fence APIs
- `common/` - Common definitions

**utils/** - Utility modules
- `frame_report/` - Frame rate reporting
- `hebc_white_list/` - HEBC (Hardware Enabled Buffer Cache) optimization
- `trace/` - System tracing integration
- `rs_frame_report_ext/` - Render service frame report extension

**sandbox/** - Sandbox utilities for process isolation

## Architecture

### Producer-Consumer Model

The Surface architecture follows a **producer-consumer pattern** with a buffer queue:

```
Application Layer (Producer)
    ↓ RequestBuffer → Free Queue
    ↓ FlushBuffer → Dirty Queue
    ↓
BufferQueue (Shared Memory Management)
    ↓ AcquireBuffer ← Dirty Queue
    ↓ ReleaseBuffer → Free Queue
    ↓
Consumer Layer (Display/Media)
```

### Buffer State Machine

```
RELEASED → REQUESTED → FLUSHED → ACQUIRED → RELEASED
   ↑                                      ↓
   └──────────────────────────────────────┘
```

Buffer states:
- `BUFFER_STATE_RELEASED` (0): Buffer is free and available
- `BUFFER_STATE_REQUESTED` (1): Buffer requested by producer
- `BUFFER_STATE_FLUSHED` (2): Buffer filled and ready for consumer
- `BUFFER_STATE_ACQUIRED` (3): Buffer acquired by consumer
- `BUFFER_STATE_ATTACHED` (4): Buffer attached to surface

### Core Modules

**surface/** - Main Surface framework
- `ProducerSurface` - Producer-side surface implementation
- `ConsumerSurface` - Consumer-side surface implementation
- `BufferQueue` - Double-ended buffer queue manager
- `SurfaceBuffer` - Graphics buffer wrapper with format, size, usage info
- `BufferQueueProducer` - IPC producer interface
- `BufferQueueConsumer` - IPC consumer interface

**buffer_handle/** - Buffer handle management
- Cross-process buffer sharing abstraction
- `BufferHandle` structure for shared memory references

**sync_fence/** - Synchronization mechanism
- `SyncFence` - Buffer synchronization fence
- `AcquireFenceManager` - Acquire fence management
- `FrameSched` - Frame scheduler for rate control

**interfaces/inner_api/** - Public APIs
- `surface/native_window.h` - Native window C API
- `surface/buffer_common.h` - Native buffer C API
- `surface/surface_type.h` - Common type definitions

## Native API (C Interface)

### NativeWindow APIs

```c
// Window creation and destruction
OHNativeWindow* OH_NativeWindow_Create(uint32_t width, uint32_t height, int32_t format);
void OH_NativeWindow_Destroy(OHNativeWindow* window);

// Buffer operations
int32_t OH_NativeWindow_RequestBuffer(OHNativeWindow* window, OHNativeWindowBuffer** buffer);
int32_t OH_NativeWindow_FlushBuffer(OHNativeWindow* window, OHNativeWindowBuffer* buffer);

// Configuration
int32_t OH_NativeWindow_SetColorSpace(OHNativeWindow* window, OH_NativeBuffer_ColorSpace colorSpace);
int32_t OH_NativeWindow_GetColorSpace(OHNativeWindow* window, OH_NativeBuffer_ColorSpace* colorSpace);
```

### NativeBuffer APIs

```c
// Buffer operations
int32_t OH_NativeBuffer_GetColorSpace(OH_NativeBuffer* buffer, OH_NativeBuffer_ColorSpace* colorSpace);
int32_t OH_NativeBuffer_SetColorSpace(OH_NativeBuffer* buffer, OH_NativeBuffer_ColorSpace colorSpace);

// Metadata operations
int32_t OH_NativeBuffer_SetMetadataValue(OH_NativeBuffer* buffer, OH_NativeBuffer_MetadataKey key, int32_t size, uint8_t* metadata);
int32_t OH_NativeBuffer_GetMetadataValue(OH_NativeBuffer* buffer, OH_NativeBuffer_MetadataKey key, int32_t* size, uint8_t** metadata);
```

## Color Space Management

**Important Note**: There is a known issue with color space mapping in `metadata_helper.cpp`.

The conversion functions `ConvertColorSpaceTypeToInfo` and `ConvertColorSpaceInfoToType` are mathematically reversible (bit-field pack/unpack), but the mapping table `NATIVE_COLORSPACE_TO_HDI_MAP` in `native_buffer.cpp` has **multiple keys mapping to the same value**:

```cpp
// These map to the same CM_ColorSpaceType value:
{OH_COLORSPACE_BT2020_HLG_FULL, CM_BT2020_HLG_FULL}        // enum value = 4
{OH_COLORSPACE_DISPLAY_BT2020_HLG, CM_DISPLAY_BT2020_HLG}  // enum value = 30

// Both evaluate to: 4 | (5 << 8) | (4 << 16) | (1 << 21) = 2360580
```

When using `OH_NativeBuffer_GetColorSpace`, the reverse lookup with `std::find_if` on `unordered_map` may return either of the keys, resulting in non-deterministic behavior.

## Metadata Types

Supported metadata types (from `buffer_common.h`):

**Color Spaces**: BT601, BT709, BT2020, SRGB, P3, AdobeRGB with FULL/LIMITED range and various transfer functions (HLG, PQ, SRGB, Linear)

**HDR Metadata**:
- `OH_VIDEO_HDR_HLG` - HLG HDR
- `OH_VIDEO_HDR_HDR10` - HDR10
- `OH_VIDEO_HDR_VIVID` - HDR Vivid

**Metadata Keys**:
- `OH_HDR_DYNAMIC_METADATA` - Dynamic HDR metadata
- `OH_HDR_STATIC_METADATA` - Static HDR metadata
- `OH_NATIVEWINDOW_DATA_SPACE` - Data space (color space + range)

## Common Patterns

### Buffer Request-Flow Cycle

1. Producer calls `RequestBuffer()` → gets buffer from Free queue
2. Producer fills buffer with graphics data
3. Producer calls `FlushBuffer()` → puts buffer in Dirty queue
4. Consumer calls `AcquireBuffer()` → gets buffer from Dirty queue
5. Consumer processes buffer (composite, encode, etc.)
6. Consumer calls `ReleaseBuffer()` → returns buffer to Free queue

### IPC Communication

- Producer and Consumer typically run in different processes
- `BufferQueueProducer` and `BufferQueueConsumer` are IPC interfaces
- Shared memory (BufferHandle) is passed via IPC handles
- Zero-copy: only handles are copied, pixel data stays in shared memory

### Synchronization

- **AcquireFence**: Signals when producer has finished writing
- **ReleaseFence**: Signals when consumer has finished reading
- Fences are file descriptors that can be waited upon
- GPU and display hardware use fences for synchronization

## Feature Flags

The build is controlled by feature flags in `graphic_surface_config.gni`:

- `surface_enable_gpu_buffer` - Enable GPU buffer support
- `surface_enable_fuzz_test` - Enable fuzz test builds
- `surface_enable_frame_report` - Enable frame rate reporting
- `surface_use_rosen_frame_report` - Use Rosen's frame report

## Development Notes

### Memory Management

- Uses smart pointers (`sptr<T>`)
- Reference counting for Surface and Buffer objects
- **Critical**: Shared memory is managed in the process that first creates the Surface
- If that process exits abnormally, serious memory leaks can occur

### Thread Safety

- BufferQueue is thread-safe for concurrent producer/consumer access
- Mutexes protect queue operations
- Condition variables signal buffer availability

### Performance Considerations

- Zero-copy design: Use for large memory transfers (display data)
- Not recommended for small transfers (causes memory fragmentation)
- Contiguous physical memory support for higher transfer rates
- HEBC (Hardware Enabled Buffer Cache) optimization for hardware buffers

### Logging and Tracing

- Uses `BLOG*` macros (e.g., `BLOGE`, `BLOGD`, `BLOGI`) from `buffer_log.h`
- Integrated with `hitrace` for system-wide performance tracing
- Frame reporting integration with Rosen render service

## Dependencies

- **buffer_handle**: Cross-process buffer sharing abstraction (shared memory handles, fd management)
- **sync_fence**: Buffer synchronization fence management (AcquireFence, ReleaseFence, timeline tracking)
- **HDI (Hardware Display Interface)**: Hardware abstraction for displays, color management, and graphics HAL
- **c_utils**: Common utilities (memory operations, string handling, error codes)
- **hilog**: System logging framework (BLOG* macros for debug/info/error/warning)
- **hitrace**: System-wide performance tracing integration (frame reporting, buffer flow tracking)
- **IPC**: OpenHarmony IPC framework for cross-process BufferQueue producer-consumer communication
- **eventhandler**: Event handling and dispatch (file descriptor monitoring, timer events)
- **Rosen (Render Service)**: Display output, frame rate reporting, and HDR metadata handling

## File Naming Conventions

- `surface_*` - Surface implementation files
- `producer_*` - Producer-related files
- `consumer_*` - Consumer-related files
- `buffer_queue*` - Buffer queue files
- `metadata_*` - Metadata handling files
- `native_*` - Native API headers (C interface)

## Related Repositories

- [graphic_graphic_2d](https://gitcode.com/openharmony/graphic_graphic_2d) - Rendering service and 2D graphics
- [window_window_manager](https://gitcode.com/openharmony/window_window_manager) - Window management service
- [multimedia_media](https://gitcode.com/openharmony/multimedia_media) - Media framework
