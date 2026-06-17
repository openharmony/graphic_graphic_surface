# BufferQueue 生产者-消费者知识入口

## 适用范围

涉及 `BufferQueue` 双队列管理、`RequestBuffer`/`FlushBuffer`/`AcquireBuffer`/`ReleaseBuffer`、Buffer 状态机、`ProducerSurface`/`ConsumerSurface`、`BufferQueueProducer`/`BufferQueueConsumer` IPC 代理、线程安全、`Connect`/`Disconnect` 时，先读本文档。

## 快速代码索引

| 方向 | 主要文件 |
|------|----------|
| BufferQueue 核心实现 | `surface/src/buffer_queue.cpp`、`surface/include/buffer_queue.h` |
| ProducerSurface（生产者代理） | `surface/src/producer_surface.cpp`、`surface/include/producer_surface.h` |
| ConsumerSurface（消费者入口） | `surface/src/consumer_surface.cpp`、`surface/include/consumer_surface.h` |
| BufferQueueProducer（IPC stub） | `surface/src/buffer_queue_producer.cpp`、`surface/include/buffer_queue_producer.h` |
| BufferQueueConsumer（消费者封装） | `surface/src/buffer_queue_consumer.cpp`、`surface/include/buffer_queue_consumer.h` |
| 错误码 | `interfaces/inner_api/common/graphic_common_c.h` |
| 类型定义 | `interfaces/inner_api/surface/surface_type.h` |
| 单测 | `surface/test/unittest/` |
| Fuzz | `surface/test/fuzztest/` |

## 术语触发词

`BufferQueue`、`RequestBuffer`、`FlushBuffer`、`AcquireBuffer`、`ReleaseBuffer`、`CancelBuffer`、`AttachBuffer`、`DetachBuffer`、`BUFFER_STATE_RELEASED`、`BUFFER_STATE_REQUESTED`、`BUFFER_STATE_FLUSHED`、`BUFFER_STATE_ACQUIRED`、`BUFFER_STATE_ATTACHED`、Free 队列、Dirty 队列、`ProducerSurface`、`ConsumerSurface`、`BufferQueueProducer`、`BufferQueueConsumer`、`Connect`、`Disconnect`、`ConnectStrictly`

## 先判断的问题

- 改动涉及生产者侧、消费者侧还是两侧？是否需要同步修改 IPC 代理？
- 是否改变 Buffer 状态机、队列容量或超时策略？
- 是否影响 IPC 通信（跨进程 buffer 缓存、死亡通知、属性监听）？
- 是否影响 buffer 配置（format、usage、width/height、colorGamut、transform）？
- 是否涉及 `Connect`/`Disconnect` 生命周期或严格断连模式？

## 关键边界

`BufferQueue` 是核心共享数据结构，`ProducerSurface` 和 `ConsumerSurface` 都委托给它操作，所有双队列管理、状态流转、buffer 分配与回收逻辑都集中在 `buffer_queue.cpp`。`ConsumerSurface` 拥有 `BufferQueue`、`BufferQueueProducer` 和 `BufferQueueConsumer` 三个对象；`ProducerSurface` 通过 `IBufferProducer` IPC 代理与 `BufferQueueProducer` 通信。这意味着任何在 `BufferQueue` 层面的改动都会同时影响生产者和消费者两侧的行为，而 IPC 路径和本地路径的行为一致性需要特别关注。

Buffer 状态机是整个队列操作的正确性基础：`RELEASED`→`REQUESTED`→`FLUSHED`→`ACQUIRED`→`RELEASED`，`ATTACHED` 可转为 `FLUSHED` 或 `RELEASED`。每个操作都校验当前状态，不合法时返回 `SURFACE_ERROR_BUFFER_STATE_INVALID`。`CancelBuffer` 将 `REQUESTED` 或 `ATTACHED` 状态转回 `RELEASED`。改状态机逻辑时必须覆盖所有合法转换路径，否则容易导致 buffer 泄漏（无法回到 `RELEASED` 状态）或死锁（状态卡住无法流转）。

`ProducerSurface` 维护 `bufferProducerCache_` 本地缓存（`map<int32_t, sptr<SurfaceBuffer>>`），避免重复 IPC 传输 `SurfaceBuffer` 数据。当 IPC 返回空 buffer 时，查本地缓存或调用 `SyncProducerCacheLocked` 同步。`AddCacheLocked` 还会通过 ioctl 设置 `DMA_BUF` type/name 用于内存泄漏追踪。IPC 缓存一致性是跨进程场景下的核心难点：`CleanCache`、`GoBackground`、进程死亡等场景都可能导致 `ProducerSurface` 本地缓存与服务端 `bufferQueueCache_` 不同步。

`BufferQueueProducer` 有 PID 门控连接：`Connect` 记录 `GetCallingPid()`，`Disconnect` 清除。严格断连模式（`ConnectStrictly`/`DisconnectStrictly`）下，`DisconnectStrictly` 后所有 `RequestBuffer`/`FlushBuffer` 返回 `GSERROR_CONSUMER_DISCONNECTED`，且无法恢复，必须重新 `ConnectStrictly`。生产者进程死亡时，`ProducerSurfaceDeathRecipient` 清理连接和缓存。`Connect`/`Disconnect` 时序问题是常见的 bug 来源：生产者未 `Connect` 就调用 `RequestBuffer`，或 `Disconnect` 后仍在使用 buffer，都会导致不可预期的行为。

`BufferQueue` 使用 `mutex_` 保护队列操作，`waitReqCon_` 条件变量在 `RequestBuffer` 阻塞模式下信号通知，`isAllocatingBufferCon_` 防止并发 buffer 分配。在热点路径（`RequestBuffer`/`FlushBuffer`）增加阻塞操作、大内存拷贝或高频日志会直接影响帧率。`AcquireBuffer` 支持 `desiredPresentTimestamp` 帧调度：如果 `desiredPresentTimestamp` > `expectPresentTimestamp`，返回 `GSERROR_NO_BUFFER_READY`；如果更新的 buffer 有更早的时间戳，旧 buffer 会被丢弃。`DropFrameLevel` 控制丢帧策略：dirty list 超过设定级别时 `DropBuffersByLevel` 丢弃最旧 buffer。

HEBC 白名单影响 usage 标记：白名单应用去掉 `BUFFER_USAGE_CPU_READ`，只保留 `BUFFER_USAGE_MEM_DMA`。请求配置变更时，`ReuseBuffer` 可能触发 fence wait（3 秒超时）和 buffer 重新分配。`RequestAndDetachBuffer`/`AttachAndFlushBuffer` 是原子操作组合，用于需要一步完成的场景。`BufferElement` 存储每个 buffer 的完整元数据：buffer、state、config、fence、timestamp、damages、HDR metadata（`hdrMetaDataType`、`metaData`、`key`、`metaDataSet`）、`presentTimestamp`、`desiredPresentTimestamp`、`isAutoTimestamp` 等。`BufferQueue` 全局唯一 ID（`uniqueId_`）用于日志追踪，`BLOG*` 宏会带此 ID。

`ProducerSurface` 支持 `RegisterReleaseListener` 注册 buffer 释放回调（两种模式：普通 `OnReleaseFunc` 和带 sequence+fence 的 `OnReleaseFuncWithSequenceAndFence`）。`ConsumerSurface` 的 `RegisterDeleteBufferListener` 使用 `compare_exchange_strong` 保证每种类型只注册一次（RT 主线程和硬件重绘线程各一）。`PreAllocBuffers` 支持在初始使用前预分配 buffer，避免第一次 `RequestBuffer` 时的分配延迟。`GetLastFlushedBuffer` 返回最近一次 flush 的 buffer 及其 4x4 变换矩阵。`SURFACE_MAX_QUEUE_SIZE`(64) 和 `SURFACE_DEFAULT_QUEUE_SIZE`(3) 的限制变更会影响上层对队列容量的假设。

## 常见风险

只改 `ProducerSurface` 或 `ConsumerSurface` 一侧，遗漏 IPC 代理（`BufferQueueProducer`/`BufferQueueConsumer`）的同步修改。这类问题在本地单元测试中不易发现，因为本地测试不走 IPC 路径，只有系统级跨进程测试才能暴露。改 `BufferQueue` 中的逻辑而忽略 `BufferQueueProducer` 的 IPC 分发和参数校验，同样会导致 IPC 路径行为与本地路径不一致。

改 Buffer 状态机逻辑时，未覆盖所有合法转换路径，导致死锁或 buffer 泄漏。状态机是整个队列正确性的基础，每条边都对应一个具体的操作和校验，漏掉任何一条路径都可能让 buffer 卡在某个中间状态无法回收。特别是 `CancelBuffer`、`AttachBuffer`/`DetachBuffer` 等非常规路径容易被忽略。

改 `BufferQueue` 容量或超时策略时，未考虑生产消费速率不匹配导致的饿死或丢帧。`SURFACE_MAX_QUEUE_SIZE`(64) 和 `SURFACE_DEFAULT_QUEUE_SIZE`(3) 的限制变更会影响上层对队列容量的假设，`waitReqCon_` 超时策略变更会影响 `RequestBuffer` 阻塞行为。

IPC buffer 缓存不一致：`ProducerSurface` 本地缓存与服务端 `bufferQueueCache_` 不同步，尤其是在 `CleanCache`、`GoBackground`、进程死亡等场景。这类问题表现为 buffer 数据错乱或访问已释放的内存，且只在跨进程场景下复现。

`Connect`/`Disconnect` 时序问题：生产者未 `Connect` 就调用 `RequestBuffer`，或 `Disconnect` 后仍在使用 buffer。严格断连模式的影响容易被低估：`DisconnectStrictly` 后无法恢复，必须重新 `ConnectStrictly`，如果在严格断连后仍有残留操作会全部返回 `GSERROR_CONSUMER_DISCONNECTED`。

在 `RequestBuffer`/`FlushBuffer` 热点路径增加阻塞操作、大内存拷贝或高频日志，影响帧率。这些操作在每帧都会执行，即使单次耗时增加几毫秒，在高帧率场景下也会显著影响性能。

`desiredPresentTimestamp` 逻辑变更导致帧调度行为异常（丢帧或延迟显示）。帧调度逻辑依赖时间戳比较和 buffer 新旧排序，逻辑变更可能导致旧帧覆盖新帧或新帧被误判为过早而丢弃。

## Ask before / 必须人工确认

- 修改 `BufferQueue` 容量、状态机、超时策略或 buffer 生命周期时。
- 修改 `Connect`/`Disconnect` 行为或严格断连模式语义时。
- 修改 `desiredPresentTimestamp` 帧调度或 `DropFrameLevel` 丢帧策略时。
- 修改 `ProducerSurface` buffer 缓存策略或 IPC 同步逻辑时。
- 修改 HEBC 白名单或 buffer usage 默认值时。
- 需要真实设备验证但当前没有设备时。

## 验证建议

- 基础构建：`hb build graphic_surface -i`
- 近端单测：`buffer_queue_test`、`buffer_queue_producer_test`、`buffer_queue_consumer_test`、`producer_surface_test`、`consumer_surface_test`、`surface_test`
- 系统测试：`surface_ipc_test_st`、`surface_ipc_with_connect_strictly_test_st`、`surface_batch_opt_with_connect_strictly_test_st`
- Fuzz：`BufferQueueFuzzTest`、`BufferQueueProducerFuzzTest`、`SurfaceConcurrentFuzzTest`
- 涉及 IPC 跨进程行为时需要系统测试和真实设备验证。
- 涉及 `Connect`/`Disconnect` 生命周期时补跑 `surface_ipc_with_connect_strictly_test_st`。

## 待补充背景

### HEBC 白名单配置与 buffer usage 默认值差异

**配置文件路径**：`etc/graphics_game/config/graphics_game.json`（相对路径）。

**JSON 格式**：`{"HEBC":{"AppName":["app1","app2"]}}`，支持数组或单个字符串。

**限制**：
- 最大白名单数量：10000（超出截断）
- 最大应用名称长度：1024（超出跳过）
- 配置文件最大大小：32MB

**HEBC 访问类型**：
- `HEBC_ACCESS_CPU_ACCESS`：HEBC 关闭，保留 `BUFFER_USAGE_CPU_READ`
- `HEBC_ACCESS_HW_ONLY`：HEBC 开启，移除 `BUFFER_USAGE_CPU_READ`，只保留 `BUFFER_USAGE_MEM_DMA`

**调试参数**：`persist.graphic.debug_hebc.disabled`（设为 1 禁用 HEBC，测试用）。

**产品形态差异**：
- 游戏类应用通常在白名单中，使用 HW_ONLY 路径
- 普通应用不在白名单中，使用 CPU_ACCESS 路径
- 不同产品可在 `graphics_game.json` 中配置不同白名单

**代码锚点**：`utils/hebc_white_list/hebc_white_list.cpp:86-121`（JSON 解析）、`surface/src/native_window.cpp:114-117`（usage 处理）、`surface/src/buffer_queue.cpp:256-273`（metadata 设置）。

### 常见线上问题模式（待团队补充案例编号）

**buffer 泄漏模式**：
- 状态机转换路径不完整，buffer 卡在 `REQUESTED` 或 `FLUSHED` 状态无法回 `RELEASED`
- `CancelBuffer` 未覆盖所有异常路径
- 进程死亡时 `ProducerSurfaceDeathRecipient` 未正确清理缓存和状态

**状态机死锁模式**：
- 生产者未 `Connect` 就调用 `RequestBuffer`
- `DisconnectStrictly` 后仍有残留操作
- 并发 buffer 分配时 `isAllocatingBufferCon_` 等待超时

**IPC 缓存不一致模式**：
- `CleanCache`、`GoBackground`、进程死亡后 `bufferProducerCache_` 与 `bufferQueueCache_` 不同步
- 返回空 buffer 时未正确查本地缓存或调用 `SyncProducerCacheLocked`
- 跨进程 buffer 引用计数错误

**典型排查命令**：
- `hilog | grep "BUFFER_STATE"`：检查状态分布
- `hitrace -t GRAPHIC_AGP`：追踪 `RequestBuffer`/`FlushBuffer`/`AcquireBuffer` 路径
- 系统诊断工具：查看队列状态和 buffer 缓存

### desiredPresentTimestamp 帧调度预期行为

**常量**：`ONE_SECOND_TIMESTAMP = 1e9`（纳秒）。

**时间戳处理逻辑**（`surface/src/buffer_queue.cpp:907-925`）：
- `desiredPresentTimestamp <= 0`：自动生成
  - 如果 `desiredPresentTimestamp == 0 && uiTimestamp != 0`，使用 `uiTimestamp`
  - 否则使用当前时间（`std::chrono::steady_clock::now()`），`isAutoTimestamp = true`
- `desiredPresentTimestamp > 0`：使用指定值，`isAutoTimestamp = false`

**AcquireBuffer 帧调度逻辑**（`surface/src/buffer_queue.cpp:977-1044`）：
- `expectPresentTimestamp <= 0`：退化到普通 `AcquireBuffer`，不做帧调度
- buffer 的 `desiredPresentTimestamp > expectPresentTimestamp` 且差值在 1 秒内：返回 `GSERROR_NO_BUFFER_READY`
- dirty list 中有多个 buffer 时，丢弃时间戳更早的旧 buffer，保留时间戳较晚或匹配的新 buffer
- `IsPresentTimestampReady` 最终校验时间戳是否就绪

**不同场景预期行为**（需团队补充典型调优参数）：
- **游戏场景**：通常使用手动时间戳，配合高帧率，期望低延迟
- **视频场景**：时间戳匹配播放帧率，期望按时呈现
- **UI 场景**：常使用自动时间戳，期望响应及时
- **调优参数**：队列容量、`DropFrameLevel`、`expectPresentTimestamp` 计算方式等

**测试锚点**：`surface/test/systemtest/buffer_with_present_timestamp_test/surface_ipc_with_pts_test.cpp`、`surface_ipc_with_invaild_pts_test.cpp`、`surface_ipc_with_dropframe_test.cpp`。

### DropFrameLevel 丢帧策略细节

**有效值**：`level >= 0`（负值返回 `GSERROR_INVALID_ARGUMENTS`）。

**策略含义**：
- `level = 0`：不丢帧，保留所有 dirty buffer
- `level > 0`：保留最新的 N（= level）帧，丢弃其余最旧的 buffer

**实现逻辑**（`surface/src/buffer_queue.cpp:1085-1110`）：
- 当 `dirtyList_.size() > dropFrameLevel_` 时触发
- 从 dirty list 队头丢弃多余 buffer，标记为 `BUFFER_STATE_ACQUIRED`
- 通过 `ReleaseBuffer` 释放，确保 buffer 回到 free list
- 触发 `OnDropBuffer` 回调通知监听者

**典型配置**：
- `level = 1`：只保留最新 1 帧，其余丢弃（激进丢帧）
- `level = 2`：保留最新 2 帧（推荐值）
- `level = 3`：保留最新 3 帧（默认队列大小）
- `level = 5`：保留最新 5 帧（宽松丢帧）

**使用场景**：
- 生产消费速率不匹配时防止 dirty list 爆满
- 游戏场景高帧率下控制延迟
- 视频场景配合时间戳调度

**测试锚点**：`surface/test/unittest/buffer_queue_test.cpp:2722-3017`（`SetDropFrameLevel001-003`、`DropBuffersByLevel001-005`）。

### 压力测试场景和性能基准数据

**系统测试文件**（`surface/test/systemtest/`）：
- `surface_ipc_with_connect_strictly_test.cpp`：IPC + 严格断连
- `surface_batch_opt_with_connect_strictly_test.cpp`：批量操作 + 严格断连
- `surface_ipc_with_pts_test.cpp`：PTS 帧调度正常场景
- `surface_ipc_with_invaild_pts_test.cpp`：PTS 帧调度异常场景（时间戳过期、差值超 1 秒）
- `surface_ipc_with_dropframe_test.cpp`：丢帧策略测试
- `native_window_clean_cache_test.cpp`：缓存清理测试
- `attach_and_detach_buffer_test/attach_and_detach_buffer_with_default_usage_test.cpp`：attach/detach + usage

**典型测试参数**（`surface_ipc_with_pts_test.cpp`）：
- buffer size：256x256（小 buffer）
- format：`GRAPHIC_PIXEL_FMT_RGBA_8888`
- usage：`BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA`
- timeout：0（非阻塞）
- desiredPresentTimestamp：当前时间 + 延迟

**性能基准数据**（待团队补充实际数值）：
- `RequestBuffer` 平均耗时（单 buffer、三 buffer）
- `FlushBuffer` 平均耗时（带 fence、带 metadata）
- `AcquireBuffer` 平均耗时（普通、带 PTS）
- IPC 跨进程往返耗时
- 高帧率场景（60fps、90fps、120fps）下的延迟和丢帧率

**性能监控命令**：
- `hitrace -t GRAPHIC_AGP`：热点路径耗时
- `hisysevent -r GRAPHIC_AGP`：帧率上报
- `FrameReport::GetInstance().SetAcquireBufferSeqWithUniqueId`：游戏帧率记录
