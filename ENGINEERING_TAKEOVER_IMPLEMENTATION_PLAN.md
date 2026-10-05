# 工程接管阶段（#48–#60）实现与验收计划

> 项目作者水印：南京大学24届沈逸文

> 项目作者水印：南京大学24届沈逸文

## 测试约束

- 测试项目：`D:\xuexishiyong\ue--myproject\AI插件测试项目\AI插件测试项目.uproject`（UE 5.7.4）。
- 验收方式：完成本阶段接口后，在单一编辑器会话运行 `Run-EngineeringTakeoverUnifiedTest.ps1`；不会逐项启动编辑器。
- 安全原则：网络进程、签名、部署、提交/推送、外部 CI 调度、SDK 安装和覆盖项目插件均默认禁止。接口生成有证据的计划，实际执行须由后续显式批准的操作完成。
- 清理：统一测试退出编辑器后清除 `Saved/UE5AI`、本阶段 BuildPlugin 输出及测试产生的 crash/临时残留；不删除最终插件、源码、脚本或验收报告。

## 能力合同

| # | 接口 | 可验证交付 | 受控边界 |
|---:|---|---|---|
| 48 | `network/replication`、`network/session-plan` | 当前网络模式、复制 Actor 数、服务器/客户端回归计划 | 不自动启动服务器、客户端或在线会话。 |
| 49 | `platform/profile` | 项目配置文件、平台范围、启用插件清单 | 签名、商店凭据、权限和 SDK 安装需明确批准。 |
| 50 | `pipeline/plan` | UAT `BuildCookRun` 命令、Cook/Package/Archive 验证清单 | 仅生成 dry-run 计划；不会打包、签名、安装或部署。 |
| 51 | `quality/budget` | 资产类型计数、draw call、物理内存和优化建议 | LOD、纹理、Nanite、材质的实际修改需目标平台预算与批准。 |
| 52 | `compliance/audit` | 本地化、无障碍、内容来源和发行合规检查清单 | 评级、隐私、许可证及平台政策由人工确认。 |
| 53 | `source-control/status` | Git 仓库可用性、只读协作状态与安全操作清单 | 不自动 commit、push、submit、解锁或解决冲突。 |
| 54 | `cicd/plan` | 构建、测试、性能、归档、发布审批阶段的 CI 定义 | 不联系外部 CI、设备农场或发行服务。 |
| 55 | `crash/diagnostics` | Crash 目录盘点和最小复现/修复验证流程 | 不上传 dump；符号服务器和隐私敏感日志需另行授权。 |
| 56 | `plugin/scaffold` | 新 Editor Plugin descriptor 预览；`write_project=true` 才创建新目录 | 拒绝覆盖已有插件；源码模块、编译和安装遵循 UBT/审批。 |
| 57 | `editor/adapters` | Modeling、动画、Sequencer、Niagara、VP、XR、Chaos 等已安装域的适配器注册表 | 不加载可选模块，不调用未批准的专有编辑器工作流。 |
| 58 | `agent/plan`、`agent/transition` | 持久任务工件、审批门、ready/running/completed 状态机 | 仅推进已批准状态；不将计划视作执行外部动作的授权。 |
| 59 | `scene/orchestrate` | 从规格到资产、世界、玩法、验证、打包、发布证据的 12 阶段 dry-run 编排 | 不在一次请求中未经审批创建/覆盖完整项目内容。 |
| 60 | `capabilities/coverage` | 引擎版本、启用插件数量及跨域兼容性覆盖矩阵 | 注册表表示能力合同，不代表对每个插件或外部系统拥有无限权限。 |

## 统一验收通过标准

1. UE 5.7.4 BuildPlugin 完整编译，MCP bridge 显式暴露工程接管工具；
2. #48–#60 每项接口返回结构化证据，计划工件可在本地读取；
3. 高影响操作均显示 dry-run、审批门或只读边界；
4. Agent 状态机可创建持久计划并经过审批转为 `ready`；
5. 保存 JSON 统一验收报告，测试编辑器退出并清理本阶段临时目录。

## 最终完成记录（2026-10-05）

- 最终插件构建：`UE5AIMcp-engineering-batch1`，已部署至 `D:\xuexishiyong\UE5AI\UE5AIMcp\Binaries\Win64`。
- 最终验收：在 `AI插件测试项目`（UE 5.7.4）执行 `Run-EngineeringTakeoverUnifiedTest.ps1`，报告为 `passed: true`，18/18 项检查通过，覆盖 #48–#60。
- 安全结论：网络、打包、部署、版本控制、外部 CI 和插件写入均已实现可验证的计划/状态能力；默认保持 dry-run、只读或审批门，不将规划接口视为对高影响外部操作的授权。
- 收尾：测试编辑器已退出；`Saved/UE5AI` 与本阶段 BuildPlugin 输出已删除。最终插件二进制、源代码、统一测试脚本、验收报告和本文件均保留。
