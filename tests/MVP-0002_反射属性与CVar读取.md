# MVP-0002：反射属性与 CVar 读取

- **测试项目：** `D:\xuexishiyong\ue--myproject\AI插件测试项目`
- **引擎：** UE 5.7.4
- **插件：** `UE5AIMcp`（r3 构建）

## 结果

1. 请求 `GET /ue5ai/cvars?filter=r.ScreenPercentage`：成功，返回 9 个匹配 CVar，包含名称、帮助与当前值。
2. 请求 `GET /ue5ai/properties?object=/Script/Engine.Default__WorldSettings`：成功，返回 `WorldSettings` 的 109 个可编辑反射属性。

两项端点均为只读；不存在任意 Python/控制台执行入口。
