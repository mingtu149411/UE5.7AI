# 生产内容阶段（#26–#47）实现与验收计划

> 项目作者水印：南京大学24届沈逸文

## 目标和测试约束

- 实现范围：路线图 #26–#47；接口通过本机 `127.0.0.1:8175` 的 UE 编辑器插件及标准 MCP bridge 暴露。
- 测试项目：`D:\xuexishiyong\ue--myproject\AI插件测试项目\AI插件测试项目.uproject`（UE 5.7.4）。
- 验收策略：所有能力完成后，在一次编辑器会话内运行一份统一测试；不为单项功能单独启动 UE。
- 测试清理：测试后删除 `/Game/UE5AI_AutoTmp`、`Saved/UE5AI` 和本阶段临时 BuildPlugin 输出；测试创建的世界 Actor 在会话内删除且不保存地图。

## 已实现的能力合同

| # | 接口 | 可验证交付 | 受控边界 |
|---:|---|---|---|
| 26 | `performance/snapshot` | 帧时间、FPS 估算、RHI draw call、物理内存及瓶颈提示 | GPU 帧时间必须用 Insights/CSV 多帧采样，不伪报单帧数。 |
| 27 | `settings/cvar`、`settings/cvar/rollback` | 运行时 CVar 前值记录、修改和回滚 | 不写入项目 INI；持久项目设置继续要求显式配置变更审批。 |
| 28 | `world/partition`、`world/level-instance` | World Partition 状态、真实 `ALevelInstance` 与 `WorldAsset` 绑定 | HLOD 构建仍由原生编辑器 Build 操作执行。 |
| 29 | `landscape/create` | 可删除的真实 Landscape Actor，带确定性高度场 | 复杂层、笔刷和外部高度图通过后续受控导入扩展。 |
| 30 | `scatter/instances` | 带种子、网格和实例计数的 ISM 散布 | 是 PCG/Foliage 兼容的可复现基础散布，不替代项目专用 PCG Graph。 |
| 31 | `data/create` | CurveFloat、DataTable、DataAsset、User Defined Enum、User Defined Struct | DataTable 默认行是 `FTableRowBase`；业务 schema 应用项目 USTRUCT。 |
| 32、47 | `ui/widget` | UMG 垂直菜单、稳定控件名称、标题和三个可导航按钮 | 业务数据绑定、本地化文本与无障碍文案由项目 UI schema 补全。 |
| 33 | `input/mapping` | Input Action、Mapping Context 和按键映射 | 由目标 Pawn/Controller 把 Context 注册进 LocalPlayerSubsystem。 |
| 34 | `blueprint/create` | 指定父类、白名单标量/结构成员变量、编译 | 未审批的任意节点执行不开放。 |
| 35 | `cpp/scaffold` | C++ UObject 模板预览或明确 `write_project=true` 的项目 Source 写入 | 不从编辑器进程启动编译；使用受审计 UBT 流水线。 |
| 36 | `analysis/dependencies` | 一跳依赖/引用者资产图 | 跨蓝图/C++ 调用链需项目代码索引。 |
| 37 | `gameplay/scaffold` | Character/Pawn/Controller Blueprint，Health/交互状态 | BT/EQS/StateTree 依项目插件与具体 schema。 |
| 38 | `gas/scaffold` | 已启用 GameplayAbilities 时创建 Gameplay Effect 骨架 | 未启用时返回 `available:false`，不生成伪资产。 |
| 39 | `material/graph` | PBR Base Color + 暴露 Roughness 节点，触发编辑器重编译 | Layered/Substrate 复杂图按规格继续增量构造。 |
| 40 | `model/procedural-box` | 有 UV、碰撞和顶点/三角形证据的程序网格 Actor | 静态网格烘焙、自动 LOD 是受控后续步骤。 |
| 41 | `dcc/job` | 外部 DCC 任务 JSON（规格、来源校验、导入要求） | 不会自行上传资产或规避版权/授权。 |
| 42、43 | `animation/montage`、`animation/validate` | Skeleton 绑定 Montage、绑定校验 | 动画数据编辑、Control Rig/IK Retargeter 需对应项目资产和插件。 |
| 44 | `sequence/add-float-track` | Level Sequence 内实际创建 2-key Float master track | 镜头绑定/渲染队列在既有序列资产和目标规范下扩展。 |
| 45 | `niagara/scaffold` | Niagara 启用时创建 System 资产，未启用时明确报告 | 发射器模块栈需 Niagara 资产编辑器模块和白名单。 |
| 46 | `audio/cue` | 可选 SoundWave 路由的 Sound Cue | MetaSound、Submix、Attenuation 按项目音频图继续编辑。 |

## 统一验收通过标准

1. 插件以 UE 5.7.4 BuildPlugin 完整编译；
2. 测试项目启动后，所有非可选端点返回成功且有结构化证据；
3. GAS/Niagara 在未启用时仅可返回正常的可用性状态，启用时必须能创建资产；
4. 所有测试 Actor 均在会话内删除，临时资产与缓存均在退出后清理；
5. 保存 JSON 报告，包含每个 #26–#47 的检查项、结果和能力边界。

## 最终完成记录（2026-10-05）

- 最终插件构建：`UE5AIMcp-production-batch12`，已部署至 `D:\xuexishiyong\UE5AI\UE5AIMcp\Binaries\Win64`。
- 最终验收：在 `AI插件测试项目`（UE 5.7.4）执行 `Run-ProductionContentUnifiedTest.ps1`，报告为 `passed: true`，共 32/32 项检查通过，覆盖 #26–#47。
- 条件能力结论：本项目未启用 GameplayAbilities，因此 #38 正确返回可用性门控；Niagara 已启用并实际创建了 System 资产。
- 收尾：统一测试已删除临时 Actor；随后删除 `Saved/UE5AI`、本阶段 BuildPlugin 输出和测试项目中本阶段产生的 crash 残留。最终插件二进制、源文件和验收报告均保留。
