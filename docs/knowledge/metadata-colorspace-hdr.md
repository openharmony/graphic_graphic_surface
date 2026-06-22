# 色彩空间、HDR Metadata 与元数据管理知识入口

## 适用范围

涉及色彩空间转换、ColorSpace 枚举、HDI CM_ColorSpaceType 映射、HDR metadata（static/dynamic）、MetadataType、DATASPACE、metadata_helper、TV PQ metadata、MetadataHelper 类时，先读本文档。

## 快速代码索引

| 方向 | 主要文件 |
|------|----------|
| MetadataHelper 实现 | `surface/src/metadata_helper.cpp`、`surface/include/metadata_helper.h` |
| NativeBuffer 色彩空间映射 | `surface/src/native_buffer.cpp` |
| NativeWindow 色彩空间映射 | `surface/src/native_window.cpp` |
| TV PQ Metadata 结构 | `surface/include/tv_pq_metadata.h` |
| 公开枚举和类型 | `interfaces/inner_api/surface/buffer_common.h`、`interfaces/inner_api/surface/surface_type.h` |
| 元数据转换工具 | `interfaces/inner_api/common/metadata_convertor.h` |
| 错误码 | `interfaces/inner_api/common/graphic_common_c.h` |
| 单测 | `surface/test/unittest/metadata_helper_test.cpp` |

## 术语触发词

`ColorSpace`、`OH_NativeBuffer_ColorSpace`、`CM_ColorSpaceType`、`ConvertColorSpaceTypeToInfo`、`ConvertColorSpaceInfoToType`、`NATIVE_COLORSPACE_TO_HDI_MAP`、`HDR`、`OH_HDR_DYNAMIC_METADATA`、`OH_HDR_STATIC_METADATA`、`OH_HDR_METADATA_TYPE`、`MetadataType`、`DATASPACE`、`MetadataHelper`、`ATTRKEY_COLORSPACE_INFO`、`ATTRKEY_HDR_*`、`PQ`、`TV PQ`、`GraphicColorGamut`

## 先判断的问题

- 改动涉及色彩空间枚举、映射表还是转换函数？
- 是否影响 HDR metadata 格式或 key？
- 是否需要处理 NATIVE_COLORSPACE_TO_HDI_MAP 多键同值的非确定性问题？
- 是否涉及 TV PQ metadata（条件编译 RS_ENABLE_TV_PQ_METADATA）？
- 是否影响公开 C API（OH_NativeBuffer_SetColorSpace/GetColorSpace）行为？

## 关键边界

`ConvertColorSpaceTypeToInfo` 将 `CM_ColorSpaceType` 位域解包为 `CM_ColorSpaceInfo`：primaries(val & PRIMARIES_MASK)、transfunc((val >> TRANSFUNC_OFFSET) & TRANSFUNC_MASK) 即 (val >> 8) & MASK、matrix((val >> MATRIX_OFFSET) & MATRIX_MASK) 即 (val >> 16) & MASK、range((val >> RANGE_OFFSET) & RANGE_MASK) 即 (val >> 21) & MASK。偏移量和掩码来自 HDI 定义。`ConvertColorSpaceInfoToType` 是反向位域打包：primaries | (transfunc << 8) | (matrix << 16) | (range << 21)，数学上可逆。

`NATIVE_COLORSPACE_TO_HDI_MAP`（`unordered_map<OH_NativeBuffer_ColorSpace, CM_ColorSpaceType>`）有 33 个条目。**已知问题**：多组键映射到相同值——`OH_COLORSPACE_BT2020_HLG_FULL`（枚举序号=4）与 `OH_COLORSPACE_DISPLAY_BT2020_HLG`（枚举序号=22）映射到相同的 `CM_ColorSpaceType` 位域编码值 `2360580`（计算公式：COLORPRIMARIES_BT2020(4) | (TRANSFUNC_HLG(5)<<8) | (MATRIX_BT2020(4)<<16) | (RANGE_FULL(1)<<21)）。类似冲突还有 `OH_COLORSPACE_SRGB_FULL`(11)/`OH_COLORSPACE_DISPLAY_SRGB`(25)、`OH_COLORSPACE_P3_FULL`(12)/`OH_COLORSPACE_DISPLAY_P3_SRGB`(26)、`OH_COLORSPACE_BT2020_PQ_FULL`(5)/`OH_COLORSPACE_DISPLAY_BT2020_PQ`(31)。这是 `OH_NativeBuffer_ColorSpace` 枚举定义本身的语义等价设计：头文件注释明确标注 `OH_COLORSPACE_DISPLAY_BT2020_HLG` 为 "equal to OH_COLORSPACE_BT2020_HLG_FULL"。`OH_NativeBuffer_GetColorSpace` 和 `OH_NativeWindow_GetColorSpace` 使用 `std::find_if` 反向查找 `NATIVE_COLORSPACE_TO_HDI_MAP`，`unordered_map` 迭代顺序不确定，导致多键同值时返回结果非确定——可能返回 `OH_COLORSPACE_BT2020_HLG_FULL` 或 `OH_COLORSPACE_DISPLAY_BT2020_HLG`。

`native_buffer.cpp` 和 `native_window.cpp` 各有一份 `NATIVE_COLORSPACE_TO_HDI_MAP` 副本，必须同步维护。`native_buffer.cpp` 有 33 条色彩空间 + 7 条 metadatatype；`native_window.cpp` 有 33 条色彩空间 + 3 条 metadatatype（仅 video HDR 类型）。

HDR metadata key：`ATTRKEY_COLORSPACE_INFO`（色彩空间信息）、`ATTRKEY_HDR_METADATA_TYPE`（HDR 元数据类型）、`ATTRKEY_HDR_STATIC_METADATA`（静态 HDR 元数据）、`ATTRKEY_HDR_DYNAMIC_METADATA`（动态 HDR 元数据，SEI 字节流）、`ATTRKEY_VIDEO_AI_HDR_LUT`（AI HDR LUT，V2_2）、`ATTRKEY_CROP_REGION`（裁剪区域）、`ATTRKEY_ROI_METADATA`（ROI 元数据，V2_2）、`ATTRKEY_ADAPTIVE_FOV_METADATA`（自适应 FOV，V2_1）、`ATTRKEY_EXTERNAL_METADATA_002`（SDR 动态元数据，V2_0）。`SetColorSpaceType` 先调用 `ConvertColorSpaceTypeToInfo` 转 Info，再通过 `MetadataHelper::ConvertMetadataToVec<CM_ColorSpaceInfo>` 序列化为 `vector<uint8_t>`，最后调用 `buffer->SetMetadata(ATTRKEY_COLORSPACE_INFO, ...)`。`GetColorSpaceType` 通过 `buffer->GetMetadata` 读取再反序列化转回 Type。

HDR metadata 类型映射（`NATIVE_METADATATYPE_TO_HDI_MAP`）：`OH_VIDEO_HDR_HLG`→`CM_VIDEO_HLG`、`OH_VIDEO_HDR_HDR10`→`CM_VIDEO_HDR10`、`OH_VIDEO_HDR_VIVID`→`CM_VIDEO_HDR_VIVID`、`OH_IMAGE_HDR_VIVID_DUAL`→`CM_IMAGE_HDR_VIVID_DUAL`、`OH_IMAGE_HDR_VIVID_SINGLE`→`CM_IMAGE_HDR_VIVID_SINGLE`、`OH_IMAGE_HDR_ISO_DUAL`→`CM_IMAGE_HDR_ISO_DUAL`、`OH_IMAGE_HDR_ISO_SINGLE`→`CM_IMAGE_HDR_ISO_SINGLE`。

TV PQ metadata（条件编译 `RS_ENABLE_TV_PQ_METADATA`，由 `graphic_surface_config.gni` 中 `graphic_surface_feature_tv_metadata_enable` 控制）：`TvPQMetadata` 结构体使用 `#pragma pack(push, 1)` 1 字节对齐，约 23 字节。包含 sceneTag、uiFrameCnt、vidFrameCnt、vidFps、视频窗口信息（`TvVideoWindow`）、scaleMode（4 bit）、dpPixFmt（4 bit）、colorimetry（4 bit）、hdr（4 bit）、hdrLuminance、hdrRatio、reserved[4]。`UpdateTVMetadataField` 是读-改-写辅助函数：先 `GetVideoTVMetadata`，执行 `OnSetFieldsFunc` lambda，再 `SetVideoTVMetadata`。

`MetadataConvertor` 模板工具（`interfaces/inner_api/common/metadata_convertor.h`，命名空间 `OHOS::MetadataManager`）与 `MetadataHelper` 的模板方法逻辑相同（`ConvertMetadataToVec`/`ConvertVecToMetadata`），但放在独立命名空间供更广泛复用。`ConvertVecToMetadata` 要求 `data.size() == sizeof(T)` 否则返回 `GSERROR_NOT_SUPPORT`。

`OH_NativeBuffer_SetMetadataValue` 按 metadataKey 分发：`OH_HDR_DYNAMIC_METADATA` 直接写、`OH_HDR_STATIC_METADATA` 先反序列化再序列化写、`OH_HDR_METADATA_TYPE` 先转换类型再写、`OH_REGION_OF_INTEREST_METADATA` 上限 `ROI_METADATA_CAPACITY`(256) 字节。`OH_NativeBuffer_GetMetadataValue` 使用 `new uint8_t[]` 分配输出缓冲区——调用方必须释放。返回 `OH_NativeBuffer_StaticMetadata` 时大小为 `sizeof(OH_NativeBuffer_StaticMetadata)`。

`GraphicColorGamut` 有 11 个值（INVALID=-1, NATIVE=0, BT601=1, BT709=2, DCI_P3=3, SRGB=4, ADOBE_RGB=5, DISPLAY_P3=6, BT2020=7, BT2100_PQ=8, BT2100_HLG=9, DISPLAY_BT2020=10），与 `OH_NativeBuffer_ColorGamut` 有对应关系但不完全相同。

## 常见风险

在 `NATIVE_COLORSPACE_TO_HDI_MAP` 新增映射时忽略多键同值问题，导致 `GetColorSpace` 反向查找非确定。新增色彩空间枚举值时必须确认编码值不与已有条目冲突，或者如果冲突是有意的，必须在文档和代码中明确说明。

只改 `native_buffer.cpp` 的映射表而遗漏 `native_window.cpp` 的副本，或反之。两份副本的同步维护是持续风险。

修改 `CM_ColorSpaceType` 位域布局时，未同步更新 `ConvertColorSpaceTypeToInfo`/`ConvertColorSpaceInfoToType` 的偏移量常量（`PRIMARIES_MASK`、`TRANSFUNC_OFFSET` 等），导致编解码不一致。

修改 HDR metadata key 或格式时，未同步检查 HDI 层和消费方（graphic_2d、RS）的假设。metadata 通过 HDI `SetMetadata`/`GetMetadata` 传递到驱动层，格式变更可能导致驱动解析错误。

TV PQ metadata 结构体大小变化（packed 1 字节对齐敏感），未验证跨进程序列化兼容性。`UpdateTVMetadataField` 的读-改-写模式在并发场景下可能导致丢失更新。

`MetadataHelper` 的模板序列化（`memcpy` 到 `vector<uint8_t>`）假设结构体无 padding，修改包含 bit-field 的结构体（如 `TvPQMetadata` 的 scaleMode:4/dpPixFmt:4）可能破坏假设。不同编译器对 bit-field 布局的处理可能不同。

`OH_NativeBuffer_GetMetadataValue` 返回 `new uint8_t[]` 需要调用方释放，文档中必须明确说明。如果调用方遗忘释放会导致内存泄漏。

`OH_NativeWindow_SetColorSpace` 通过 `SetUserData("ATTRKEY_COLORSPACE_INFO", ...)` 存储为字符串，`GetColorSpace` 通过 `GetUserData` 反向查找。这种 UserData 方式与 NativeBuffer 的 HDI `SetMetadata` 方式是两套不同路径，行为可能不一致。

## Ask before / 必须人工确认

- 修改 ColorSpace 枚举值、映射表或位域布局时。
- 修改 HDR metadata 格式、key 或序列化方式时。
- 新增 `NATIVE_COLORSPACE_TO_HDI_MAP` 条目时（必须确认正向和反向查找确定性）。
- 修改 TV PQ metadata 结构体或条件编译开关时。
- 修改公开 C API（SetColorSpace/GetColorSpace）行为时。
- 需要真实设备验证但当前没有设备时。

## 验证建议

- 基础构建：`hb build graphic_surface -i`
- 近端单测：`metadata_helper_test`、`native_buffer_test`
- Fuzz：`NativeBufferFuzzTest`
- 涉及色彩空间映射时重点测试正向和反向转换的一致性，尤其是多键同值条目。
- 涉及 HDR metadata 时需要真实设备验证显示效果。
- 涉及 TV PQ metadata 时需要 TV 设备验证。

## 待补充背景

### NATIVE_COLORSPACE_TO_HDI_MAP 多键同值问题

**问题根源**：
- 映射表使用 `unordered_map<OH_NativeBuffer_ColorSpace, CM_ColorSpaceType>`（`surface/src/native_window.cpp:42`、`surface/src/native_buffer.cpp:36`）
- 反向查找使用 `std::find_if`，迭代顺序不确定

**已知冲突项**（位域编码值相同，枚举序号不同）：
- `OH_COLORSPACE_BT2020_HLG_FULL`(序号=4) 与 `OH_COLORSPACE_DISPLAY_BT2020_HLG`(序号=22)：`CM_ColorSpaceType` = 2360580
- `OH_COLORSPACE_SRGB_FULL`(序号=11) 与 `OH_COLORSPACE_DISPLAY_SRGB`(序号=25)：`CM_ColorSpaceType` 相同
- `OH_COLORSPACE_P3_FULL`(序号=12) 与 `OH_COLORSPACE_DISPLAY_P3_SRGB`(序号=26)：`CM_ColorSpaceType` 相同
- `OH_COLORSPACE_BT2020_PQ_FULL`(序号=5) 与 `OH_COLORSPACE_DISPLAY_BT2020_PQ`(序号=31)：`CM_ColorSpaceType` 相同

注：枚举序号对应 `interfaces/inner_api/surface/buffer_common.h` 中 `OH_NativeBuffer_ColorSpace` 的定义顺序。

**位域编码公式**：
```
CM_ColorSpaceType = primaries | (transfunc << 8) | (matrix << 16) | (range << 21)
```

**临时规避方案**：
- 使用 `SetColorSpace` 时选择非冲突枚举值（如 `OH_COLORSPACE_SRGB_FULL` 而非 `OH_COLORSPACE_DISPLAY_SRGB`）
- 如果必须使用 `DISPLAY_*` 枚举，理解 `GetColorSpace` 可能返回对应的非 DISPLAY 值
- 在应用层做二次校验，明确期望的色彩空间类型

**官方解决方案（待确认）**：
- 可能方案 1：调整 `DISPLAY_*` 枚举值位域编码，避免冲突（需要公开 API 变更）
- 可能方案 2：反向查找改用有序容器或确定性查找算法（需要代码修改）
- 可能方案 3：在文档中明确说明冲突项的语义等价性，应用层自行处理

### 各产品形态 HDR 能力矩阵

**HDR metadata 类型映射**（`surface/src/native_window.cpp:77-81`、`surface/src/native_buffer.cpp`）：
- `OH_VIDEO_HDR_HLG` → `CM_VIDEO_HLG`
- `OH_VIDEO_HDR_HDR10` → `CM_VIDEO_HDR10`
- `OH_VIDEO_HDR_VIVID` → `CM_VIDEO_HDR_VIVID`
- `OH_IMAGE_HDR_VIVID_DUAL` → `CM_IMAGE_HDR_VIVID_DUAL`
- `OH_IMAGE_HDR_VIVID_SINGLE` → `CM_IMAGE_HDR_VIVID_SINGLE`
- `OH_IMAGE_HDR_ISO_DUAL` → `CM_IMAGE_HDR_ISO_DUAL`
- `OH_IMAGE_HDR_ISO_SINGLE` → `CM_IMAGE_HDR_ISO_SINGLE`

**产品形态差异（待团队补充）**：
- **高端手机**：支持 HLG/HDR10/VIVID/ISO HDR，支持 dynamic metadata
- **中端手机**：支持 HLG/HDR10，可能不支持 VIVID 和 dynamic metadata
- **低端手机**：可能只支持基础 HDR10 static metadata
- **TV 设备**：支持完整 HDR 类型，包括 PQ/HLG 和 dynamic metadata
- **平板设备**：HDR 能力介于手机和 TV 之间

**验证建议**：
- `native_buffer_test`：验证 metadata 设置/读取
- `metadata_helper_test`：验证色彩空间转换
- 真实设备上通过 `OH_NativeBuffer_SetMetadataValue`/`GetMetadataValue` 测试 HDR metadata 支持
- 通过显示效果验证 HDR metadata 是否正确传递到驱动层

### TV PQ metadata 使用场景

**条件编译开关**：`RS_ENABLE_TV_PQ_METADATA`（由 `graphic_surface_config.gni` 中 `graphic_surface_feature_tv_metadata_enable` 控制）

**结构体定义**（`surface/include/tv_pq_metadata.h`）：
- `#pragma pack(push, 1)` 1 字节对齐
- 约 23 字节：sceneTag、uiFrameCnt、vidFrameCnt、vidFps、视频窗口信息、scaleMode(4bit)、dpPixFmt(4bit)、colorimetry(4bit)、hdr(4bit)、hdrLuminance、hdrRatio、reserved[4]

**典型使用场景（待团队补充）**：
- TV 产品 PQ（Picture Quality）调优场景
- 视频播放 HDR metadata 传递
- UI 框架与视频窗口混合显示
- 动态 PQ 参数调整（亮度、对比度、色域）

**验证命令（待团队补充）**：
- TV 设备上播放 HDR 视频内容
- 检查 PQ metadata 是否正确传递到显示驱动
- 通过 TV PQ 调优界面验证参数生效
- dump buffer metadata 检查 TvPQMetadata 结构完整性

**注意事项**：
- `UpdateTVMetadataField` 使用读-改-写模式，并发场景可能丢失更新
- bit-field（scaleMode:4/dpPixFmt:4）布局依赖编译器，跨编译器可能不一致
- 1 字节对齐的结构体序列化时需确认跨进程兼容性
