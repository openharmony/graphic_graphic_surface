# 图形 Surface 知识库索引

## 概述

本知识库包含 graphic_surface 模块的稳定背景知识，按核心场景组织。改动前先根据场景或关键词定位对应文档，再深入阅读。

**入口**：AGENTS.md → **索引**：本文档 → **知识文件**：`docs/knowledge/*.md`

---

## 场景索引

按改动场景快速定位知识文件：

| 场景分类 | 知识文件 | 关键主题 |
|---------|---------|---------|
| **BufferQueue 与生产消费** | [buffer-queue-producer-consumer.md](buffer-queue-producer-consumer.md) | 双队列管理、状态机、IPC 代理、线程安全、Connect/Disconnect |
| **BufferHandle 与跨进程** | [buffer-handle-cross-process.md](buffer-handle-cross-process.md) | fd 传递、mmap、零拷贝、生命周期、HDI 接口、内存回收 |
| **SyncFence 同步** | [sync-fence-synchronization.md](sync-fence-synchronization.md) | fence 等待/合并、Acquire/Release Fence、帧调度、GPU subhealth |
| **色彩空间与 HDR** | [metadata-colorspace-hdr.md](metadata-colorspace-hdr.md) | ColorSpace 映射、HDR metadata、DATASPACE、多键同值问题 |
| **Native API** | [native-api-surface.md](native-api-surface.md) | OH_NativeWindow/NativeBuffer C API、配置默认值、错误码 |
| **诊断与工具** | [utils-frame-report-hebc-trace.md](utils-frame-report-hebc-trace.md) | FrameReport、HEBC 白名单、trace 宏、RS 扩展 |

---

## 术语索引

按关键词定位知识文件：

| 触发词 | 优先读取 | 核心关注点 |
|-------|---------|-----------|
| `BufferQueue`、`RequestBuffer`、`FlushBuffer`、`AcquireBuffer`、`ReleaseBuffer`、`CancelBuffer`、`AttachBuffer`、`DetachBuffer`、Free/Dirty 队列、`BUFFER_STATE_*`、`ProducerSurface`、`ConsumerSurface`、`BufferQueueProducer`、`BufferQueueConsumer`、`Connect`、`Disconnect` | [buffer-queue-producer-consumer.md](buffer-queue-producer-consumer.md) | 双队列运作、状态机流转、IPC 代理、线程安全、连接生命周期 |
| `BufferHandle`、fd、`reserveFds`、`reserveInts`、mmap、零拷贝、共享内存、DMA-BUF、`virAddr`、`phyAddr`、`SurfaceBuffer`、`SurfaceBufferImpl`、引用计数、`AllocateBufferHandle`、`FreeBufferHandle`、`WriteBufferHandle`、`ReadBufferHandle`、`CloneDmaBufferHandle`、HDI、`IDisplayBuffer`、内存回收、内存恢复 | [buffer-handle-cross-process.md](buffer-handle-cross-process.md) | 跨进程传输、fd 生命周期、内存泄漏、HDI 降级 |
| `SyncFence`、`AcquireFence`、`ReleaseFence`、fence、wait、merge、`SYNC_IOC_MERGE`、`SyncFenceTracker`、`AcquireFenceManager`、`FrameSched`、GPU subhealth、`UniqueFd`、`FENCE_PENDING_TIMESTAMP`、`INVALID_FENCE`、`OH_NativeFence` | [sync-fence-synchronization.md](sync-fence-synchronization.md) | fence 语义、硬件同步、帧率控制、异步追踪 |
| `ColorSpace`、`OH_NativeBuffer_ColorSpace`、`CM_ColorSpaceType`、`ConvertColorSpaceTypeToInfo`、`ConvertColorSpaceInfoToType`、`NATIVE_COLORSPACE_TO_HDI_MAP`、HDR、`OH_HDR_DYNAMIC_METADATA`、`OH_HDR_STATIC_METADATA`、`OH_HDR_METADATA_TYPE`、`MetadataType`、DATASPACE、`MetadataHelper`、`ATTRKEY_COLORSPACE_INFO`、`ATTRKEY_HDR_*`、PQ、`TV PQ`、`GraphicColorGamut` | [metadata-colorspace-hdr.md](metadata-colorspace-hdr.md) | 色彩空间映射、HDR metadata、位域转换、多键同值 |
| `OH_NativeWindow`、`OH_NativeBuffer`、`OHNativeWindow`、`OHNativeWindowBuffer`、`NativeWindowHandleOpt`、`SetColorSpace`、`GetColorSpace`、`SetMetadataValue`、`GetMetadataValue`、`NativeBufferAlloc`、`NativeBufferReference`、`NativeBufferUnreference`、`BufferRequestConfig`、`BufferFlushConfig`、`OH_NativeBuffer_Config`、`OHSurfaceSource`、`OHScalingMode`、usage、format | [native-api-surface.md](native-api-surface.md) | C API 行为、错误码、配置参数、默认值 |
| `FrameReport`、`FrameReportExt`、`rs_frame_report_ext`、HEBC、`HebcWhiteList`、`surface_trace`、`SURFACE_TRACE_*`、`HITRACE_TAG_GRAPHIC_AGP`、游戏帧率、应用框架类型、白名单配置、trace 开销 | [utils-frame-report-hebc-trace.md](utils-frame-report-hebc-trace.md) | 诊断上报、白名单配置、trace 热点开销 |

---

## 知识文件摘要

### [buffer-queue-producer-consumer.md](buffer-queue-producer-consumer.md)

**BufferQueue 生产者-消费者模型核心知识**

- **双队列管理**：Free 队列（可用 buffer）、Dirty 队列（已填充 buffer）、状态机（RELEASED→REQUESTED→FLUSHED→ACQUIRED→RELEASED）
- **IPC 代理**：ProducerSurface 通过 IBufferProducer 代理与 BufferQueueProducer 通信，ConsumerSurface 拥有 BufferQueue、BufferQueueProducer、BufferQueueConsumer
- **线程安全**：mutex_ 保护队列、waitReqCon_ 条件变量、isAllocatingBufferCon_ 防并发分配
- **关键操作**：RequestBuffer、FlushBuffer、AcquireBuffer、ReleaseBuffer、CancelBuffer、AttachBuffer、DetachBuffer
- **连接管理**：Connect/Disconnect、PID 门控、严格断连模式、进程死亡处理
- **代码锚点**：`surface/src/buffer_queue.cpp`、`surface/src/producer_surface.cpp`、`surface/src/consumer_surface.cpp`
- **验证重点**：buffer_queue_test、buffer_queue_producer_test、buffer_queue_consumer_test、producer_surface_test、consumer_surface_test

### [buffer-handle-cross-process.md](buffer-handle-cross-process.md)

**BufferHandle 跨进程共享内存核心知识**

- **结构设计**：柔性数组 reserve[0]（前 reserveFds 个是 fd、后 reserveInts 个是 int）、上限 1024
- **跨进程语义**：fd 通过 Binder IPC 传递、内核创建新 fd 指向同一 DMA-BUF、virAddr 跨进程无效
- **生命周期**：AllocateBufferHandle/FreeBufferHandle（一对一）、SurfaceBufferImpl 通过 HDI IDisplayBuffer 分配/释放/映射
- **引用计数**：sptr 管理、全局序列号（高 16 位 PID + 低 16 位自增）、全局 bufferId（高 16 位 PID + 低 48 位自增）
- **内存回收**：TryReclaim/TryResumeIfNeeded、回收后未恢复访问 virAddr 会导致段错误
- **代码锚点**：`buffer_handle/src/buffer_handle.cpp`、`surface/src/surface_buffer_impl.cpp`、`surface/src/buffer_utils.cpp`
- **验证重点**：BufferHandleTest、surface_buffer_impl_test、buffer_utils_test

### [sync-fence-synchronization.md](sync-fence-synchronization.md)

**SyncFence 同步机制核心知识**

- **基础语义**：基于 Linux sync_file fd、UniqueFd RAII 管理、不可拷贝/不可移动、sptr 引用计数
- **等待与合并**：poll() 等待（支持超时）、SYNC_IOC_MERGE 合并（非对称语义）
- **异步追踪**：SyncFenceTracker 使用 EventRunner/EventHandler 异步等待、GPU subhealth 检测（12ms 阈值、每日 200 次上限）
- **帧调度**：FrameSched 动态加载 libframe_ui_intf.z.so、SendFenceId/MonitorGpuStart/MonitorGpuEnd
- **C API**：OH_NativeFence 对应 Native Fence C API
- **代码锚点**：`sync_fence/src/sync_fence.cpp`、`sync_fence/src/acquire_fence_manager.cpp`、`sync_fence/src/frame_sched.cpp`
- **验证重点**：sync_fence_test、native_fence_test

### [metadata-colorspace-hdr.md](metadata-colorspace-hdr.md)

**色彩空间、HDR Metadata 与元数据管理核心知识**

- **ColorSpace 转换**：ConvertColorSpaceTypeToInfo（位域解包）、ConvertColorSpaceInfoToType（位域打包）
- **映射表**：NATIVE_COLORSPACE_TO_HDI_MAP（33 条）、多键同值问题（OH_COLORSPACE_BT2020_HLG_FULL 与 OH_COLORSPACE_DISPLAY_BT2020_HLG 映射相同值）
- **HDR metadata key**：ATTRKEY_COLORSPACE_INFO、ATTRKEY_HDR_METADATA_TYPE、ATTRKEY_HDR_STATIC_METADATA、ATTRKEY_HDR_DYNAMIC_METADATA、ATTRKEY_VIDEO_AI_HDR_LUT、ATTRKEY_CROP_REGION、ATTRKEY_ROI_METADATA、ATTRKEY_ADAPTIVE_FOV_METADATA、ATTRKEY_EXTERNAL_METADATA_002
- **MetadataType 映射**：OH_VIDEO_HDR_HLG/HR10/VIVID、OH_IMAGE_HDR_VIVID_DUAL/SINGLE、OH_IMAGE_HDR_ISO_DUAL/SINGLE
- **TV PQ metadata**：条件编译 RS_ENABLE_TV_PQ_METADATA、1 字节对齐、约 23 字节结构体
- **代码锚点**：`surface/src/metadata_helper.cpp`、`surface/src/native_buffer.cpp`、`surface/src/native_window.cpp`
- **验证重点**：metadata_helper_test、native_buffer_test、对应 XTS

### [native-api-surface.md](native-api-surface.md)

**Native API（Surface/NativeWindow/NativeBuffer）核心知识**

- **默认配置**：usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_MEM_DMA、format = GRAPHIC_PIXEL_FMT_RGBA_8888、strideAlignment = 8、timeout = 3000ms、colorGamut = GRAPHIC_COLOR_GAMUT_SRGB
- **类型检查**：NativeWindowMagic（NATIVE_OBJECT_MAGIC_WINDOW/BUFFER/INVALID）、OH_NativeWindow_GetNativeObjectMagic
- **变参操作**：OH_NativeWindow_NativeWindowHandleOpt（25 个操作码）、参数类型必须严格遵守文档
- **所有权语义**：OH_NativeBuffer_GetMetadataValue 使用 new uint8_t[] 分配、调用方必须释放
- **错误码**：SURFACE_ERROR_*、NATIVE_ERROR_*、GSErrorStr 转字符串、GSError 比较只比基码
- **代码锚点**：`surface/src/native_window.cpp`、`surface/src/native_buffer.cpp`、`interfaces/inner_api/surface/external_window.h`
- **验证重点**：native_window_test、native_buffer_test、对应 XTS

### [utils-frame-report-hebc-trace.md](utils-frame-report-hebc-trace.md)

**Utils：FrameReport、HEBC、Trace 与 RS FrameReportExt 核心知识**

- **Surface Trace**：`utils/trace/surface_trace.h` 只提供宏封装、无独立单测、修改必须检查实际使用方
- **HEBC 白名单**：HebcWhiteList 解析配置、命中时移除 BUFFER_USAGE_CPU_READ、保留 BUFFER_USAGE_MEM_DMA
- **FrameReport**：帧率/帧耗时上报、诊断辅助能力
- **RS FrameReportExt**：扩展上报能力、动态加载
- **性能约束**：热点路径（RequestBuffer/FlushBuffer/AcquireBuffer/ReleaseBuffer）禁止增加字符串拼接、文件读取、JSON 解析、同步 IPC、高频 INFO 日志
- **代码锚点**：`utils/frame_report/`、`utils/hebc_white_list/`、`utils/trace/surface_trace.h`、`utils/rs_frame_report_ext/`
- **验证重点**：frame_report_test、hebc_white_list_test、rs_frame_report_ext_test、实际使用方 trace 结果

---

## 核心文件索引

### NativeWindow 文件结构

NativeWindow 是图形 Surface 框架的核心对外 C API 模块，提供生产者侧 Surface 操作接口。

#### 头文件

| 文件路径 | 说明 | 主要内容 |
|---------|------|---------|
| `interfaces/inner_api/surface/external_window.h` | **公开 C API 头文件**（Native API） | OHNativeWindow/OHNativeWindowBuffer 类型定义、OH_NativeWindow_* 函数声明、BufferRequestConfig/BufferFlushConfig 配置结构、OH_NativeWindow_NativeWindowHandleOpt 操作码定义、变参函数接口 |
| `surface/include/native_window.h` | **内部实现头文件** | NativeWindow/NativeWindowBuffer 结构体定义、NativeWindowMagic 类型检查机制、内部字段（surface、uiTimestamp、bufferCache_、desiredPresentTimestamp、appFrameworkType_） |

#### 源文件

| 文件路径 | 说明 | 主要实现 |
|---------|------|---------|
| `surface/src/native_window.cpp` | **NativeWindow API 实现**（1167 行） | CreateNativeWindow/DestroyNativeWindow、RequestBuffer/FlushBuffer、OH_NativeWindow_NativeWindowHandleOpt（25 个操作码分发）、NATIVE_COLORSPACE_TO_HDI_MAP（33 条映射）、SetColorSpace/GetColorSpace（通过 UserData 存储）、SetMetadataValue/GetMetadataValue、HEBC 白名单应用 usage 调整 |

#### 测试文件

| 文件路径 | 说明 |
|---------|------|
| `surface/test/unittest/native_window_test.cpp` | 单元测试 |
| `surface/test/systemtest/native_window_test.cpp` | 系统测试 |
| `surface/test/systemtest/native_window_buffer_test.cpp` | Buffer 相关系统测试 |
| `surface/test/systemtest/clean_cache_test/native_window_clean_cache_test.cpp` | 缓存清理测试 |

#### 核心结构体

**NativeWindow**：
```cpp
struct NativeWindow : public NativeWindowMagic {
    OHOS::sptr<OHOS::Surface> surface;          // 持有 Surface 引用
    int64_t uiTimestamp = 0;                     // UI 时间戳
    std::unordered_map<uint32_t, NativeWindowBuffer*> bufferCache_;  // buffer 缓存
    std::atomic<int64_t> desiredPresentTimestamp{0};  // 期望呈现时间戳
    char* appFrameworkType_ = nullptr;           // 应用框架类型
    std::once_flag appFrameworkTypeOnceFlag_;    // appFrameworkType 初始化标记
    std::mutex mutex_;                           // 保护 bufferCache_
};
```

**NativeWindowBuffer**：
```cpp
struct NativeWindowBuffer : public NativeWindowMagic {
    OHOS::sptr<OHOS::SurfaceBuffer> sfbuffer;    // 持有 SurfaceBuffer 引用
    int64_t uiTimestamp = 0;                     // UI 时间戳
};
```

**NativeWindowMagic**（类型检查）：
```cpp
struct NativeWindowMagic : public OHOS::RefBase {
    NativeObjectMagic magic;  // NATIVE_OBJECT_MAGIC_WINDOW/BUFFER/INVALID
};
```

#### 关键 API

| API | 说明 | 注意事项 |
|-----|------|---------|
| `OH_NativeWindow_NativeWindowHandleOpt` | 变参函数，25 个操作码（SET_USAGE/GET_USAGE/SET_FORMAT/GET_FORMAT/...） | 参数类型必须严格遵守文档，编译器无法检查变参类型 |
| `OH_NativeWindow_RequestBuffer` | 请求 buffer | 返回 OHNativeWindowBuffer，需配合 SyncFence |
| `OH_NativeWindow_FlushBuffer` | 提交 buffer | 带 fence 和 timestamp |
| `OH_NativeWindow_SetColorSpace` | 设置色彩空间 | 通过 UserData("ATTRKEY_COLORSPACE_INFO") 存储字符串，存在多键同值非确定性 |
| `OH_NativeWindow_GetColorSpace` | 获取色彩空间 | std::find_if 反向查找，多键同值时结果不确定 |
| `OH_NativeWindow_GetNativeObjectMagic` | 获取 magic 类型 | 用于检查对象类型和生命周期（INVALID 表示已析构） |

#### 色彩空间映射表

`native_window.cpp` 包含 `NATIVE_COLORSPACE_TO_HDI_MAP`（33 条），与 `native_buffer.cpp` 副本必须同步维护：
- OH_COLORSPACE_BT2020_HLG_FULL（枚举序号=4）与 OH_COLORSPACE_DISPLAY_BT2020_HLG（枚举序号=22）映射相同值（CM_BT2020_HLG_FULL = 2360580）
- OH_COLORSPACE_SRGB_FULL(11) 与 OH_COLORSPACE_DISPLAY_SRGB(25) 映射相同值
- OH_COLORSPACE_P3_FULL(12) 与 OH_COLORSPACE_DISPLAY_P3_SRGB(26) 映射相同值
- OH_COLORSPACE_BT2020_PQ_FULL(5) 与 OH_COLORSPACE_DISPLAY_BT2020_PQ(31) 映射相同值

---

### NativeBuffer 文件结构

NativeBuffer 是图形 Surface 框架的核心对外 C API 模块，提供独立 buffer 分配和操作接口。

#### 头文件

| 文件路径 | 说明 | 主要内容 |
|---------|------|---------|
| `interfaces/inner_api/surface/native_buffer.h` | **公开 C API 头文件**（Native API） | OH_NativeBuffer 类型定义、OH_NativeBuffer_Config 配置结构、OH_NativeBuffer_Usage 枚举、OH_NativeBuffer_ColorGamut 枚举、OH_NativeBuffer_Plane/Planes 结构（多平面）、OH_NativeBuffer_* 函数声明 |
| `interfaces/inner_api/surface/native_buffer_inner.h` | **内部 API 头文件** | OH_NativeBuffer_GetBufferHandle、OH_NativeBuffer_GetNativeBufferConfig、OH_NativeBufferFromNativeWindowBuffer |

#### 源文件

| 文件路径 | 说明 | 主要实现 |
|---------|------|---------|
| `surface/src/native_buffer.cpp` | **NativeBuffer API 实现**（523 行） | OH_NativeBuffer_Alloc（创建 SurfaceBufferImpl、设置 DMA-BUF name）、OH_NativeBuffer_Reference/Unreference、OH_NativeBuffer_Map/Unmap、OH_NativeBuffer_GetSeqNum、SetColorSpace/GetColorSpace（通过 HDI SetMetadata）、SetMetadataValue/GetMetadataValue、NATIVE_COLORSPACE_TO_HDI_MAP（33 条）、NATIVE_METADATATYPE_TO_HDI_MAP（7 条） |

#### 测试文件

| 文件路径 | 说明 |
|---------|------|
| `surface/test/unittest/native_buffer_test.cpp` | 单元测试 |

#### 核心结构体

**OH_NativeBuffer_Config**：
```cpp
typedef struct {
    int32_t width;           // 宽度（像素）
    int32_t height;          // 高度（像素）
    int32_t format;          // PixelFormat
    int32_t usage;           // buffer usage 组合
    int32_t stride;          // 内存步长（字节）
} OH_NativeBuffer_Config;
```

**OH_NativeBuffer_Planes**（多平面，since 12）：
```cpp
typedef struct {
    uint64_t offset;         // 偏移（字节）
    uint32_t rowStride;      // 行步长
    uint32_t columnStride;   // 列步长
} OH_NativeBuffer_Plane;

typedef struct {
    uint32_t planeCount;              // 平面数量
    OH_NativeBuffer_Plane planes[4];  // 平面数组（最多 4 个）
} OH_NativeBuffer_Planes;
```

#### 关键 API

| API | 说明 | 注意事项 |
|-----|------|---------|
| `OH_NativeBuffer_Alloc` | 分配 buffer | 创建 SurfaceBufferImpl、设置 strideAlignment=8、colorGamut=SRGB、DMA-BUF 标记为 external，失败返回 nullptr |
| `OH_NativeBuffer_Reference` | 增加引用计数 | 返回错误码 |
| `OH_NativeBuffer_Unreference` | 减少引用计数 | 引用计数为 0 时销毁 buffer |
| `OH_NativeBuffer_Map` | 映射到虚拟地址 | CPU 访问前必须映射 |
| `OH_NativeBuffer_Unmap` | 解除映射 | CPU 访问后必须解除 |
| `OH_NativeBuffer_GetSeqNum` | 获取序列号 | 全局唯一 |
| `OH_NativeBuffer_SetColorSpace` | 设置色彩空间 | 通过 HDI SetMetadata(ATTRKEY_COLORSPACE_INFO) 存储 |
| `OH_NativeBuffer_GetColorSpace` | 获取色彩空间 | 通过 HDI GetMetadata 反向查找 |
| `OH_NativeBuffer_SetMetadataValue` | 设置 metadata | 按 metadataKey 分发（DYNAMIC/STATIC/METADATA_TYPE/ROI），ROI 上限 256 字节 |
| `OH_NativeBuffer_GetMetadataValue` | 获取 metadata | **使用 new uint8_t[] 分配**，调用方必须释放 |
| `OH_NativeBuffer_MapPlanes` | 多平面映射（since 12） | 返回所有图像平面信息 |
| `OH_NativeBuffer_FromNativeWindowBuffer` | 类型转换（since 12） | OHNativeWindowBuffer → OH_NativeBuffer |
| `OH_NativeBuffer_MapWaitFence`（since 23） | 映射并等待 fence | 成功时 fenceFd 不需要调用方关闭 |
| `OH_NativeBuffer_WriteToParcel`/`ReadFromParcel`（since 23） | IPC 序列化 | ReadFromParcel 会增加引用计数，需配合 Unreference |
| `OH_NativeBuffer_IsSupported`（since 23） | 检查配置是否支持 | gralloc 能力查询 |

#### Usage 枚举

```cpp
typedef enum OH_NativeBuffer_Usage {
    NATIVEBUFFER_USAGE_CPU_READ = (1ULL << 0),        // CPU 读
    NATIVEBUFFER_USAGE_CPU_WRITE = (1ULL << 1),       // CPU 写
    NATIVEBUFFER_USAGE_MEM_DMA = (1ULL << 3),         // DMA buffer
    NATIVEBUFFER_USAGE_MEM_MMZ_CACHE = (1ULL << 5),   // MMZ with cache（since 20）
    NATIVEBUFFER_USAGE_HW_RENDER = (1ULL << 8),       // GPU 写
    NATIVEBUFFER_USAGE_HW_TEXTURE = (1ULL << 9),      // GPU 读
    NATIVEBUFFER_USAGE_CPU_READ_OFTEN = (1ULL << 16), // 频繁 CPU 读
    NATIVEBUFFER_USAGE_ALIGNMENT_512 = (1ULL << 18),  // 512 字节对齐
} OH_NativeBuffer_Usage;
```

#### ColorGamut 枚举（since 12）

```cpp
typedef enum OH_NativeBuffer_ColorGamut {
    NATIVEBUFFER_COLOR_GAMUT_NATIVE = 0,
    NATIVEBUFFER_COLOR_GAMUT_STANDARD_BT601 = 1,
    NATIVEBUFFER_COLOR_GAMUT_STANDARD_BT709 = 2,
    NATIVEBUFFER_COLOR_GAMUT_DCI_P3 = 3,
    NATIVEBUFFER_COLOR_GAMUT_SRGB = 4,
    NATIVEBUFFER_COLOR_GAMUT_ADOBE_RGB = 5,
    NATIVEBUFFER_COLOR_GAMUT_DISPLAY_P3 = 6,
    NATIVEBUFFER_COLOR_GAMUT_BT2020 = 7,
    NATIVEBUFFER_COLOR_GAMUT_BT2100_PQ = 8,
    NATIVEBUFFER_COLOR_GAMUT_BT2100_HLG = 9,
    NATIVEBUFFER_COLOR_GAMUT_DISPLAY_BT2020 = 10,
} OH_NativeBuffer_ColorGamut;
```

#### 色彩空间与 MetadataType 映射表

`native_buffer.cpp` 包含两份映射表：
- **NATIVE_COLORSPACE_TO_HDI_MAP**（33 条）：与 `native_window.cpp` 副本必须同步
- **NATIVE_METADATATYPE_TO_HDI_MAP**（7 条）：
  - OH_VIDEO_HDR_HLG → CM_VIDEO_HLG
  - OH_VIDEO_HDR_HDR10 → CM_VIDEO_HDR10
  - OH_VIDEO_HDR_VIVID → CM_VIDEO_HDR_VIVID
  - OH_IMAGE_HDR_VIVID_DUAL → CM_IMAGE_HDR_VIVID_DUAL
  - OH_IMAGE_HDR_VIVID_SINGLE → CM_IMAGE_HDR_VIVID_SINGLE
  - OH_IMAGE_HDR_ISO_DUAL → CM_IMAGE_HDR_ISO_DUAL
  - OH_IMAGE_HDR_ISO_SINGLE → CM_IMAGE_HDR_ISO_SINGLE

---

### NativeImage 文件结构

NativeImage 是图形 Surface 框架的核心对外 C API 模块，提供 OpenGL ES 纹理与 Surface 的绑定能力以及消费者 Surface 操作接口。**实现位于 `graphic_2d` 仓库**。

#### 头文件

| 文件路径 | 说明 | 主要内容 |
|---------|------|---------|
| `graphic_2d/interfaces/inner_api/surface/native_image.h` | **公开 C API 头文件**（Native API，466 行） | OH_NativeImage 类型定义、OH_OnFrameAvailableListener 回调结构、OH_NativeImage_* 函数声明、OH_ConsumerSurface_* 函数声明（消费者 Surface API） |

#### 核心结构体

**OH_OnFrameAvailableListener**（帧可用回调）：
```cpp
typedef void (*OH_OnFrameAvailable)(void *context);

typedef struct OH_OnFrameAvailableListener {
    void *context;                    // 用户上下文
    OH_OnFrameAvailable onFrameAvailable;  // 回调函数
} OH_OnFrameAvailableListener;
```

#### 关键 API

| API | 说明 | 版本 | 注意事项 |
|-----|------|------|---------|
| `OH_NativeImage_Create` | 创建 NativeImage（绑定 GL 纹理） | since 9 | 参数：textureId、textureTarget，返回 OH_NativeImage* |
| `OH_NativeImage_Destroy` | 销毁 NativeImage | since 9 | 参数为 OH_NativeImage**，销毁后指针置 null |
| `OH_NativeImage_AcquireNativeWindow` | 获取关联的 NativeWindow | since 9 | 返回 OHNativeWindow*，用于生产者侧配置 |
| `OH_NativeImage_AttachContext` | 附加到 GL 上下文 | since 9 | 绑定到 GL_TEXTURE_EXTERNAL_OES |
| `OH_NativeImage_DetachContext` | 从 GL 上下文分离 | since 9 | - |
| `OH_NativeImage_UpdateSurfaceImage` | 更新纹理内容 | since 9 | 用已获取的 buffer 更新 GL 纹理 |
| `OH_NativeImage_GetTimestamp` | 获取时间戳 | since 9 | 最近一次 UpdateSurfaceImage 的时间戳 |
| `OH_NativeImage_GetTransformMatrix` | 获取变换矩阵 | since 9 | deprecated since 12，使用 GetTransformMatrixV2 |
| `OH_NativeImage_GetTransformMatrixV2` | 获取变换矩阵（新） | since 12 | 4x4 矩阵，错误码 40001000（image 为 NULL） |
| `OH_NativeImage_GetBufferMatrix` | 获取带裁剪的变换矩阵 | since 14 | 非线程安全，结合 crop rect |
| `OH_NativeImage_GetSurfaceId` | 获取 Surface ID | since 11 | - |
| `OH_NativeImage_SetOnFrameAvailableListener` | 设置帧可用回调 | since 11 | - |
| `OH_NativeImage_UnsetOnFrameAvailableListener` | 移除帧可用回调 | since 11 | - |
| `OH_NativeImage_AcquireNativeWindowBuffer` | 获取 buffer（消费者） | since 12 | 不能与 UpdateSurfaceImage 同时使用，需配合 ReleaseNativeWindowBuffer |
| `OH_NativeImage_ReleaseNativeWindowBuffer` | 释放 buffer | since 12 | fenceFd 由系统关闭 |
| `OH_ConsumerSurface_Create` | 创建消费者 Surface | since 12 | 不能与 UpdateSurfaceImage 同时使用 |
| `OH_ConsumerSurface_SetDefaultUsage` | 设置默认 usage | since 13 | 非线程安全 |
| `OH_ConsumerSurface_SetDefaultSize` | 设置默认尺寸 | since 13 | 非线程安全 |
| `OH_NativeImage_SetDropBufferMode` | 设置丢帧模式 | since 17 | 非线程安全，建议创建后立即调用 |
| `OH_NativeImage_CreateWithSingleBufferMode` | 创建（单缓冲模式） | since 22 | 参数：textureId、textureTarget、singleBufferMode |
| `OH_ConsumerSurface_CreateWithSingleBufferMode` | 创建消费者（单缓冲） | since 22 | - |
| `OH_NativeImage_ReleaseTextImage` | 释放单缓冲模式纹理 | since 22 | 非线程安全，建议生产者 flush 后调用 |
| `OH_NativeImage_GetColorSpace` | 获取色彩空间 | since 22 | 非线程安全 |
| `OH_NativeImage_AcquireLatestNativeWindowBuffer` | 获取最新 buffer | since 22 | 丢弃其它 buffer，生产者收到 release 回调 |
| `OH_NativeImage_IsReleased` | 检查纹理是否已释放 | since 23 | 非线程安全 |
| `OH_NativeImage_Release` | 清理所有缓存并分离 | since 23 | 非线程安全 |

#### 使用场景

**场景 1：OpenGL ES 纹理绑定**
```cpp
// 创建 NativeImage 并绑定 GL 纹理
OH_NativeImage* image = OH_NativeImage_Create(textureId, GL_TEXTURE_EXTERNAL_OES);
// 获取 NativeWindow 用于生产者配置
OHNativeWindow* window = OH_NativeImage_AcquireNativeWindow(image);
// 生产者填充 buffer → UpdateSurfaceImage 更新纹理
OH_NativeImage_UpdateSurfaceImage(image);
// 获取变换矩阵用于渲染
float matrix[16];
OH_NativeImage_GetTransformMatrixV2(image, matrix);
```

**场景 2：消费者 Surface（非 GL）**
```cpp
// 创建消费者 Surface
OH_NativeImage* image = OH_ConsumerSurface_Create();
// 设置默认配置
OH_ConsumerSurface_SetDefaultUsage(image, NATIVEBUFFER_USAGE_CPU_READ);
OH_ConsumerSurface_SetDefaultSize(image, 1920, 1080);
// 获取 NativeWindow 用于生产者连接
OHNativeWindow* window = OH_NativeImage_AcquireNativeWindow(image);
// 获取 buffer
OHNativeWindowBuffer* buffer = nullptr;
int fenceFd = -1;
OH_NativeImage_AcquireNativeWindowBuffer(image, &buffer, &fenceFd);
// 处理 buffer 内容
OH_NativeImage_ReleaseNativeWindowBuffer(image, buffer, fenceFd);
```

#### 与 Surface/NativeBuffer 的关系

- **NativeImage 作为消费者**：通过 `OH_NativeImage_AcquireNativeWindow` 获取 OHNativeWindow，供生产者（如视频解码器、相机）连接
- **生产消费模型**：生产者通过 NativeWindow RequestBuffer → 填充 → FlushBuffer → NativeImage AcquireNativeWindowBuffer → 处理 → ReleaseNativeWindowBuffer
- **GL 纹理绑定**：UpdateSurfaceImage 自动将 buffer 内容更新到 GL_TEXTURE_EXTERNAL_OES 纹理
- **帧可用回调**：SetOnFrameAvailableListener 监听新帧到达，避免轮询

#### 跨仓库依赖

| 依赖项 | 仓库 | 说明 |
|-------|------|------|
| NativeWindow/NativeBuffer | `graphic_surface` | native_image.h 依赖 native_buffer.h，使用 OHNativeWindow/OHNativeWindowBuffer 类型 |
| NativeImage 实现 | `graphic_2d` | 消费者 Surface、GL 纹理绑定、帧回调等实现 |
| OpenGL ES | 系统库 | GL_TEXTURE_EXTERNAL_OES 纹理目标 |

#### 参考知识文件

改动涉及 NativeImage 与 Surface/NativeBuffer 交互时，参考本仓库：
- [buffer-queue-producer-consumer.md](buffer-queue-producer-consumer.md)：生产消费模型、状态机、IPC 代理
- [buffer-handle-cross-process.md](buffer-handle-cross-process.md)：跨进程传输、fd 生命周期
- [native-api-surface.md](native-api-surface.md)：OHNativeWindow/NativeBuffer C API

---

## 使用建议

1. **改动前定位**：根据 AGENTS.md "知识路由"表格或本文档术语索引，先定位对应知识文件
2. **快速判断**：阅读知识文件的"先判断的问题"部分，确认改动范围
3. **理解边界**：阅读"关键边界"部分，理解核心约束和设计决策
4. **识别风险**：阅读"常见风险"部分，避免典型错误
5. **代码锚点**：根据"快速代码索引"定位关键代码文件
6. **验证重点**：根据知识文件验证矩阵和 AGENTS.md "构建和验证"部分选择测试目标
7. **多场景交叉**：一个任务跨多个场景时，同时读取多个知识文件

---

## 维护说明

- **新增知识文件**：在本索引的"场景索引"和"术语索引"中添加对应条目，在"知识文件摘要"中添加详细说明
- **更新知识文件**：同步更新本索引中的摘要描述和关键词映射
- **新增触发词**：在"术语索引"中添加新条目，指向对应知识文件
- **废弃知识文件**：从本索引移除对应条目，在知识文件头部标记废弃说明