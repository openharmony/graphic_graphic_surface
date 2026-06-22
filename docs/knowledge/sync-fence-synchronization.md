# SyncFence 同步机制知识入口

## 适用范围

涉及 `SyncFence` 创建/等待/合并、`AcquireFence`/`ReleaseFence`、fence fd 生命周期、`SyncFenceTracker` 异步追踪、`AcquireFenceManager`、`FrameSched` 帧调度、GPU subhealth 检测、Native Fence C API 时，先读本文档。

## 快速代码索引

| 方向 | 主要文件 |
|------|----------|
| SyncFence 核心 | `sync_fence/src/sync_fence.cpp`、`interfaces/inner_api/sync_fence/sync_fence.h` |
| AcquireFenceManager | `sync_fence/src/acquire_fence_manager.cpp`、`sync_fence/include/acquire_fence_manager.h` |
| FrameSched 帧调度 | `sync_fence/src/frame_sched.cpp`、`sync_fence/include/frame_sched.h` |
| SyncFenceTracker | `sync_fence/src/sync_fence_tracker.cpp`、`sync_fence/include/sync_fence_tracker.h` |
| Native Fence C API | `sync_fence/src/native_fence.cpp` |
| 错误码 | `interfaces/inner_api/common/graphic_common_c.h` |
| 单测 | `sync_fence/test/unittest/` |
| Fuzz | `sync_fence/test/fuzztest/` |

## 术语触发词

`SyncFence`、`AcquireFence`、`ReleaseFence`、fence、wait、merge、`SYNC_IOC_MERGE`、`SyncFenceTracker`、`AcquireFenceManager`、`FrameSched`、GPU subhealth、`UniqueFd`、`FENCE_PENDING_TIMESTAMP`、`INVALID_FENCE`、`OH_NativeFence`

## 先判断的问题

- 改动涉及 fence 创建/等待/合并语义，还是异步追踪/帧调度策略？
- 是否影响 GPU/Display 硬件同步模型？
- 是否影响 fence fd 跨进程传递（IPC parcel）？
- 是否涉及 `FrameSched` 动态库加载或 QoS 策略？
- 是否涉及 GPU subhealth 检测阈值或上报策略？

## 关键边界

`SyncFence` 基于 Linux sync_file fd，通过 `UniqueFd` RAII 管理生命周期（析构自动关闭 fd）。不可拷贝/不可移动（deleted copy/move constructors 和 assignment operators）。引用计数通过 `RefBase`/`sptr` 管理。这意味着所有 `SyncFence` 实例只能通过 `sptr` 持有和传递，不存在值语义的拷贝场景，但要注意 `sptr` 引用计数降到 0 时 fd 会被立即关闭。

Wait 使用 `poll()` 系统调用，支持超时（毫秒）。`EINTR`/`EAGAIN` 自动重试。超时返回 `-ETIME`。fd 无效（< 0）时直接返回 `-1`。这是最基础的 fence 同步原语，所有上层等待逻辑都委托给这个方法，超时策略的变更会影响整个 fence 等待链路的行为。

`MergeFence` 使用 `SYNC_IOC_MERGE` ioctl 合并两个 fence。两个都有效时真正合并；只有一个有效时与自身合并（创建同一 timeline 的重复 fence）；都无效返回 `INVALID_FENCE`。合并语义的非对称性容易被忽略：当只有一个 fence 有效时，合并结果是一个新的 fd 但指向同一 timeline，而不是直接返回原始 fence。这意味着合并后的 `SyncFence` 与原始 `SyncFence` 是不同的对象，关闭互不影响。

`SyncFileReadTimestamp` 通过 `SYNC_IOC_FILE_INFO` ioctl 读取 fence 时间戳。所有 sync point 已信号时返回最大时间戳；有活跃点时返回 `FENCE_PENDING_TIMESTAMP`（`INT64_MAX`）；fd 无效返回 `INVALID_TIMESTAMP`（-1）。这个方法被广泛用于判断 fence 是否已信号，是 `SyncFenceTracker` 决定是否投递异步任务的关键依据。

`GetFenceInfo` 的 `num_fences` 范围校验 [1, `MAX_FENCE_NUM`(65535)]，溢出检查 buffer size 计算。分配失败或 ioctl 失败返回空 vector。这是读取 fence 详细信息的唯一接口，参数校验不当可能导致内核 ioctl 返回错误或内存分配失败。

`SyncFenceTracker` 使用 `EventRunner`/`EventHandler` 异步等待 fence。GPU fence 追踪（`isGpuFence_=true`，名字为 "Acquire Fence"）会检测 subhealth：等待超过 `GPU_SUBHEALTH_EVENT_THRESHOLD`（12ms）上报 `HiSysEvent`，每日上限 `GPU_SUBHEALTH_EVENT_LIMIT`（200 次）。`CheckGpuSubhealthEventLimit` 按日历日重置计数器。GPU subhealth 检测是通过 fence 等待时延间接推断 GPU 负载状况的机制，阈值选择需要平衡检测灵敏度和误报率。

`AcquireFenceManager` 是静态单例，内部委托给 `SyncFenceTracker`（名字 "Acquire Fence"）。这个名字触发 GPU subhealth 监控和 `FrameSched` 集成。`TrackFence` 是唯一公开方法。单例设计意味着所有 acquire fence 追踪共享同一个 tracker，名字参数的选择直接决定了 subhealth 监控和帧调度是否生效。

`FrameSched` 通过 `dlopen` `libframe_ui_intf.z.so` 动态加载（`RTLD_LAZY`）。所有函数可选——加载失败时为 no-op，只打错误日志。`SendFenceId` 用于 GPU 频率调整；`MonitorGpuStart`/`MonitorGpuEnd` 标记 GPU 工作区间；`SetFrameParam` 设置帧调度参数；`IsScbScene` 判断是否在 SCB 场景。动态加载的 no-op safe 设计意味着上层代码不能假设 `FrameSched` 调用一定会生效，需要考虑两种情况下的行为一致性。

`SyncFenceTrackerManager` 按 `screenId` 管理 `SyncFenceTracker` 实例，使用 `shared_mutex` 双检锁（读锁查找，写锁创建）。`GetSyncFenceTracker(name, screenId)` 返回已有实例或创建新实例。多屏幕场景下每个屏幕有独立的 tracker，双检锁保证并发安全的同时尽量减少锁竞争。

条件编译 `FENCE_SCHED_ENABLE` 时，追踪线程通过 `/proc/thread-self/sched_qos_ctrl` ioctl 设置 `QOS_USER_INTERACTIVE` 调度优先级，最小化 fence wait 延迟。这是针对实时性要求高的场景的优化，编译时决定是否启用，不是运行时开关。

Native Fence C API（`OH_NativeFence_Wait`/`OH_NativeFence_WaitForever`/`OH_NativeFence_IsValid`/`OH_NativeFence_Close`）内部 `dup` fd，不获取调用方 fd 所有权。调用方必须自行关闭原始 fd。`OH_NativeFence_Close` 是便利函数，用于调用方关闭自己的 fd。fd 所有权模型是 Native API 设计的核心约定，违反此约定会导致 fd 泄漏或 double close。

IPC parcel：`WriteToMessageParcel` 先用 `fcntl(F_GETFL)` 校验 fd 有效性（`EBADF` 检查）再写入；`ReadFromMessageParcel` 支持安全 fd 读取函数（`readSafeFdFunc` 参数）。跨进程传递 fence fd 时，写入端会提前校验 fd 有效性，避免将无效 fd 传递到对端进程。

`SyncFenceTracker` 的 `TrackFence` 方法：fence 为 `nullptr` 时直接返回；已信号（`SyncFileReadTimestamp` != `FENCE_PENDING_TIMESTAMP`）时直接增加 `fencesQueued_` 和 `fencesSignaled_` 计数，不投递异步任务；未信号时投递 `Loop` 任务到 `EventHandler`。这种优化避免了对已信号 fence 的不必要的异步等待开销。

`GetFrameRate` 从 `frameStartTimes_` 队列计算帧率：`FRAME_PERIOD`(1000) * (numFrames - 1) / interval。`SyncFence::INVALID_FENCE` 是静态单例（fd = -1），`SyncFence::INVALID_TIMESTAMP` = -1，`SyncFence::FENCE_PENDING_TIMESTAMP` = `INT64_MAX`。`fencesQueued_` 和 `fencesSignaled_` 是 `atomic<uint32_t>`，用于无锁追踪 fence 提交和完成数量。

`WaitFence` 方法：如果 `isGpuFreq_` 为 true 且是 GPU fence，用 `MonitorGpuStart`/`MonitorGpuEnd` 包裹 `Wait` 调用。默认超时 `SYNC_TIME_OUT` = 3000ms。系统属性影响行为：`persist.deadline.gpu_enable` 控制 GPU fence 追踪开关；`persist.deadline.gpu_freq_enable`（默认 true）控制 GPU 频率调整集成。

## 常见风险

fence fd 泄漏：`UniqueFd` 析构关闭 fd，但如果通过 `Get()` 暴露原始 fd 被外部关闭，会导致 double close。`Get()` 方法文档明确说明调用方不能关闭或修改返回的 fd。这是一个文档约定而非代码强制约束，违反时行为未定义，且 double close 在多线程环境下可能导致关闭其它线程刚打开的 fd。

`MergeFence` 合并后未正确管理新 fd 的生命周期：合并创建的新 fd 由新 `SyncFence` 的 `UniqueFd` 管理，但如果调用方在 `MergeFence` 返回前异常退出，新 fd 可能泄漏。合并操作涉及内核 ioctl 和新 fd 分配，中间状态的处理需要保证异常路径下的资源回收。

`SyncFenceTracker` 异步等待时 fence 对象被提前释放（`sptr` 引用计数降为 0）：`TrackFence` 通过 `sptr` 持有 fence 引用，但 `Loop` 任务中也需要确保 fence 不被释放。如果 `TrackFence` 的调用方是 fence 的最后一个持有者，在异步任务执行前释放了所有引用，fence 对象和底层 fd 都会被销毁，导致异步等待失败或访问无效 fd。

GPU subhealth 检测阈值或上报频率不合理，导致误报或漏报：12ms 阈值对高性能设备可能太宽松，对低性能设备可能太严格。200 次/日上限可能在密集 GPU 场景下不够用。阈值的选取需要在设备性能差异和检测灵敏度之间做权衡，不同产品形态可能需要不同的阈值配置。

`FrameSched` 动态库在目标设备不可用，依赖其行为的代码路径失效：所有 `FrameSched` 调用都是 no-op safe 的，但如果上层逻辑假设 `SendFenceId` 一定会生效，可能产生预期外的行为。例如上层可能根据 fence id 的发送来调整渲染策略，当 `FrameSched` 不可用时这些策略不会执行，但上层可能没有处理这种降级场景。

Native Fence API 调用方误解 fd 所有权模型：调用 `OH_NativeFence_Wait` 后认为原始 fd 已被消费，不再需要关闭。实际上 API 内部 `dup` 了 fd，原始 fd 仍需调用方管理。这种误解可能导致 fd 泄漏（调用方不再关闭原始 fd）或 double close（调用方错误地认为 fd 已被 API 关闭又再次关闭）。

fence wait 超时设置不当：3 秒默认超时（`SYNC_TIME_OUT`）在大多数场景足够，但在某些低帧率或复杂合成场景下可能导致频繁超时误判。超时返回 `-ETIME` 后上层如何处理也需要明确：是重试、报错还是忽略，不同策略会导致不同的系统行为。

`SyncFenceTrackerManager` 的 `shared_mutex` 双检锁在极端并发下可能创建多个 tracker 实例（写锁内的二次检查缓解了此问题，但如果 `screenId` 变化频繁仍需注意）。双检锁模式在写锁获取后还有一次检查，正常情况下不会创建重复实例，但如果锁的实现或内存序有问题，理论上仍存在风险。

## Ask before / 必须人工确认

- 修改 fence wait/merge 语义或超时策略时。
- 修改 GPU subhealth 检测阈值或上报策略时。
- 修改 `FrameSched` 帧调度策略或动态库加载逻辑时。
- 修改 `SyncFenceTracker` 异步追踪线程模型时。
- 修改 Native Fence C API 的 fd 所有权语义时。
- 需要真实设备验证但当前没有设备时。

## 验证建议

- 基础构建：`hb build graphic_surface -i`
- 近端单测：`sync_fence_test`（含 `acquire_fence_manager_test`、`frame_sched_test`、`sync_fence_tracker_test`）、`native_fence_test`
- Fuzz：`NativeFenceFuzzTest`
- 涉及 GPU fence 或硬件同步时需要真实设备验证。
- 涉及 `FrameSched` 集成时需要确认目标设备 `libframe_ui_intf.z.so` 可用性。

## 待补充背景

### GPU fence 时延分布与 subhealth 阈值调优

**关键参数**（定义位置 `sync_fence/include/sync_fence_tracker.h:38-39`）：
- `GPU_SUBHEALTH_EVENT_THRESHOLD`：12ms（超过此阈值上报 subhealth）
- `GPU_SUBHEALTH_EVENT_LIMIT`：200 次/日（每日上限）
- `SYNC_TIME_OUT`：3000ms（默认 fence wait 超时）

**时延判断逻辑**：
- `SyncFileReadTimestamp()` 返回 `FENCE_PENDING_TIMESTAMP`（`INT64_MAX`）表示 fence 未 signal
- 已 signal 返回最大时间戳
- fd 无效返回 `INVALID_TIMESTAMP`（-1）

**调优建议**（待团队补充实际数值）：
- **高性能设备**：12ms 阈值可能宽松，建议根据实际 GPU fence 分布调整（如 8-10ms）
- **中端设备**：12ms 阈值适中，保持默认
- **低端设备**：12ms 阈值可能严格，建议放宽到 15-20ms 避免误报
- **游戏场景**：GPU 负载高，fence wait 时延可能频繁超过阈值，需单独统计
- **视频场景**：GPU 负载相对稳定，时延波动较小

**排查命令**：
- `hilog | grep "GPU_SUBHEALTH_MONITORING"`：查看 subhealth 上报
- `hitrace -t GRAPHIC_AGP`：追踪 fence wait 耗时
- `hisysevent -l | grep GPU`：查看 GPU 相关事件

### FrameSched 动态库可用性与行为差异

**动态库路径**：`libframe_ui_intf.z.so`（常量定义位置 `sync_fence/src/frame_sched.cpp:39`，即 `FRAME_AWARE_SO_PATH`）

**加载方式**：`dlopen(..., RTLD_LAZY)`，失败时所有调用为 no-op

**导出函数**（可选，加载失败时为 nullptr）：
- `SendFenceId(uint32_t fenceId)`：发送 fence id 用于 GPU 频率调整
- `MonitorGpuStart(uint32_t fenceId)`：标记 GPU 工作开始
- `MonitorGpuEnd()`：标记 GPU 工作结束
- `IsScbScene()`：判断是否在 SCB 场景
- `SetFrameParam(int requestId, int load, int schedFrameNum, int value)`：设置帧调度参数
- `Init()`：初始化（延迟加载）

**系统属性开关**：
- `persist.deadline.gpu_enable`：GPU fence 追踪开关
- `persist.deadline.gpu_freq_enable`（默认 true）：GPU 频率调整集成开关

**设备可用性差异**（待团队补充）：
- **标准设备**：通常提供 `libframe_ui_intf.z.so`，所有功能可用
- **低配设备**：可能不提供动态库，所有 `FrameSched` 调用降级为 no-op
- **TV 设备**：可能有特殊帧调度策略（SCB 场景）
- **跨平台构建**：`ROSEN_TRACE_DISABLE` 宏可能影响条件编译路径

**降级处理**：
- 所有 `FrameSched` 方法内部检查 `schedSoLoaded_` 和函数指针
- 加载失败或函数缺失时只打印错误日志，不影响主流程
- 上层代码不能假设 `SendFenceId`/`MonitorGpuStart`/`MonitorGpuEnd` 一定会生效

### 常见线上问题模式（待团队补充案例编号）

**fence 超时模式**：
- 3 秒超时（`SYNC_TIME_OUT`）在高负载场景频繁触发，返回 `-ETIME`
- GPU/Display 硬件异常导致 fence 长时间未 signal
- 跨进程传递后 fd 被错误关闭，wait 操作无效 fd 返回 -1

**GPU subhealth 误报模式**：
- 阈值设置不合理，高性能设备正常 fence wait 也被误判为 subhealth
- 每日 200 次上限在密集 GPU 场景下不够用，触发后停止上报
- `CheckGpuSubhealthEventLimit` 按日历日重置，跨日场景可能有计数器边界问题

**帧调度失效模式**：
- `libframe_ui_intf.z.so` 在目标设备不可用，所有 `FrameSched` 调用降级为 no-op
- 动态库加载成功但部分函数缺失（`dlsym` 返回 nullptr），功能不完整
- GPU 频率调整开关（`persist.deadline.gpu_freq_enable`）关闭，`MonitorGpuStart`/`MonitorGpuEnd` 不生效

**排查建议**：
- `dlopen` 失败时日志："dlopen libframe_ui_intf.so failed! error = ..."
- `dlsym` 失败时日志："Get <函数名> symbol failed: ..."
- fence wait 超时时返回 `-ETIME`，上层需检查返回值并处理
