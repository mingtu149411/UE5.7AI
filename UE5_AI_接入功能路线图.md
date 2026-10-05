# AI 接入 UE5：全工作覆盖功能路线图

> 项目作者水印：南京大学24届沈逸文

> 目标：为 Codex、GPT、Claude 或本地模型提供一套以 MCP 为入口、以 UE5 Editor 插件为执行端的能力面。列表按**实现难度由低到高**排列；每项建立在前项可用的前提上。
>
> “完全接手”在本文中指：AI 能以结构化接口完成 UE5 编辑器、项目文件、构建、测试、打包和发布管线中的全部常规操作，并能读取、验证、恢复其结果；不等于 AI 能在没有需求、资产来源、授权或算力的前提下保证艺术与商业质量。

## 统一前提与架构

```text
AI Agent
  └─ MCP client
       └─ UE5 MCP Bridge（本机，权限/审计/队列）
            ├─ UE C++ Editor API / AssetRegistry / Slate
            ├─ Editor Scripting + Python
            ├─ Remote Control API（参数与远程实例）
            ├─ Commandlet / UAT / UBT / UnrealBuildTool
            └─ 外部工具适配器（DCC、版本控制、CI、资产库）
```

每个写操作必须支持：`plan → execute → compile/save → validate → capture evidence → rollback`。服务只应监听 `127.0.0.1`；默认只读，破坏性操作需显式授权并使用 Unreal Transaction 与 Git checkpoint。

## 功能清单（由低到高）

| # | 功能 | 主要接口/实现 | 交付与验收 |
|---:|---|---|---|
| 1 | MCP 连通性、健康检查、版本与项目识别 | HTTP/stdio MCP、`.uproject` 解析 | 能报告 UE 版本、项目路径、插件状态和连接状态。 |
| 2 | 只读项目文件树与文本配置读取 | 文件白名单、INI/JSON/uproject 解析 | 能列出并解释模块、Target、插件、地图和配置差异。 |
| 3 | 编辑器状态读取 | `GEditor`、选择集、编辑器模式、PIE 状态 | 能读取当前关卡、选中 Actor、活动世界、当前模式。 |
| 4 | 当前视口截图与基础图像描述输入 | Viewport 截图、相机 Transform、图像回传 | 能返回截图、分辨率、视角、相机位姿和渲染模式。 |
| 5 | 日志、Message Log、崩溃/编译错误读取 | Output Log、Asset Compile、UBT/UAT 输出 | 能结构化归类 warning/error，并定位关联资产或源码。 |
| 6 | Asset Registry 检索 | `IAssetRegistry` | 能按路径、类、Tag、依赖、引用者、大小查询资产。 |
| 7 | World Outliner/关卡结构导出 | `UWorld`、Actor/Component 反射 | 能导出 Actor 层级、组件、Transform、标签、文件夹和 Data Layer。 |
| 8 | 属性反射读取 | `FProperty`、Details 面板元数据 | 能读取任意授权 UObject 的可编辑属性、类型、范围、说明和当前值。 |
| 9 | 控制台命令与 CVar 的只读检索 | Console Manager | 能查 CVar、帮助、当前值、可用控制台命令。 |
| 10 | 资产/场景诊断报告 | 聚合 #2–#9 | 一条命令形成项目、地图、性能风险、缺失引用和错误摘要。 |
| 11 | 受控保存、重载与撤销/重做 | Editor Transaction、Package 保存 | 能保存指定资产、撤销本轮修改、在失败后恢复检查点。 |
| 12 | Actor 基础 CRUD | Editor Level Library / C++ | 能创建、查找、重命名、移动、复制、删除普通 Actor。 |
| 13 | Actor/Component 属性批量调整 | 反射写入、事务 | 能以筛选条件批量调整 Transform、Tag、可见性、碰撞等。 |
| 14 | 灯光、相机、后处理参数编辑 | Light/Camera/PostProcess API | 能按自然语言目标设置曝光、色温、阴影、焦距与景深。 |
| 15 | 文件夹、标签、Layer、Data Layer 管理 | World/Level 编辑 API | 能组织场景对象并提供变更清单。 |
| 16 | 基础静态网格布局 | Actor Spawn、Spline | 能生成地面、墙体、道路、阵列、散布和基本灰盒场景。 |
| 17 | 材质实例读取与参数编辑 | `UMaterialInstance*` | 能创建实例、继承、设置标量/向量/纹理/静态开关参数。 |
| 18 | Texture/Static Mesh/Skeletal Mesh 导入与重导入 | Asset Tools、Interchange | 能导入指定格式资产，设置路径、命名、导入选项和重导入来源。 |
| 19 | Blueprint 资产的只读结构导出 | `UBlueprint`、`UEdGraph` | 能读取变量、组件、函数、事件、节点、引脚、连线和编译状态。 |
| 20 | Material Graph 的只读结构导出 | Material Expression API | 能导出节点图、参数、连线、输出和编译错误。 |
| 21 | Niagara、动画、Sequencer 的只读摘要 | 各资产反射与轨道 API | 能列出发射器、骨架、动画片段、轨道、镜头和关键帧。 |
| 22 | 引用完整性与资产治理检查 | Asset Registry、Data Validation | 能发现缺失资源、循环引用、孤儿资产、重名、路径违规和未保存包。 |
| 23 | 视口/场景视觉验证循环 | 截图 + VLM + 结构化目标 | AI 能在操作后对比构图、物体存在性、色彩与相机目标并迭代。 |
| 24 | 运行 PIE、停止、采集运行日志 | Editor Play Session API | 能启动指定地图/模式、监控错误、停止并输出结果。 |
| 25 | 自动化测试发现与执行 | Automation Framework/Gauntlet | 能列出、运行、筛选自动化测试并归档报告与截图。 |
| 26 | 性能快照 | `stat`、Insights、CSV profiler | 能采集 FPS、帧时间、CPU/GPU、Draw Call、内存和瓶颈摘要。 |
| 27 | 受控 CVar/项目设置调整 | Console Manager、Project Settings | 能修改渲染/性能设置，记录前后值并支持回滚。 |
| 28 | World Partition、HLOD、Level Instance 操作 | World Partition Editor API | 能创建/加载单元、配置 HLOD、管理大世界分区和层级实例。 |
| 29 | Landscape 基础生成与编辑 | Landscape Editor API | 能创建地形、设高度图、图层、材质和植被基础参数。 |
| 30 | Foliage/PCG 基础生成 | Foliage/PCG API | 能按体积、坡度、密度和种子生成可复现的植被/散布。 |
| 31 | Data Asset、Data Table、Curve、Enum/Struct 编辑 | Asset Tools、数据序列化 | 能创建并维护游戏配置数据、校验 schema 与引用。 |
| 32 | UI/UMG 读取、创建与基础布局 | Widget Blueprint/Slate API | 能创建 Widget、层级、常用控件、样式和绑定占位。 |
| 33 | 输入映射与基础玩家控制配置 | Enhanced Input、Blueprint/C++ | 能创建 Action/Mapping Context 并接入角色/控制器。 |
| 34 | Blueprint 的安全编辑子集 | 节点工厂、Schema 校验、编译 | 能创建变量、组件、函数、事件图及白名单节点，连接后编译验证。 |
| 35 | C++ 模块/类生成、编辑与编译 | 模板、UBT、Live Coding | 能创建 Gameplay 类、修改源码、编译、解析错误并回归测试。 |
| 36 | 蓝图/C++ 跨边界分析 | 反射、代码索引、资产图 | 能追踪调用链、委托、接口、GAS 与数据流，并标出风险。 |
| 37 | Gameplay Framework 与 AI 基础构建 | Pawn/Controller/Subsystem、BT/EQS/StateTree | 能创建可运行的角色、敌人、交互、任务状态与基础 AI。 |
| 38 | Gameplay Ability System 配置 | GAS 资产/Attribute/Effect/Ability API | 能创建能力、属性、效果、标签、网络预测配置并做功能测试。 |
| 39 | 复杂材质图创建与自动修复 | Material Expression 图编辑、编译诊断 | 能从规格生成 PBR/Layered 材质，处理类型/引脚错误并输出实例接口。 |
| 40 | Procedural Mesh/Geometry Script 建模 | Dynamic Mesh、Geometry Script | 能生成可参数化的建筑、道具、地形细节，生成 UV、碰撞和基础 LOD。 |
| 41 | 外部 DCC 与生成资产编排 | Blender/Houdini/图像或 3D 生成服务适配 | 能委派建模、烘焙、贴图，再导入、命名、验证版权与技术规格。 |
| 42 | 骨架、动画资产、Blend Space、Montage 编辑 | Animation Data Controller | 能创建/编辑动画数据、混合空间、Montage、通知与状态机基础结构。 |
| 43 | Control Rig、IK、重定向与动画验证 | Control Rig/IK Rig/Retargeter API | 能配置绑定与重定向，检测脚滑、骨骼错误和运行时异常。 |
| 44 | Sequencer/Cinematics 全编辑 | MovieScene API | 能创建镜头、轨道、Section、关键帧、事件、渲染队列和审片截图。 |
| 45 | Niagara 全编辑与特效调优 | Niagara Graph/Emitter/System API | 能创建发射器、模块栈、参数、材质绑定、LOD 与性能验证。 |
| 46 | Audio 资产、MetaSound、Sound Cue、混音编辑 | Audio Engine/MetaSound API | 能导入、路由、编辑节点图、触发器、衰减和混音配置。 |
| 47 | UMG/Slate 复杂交互、数据绑定与无障碍 | Widget Graph、Slate、自动化 UI 测试 | 能完成菜单/HUD/设置页、输入导航、本地化与视觉回归检查。 |
| 48 | 网络、多玩家、Replication 与会话 | NetDriver、Online Subsystem、测试多进程 | 能配置 Replication、RPC、会话、专用服务器，并执行网络回归测试。 |
| 49 | 平台、插件、SDK 与项目配置全管理 | Target/Build.cs/INI/插件管理 | 能配置 Windows/主机/移动端/XR 平台、签名、权限与第三方 SDK。 |
| 50 | Shader、Cook、Package、Deploy 全流水线 | UBT/UAT/BuildGraph/Commandlet | 能构建、Cook、打包、安装、启动、收集日志、解析失败并重试。 |
| 51 | 渲染质量、内存、资产预算自动优化 | Insights、RenderDoc/统计、规则引擎 | 能针对目标平台优化 LOD、纹理、Nanite、材质、光照与加载策略。 |
| 52 | 本地化、可访问性、合规与内容审核 | Localization Dashboard、规则引擎 | 能维护文本、字体、文化配置、可访问性检查及内容清单。 |
| 53 | 版本控制与多用户协作 | Git/Perforce/Unreal Revision Control | 能分支、提交、资产锁定、冲突预检、变更审查和可恢复合并。 |
| 54 | CI/CD、夜间验证与发行编排 | CI、BuildGraph、设备农场 | 能在隔离环境自动构建、测试、性能回归、制品归档和发布候选生成。 |
| 55 | 崩溃诊断、最小复现与自动修复提案 | Crash Reporter、调用栈、符号、测试 | 能聚合崩溃、定位变更、构建复现工程、提出并验证修复。 |
| 56 | 插件/编辑器扩展开发 | C++/Slate/ToolMenus/Editor Utility | 能为缺失能力新建 UE 插件、面板、命令、资产动作，并自测安装流程。 |
| 57 | 任意编辑器模式与内部工作流适配 | 专用 C++ Toolset + 版本兼容层 | 为 Modeling、Animation、Virtual Production、XR、Simulation 等每一编辑器域补齐语义工具。 |
| 58 | 可恢复的长程自主代理 | 任务规划、状态机、Artifact 状态、人工审批门 | 能连续完成多小时工作：规划、执行、观察、修复、提交证据、遇权限边界暂停。 |
| 59 | 跨域“从零场景”生成闭环 | #1–#58 的编排层 | 从场景规格生成地图、资产清单、灰盒、材质、光照、交互、动画/特效、测试、性能报告与可运行包。 |
| 60 | UE5 全域能力覆盖与持续兼容 | Toolset 注册表、API 发现、回归矩阵 | 对每个已安装引擎插件、项目插件与编辑器模式暴露受权限控制的读取、编辑、验证和恢复能力。 |

## 最终闭环的最小验收标准

当 #1–#60 完成后，AI 对任何授权 UE5 项目应能：

1. 读取项目、世界、资产、图表、源码、运行日志与视口证据；
2. 将自然语言规格拆为可审计的资产和任务计划；
3. 创建或导入内容，编辑关卡、材质、蓝图/C++、动画、特效、UI、音频和项目设置；
4. 编译、保存、运行、自动测试、截图/性能验证，并根据错误迭代修复；
5. 构建、Cook、打包、部署、归档制品并提交可复现的版本控制记录；
6. 对无法安全决定的事项（版权素材、商业审美、支付/发布凭据、不可逆生产发布）请求人类批准，而不是擅自执行。

## 实施优先级建议

- **MVP（#1–#25）**：让 AI 可靠看懂、搭建并验证基础场景；已足以用于灰盒、灯光、参数迭代和自动测试。
- **生产内容阶段（#26–#47）**：覆盖大世界、材质、蓝图、程序化建模、动画、Sequencer、Niagara、音频与 UI。
- **工程接管阶段（#48–#60）**：覆盖网络、平台构建、优化、版本控制、CI/CD、崩溃修复、插件开发和长程自治。

## 不应被技术接口“自动解决”的边界

- AI 可以操作 DCC 或生成服务，但不能替你取得第三方素材的版权、商标、肖像及 SDK 授权。
- “美术最终品质”必须先转化为可验证的风格板、参考资产、预算和验收指标；否则只能得到主观上不稳定的结果。
- UE 源码未公开或未暴露的内部功能，必须通过自定义 C++ Editor Toolset 补桥；引擎升级后需要重新跑兼容性回归。
- 发布到商店、签名、支付、删除资产、强推版本库等外部或不可逆动作应始终保留审批门。
