# MVP-0006：Actor CRUD 统一集成测试

- **测试项目：** `D:\xuexishiyong\ue--myproject\AI插件测试项目`
- **插件批次：** r9-fix
- **方式：** 单次启动、端口就绪轮询、原子化统一回归、最后自动退出。

## 断言结果

| 断言 | 结果 |
|---|---|
| 初始 Actor 数量 | 144 |
| `POST /actors/spawn` | 成功 |
| 创建后 Actor 数量 | 145 |
| `DELETE /actors?object=...` | 成功 |
| 删除后 Actor 数量 | 144 |
| `/logs` | 成功，300 行环形缓冲 |
| `/viewport/screenshot` | 成功，截图文件存在 |
| `/test-shutdown` | 成功，测试 UE 进程已退出 |

创建和删除均在编辑器 Game Thread 上执行，并由 `FScopedTransaction` 包裹。
