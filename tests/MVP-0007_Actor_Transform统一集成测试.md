# MVP-0007：Actor Transform 统一集成测试

插件批次：`r10-fix`；测试项目：`AI插件测试项目`。

单次编辑器实例内完成：创建 StaticMeshActor → Transform/Tag 编辑 → 快照 → 删除 → 日志/截图 → 自动退出。

| 检查 | 结果 |
|---|---|
| 初始/最终 Actor 数量 | 144 / 144 |
| 创建后 Actor 数量 | 145 |
| Transform 请求 | 成功 |
| 实际 Translation | `(120, 240, 360)` |
| 实际 Rotation | `(Pitch 10, Yaw 20, Roll 30)` |
| 实际 Scale | `(1, 2, 3)` |
| 删除、日志、截图、自动退出 | 全部成功 |

此前基础 `AActor` 无根组件造成 Transform 未生效的问题已由 `AStaticMeshActor` 修正。
