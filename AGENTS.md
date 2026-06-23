# 图形 Surface 框架指引

## 项目定位

本仓库对应 OpenHarmony `foundation/graphic/graphic_surface`。优先按这些目录定位问题：

- `surface/src/`、`surface/include/`：BufferQueue、ProducerSurface、ConsumerSurface、BufferQueueProducer、BufferQueueConsumer、SurfaceBufferImpl、metadata_helper、native_window、native_buffer。
- `buffer_handle/src/`：BufferHandle 生命周期和跨进程序列化。
- `sync_fence/src/`、`sync_fence/include/`：SyncFence、AcquireFenceManager、FrameSched、SyncFenceTracker。
- `interfaces/inner_api/surface/`：公开 C API 头文件（NativeWindow、NativeBuffer、Surface 类型）。
- `interfaces/inner_api/buffer_handle/`：BufferHandle 公开头文件。
- `interfaces/inner_api/sync_fence/`：SyncFence 公开头文件。
- `interfaces/inner_api/common/`：错误码、通用工具。
- `utils/`：frame_report、hebc_white_list、trace、rs_frame_report_ext。
- `surface/test/`、`buffer_handle/test/`、`sync_fence/test/`、`utils/**/test/`：单测、fuzz、系统测试。

## 典型工作流

1. 先判断改动场景，读取下文知识路由和对应 `docs/knowledge/` 文档；一个任务跨多个场景时，按影响面同时读取多个入口。详细索引见 [`docs/knowledge/INDEX.md`](docs/knowledge/INDEX.md)。
2. 定位公开接口和内部实现边界：先看 `interfaces/inner_api/`，再看 `surface/src/`、`buffer_handle/src/`、`sync_fence/src/`；内部声明同步看 `surface/include/` 和 `sync_fence/include/`。
3. 改动涉及公开 API、色彩空间映射、HDR metadata、BufferHandle 结构、BufferQueue 状态机、SyncFence 语义或跨进程传输时，先确认生命周期、安全边界、错误码映射和 API 兼容性。
4. 小步修改，就近复用项目已有宏、错误码、日志和测试资源。
5. 按下文验证矩阵和知识文档中的验证重点跑最近目标；涉及真实硬件、Surface、DMA 或显示效果时，补充真实设备上的验证结果。
6. 最终回复要写明读取过的知识文档、完成的验证、未覆盖的 XTS/真实设备缺口，以及是否已提交或 push。
7. 提交和 push 前按下文"提交和推送"要求完成检查。

## 依赖和接口边界

本仓对外依赖在 `bundle.json` 中声明，常见跨子系统边界包括：

- 图形和显示：`graphic_2d`、`drivers_interface_display`（HDI IDisplayBuffer）、`window_window_manager`。
- 媒体：`multimedia_image_framework`（图片解码/编码的 Surface 消费）。
- 运行时和 IPC：`ipc`、`samgr`、`eventhandler`、`c_utils`。
- 安全和系统：`access_token`、`selinux_adapter`、`init`、`bounds_checking_function`。
- 诊断和追踪：`hilog`、`hitrace`、`hisysevent`、`hicollie`。

改动触达上述依赖的接口、枚举、buffer 语义、错误码或能力查询时，不要只在本仓内闭环；需要检查依赖方公开头文件、运行时能力和调用方假设，并在提交说明中写明跨仓影响和验证方式。

## 构建和验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。产品、out 目录和可用 target 以当前工程配置为准。

```sh
hb build graphic_surface -i
hb build graphic_surface -i --skip-download --build-target <target>
```

按改动范围选择最近的构建或测试目标，具体目标优先参考知识文档和对应 `BUILD.gn`。涉及对外接口或 API 行为时，还需要验证对应 XTS 用例。涉及 Surface、DMA、HDI、色彩空间或 HDR 显示效果时，需要补充真实设备上的验证结果。当前没有真实设备时，不要声称已完成完整验证；应记录缺失原因、已完成的本地验证和仍需补充的真实设备验证项，并等待人工确认是否继续 push。

任务级验证参考：

| 改动类型 | 近端验证 | 额外要求 |
| --- | --- | --- |
| 文档、知识路由、注释 | 检查链接、路径、术语和代码锚点是否存在 | 不改行为时通常不需要构建；若文档影响工作流，说明未跑构建原因 |
| C++ 内部实现（BufferQueue/Surface/SyncFence） | `graphic_surface` 构建 + 对应模块最近单测 | 关注错误码、日志、资源释放和异常路径 |
| 公开 API（OH_NativeWindow/OH_NativeBuffer） | 对应单测 + XTS | 必须验证或说明对应 XTS；检查错误码和默认值兼容 |
| 色彩空间、HDR metadata、BufferHandle 跨进程 | 对应单测 + fuzz | 补跑相关 fuzz，覆盖截断、畸形、溢出和异常 metadata |
| Surface、DMA、HDI、显示效果 | 最近单测 + 相关 fuzz | 需要真实设备验证；没有设备时记录缺口并等待人工确认 |

常用命令模板：

```sh
hb build graphic_surface -i
hb build graphic_surface -i --skip-download --build-target <target>
```

单测 target 以本仓 `BUILD.gn` 中实际存在的 `ohos_unittest("<target>")` 为准；fuzz target 以各模块 `**/test/fuzztest/**/BUILD.gn` 中实际存在的 `ohos_fuzztest("<target>")` 为准。不要凭模块名臆造 target。

XTS 用例不在本仓完整维护。涉及公开 API、错误码、默认值或兼容性时，必须查 OpenHarmony XTS 仓、CI 配置或团队用例映射；查不到时，在最终回复中明确写"XTS 目标未确认"，并列出已跑的本仓单测/fuzz 和需要人工补充确认的 API 场景。

真实设备验证记录至少包含：产品名、设备形态、系统版本或镜像标识、输入样例、触发 API/命令、关键 buffer 参数或格式参数、期望结果、实际结果。没有真实设备时，不要继续自行 push 涉及设备行为的改动，除非用户或模块责任人明确确认可以先合入。

## 提交和推送

所有提交在 push 之前，必须完成 `stability-code-review` 检视，并确认没有遗留问题。`stability-code-review` 是团队外部检查工具/skill，不是 Agent 内置能力；当前环境不可用时，优先使用团队提供的已安装命令或 skill。确需临时安装时，不要在本仓直接生成 `node_modules` 或 lockfile，使用仓外临时目录：

```sh
tmpdir=$(mktemp -d)
npm --prefix "$tmpdir" i @ohos-graphics/stability-code-review
```

安装后按工具说明从该临时目录执行检视。如果安装或执行失败，不要继续 push；需要在回复或 PR 说明中记录失败原因，并等待人工确认。以下为 Agent 提交约定，可能与历史人工提交风格不同；人工提交按团队现有规范执行。提交建议使用 `git commit -s` 自动生成 `Signed-off-by`，其姓名和邮箱来自 `git config user.name` 与 `git config user.email`。同时在 commit message 末尾额外空一行写入 `Co-Authored-By: Agent`：

```text
<type>(<scope>): <summary>

<body，可选>

Signed-off-by: <name> <email>

Co-Authored-By: Agent
```

没有明确项目要求时，`type` 优先使用 `fix`、`feat`、`refactor`、`test`、`docs`、`build`，`scope` 使用模块名或目录名。若关联 issue、缺陷单或需求单，在 body 中写清编号和影响范围。

## 知识路由

稳定背景知识采用三层结构：**AGENTS.md（入口）→ `docs/knowledge/INDEX.md`（索引目录）→ 具体知识文件**。

改动前按场景或关键词定位知识文件，详细索引和摘要见 [`docs/knowledge/INDEX.md`](docs/knowledge/INDEX.md)。

快速参考：

| 场景分类 | 知识文件 | 核心主题 |
| --- | --- | --- |
| BufferQueue 与生产消费 | `docs/knowledge/buffer-queue-producer-consumer.md` | 双队列、状态机、IPC 代理、线程安全 |
| BufferHandle 与跨进程 | `docs/knowledge/buffer-handle-cross-process.md` | fd 传递、零拷贝、生命周期、HDI |
| SyncFence 同步 | `docs/knowledge/sync-fence-synchronization.md` | fence 语义、帧调度、GPU subhealth |
| 色彩空间与 HDR | `docs/knowledge/metadata-colorspace-hdr.md` | ColorSpace 映射、HDR metadata |
| Native API | `docs/knowledge/native-api-surface.md` | C API、错误码、默认配置 |
| 诊断与工具 | `docs/knowledge/utils-frame-report-hebc-trace.md` | FrameReport、HEBC、trace |

常用术语快速定位：

| 触发词示例 | 知识文件 |
| --- | --- |
| `BufferQueue`、`RequestBuffer`、`FlushBuffer`、`AcquireBuffer`、`ReleaseBuffer`、`BUFFER_STATE_*` | `buffer-queue-producer-consumer.md` |
| `BufferHandle`、fd、mmap、零拷贝、共享内存、`SurfaceBuffer` 引用 | `buffer-handle-cross-process.md` |
| `SyncFence`、`AcquireFence`、`ReleaseFence`、merge、wait、`FrameSched` | `sync-fence-synchronization.md` |
| `ColorSpace`、HDR、metadata、`DATASPACE`、`OH_HDR_*` | `metadata-colorspace-hdr.md` |
| `OH_NativeWindow`、`OH_NativeBuffer`、usage、format | `native-api-surface.md` |
| `FrameReport`、`HEBC`、`surface_trace`、`rs_frame_report_ext` | `utils-frame-report-hebc-trace.md` |

完整术语索引、场景分类、知识文件摘要和使用建议见 [`docs/knowledge/INDEX.md`](docs/knowledge/INDEX.md)。

## 项目约束

不要做：

- 不要在 BufferQueue 热点路径（RequestBuffer/FlushBuffer/AcquireBuffer/ReleaseBuffer）中增加全量扫描、重复大内存拷贝或高频 INFO 日志。
- 不要只改 BufferQueueProducer 或 BufferQueueConsumer 一侧来改变 buffer 行为；生产消费模型变更必须同步检查两侧和 IPC 代理。
- 不要把 BufferHandle 当普通结构体分析；它的 fd 数组、reserve 数据和跨进程语义会影响内存安全和零拷贝正确性。
- 不要在 NATIVE_COLORSPACE_TO_HDI_MAP 新增映射时忽略多键同值问题；新增 ColorSpace 枚举必须确认正向和反向查找都是确定性的。
- 不要只改 NativeWindow 或 NativeBuffer 的 C API 来改变公开行为；色彩空间、metadata、format、usage 等会影响上层调用方。
- 不要把缺失真实设备验证的 Surface、DMA、HDI、色彩空间或 HDR 改动描述为完整验证。
- 不要执行破坏性 git/文件操作或大范围机械重构，除非用户明确要求。

Ask before / 必须人工确认：

以下场景不是普通"建议确认"，而是 Agent 继续修改、提交或 push 前的门禁。触发后要向用户或模块责任人说明影响面、已读文档、计划改动和拟验证项，得到明确答复后再继续。

- 改公开 API/ABI、枚举值、结构体字段、错误码或默认值前，先确认兼容策略。
- 改 ColorSpace 映射、HDR metadata 格式或 metadata key 时，先确认 NATIVE_COLORSPACE_TO_HDI_MAP 一致性和上游 HDI 依赖。
- 改 BufferQueue 容量、状态机、超时策略或 buffer 生命周期时，先确认生产消费两侧和 IPC 代理是否同步。
- 改 BufferHandle 结构、fd 语义、reserve 数据或跨进程传输时，先确认所有消费方（graphic_2d、window_manager、multimedia）的调用假设。
- 改 SyncFence merge/wait 语义或 FrameSched 帧率策略时，先确认 GPU/Display 硬件同步和上游 RS 依赖。
- 改 Surface usage、format、分配策略或 HEBC 白名单时，先确认 gralloc 能力和产品形态差异。
- 需要真实设备验证但当前没有设备时，先确认是否允许只带本地验证结果继续。
- 改上述行为时，要同步检查错误码和 API 映射，包括 `interfaces/inner_api/common/graphic_common_c.h`、`interfaces/inner_api/surface/surface_type.h`、`interfaces/inner_api/surface/buffer_common.h` 以及对应公开 API 适配代码。

C++ 改动优先复用附近的 `BLOG*`、`GSERROR_*`、`SURFACE_ERROR_*` 等项目宏、错误码和日志习惯。

## 完成定义

Agent 最终回复必须包含：

- 读取过的知识文档和对应场景。
- 修改的文件、行为影响面和明确未修改的关键文件。
- 已执行的构建、单测、fuzz 或真实设备验证命令；未执行时说明原因。
- 真实设备验证无法确认时，列出缺口和需要人工确认的问题。
- 若涉及提交或 push，说明 `stability-code-review` 结果、commit message 是否包含 `Signed-off-by` 和 `Co-Authored-By: Agent`。
