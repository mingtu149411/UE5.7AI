# MVP（#1–#25）实施状态

> 项目作者水印：南京大学24届沈逸文

## 最终状态（2026-10-05）

**MVP #1–#25：25 / 25 已实现并通过一次完整统一验收。**

- 测试项目：`D:\xuexishiyong\ue--myproject\AI插件测试项目`（UE 5.7.4）。
- 最终报告：`tests/MVP_FINAL_UNIFIED_TEST_RESULT.json`，`passed: true`。
- 最终验收覆盖本机标准 MCP stdio bridge、读取、场景编辑、材质、导入/重导入、资产摘要与校验、截图比对、PIE 以及自动化发现/执行/报告。
- 资产校验器同时发现测试工程既有的 18 条缺失示例资源引用（Mannequin/MetaHuman/ControlRig/DemoTemplate）；它们并非本插件或本次测试创建。接口将其作为诊断 `issues` 返回，不会误报为扫描执行失败。

## 阶段 A：连接与只读基础

- [x] #1 MCP/本地桥接服务身份与健康检查（本机 HTTP 执行端 + 标准 MCP stdio bridge）。
- [x] #2 项目识别（项目目录、UE 版本）。
- [x] #3 编辑器/PIE 状态读取。
- [x] #4 当前视口截图（等待实际文件写入，必要时回退到最近有效帧）。
- [x] #5 编辑器日志的受限读取。
- [x] #6 Asset Registry 摘要读取。
- [x] #7 World/Actor 层级和 Transform 预览。
- [x] #8 任意授权 UObject 的反射属性读取。
- [x] #9 CVar/控制台命令检索。
- [x] #10 汇总诊断报告。

## 阶段 B：安全编辑与内容操作

- [x] #11 保存、撤销/重做检查点。
- [x] #12 Actor 基础 CRUD（创建/删除由事务保护）。
- [x] #13 Actor 基础属性编辑（Transform 与 Tag，事务保护）。
- [x] #14 灯光、相机与后处理基础编辑。
- [x] #15 文件夹、Tag、编辑器 Layer 与 Data Layer 绑定标记管理。
- [x] #16 静态网格灰盒布局。
- [x] #17 材质实例创建和标量参数编辑。
- [x] #18 自动导入与重导入。
- [x] #19 Blueprint 图只读导出。
- [x] #20 Material Graph 只读导出。
- [x] #21 Niagara、动画、Sequencer 通用资产摘要（已以 Level Sequence 验收）。
- [x] #22 资产引用完整性检查与问题列表输出。

## 阶段 C：验证与运行时反馈

- [x] #23 视口视觉验证循环（截图 SHA-1 证据 + 客户端语义审阅路径）。
- [x] #24 启动/停止 PIE 与运行日志采集。
- [x] #25 自动化测试发现、执行与报告。

## 阶段 D：标准 MCP 客户端接入

- [x] 建立标准 MCP stdio bridge，将 AI 工具调用安全转发到本地 UE 执行端。
- [x] 为 Codex 提供最小工具白名单与明确的本机回环边界。

所有勾选均须以 `tests/` 中针对“AI插件测试项目”的可复现证据为前提。

## 当前准确进度

**100%（25 / 25）**。后续阶段应在此稳定 MVP 之上扩展具体 Niagara、动画、Control Rig、蓝图写入与材质图写入操作。
