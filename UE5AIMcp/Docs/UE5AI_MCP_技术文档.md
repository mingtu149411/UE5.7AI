# UE5 AI MCP 技术文档

> **项目作者水印：南京大学24届沈逸文**
> 文档版本：0.4.0-showcase-levels ｜ 更新日期：2026-10-05
> 适用对象：UE 5.7 编辑器、`UE5AIMcp` 编辑器插件与本地 MCP 桥接程序。

## 1. 项目定位

UE5 AI MCP 是一个运行在 Unreal Editor 内的本地自动化桥接插件。它将受控的编辑器能力封装为 HTTP 路由，并由 Python MCP（Model Context Protocol）stdio 桥接程序转换为 AI 客户端可调用的工具。

设计目标不是把编辑器完全暴露给任意脚本，而是提供**可枚举、可审计、显式白名单**的工作流能力：读取编辑器状态、创建或修改资产、组织场景、运行 PIE、保存项目及执行诊断。

```
AI 客户端（Codex / 其他 MCP 客户端）
        │  MCP JSON-RPC / stdio
        ▼
bridge/ue5ai_mcp_bridge.py
        │  localhost HTTP:8175
        ▼
UE5AIMcp Editor Module
        │  路由白名单 + Game Thread 调度
        ▼
Unreal Editor 子系统 / 资产工具 / 世界与关卡
        │
        ▼
Content、地图、蓝图、材质、PIE 与诊断结果
```

插件使用本机回环接口，不提供公网监听、用户鉴权或远程协作服务；因此它适用于本机受控的开发环境。

## 2. 工程组成

| 位置 | 责任 |
| --- | --- |
| `UE5AIMcp.uplugin` | 插件描述、UE 5.7 兼容性、编辑器模块和依赖插件声明。 |
| `Source/UE5AIMcp/Private/UE5AIMcpModule.cpp` | HTTP 路由注册、参数校验、编辑器 API 调用、异步调度、JSON 应答。 |
| `Source/UE5AIMcp/Public/UE5AIStylizedGrassInteractor.h` | 草地演示用交互器 Actor 的公开接口。 |
| `Source/UE5AIMcp/Private/UE5AIStylizedGrassInteractor.cpp` | 场景捕获、RT 写入、玩家位置跟踪与草地交互运行逻辑。 |
| `Source/UE5AIMcp/UE5AIMcp.Build.cs` | C++ 模块及 Unreal Editor 依赖。 |
| `bridge/ue5ai_mcp_bridge.py` | MCP JSON-RPC stdio 适配器；将已列出的工具映射到本地路由。 |
| `Config/DefaultUE5AIMcp.ini` | 插件级项目作者水印元数据。 |

模块类型为 `Editor`，主要功能只在编辑器进程内有效。它不是可直接随成品游戏打包的运行时远程控制模块。

### 2.1 主要编译依赖

基础依赖包括 `Core`、`CoreUObject`、`Engine`、`UnrealEd`、`Projects`、`AssetRegistry`、`AssetTools`、`HTTPServer`、`Json` 与 `JsonUtilities`。为支持完整编辑工作流，工程还引用：

- 蓝图与关卡：`Kismet`、`KismetCompiler`、`BlueprintGraph`、`LevelEditor`、`EditorSubsystem`；
- 动画、序列与音频：`LevelSequence`、`MovieScene`、`MovieSceneTracks`、`AudioEditor`；
- 地形、输入和 UI：`Landscape`、`EnhancedInput`、`UMG`、`UMGEditor`、`Slate`、`SlateCore`；
- 内容生产：`ProceduralMeshComponent`、`Niagara`、`NiagaraEditor`；
- 渲染支持：`RHI`。

这些依赖只表明编译时可调用相应编辑器 API；每条路由仍需要在实现中显式登记，不能据此推断插件自动拥有 Unreal 的全部权限。

## 3. 服务生命周期与线程模型

模块启动时创建 HTTP 路由监听器，默认端口为 **8175**。请求处理分成两个层次：

1. HTTP 线程处理请求方法、路径、JSON 结构与参数范围；
2. 会触碰 UObjects、世界、关卡、资产注册表或编辑器子系统的操作通过 `AsyncTask(ENamedThreads::GameThread, …)` 转入游戏线程执行。

该分层避免在 HTTP 工作线程直接修改 Unreal 对象。调用方应把所有写操作视为异步任务：收到 HTTP 成功响应只表示该请求已被接受或已完成规定步骤；应再调用读取/验证路由确认最终状态。

资产导入是特殊情况。导入任务会在后续 tick 中交给 Interchange/资产工具处理，随后对涉及包调用 `MarkPackageDirty` 与 `UEditorLoadingAndSavingUtils::SavePackages`。这样可避免“返回了导入对象但重启编辑器后资产消失”的瞬态问题。

## 4. 路由能力分组

以下是 0.4.0 已登记的主要能力。实际调用参数和返回字段以桥接器暴露的 MCP 工具定义及当前插件源码为准。

| 类别 | 读取/诊断 | 写入/执行 |
| --- | --- | --- |
| 编辑器概览 | `/ue5ai/health`、`/tools`、`/snapshot`、`/diagnostics`、`/logs` | `/test-shutdown` |
| 视口与场景 | `/viewport/verify` | `/viewport/screenshot`、`/actors/spawn`、`/actors/transform`、`/actors/organize`、`/actors`（DELETE）、`/layout/grid`、`/lights/point`、`/camera/postprocess` |
| 资产与内容 | `/assets/validate`、`/assets/class-summary`、`/analysis/dependencies` | `/assets/import`、`/assets/reimport`、`/material/instance`、`/material/graph`、`/model/procedural-box`、`/dcc/job` |
| 蓝图与 C++ | `/blueprint/summary` | `/blueprint/create`、`/cpp/scaffold`、`/gameplay/scaffold`、`/gas/scaffold` |
| 地图与世界 | `/world/partition` | `/world/level-instance`、`/landscape/create`、`/scatter/instances`、`/data/create`、`/scene/orchestrate` |
| UI、输入与运行 | `/properties`、`/cvars` | `/ui/widget`、`/input/mapping`、`/pie/start`、`/pie/stop`、`/project/save-dirty` |
| 动画、序列、特效、声音 | `/sequence/summary`、`/animation/validate` | `/sequence/create`、`/sequence/add-float-track`、`/animation/montage`、`/niagara/scaffold`、`/audio/cue` |
| 性能、质量与工程治理 | `/performance/snapshot`、`/quality/budget`、`/compliance/audit`、`/source-control/status`、`/crash/diagnostics`、`/network/replication`、`/platform/profile` | `/settings/cvar`、`/settings/cvar/rollback`、`/automation/run`、`/pipeline/plan`、`/cicd/plan`、`/plugin/scaffold` |
| 展示关卡 | `/showcase/grass/status` | `/showcase/create`、`/showcase/enhance`、`/showcase/grass/rebuild` |

补充能力包括撤销/重做（`/transaction/undo`、`/transaction/redo`）、自动化发现与报告（`/automation/discover`、`/automation/report`）、智能体计划与状态迁移（`/agent/plan`、`/agent/transition`）以及网络会话规划（`/network/session-plan`）。

## 5. MCP 桥接层

桥接器通过 stdin/stdout 处理 MCP JSON-RPC。它只注册源码中明确声明的工具，而不是转发任意 URL、命令或 UE 控制台语句。这个限制是安全设计的一部分：AI 能调用的范围可以通过版本控制审查。

桥接器默认请求 `http://127.0.0.1:8175`。典型链路为：MCP 客户端发起工具调用 → Python 解析输入并校验工具名 → localhost HTTP 请求 → 插件返回 JSON → 桥接器将其包装为 MCP 结果。

桥接层不保存项目密码、云凭据或资产服务器令牌。若在团队环境接入源代码管理、构建农场或 DCC，建议由外围受控服务提供短期凭证，而不要写进桥接脚本。

## 6. 数据持久化原则

### 6.1 可持久化内容

资产、地图、蓝图、材质实例和配置变更必须满足以下条件才可视为完成：

1. 目标 `UPackage` 已标脏；
2. 保存操作成功返回；
3. 资产注册表可重新发现目标路径；
4. 关闭并重新打开编辑器后仍可加载。

### 6.2 动态组件的陷阱

只调用 `NewObject`/`RegisterComponent` 创建的运行时组件不一定随 Actor 写入地图。本项目的草场重建逻辑已采用 `AddInstanceComponent` 后再注册组件，使 HISM 草簇成为关卡 Actor 的实例组件并可在重开编辑器后复现。

### 6.3 二进制资产边界

`.uasset`、`.umap`、`.dll` 是二进制或 Unreal 私有序列化格式，不能通过插入文本的方式加水印，否则会损坏资源。作者水印因此放置在插件清单、插件配置、项目设置、源码注释、说明文件和作者声明文件中；运行时资产依旧保留其正常的 Unreal 元数据结构。

## 7. 二次元交互草地实现

该展示功能位于测试项目：

`/Game/AI插件测试/关卡二_交互草地/关卡二_全局风场草地`

它采用“地表材质 + HISM 卡片草簇 + 风场 + RT 角色交互”的组合，而非把整张地面当成一张草贴图：

- 地表材质 `M_二次元草地地表` 负责底色、明暗层次和远景连续性；
- `T_二次元草簇` 是原创风格化竖直卡片纹理，`M_二次元草簇_RT交互` 使用 Masked、双面和实例化静态网格；
- 近景与中景由五个 HISM 分层组件组成，共持久化 5,840 个实例；
- 全局风为 `Time → Sine → 草叶高度遮罩 → 风向量` 的 World Position Offset；
- 角色交互由 `BP_第三人称RT草地交互控制器` 继承原生 `AUE5AIStylizedGrassInteractor`，将玩家 XY 位置写入 `RT_草地交互与风场`；
- 草材质以 `WorldPosition * (1 / 29000) + 0.5` 投影至 RT，读取 R 通道后以草叶高度遮罩调制压草偏移；RT 与全局风向量叠加。

场景捕获相机为正交投影，中心高度约 12,000 cm、`OrthoWidth=29,000`，每秒最多捕获 30 次。仅交互代理对 SceneCapture 可见，避免把地表和草簇本身反馈进交互纹理。

### 7.1 已规避的实现问题

| 问题 | 原因 | 修正 |
| --- | --- | --- |
| 草材质显示为默认黑色 | LWC `float3` 世界坐标直接连接到 RT 采样器的 `float2` UV。 | 在 `RT交互二维UV` 节点显式取 RG 后采样。 |
| 导入纹理重启后丢失 | 只完成导入任务，没有显式保存资产包。 | 标脏并调用编辑器保存 API，后续重新打开验证。 |
| HISM 重启后缺失 | 动态组件不是 Actor 实例组件。 | `AddInstanceComponent` + 注册 + 保存地图。 |
| PIE 没有第三人称角色可跟踪 | 地图使用通用默认 Pawn。 | 指定第三人称 GameMode，并放置中文标记 PlayerStart。 |

## 8. 安全、审计与限制

1. **白名单优先**：新增能力必须新增明确路由和桥接工具，不能让模型任意执行控制台命令或本机 shell 命令。
2. **本机接口**：默认仅可由本机访问。若改为局域网监听，必须另行实现认证、TLS、访问控制和请求审计。
3. **写入应可恢复**：写入前使用快照、命名空间和 Unreal 事务；大规模操作后保存并记录创建资产列表。
4. **自动化不等于语义正确**：插件能生成与修改内容，但艺术风格、游戏性、性能预算和授权素材合规仍需人工验收。
5. **截图限制**：非活动视口或未产生新渲染帧时，截图请求可能超时；应结合材质编译日志、资产校验、PIE 状态和运行时状态路由判断，而不能只依赖截图。
6. **覆盖范围限制**：当前版本是编辑器自动化工具集，未覆盖所有 UE 面板和全部私有编辑器 API；复杂建模、骨骼蒙皮、Control Rig、烘焙、多人联机与打包发布仍需要新增专用工具和测试。

## 9. 构建与验证基线

已验证的本机构建目标为：

```text
D:\xuexishiyong\UE\UE_5.7\Engine\Build\BatchFiles\Build.bat
UnrealEditor Win64 Development
-Project="D:\xuexishiyong\ue--myproject\AI插件测试项目\AI插件测试项目.uproject"
-plugin="D:\xuexishiyong\UE5AI\UE5AIMcp\UE5AIMcp.uplugin"
```

交付前至少执行：插件编译、启动编辑器、`/ue5ai/health`、`/assets/validate`、目标关卡加载、PIE 启停、保存后重启加载。草地关卡的最终 PIE 状态已确认交互器已找到第三人称 Pawn、RT 已分配且存在最近一次捕获位置。

## 10. 扩展建议

后续增加工具时，建议沿用“数据模型 → 路由参数校验 → 游戏线程执行 → 保存/验证 → MCP 工具说明”的链路，并为每项写操作补充：幂等策略、失败回滚、受影响包列表、性能上限和可自动化测试用例。对于视觉生产能力，应优先将风格参数暴露为 Material Instance/数据资产，而不是把常量散落在 C++ 或蓝图中。
