# Native API（Surface/NativeWindow/NativeBuffer）知识入口

## 适用范围

涉及 `OH_NativeWindow_*`、`OH_NativeBuffer_*` 公开 C API、NativeWindow 配置、NativeBuffer 操作、Surface 配置（format/usage/queue size/timeout）、BufferExtraData、SurfaceDelegate/TunnelHandle、错误码映射时，先读本文档。

## 快速代码索引

| 方向 | 主要文件 |
|------|----------|
| NativeWindow API 实现 | `surface/src/native_window.cpp` |
| NativeBuffer API 实现 | `surface/src/native_buffer.cpp` |
| 公开 NativeWindow 头文件 | `interfaces/inner_api/surface/external_window.h` |
| 公开 NativeBuffer 头文件 | `interfaces/inner_api/surface/native_buffer.h` |
| 公共类型定义 | `interfaces/inner_api/surface/buffer_common.h`、`interfaces/inner_api/surface/surface_type.h` |
| 错误码 | `interfaces/inner_api/common/graphic_common_c.h`、`interfaces/inner_api/common/graphic_common.h` |
| Surface 基类 | `interfaces/inner_api/surface/surface.h` |
| 单测 | `surface/test/unittest/native_window_test.cpp`、`surface/test/unittest/native_buffer_test.cpp` |
| 系统测试 | `surface/test/systemtest/` |

## 术语触发词

`OH_NativeWindow`、`OH_NativeBuffer`、`OHNativeWindow`、`OHNativeWindowBuffer`、`NativeWindowHandleOpt`、`RequestBuffer`、`FlushBuffer`、`SetColorSpace`、`GetColorSpace`、`SetMetadataValue`、`GetMetadataValue`、`NativeBufferAlloc`、`NativeBufferReference`、`NativeBufferUnreference`、`BufferRequestConfig`、`BufferFlushConfig`、`OH_NativeBuffer_Config`、`OHSurfaceSource`、`OHScalingMode`、`usage`、`format`

## 先判断的问题

- 改动涉及 NativeWindow 还是 NativeBuffer API？两者是否需要同步？
- 是否改变公开 API 行为（参数校验、返回值、错误码）？
- 是否影响 Surface 配置默认值（usage、format、timeout、colorGamut）？
- 是否涉及 deprecated API 的兼容性？
- 是否影响 NativeWindow 内部缓存（bufferCache_）管理？

## 关键边界

NativeWindow 默认配置：usage = `BUFFER_USAGE_CPU_READ | BUFFER_USAGE_MEM_DMA`、format = `GRAPHIC_PIXEL_FMT_RGBA_8888`、strideAlignment = 8、timeout = 3000ms、colorGamut = `GRAPHIC_COLOR_GAMUT_SRGB`、transform = `GRAPHIC_ROTATE_NONE`。HEBC 白名单应用去掉 `BUFFER_USAGE_CPU_READ`，只保留 `BUFFER_USAGE_MEM_DMA`。这些默认值是上层应用对 Surface 行为的基础假设，修改任何一个都可能影响渲染管线、内存分配策略和跨进程 buffer 传输效率。

`OHNativeWindow` 通过 `NativeWindowMagic` 字段做运行时类型检查：`NATIVE_OBJECT_MAGIC_WINDOW` 标识 `OHNativeWindow`、`NATIVE_OBJECT_MAGIC_WINDOW_BUFFER` 标识 `OHNativeWindowBuffer`、`NATIVE_OBJECT_MAGIC_WINDOW_INVALID` 在析构时设置。`OH_NativeWindow_GetNativeObjectMagic` 用于检查对象类型。这意味着使用已析构的 NativeWindow/NativeWindowBuffer 对象时，magic 字段已被置为 `INVALID`，调用方可以通过此机制检测 use-after-free，但如果跳过检查直接访问则会导致未定义行为。

`OH_NativeWindow_NativeWindowHandleOpt` 使用变参函数 + `operationMap` 分发，支持 25 个操作码（SET_USAGE=5、GET_USAGE=4、SET_FORMAT=3、GET_FORMAT=2、SET_BUFFER_GEOMETRY=0、GET_BUFFER_GEOMETRY=1、SET_STRIDE=6 deprecated since 16、GET_STRIDE=7 deprecated since 16、SET_TIMEOUT=10、GET_TIMEOUT=11、SET_COLOR_GAMUT=12、GET_COLOR_GAMUT=13、SET_TRANSFORM=14、GET_TRANSFORM=15、SET_UI_TIMESTAMP=16、GET_BUFFERQUEUE_SIZE=17、SET_SOURCE_TYPE=18、GET_SOURCE_TYPE=19、SET_APP_FRAMEWORK_TYPE=20、GET_APP_FRAMEWORK_TYPE=21、SET_HDR_WHITE_POINT_BRIGHTNESS=22、SET_SDR_WHITE_POINT_BRIGHTNESS=23、SET_DESIRED_PRESENT_TIMESTAMP=24）。变参函数参数类型不匹配会导致未定义行为，编译器无法检查变参类型，文档中每个操作码的参数类型必须严格遵守。

`OH_NativeBuffer_Alloc` 创建 `SurfaceBufferImpl`，设置 strideAlignment=8、colorGamut=SRGB、transform=NONE，调用 `Alloc()`，标记 DMA-BUF 为 external（通过 ioctl `DMA_BUF_SET_NAME_A`）。失败返回 nullptr。调用方必须检查返回值，否则后续对 nullptr 的解引用会直接崩溃。

`OH_NativeBuffer_GetMetadataValue` 使用 `new uint8_t[]` 分配输出缓冲区——调用方必须释放。`OH_NativeBuffer_SetMetadataValue` 按 metadataKey 分发：DYNAMIC/STATIC/METADATA_TYPE/ROI。ROI 上限 `ROI_METADATA_CAPACITY`(256) 字节。这是 C API 设计的典型所有权问题：输出缓冲区的生命周期管理完全依赖调用方，文档和头文件注释中必须明确说明所有权转移语义。

`OH_NativeWindow_SetColorSpace` 通过 `SetUserData("ATTRKEY_COLORSPACE_INFO", ...)` 存储为字符串；`GetColorSpace` 通过 `GetUserData` 反向查找。这与 NativeBuffer 的 HDI SetMetadata 方式不同，存在非确定性问题：多个枚举项可能编码为相同的字符串值，反向查找时 `std::find_if` 在 `unordered_map` 上可能返回不同枚举项，导致跨版本行为不一致（参见 `docs/knowledge/metadata-colorspace-hdr.md`）。

`OH_NativeWindow_SetMetadataValue`/`GetMetadataValue` 通过 Surface UserData 存储和读取元数据字符串，不经过 HDI。`OH_NativeWindow_NativeWindowSetMetaData` 和 `NativeWindowSetMetaDataSet` 是 deprecated since 10 的旧接口。两套接口的存储路径不同，混用可能导致数据读写不一致。

错误码映射：`SURFACE_ERROR_OK`(0)、`SURFACE_ERROR_INVALID_PARAM`(40001000)、`SURFACE_ERROR_NOT_SUPPORT`(50102000)、`SURFACE_ERROR_UNKOWN`(50002000)、`SURFACE_ERROR_BUFFER_STATE_INVALID`(41207000)、`NATIVE_ERROR_OK`(0)、`NATIVE_ERROR_INVALID_ARGUMENTS`(40001000)、`NATIVE_ERROR_UNSUPPORTED`(50102000)。`GSErrorStr` 函数将错误码转为可读字符串（如 `"<500 api call failed>with low error <Not supported>"`）。`GSError` 比较运算符只比较基码（除以 `LOWERROR_MAX`=1000），所以不同 low error 的同类错误被视为相等，这在错误码过滤或断言时可能掩盖细节差异。

`OH_NativeWindow_WriteToParcel`/`ReadFromParcel` 和 `OH_NativeBuffer_WriteToParcel`/`ReadFromParcel` 用于跨进程 IPC 序列化。NativeWindow parcel 传递 Surface 引用；NativeBuffer parcel 传递 BufferHandle。序列化格式变更会导致跨版本 IPC 不兼容，需要特别注意前向兼容性。

多个 API 版本共存：deprecated（since 8/9/10/12）和当前版本。`OH_NativeWindow_CreateNativeWindow`（deprecated since 12，用 `OH_NativeWindow_CreateNativeWindowFromSurfaceId` 替代）、`OH_NativeWindow_NativeWindowSetScalingMode`（deprecated since 10，用 `OH_NativeWindow_NativeWindowSetScalingModeV2` 替代）等。deprecated 版本仍需维护兼容性，不能在移除前改变行为或返回值。

`NativeWindowLockBuffer`/`UnlockAndFlushBuffer` 是 CPU 直接渲染模式：LockBuffer 请求 buffer、等 fence、映射；UnlockAndFlushBuffer 解除映射并刷回。同一时刻只允许一个 locked buffer（`mLockedBuffer_`），这意味着 CPU 渲染场景下无法双 buffer 流水线，是性能瓶颈所在。

`appFrameworkType` 上限 `MAXIMUM_LENGTH_OF_APP_FRAMEWORK`(64) 字节，影响 FrameReport 游戏帧率追踪。`OHSurfaceSource` 区分 DEFAULT/UI/GAME/CAMERA/VIDEO/LOWPOWERVIDEO。`DAMAGES_MAX_SIZE` = 1000 限制 Region 中的 damage rect 数量。`META_DATA_MAX_SIZE` = 3000 限制 HDR metadata 字节数。`SURFACE_MAX_USER_DATA_COUNT` = 1000 限制 UserData 键值对数量。这些上限值的变更会影响上层模块的数据完整性假设。

`OH_NativeWindow_NativeWindowRequestBuffer` 返回 buffer + fence fd（`int*`）。如果 buffer 已在 `bufferCache_` 中命中，只更新 fence 不重新传输 buffer 数据。`OH_NativeWindow_GetSurfaceId`/`OH_NativeWindow_CreateNativeWindowFromSurfaceId` 通过 SurfaceId 跨进程传递 Surface 引用，避免直接暴露 `sptr<Surface>`。`OH_NativeWindow_NativeWindowAttachBuffer`/`DetachBuffer` 用于外部 buffer 的挂载/卸载。`CreateNativeWindowFromSurface` 初始化时会注册到 SurfaceUtils 全局缓存（`RegisterSurface`），同时初始化 APS（Advanced Power Saving）插件。

WEAK_ALIAS 声明：内部函数名（如 `NativeWindowRequestBuffer`）通过 `__attribute__((weak, alias))` 别名为公开名（`OH_NativeWindow_NativeWindowRequestBuffer`）。`OH_NativeBuffer_IsSupported` 检查 width>0、height>0、format 在有效范围内。`OH_NativeBuffer_MapAndGetConfig` 是 Map + GetConfig 的组合操作，减少一次函数调用开销。`OH_NativeBuffer_MapWaitFence` 先 Map 再无限等待 sync fence，确保 buffer 数据可用后再返回。

## 常见风险

只改 NativeWindow 或 NativeBuffer 一侧 API，遗漏对端同步。两者共用色彩空间映射表和错误码，修改一侧时必须检查另一侧。例如修改 `OH_NativeWindow_SetColorSpace` 的映射逻辑而不检查 `OH_NativeBuffer_SetColorSpace`，会导致同一色彩空间在两个 API 上返回不同结果。

改公开 API 参数校验或返回值时，影响 XTS 兼容性。特别是错误码变化可能导致上层应用行为异常：应用可能依赖特定的错误码值做分支判断，错误码数值变化或新增错误码类型都会导致应用走错分支。此类问题在单元测试中不易发现，因为单测通常只检查成功路径。

`NativeWindowHandleOpt` 变参函数参数类型不匹配导致未定义行为：调用方传入类型错误的变参会破坏栈，且编译器无法检查。例如 SET_USAGE 期望 `uint64_t*` 但传入 `int*`，在 64 位系统上会读到垃圾数据。这类问题在 debug 构建中可能不暴露，只在特定编译优化级别或特定架构上才触发崩溃。

`OH_NativeBuffer_GetMetadataValue` 返回的 `new uint8_t[]` 未被调用方释放，导致内存泄漏。这是 C API 设计的常见问题，文档和头文件注释中必须明确说明所有权。更隐蔽的是，调用方可能假设缓冲区是栈分配或静态分配的，从而根本不尝试释放。

`OH_NativeWindow_SetColorSpace`/`GetColorSpace` 的非确定性问题导致跨版本行为不一致。SetColorSpace 写入编码值，GetColorSpace 反向查找可能返回不同枚举项。当多个枚举项映射到相同的编码值时，`std::find_if` 在 `unordered_map` 上的返回结果取决于迭代顺序，这是不可控的。

deprecated API 仍被上层使用，删除或改变行为导致回归。deprecated API 的行为必须保持不变直到正式移除。常见的错误是在 deprecated API 中加入新的参数校验或返回新的错误码，虽然意图是改善鲁棒性，但实际上破坏了依赖旧行为的上层代码。

Surface UserData 方式存储 metadata 的编码格式变更导致旧数据无法解析。UserData 是字符串 key-value 对，编码格式变更会影响读写兼容性。如果新版本写入的格式旧版本无法识别，或者旧版本写入的格式新版本解析出错，都会导致色彩空间、HDR 元数据等关键信息丢失。

NativeWindow 内部 `bufferCache_` 的线程安全：`mutex_` 保护，但 `OH_NativeWindow_CleanCache` 标记为 non-thread-safe，调用方需自行保证线程安全。`OH_NativeWindow_PreAllocBuffers` 也是 non-thread-safe 的。在多线程场景下调用这些接口可能导致缓存状态不一致，进而引发 buffer 数据错乱或访问已释放内存。

`OH_NativeWindow_NativeWindowFlushBuffer` 中 Region 的 rects 为 nullptr 或 rectNumber 为 0 时，使用整个 buffer 作为 damage。这个默认行为变更会影响脏区传递：如果改为不传 damage 而非全 buffer damage，消费端可能跳过合成导致画面不更新；如果改为更严格的校验导致返回错误，上层未处理该错误的代码路径会崩溃。

## Ask before / 必须人工确认

- 修改公开 API 函数签名、参数、返回值或错误码时。
- 修改 NativeWindow 默认配置（usage、format、timeout 等）时。
- 修改 `OH_NativeWindow_SetColorSpace`/`GetColorSpace` 行为或映射表时。
- 修改 `OH_NativeBuffer_SetMetadataValue`/`GetMetadataValue` 的 key 分发逻辑时。
- 新增或删除 `NativeWindowOperation` 操作码时。
- 修改 deprecated API 的行为时。
- 需要真实设备验证但当前没有设备时。

## 验证建议

- 基础构建：`hb build graphic_surface -i`
- 近端单测：`native_window_test`、`native_buffer_test`
- 系统测试：`native_window_test_st`、`native_window_buffer_test_st`、`native_window_clean_cache_test_st`
- Fuzz：`NativeWindowFuzzTest`、`NativeBufferFuzzTest`
- 涉及公开 API 行为变化时需要补充对应 XTS 用例。
- 涉及 Surface/DMA/HDI 行为时需要真实设备验证。

## 待补充背景

### deprecated API 依赖情况与迁移时间表

**已 deprecated API**（根据代码注释）：
- `OH_NativeWindow_CreateNativeWindow`（deprecated since 12，替代：`OH_NativeWindow_CreateNativeWindowFromSurfaceId`）
- `OH_NativeWindow_NativeWindowSetScalingMode`（deprecated since 10，替代：`OH_NativeWindow_NativeWindowSetScalingModeV2`）
- `OH_NativeWindow_NativeWindowSetMetaData`/`NativeWindowSetMetaDataSet`（deprecated since 10）
- `SET_STRIDE`/`GET_STRIDE`（deprecated since 16，operation code 6/7）

**依赖情况（待团队补充）**：
- 上层模块（RS、multimedia、window_manager）对 deprecated API 的实际调用情况
- 第三方应用对 deprecated API 的依赖情况
- 迁移时间表：何时正式移除 deprecated API

**兼容性要求**：
- deprecated API 必须保持原有行为直到正式移除
- 不能在 deprecated API 中新增参数校验或改变返回值
- 新代码必须使用替代 API，避免引入新的 deprecated 依赖

**迁移建议**：
- `CreateNativeWindow` → `CreateNativeWindowFromSurfaceId`：避免直接暴露 Surface 指针
- `NativeWindowSetScalingMode` → `NativeWindowSetScalingModeV2`：使用扩展版本
- `NativeWindowSetMetaData` → `SetMetadataValue`：使用标准化 metadata API

### XTS 用例映射与覆盖范围

**本仓单测/fuzz**：
- `native_window_test`：NativeWindow API 单测
- `native_buffer_test`：NativeBuffer API 单测
- `NativeWindowFuzzTest`：NativeWindow fuzz 测试
- `NativeBufferFuzzTest`：NativeBuffer fuzz 测试

**系统测试**：
- `native_window_test_st`：NativeWindow 系统测试
- `native_window_buffer_test_st`：NativeBuffer 系统测试
- `native_window_clean_cache_test_st`：缓存清理系统测试

**XTS 覆盖范围（待团队补充）**：
- OpenHarmony XTS 仓中对应 graphic_surface 的测试用例
- 公开 C API（OH_NativeWindow_*/OH_NativeBuffer_*）的 XTS 用例映射
- 色彩空间、HDR metadata、usage/format 配置的 XTS 用例
- 错误码兼容性 XTS 用例

**验证建议**：
- 涉及公开 API 变化时，必须确认 XTS 用例是否需要同步修改
- 新增公开 API 时，必须补充对应 XTS 用例
- 错误码数值变化时，必须检查 XTS 断言是否依赖错误码值

### API 兼容性与缓存典型案例（待团队补充案例编号）

**错误码映射问题**：
- `GSError` 比较运算符只比较基码（除以 `LOWERROR_MAX`=1000），不同 low error 的同类错误被视为相等
- 上层应用可能依赖特定错误码数值做分支判断，数值变化导致应用走错分支
- 新增错误码类型时，上层未处理导致默认错误处理逻辑执行

**变参函数参数类型问题**：
- `NativeWindowHandleOpt` 变参函数，编译器无法检查参数类型
- SET_USAGE 期望 `uint64_t*` 但传入 `int*`，在 64 位系统上读到垃圾数据
- 参数类型错误导致栈破坏，只在特定优化级别或架构上触发崩溃

**buffer 缓存一致性问题**：
- `bufferCache_` 在 `RequestBuffer` 时缓存 buffer+fence，后续调用直接从缓存返回
- 缓存命中时只更新 fence 不重新传输 buffer 数据，跨进程场景下数据可能不一致
- `CleanCache` 标记为 non-thread-safe，多线程调用可能导致缓存状态混乱
- 进程死亡后缓存未正确清理，导致后续 `RequestBuffer` 返回已释放的 buffer

**色彩空间非确定性问题**：
- `SetColorSpace` 写入编码值，`GetColorSpace` 反向查找可能返回不同枚举项
- 多个枚举项映射到相同编码值时，`std::find_if` 在 `unordered_map` 上返回不确定结果
- 跨版本行为不一致：同一应用在不同运行可能得到不同色彩空间值
