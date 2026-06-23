# Utils：FrameReport、HEBC、Trace 与 RS FrameReportExt 知识入口

## 适用范围

涉及帧率/帧耗时上报、HEBC 白名单、Surface trace 宏、RS frame report 扩展、游戏/应用框架类型诊断、诊断开销或白名单配置解析时，先读本文档。

## 快速代码索引

| 方向 | 主要文件 |
|------|----------|
| FrameReport | `utils/frame_report/src/frame_report.cpp`、`utils/frame_report/export/frame_report.h` |
| HEBC 白名单 | `utils/hebc_white_list/hebc_white_list.cpp`、`utils/hebc_white_list/hebc_white_list.h` |
| Surface Trace | `utils/trace/surface_trace.h` |
| RS FrameReportExt | `utils/rs_frame_report_ext/src/rs_frame_report_ext.cpp`、`utils/rs_frame_report_ext/include/rs_frame_report_ext.h` |
| 使用方 | `surface/src/buffer_queue.cpp`、`surface/src/native_window.cpp`、`surface/src/buffer_client_producer.cpp`、`surface/src/buffer_queue_producer.cpp` |

## 术语触发词

`FrameReport`、`FrameReportExt`、`rs_frame_report_ext`、`HEBC`、`HebcWhiteList`、`surface_trace`、`SURFACE_TRACE_*`、`HITRACE_TAG_GRAPHIC_AGP`、`frame_report_test`、`hebc_white_list_test`、游戏帧率、应用框架类型、白名单配置、trace 开销

## 先判断的问题

- 改动是诊断上报、配置解析、trace 宏，还是 Surface 热点路径里的调用点？
- 是否会改变 HEBC 白名单命中后的 usage 默认值或 buffer 分配策略？
- 是否会在 `RequestBuffer`、`FlushBuffer`、`AcquireBuffer` 等热点路径增加字符串拼接、文件读取、JSON 解析或高频上报？
- 是否依赖目标设备上的 hitrace、hisysevent、FrameReport 或 RS 扩展能力？

## 关键边界

`utils/trace/surface_trace.h` 只提供宏封装，没有独立 `BUILD.gn` 和单测。修改 trace 宏时必须检查实际使用方，尤其是 `buffer_queue.cpp`、`producer_surface.cpp`、`native_window.cpp`、`surface_buffer_impl.cpp` 等热点路径，避免把原本可控的 trace 变成常驻开销。

HEBC 白名单由 `HebcWhiteList` 解析配置并影响 Surface/NativeWindow 的 usage 选择。白名单命中时会移除 `BUFFER_USAGE_CPU_READ`，保留 `BUFFER_USAGE_MEM_DMA`，这会影响 CPU 访问能力、DMA 路径和产品形态差异。修改白名单路径、JSON 格式或命中逻辑时，要同步检查 `surface/src/native_window.cpp`、`surface/src/buffer_queue.cpp` 和 `surface/src/buffer_client_producer.cpp`。

FrameReport 和 RS FrameReportExt 属于诊断/性能辅助能力。修改上报字段、触发频率或应用框架类型识别时，不要只看工具实现，还要检查调用方是否在每帧路径触发。日志、trace 和上报要保持低开销，避免在高帧率场景中引入额外卡顿。

## 常见风险

在热点路径中加入配置文件读取、JSON 解析、同步 IPC、重复字符串格式化或高频 INFO 日志，导致帧率下降。诊断能力本身用于观测性能，不能成为新的性能问题。

修改 HEBC 白名单命中逻辑后，只跑白名单单测但不检查 Surface usage 结果，可能导致 buffer CPU 访问能力变化而未被发现。HEBC 行为需要结合 Surface/NativeWindow 调用方一起验证。

修改 trace 宏或 tag 后未验证实际消费者，可能导致 trace 缺失、tag 错误或商业 trace 能力失效。宏本身没有独立单测，必须通过使用方构建和目标设备 trace 结果确认。

## Ask before / 必须人工确认

- 修改 HEBC 白名单配置格式、默认路径、命中策略或 usage 影响时。
- 修改 FrameReport/RS FrameReportExt 上报字段、事件名、频率或开关策略时。
- 修改 `SURFACE_TRACE_*` 宏语义、trace tag 或热点路径 trace 行为时。
- 需要真实设备验证但当前没有设备时。

## 验证建议

- 基础构建：`hb build graphic_surface -i`
- FrameReport 单测：`frame_report_test`
- FrameReport fuzz：`FrameReportFuzzTest`
- HEBC 单测：`hebc_white_list_test`
- RS FrameReportExt 单测：`rs_frame_report_ext_test`
- 修改 trace 宏时，补跑触达的 Surface/NativeWindow 最近单测；需要确认 trace 输出时，必须在真实设备或可采集 hitrace 的环境验证。
- 修改 HEBC usage 行为时，补跑相关 Surface/NativeWindow 单测，并记录产品形态和真实设备验证缺口。

## 待补充背景

### FrameReport/RS FrameReportExt 验证命令与事件字段

**FrameReport 动态库**：`libgame_acc_sched_client.z.so`（常量定义位置 `utils/frame_report/src/frame_report.cpp:39`）

**导出函数**：
- `GAS_NotifyFrameInfo(pid, layerName, timeStamp, bufferMsg, uniqueId)`：上报帧信息到游戏调度服务

**上报字段**（`utils/frame_report/export/frame_report.h`）：
- `activelyPid_`：活跃游戏 PID
- `activelyUniqueId_`：活跃游戏 Surface uniqueId
- `pendingBufferNum_`：待处理 buffer 数量
- `lastSwapBufferTime_`：最近 swap buffer 时间
- `dequeueBufferTime_`：dequeue buffer 时间
- `queueBufferTime_`：queue buffer 时间
- `flushBufferSysTime_`/`flushBufferSequence_`：flush buffer 时间戳和序列号
- `acquireBufferSysTime_`/`acquireBufferSequence_`：acquire buffer 时间戳和序列号
- `presentFenceSysTime_`/`presentFenceSequence_`：present fence 时间戳和序列号
- `lastReleaseSysTime_`：最近 release 时间

**验证命令（待团队补充实际事件名）**：
- `hilog | grep "FrameReport"`：查看 FrameReport 日志
- `hilog | grep "GAS_NotifyFrameInfo"`：查看帧上报日志
- `hisysevent -l | grep GAME`：查看游戏相关事件（事件名待确认）
- `hitrace -t GRAPHIC_AGP`：追踪帧上报路径（`SetAcquireBufferSeqWithUniqueId` 等）

**RS FrameReportExt 动态库**：`libframe_ui_intf.z.so`（常量定义位置 `utils/rs_frame_report_ext/src/rs_frame_report_ext.cpp:25`）

**导出函数**：
- `GetSenseSchedEnable()`：获取帧调度开关状态
- `Init()`：初始化
- `HandleSwapBuffer()`：处理 swap buffer 事件

**验证命令**：
- `hilog | grep "RsFrameReportExt"`：查看 RS FrameReportExt 日志
- `hitrace -t GRAPHIC_AGP`：追踪 `HandleSwapBuffer` 调用

**调试开关**（待确认系统属性）：
- 帧调度开关可能由系统属性控制（具体属性名待团队补充）
- 游戏场景上报频率可能可配置

### HEBC 白名单配置与产品差异

**配置路径**：`etc/graphics_game/config/graphics_game.json`（相对路径）

**JSON 格式**：
```json
{"HEBC":{"AppName":["app1","app2","..."]}}
```

**限制参数**：
- 最大白名单数量：10000（超出截断）
- 最大应用名称长度：1024（超出跳过）
- 配置文件最大大小：32MB

**产品差异（待团队补充）**：
- **游戏手机**：白名单通常包含主流游戏应用，HEBC 开启，usage 为 `BUFFER_USAGE_MEM_DMA`
- **普通手机**：白名单可能包含少量高性能应用，大部分应用 HEBC 关闭
- **TV 设备**：白名单可能包含视频播放应用
- **平板设备**：白名单介于手机和 TV 之间
- **不同厂商产品**：白名单配置差异较大，需根据实际产品确认

**默认配置示例（待团队补充）**：
- 某产品默认白名单内容
- 某产品无白名单配置时的默认行为
- HEBC 白名单配置更新方式（动态更新/重启生效）

**验证方式**：
- `hebc_white_list_test`：单测验证 JSON 解析
- `native_window_test`：验证 usage 处理
- 真实设备：检查 `/proc/self/cmdline` 中的应用名是否在白名单中

### hitrace/hisysevent 采集命令与验收样例

**hitrace 采集命令**：
```bash
# 基础采集（GRAPHIC_AGP tag）
hitrace -t GRAPHIC_AGP

# 带时长采集（10 秒）
hitrace -t GRAPHIC_AGP -b 10240 -t 10

# 保存到文件
hitrace -t GRAPHIC_AGP -o /data/trace_output.ftrace

# 追踪特定事件
hitrace -t GRAPHIC_AGP | grep "RequestBuffer"
hitrace -t GRAPHIC_AGP | grep "FlushBuffer"
hitrace -t GRAPHIC_AGP | grep "AcquireBuffer"
```

**关键 trace 点**（`utils/trace/surface_trace.h`）：
- `SURFACE_TRACE_BEGIN(name)`：开始 trace（tag = `HITRACE_TAG_GRAPHIC_AGP | HITRACE_TAG_COMMERCIAL`）
- `SURFACE_TRACE_END()`：结束 trace
- `SURFACE_TRACE_NAME(name)`：命名 trace 点
- `SURFACE_TRACE_NAME_FMT(fmt, ...)`：格式化命名 trace 点
- `SURFACE_TRACE_INT(name, value)`：数值 trace

**典型 trace 输出样例（待团队补充）**：
- `RequestBuffer` trace：包含 sequence、width、height、format、usage
- `FlushBuffer` trace：包含 sequence、fence fd、desiredPresentTimestamp
- `AcquireBuffer` trace：包含 sequence、timestamp、damages
- `DropBuffer` trace：包含 drop reason、dropFrameLevel

**hisysevent 采集命令**：
```bash
# 查看事件列表
hisysevent -l

# 实时监听
hisysevent -r

# 过滤 GRAPHIC_AGP 相关事件
hisysevent -l | grep GRAPHIC

# 过滤 GPU 相关事件
hisysevent -l | grep GPU
```

**关键事件（待团队补充事件名）**：
- GPU subhealth 事件：`GPU_SUBHEALTH_MONITORING`（`sync_fence/src/sync_fence_tracker.cpp:220`）
- 帧率上报事件：事件名待确认
- 游戏场景事件：事件名待确认

**验收样例（待团队补充）**：
- 正常场景 hitrace 输出样例（60fps 游戏）
- 异常场景 hitrace 输出样例（buffer 泄漏、fence 超时）
- hisysevent 事件样例（GPU subhealth 上报）
