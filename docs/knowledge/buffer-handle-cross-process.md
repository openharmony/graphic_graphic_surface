# BufferHandle 跨进程共享内存知识入口

## 适用范围

涉及 `BufferHandle` 结构、fd 传递、mmap 映射、跨进程共享内存、零拷贝语义、`SurfaceBufferImpl` 生命周期、引用计数、序列化/反序列化、HDI `IDisplayBuffer`、内存回收/恢复时，先读本文档。

## 快速代码索引

| 方向 | 主要文件 |
|------|----------|
| BufferHandle 定义 | `interfaces/inner_api/buffer_handle/buffer_handle.h` |
| BufferHandle 生命周期 | `buffer_handle/src/buffer_handle.cpp` |
| Parcel 序列化 | `interfaces/inner_api/buffer_handle/buffer_handle_parcel.h` |
| 工具函数声明 | `interfaces/inner_api/buffer_handle/buffer_handle_utils.h` |
| SurfaceBufferImpl 实现 | `surface/src/surface_buffer_impl.cpp`、`surface/include/surface_buffer_impl.h` |
| IPC 工具函数 | `surface/src/buffer_utils.cpp`、`surface/include/buffer_utils.h` |
| 错误码 | `interfaces/inner_api/common/graphic_common_c.h` |
| 单测 | `buffer_handle/test/unittest/`、`surface/test/unittest/` |

## 术语触发词

`BufferHandle`、fd、`reserveFds`、`reserveInts`、mmap、零拷贝、共享内存、DMA-BUF、`virAddr`、`phyAddr`、`SurfaceBuffer`、`SurfaceBufferImpl`、引用计数、`sptr`、`AllocateBufferHandle`、`FreeBufferHandle`、`WriteBufferHandle`、`ReadBufferHandle`、`CloneDmaBufferHandle`、`FlushCache`、`InvalidateCache`、HDI、`IDisplayBuffer`、内存回收、内存恢复

## 先判断的问题

- 改动涉及 `BufferHandle` 结构体字段、fd 语义还是序列化/反序列化逻辑？
- 是否影响跨进程 buffer 共享？消费方（graphic_2d、window_manager、multimedia）的调用假设是否需要同步？
- 是否影响 `SurfaceBufferImpl` 的分配/释放/映射/缓存一致性？
- 是否涉及 HDI `IDisplayBuffer` 接口变更或 `DeathRecipient` 处理？
- 是否涉及内存回收（`TryReclaim`/`TryResumeIfNeeded`）行为？

## 关键边界

`BufferHandle` 使用柔性数组成员 `reserve[0]`，前 `reserveFds` 个是 fd，后 `reserveInts` 个是 int。`reserveFds`/`reserveInts` 上限为 `BUFFER_HANDLE_RESERVE_MAX_SIZE`（1024）。分配超出此限制会直接返回 nullptr。这一上限约束了跨进程传输的 reserve 数量，任何试图突破此限制的改动都需要重新评估 IPC 序列化/反序列化的安全边界。

fd 通过 Binder IPC `WriteFileDescriptor`/`ReadFileDescriptor` 传递，内核在接收进程创建新 fd 指向同一物理内存（DMA-BUF）。每个进程必须自行 mmap 获取 `virAddr`；`virAddr` 跨进程无效，错误传递会导致对端访问非法内存。这是零拷贝语义的核心：只有 fd 被传递，物理内存不拷贝，但每个进程的虚拟地址映射是独立的，任何试图跨进程传递 `virAddr` 的代码都是错误的。

`AllocateBufferHandle` 单次 malloc 分配（`sizeof(BufferHandle) + sizeof(int32_t) * (reserveFds + reserveInts)`），`memset_s` 零填充，所有 fd 初始化为 -1。`FreeBufferHandle` 关闭所有 fd（`handle->fd` 和 `reserve[0..reserveFds-1]`）后 free。任何路径遗漏关闭都会导致 fd 泄漏。分配和释放是一对一的，任何拆分分配/释放逻辑的尝试都需要仔细追踪 fd 所有权。

`SurfaceBufferImpl` 通过 HDI `IDisplayBuffer` 分配（`AllocMem`）/释放（`FreeMem`）/映射（`Mmap`）buffer。HDI 服务死亡时，`FreeBufferHandleLocked` 降级为手动 `FreeBufferHandle`。`GetOrResetDisplayBuffer` 懒初始化 `g_displayBuffer` 并注册 `DeathRecipient`；`GetDisplayBuffer` 只返回当前实例不尝试重连（用于 `FreeBufferHandleLocked` 场景避免不当重连）。这意味着在 HDI 服务死亡后的释放路径上，系统不会尝试恢复 HDI 连接，而是直接走本地 fd 关闭和 free，确保 buffer 资源不会因为 HDI 服务不可用而永久泄漏。

`SurfaceBufferImpl` 全局序列号方案：高 16 位 PID + 低 16 位自增计数器（`g_seqBitset` 追踪，允许回收）。全局 bufferId：高 16 位 PID + 低 48 位自增 `g_nextId`。`g_seqNumMutex` 保护这两个全局状态。序列号和 bufferId 的设计确保了跨进程场景下的唯一性：PID 高位保证了不同进程不会冲突，低位自增保证了同一进程内的唯一性。`g_seqBitset` 的回收机制允许序列号复用，避免了长期运行场景下序列号耗尽。

`SurfaceBufferImpl` 支持内存回收：`TryReclaim` 通过 dlopen 的 `libmemmgrclient.z.so` 调用 `reclaimFunc_` 释放 buffer 内存（设 `isReclaimed_=true`）；`TryResumeIfNeeded` 调用 `resumeFunc_` 恢复。内存回收后未调用 `TryResumeIfNeeded` 就访问 `virAddr` 会导致段错误。内存回收是系统级内存压力下的保护机制，但恢复操作是同步的，在高频访问场景下可能引入延迟。任何依赖 `virAddr` 的代码路径都必须在访问前检查 `isReclaimed_` 状态。

metadata 操作通过 HDI `SetMetadata`/`GetMetadata`，`metaDataCache_`（`map<uint32_t, vector<uint8_t>>`）提供本地缓存减少 HDI 调用。key == 0 或 key >= `ATTRKEY_END` 被拒绝。本地缓存与 HDI 侧的状态一致性由写入时更新缓存保证，但跨进程场景下其它进程的 metadata 变更不会自动同步到本地缓存。

`SyncFence` 集成：`SetAndMergeSyncFence` 合并新旧 fence，如果已有有效 fence 则通过 `SyncFence::MergeFence` 合并，否则直接存储。fence 合并保证了生产者和消费者的同步语义：只有当所有 fence 都 signal 后，buffer 才被认为可用。

`buffer_utils.cpp` 提供 IPC 序列化工具：`ReadSurfaceBufferImpl`/`WriteSurfaceBufferImpl`（基础版，只传 handle 和 sequence）、`ReadSurfaceBufferImplWithAllProperties`/`WriteSurfaceBufferImplWithAllProperties`（完整版，含 colorGamut、transform、scalingMode、crop、syncFence 等）、`ReadRequestConfig`/`WriteRequestConfig`、`ReadHDRMetaData`/`WriteHDRMetaData`、`ReadExtDataHandle`/`WriteExtDataHandle` 等。基础版和完整版的区别在于传输的属性范围，使用错误的版本会导致属性丢失或 IPC 负载过大。

`CloneBuffer` 用于调试 dump：多线程并行拷贝（1MB 块），线程数 = `hardware_concurrency`，最少 1。并行拷贝提高了大 buffer dump 的速度，但引入了多线程复杂度，需要确保 buffer 数据在拷贝期间不被修改。

`DumpToFileAsync`：检查 `/data/bq_dump` 或 `/data/storage/el1/base/bq_dump` 标记文件存在时，复制 buffer 数据后异步写入 .raw 文件。异步写入避免阻塞主链路，但标记文件检查和 buffer 数据拷贝仍在主链路上执行，高频 dump 场景下可能影响性能。

`readSafeFdFunc` 参数允许安全加固 fd 读取，防止 IPC 反序列化中的 fd 泄漏或未授权访问。`WriteFileDescriptor` 先用 `fcntl(F_GETFL)` 校验 fd 有效性再写入。这一安全机制确保了恶意构造的 IPC 消息不会通过无效 fd 注入攻击，但也意味着写入端必须确保 fd 在写入时是有效的。

`SurfaceBufferImpl` 支持 `RegisterBufferDestructorCallBack` 注册析构回调，传入 `bufferId_`，只允许注册一个。析构回调用于上层感知 buffer 释放时机，但单一回调的限制意味着如果有多个模块需要感知释放，需要自行设计委托机制。

Alloc 支持 `previousBuffer` 参数复用 buffer：有 `previousBuffer` 时调用 `ReAllocMem`，否则调用 `AllocMem`。成功后调用 `RegisterBuffer`。支持 HEBC debug override（`persist.graphic.debug_hebc.disabled`）。buffer 复用减少了分配/释放开销，但 `ReAllocMem` 的语义是"如果可能就复用，否则重新分配"，上层不能假设复用一定成功。

`FlushCache`/`InvalidateCache` 通过 HDI 做缓存一致性操作。`BUFFER_USAGE_PROTECTED` 的 buffer 跳过 Map 映射。缓存一致性是零拷贝语义正确性的关键：生产者写入后必须 `FlushCache` 刷出 CPU 缓存，消费者读取前必须 `InvalidateCache` 使缓存行失效，否则可能读到陈旧数据。Protected buffer 由于安全要求不映射到 CPU 地址空间，只能通过 GPU/DMA 硬件访问。

## 常见风险

跨进程传输后 fd 无效或重复关闭：`ReadBufferHandle` 返回的 fd 所有权归调用方，必须由调用方或最终由 `SurfaceBufferImpl` 析构关闭。如果多个路径都试图关闭同一个 fd，会导致 double close。double close 在 Linux 下的危害远超预期：关闭后的 fd 号可能被内核快速分配给其它资源（如新打开的文件或 socket），再次关闭会误关不相关的资源，导致难以追踪的文件描述符泄漏或数据损坏。

`BufferHandle` 结构变更时未同步所有序列化/反序列化路径，导致跨进程兼容性问题。特别注意 reserve 布局变更会影响 `AllocateBufferHandle` 的大小计算。这类兼容性问题在本地测试中无法暴露，因为本地测试通常同一进程内传递 `BufferHandle` 不走序列化/反序列化路径，只有跨进程系统测试才能发现。而且如果新旧版本混部署，可能导致序列化格式不兼容，引发数据损坏或 crash。

`virAddr` 是进程本地映射，跨进程无效；错误传递 `virAddr` 导致对端访问非法内存。这是零拷贝架构下最常见的错误模式：开发者习惯性地将指针作为参数传递，但在跨进程场景下 `virAddr` 只在本进程有效。对端进程必须通过自己的 mmap 获取虚拟地址，任何直接使用对端传来的 `virAddr` 的代码都会导致段错误或更隐蔽的内存损坏。

`SurfaceBufferImpl` 析构时 HDI 服务已死亡，`FreeBufferHandleLocked` 降级路径遗漏关闭 reserve fds。降级路径使用 `FreeBufferHandle` 手动关闭 fd 并 free，但如果有新 fd 在 HDI 死亡后被添加到 reserve 中，这些 fd 可能不会被关闭。降级路径是异常恢复逻辑，测试覆盖通常不足，且在 HDI 服务死亡后很难构造完整的测试场景，因此 fd 泄漏风险容易被忽视。

内存回收后未调用 `TryResumeIfNeeded` 就访问 `virAddr`，导致段错误。`TryReclaim` 设置 `isReclaimed_=true` 后，后续访问需要先检查并恢复。这类问题在内存压力测试中容易出现，但在常规测试中不易复现，因为内存回收只在系统内存紧张时触发。任何直接解引用 `virAddr` 而不检查 `isReclaimed_` 状态的代码路径都是潜在风险。

`g_seqBitset` 和 `g_nextId` 的全局状态在多线程环境下的竞争：`g_seqNumMutex` 保护，但如果有代码路径绕过锁直接访问，可能导致序列号冲突。序列号冲突的后果是两个不同的 `SurfaceBuffer` 拥有相同的序列号，这会导致 IPC 缓存查找错误，表现为生产者拿到错误的 buffer 数据。

parcel 操作中任何一步失败都必须释放已分配的资源（部分读取的 `BufferHandle`），否则导致内存泄漏。`ReadBufferHandle` 在失败时调用 `FreeBufferHandle` 清理。但如果有自定义的扩展序列化逻辑绕过了标准清理路径，或者在清理前抛出了异常，已分配的 `BufferHandle` 就会泄漏。部分读取的 `BufferHandle` 特别危险：fd 已经被读取但尚未全部处理，如果不清理，这些 fd 会一直打开。

`CloneDmaBufferHandle` 通过 HDI 克隆 DMA buffer 句柄用于跨进程共享，每个进程获得自己的 fd 引用同一物理内存。如果 HDI 克隆失败，需要 fallback 策略。克隆失败意味着无法建立新的 DMA-BUF 引用，此时如果 fallback 策略不当（如直接传递原始 fd 而不克隆），可能导致原始 fd 的所有权混乱，进而引发 fd 泄漏或 double close。

`buffer_handle_utils.h` 的公开声明参数顺序是 `AllocateBufferHandle(reserveInts, reserveFds)`，但当前实现按 `AllocateBufferHandle(reserveFds, reserveInts)` 解释参数。由于两个参数同为 `uint32_t`，编译器无法发现交换问题；新增调用点如果只按头文件注释传参，会把 reserve fd 数和 reserve int 数反过来，进而破坏 fd 初始化、parcel 读写和释放路径。新增或修改调用前必须对照 `buffer_handle/src/buffer_handle.cpp` 和既有调用方式确认真实语义。

## Ask before / 必须人工确认

- 修改 `BufferHandle` 结构体字段、fd 语义或 reserve 布局时。
- 修改跨进程序列化/反序列化格式时。
- 修改 `SurfaceBufferImpl` 分配/释放/映射路径时。
- 修改 HDI `IDisplayBuffer` 接口调用或 `DeathRecipient` 处理时。
- 修改内存回收/恢复行为时。
- 修改全局序列号方案时。
- 需要真实设备验证但当前没有设备时。

## 验证建议

- 基础构建：`hb build graphic_surface -i`
- 近端单测：`BufferHandleTest`、`surface_buffer_impl_test`、`buffer_utils_test`
- 系统测试：`surface_ipc_test_st`
- Fuzz：`BufferUtilsFuzzTest`、`SurfaceBufferFuzzTest`
- 涉及跨进程传输时需要系统测试验证。
- 涉及 HDI 或 DMA-BUF 行为时需要真实设备验证。

## 待补充背景

### HDI IDisplayBuffer 能力差异与 fallback 策略

**HDI 接口依赖**（`surface/src/surface_buffer_impl.cpp`）：
- `AllocMem`：分配 buffer
- `ReAllocMem`：复用 buffer（需 previousBuffer 参数）
- `FreeMem`：释放 buffer
- `Mmap`：映射 buffer 到进程地址空间
- `RegisterBuffer`：注册 buffer（可选，返回 `GRAPHIC_DISPLAY_NOT_SUPPORT` 时忽略）
- `SetMetadata`/`GetMetadata`：metadata 操作
- `FlushCache`/`InvalidateCache`：缓存一致性

**HDI 服务死亡处理**：
- `GetOrResetDisplayBuffer`：懒初始化 `g_displayBuffer`，注册 `DeathRecipient`
- `GetDisplayBuffer`：只返回当前实例不尝试重连（用于 `FreeBufferHandleLocked` 场景）
- `FreeBufferHandleLocked`：HDI 不可用时降级为手动 `FreeBufferHandle`

**能力差异（待团队补充）**：
- **标准设备**：支持完整 `AllocMem`/`FreeMem`/`Mmap`/`RegisterBuffer`/`SetMetadata`/`FlushCache`
- **低配设备**：可能不支持 `RegisterBuffer`（返回 `GRAPHIC_DISPLAY_NOT_SUPPORT`）
- **虚拟设备**：可能不支持 DMA-BUF，需要 fallback 到普通内存分配
- **跨平台构建**：`ROSEN_TRACE_DISABLE` 可能影响 HDI 路径

**fallback 策略**：
- `RegisterBuffer` 失败且返回 `GRAPHIC_DISPLAY_NOT_SUPPORT` 时忽略，不报错
- HDI 服务死亡时，`FreeBufferHandleLocked` 降级为手动关闭 fd + free
- 缓存一致性操作失败时，上层需要处理数据同步问题

**调试参数**：
- `persist.graphic.debug_hebc.disabled`（设为 1）：禁用 HEBC，强制 `BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA`

### 内存回收/恢复触发场景与时延

**内存管理动态库**：`libmemmgrclient.z.so`（`surface/src/surface_buffer_impl.cpp`）

**回收/恢复接口**：
- `TryReclaim`：调用 `reclaimFunc_` 释放 buffer 内存，设 `isReclaimed_=true`
- `TryResumeIfNeeded`：调用 `resumeFunc_` 恢复 buffer 内存

**触发场景（待团队补充）**：
- **系统内存紧张**：内存管理服务通知 graphic_surface 回收 buffer
- **后台应用**：应用进入后台后 buffer 可能被回收
- **长期未使用**：buffer 长时间未访问可能被回收
- **多 buffer 场景**：多个 buffer 共存时，优先回收未使用的

**恢复时延（待团队补充）**：
- 同步恢复调用，在高频访问场景可能引入延迟
- 典型恢复时延范围（取决于 buffer 大小和硬件）
- 恢复失败时的处理策略（应用层是否需要重试）

**安全检查**：
- 任何直接解引用 `virAddr` 的代码路径必须在访问前检查 `isReclaimed_` 状态
- 如果 `isReclaimed_=true`，必须先调用 `TryResumeIfNeeded` 恢复内存
- Protected buffer（`BUFFER_USAGE_PROTECTED`）跳过 mmap，不受内存回收影响

### BufferHandle 跨进程典型问题（待团队补充案例编号）

**fd 泄漏模式**：
- `ReadBufferHandle` 返回的 fd 所有权归调用方，必须由调用方或 `SurfaceBufferImpl` 析构关闭
- 多个路径都试图关闭同一个 fd，导致 double close
- double close 后 fd 号可能被内核分配给其它资源，再次关闭误关不相关资源
- `reserve[0..reserveFds-1]` 在 HDI 死亡后的降级路径可能遗漏关闭

**mmap 失败模式**：
- `virAddr` 跨进程无效，对端必须自行 mmap
- 错误传递 `virAddr` 导致对端访问非法内存，段错误或内存损坏
- Protected buffer 跳过 mmap，只能通过 GPU/DMA 硬件访问
- mmap 失败时返回 `MAP_FAILED` 或 nullptr，上层必须检查返回值

**HDI 服务死亡模式**：
- `AllocMem` 失败返回 `GRAPHIC_DISPLAY_FAILURE` 或其它错误码
- `FreeMem` 失败时降级为手动 `FreeBufferHandle`
- `DeathRecipient` 触发后，后续 HDI 调用不可用，需要重新初始化
- 跨进程传递后，对端进程 HDI 服务可能先死亡，导致 buffer 无法正常释放

**序列化兼容性模式**：
- `BufferHandle` 结构变更时未同步所有序列化/反序列化路径
- reserve 布局变更影响 `AllocateBufferHandle` 大小计算
- 新旧版本混部署时，序列化格式不兼容导致数据损坏或 crash
- `buffer_handle_utils.h` 参数顺序（`reserveInts, reserveFds`）与实现不一致

**排查建议**：
- 使用 `lsof` 检查进程 fd 数量，发现 fd 泄漏
- 使用 `pmap` 检查进程 mmap 区域，发现内存映射问题
- `hilog | grep "BufferHandle"`：追踪分配/释放路径
- `hitrace -t GRAPHIC_AGP`：追踪 HDI 调用路径
