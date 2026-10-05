# 项目作者水印

> **项目作者：南京大学24届沈逸文**

本水印适用于 `UE5AI` 中的 UE5AI MCP 插件、技术文档、开发过程报告、使用说明，以及用于验证的“AI插件测试项目”。

已写入的位置：

- 插件清单 `UE5AIMcp/UE5AIMcp.uplugin` 的 `CreatedBy` 与说明字段；
- 插件配置 `UE5AIMcp/Config/DefaultUE5AIMcp.ini`；
- 测试项目 `Config/DefaultGame.ini` 的公司名与版权声明；
- 本项目新建的文档与后续新增的源码文件头部。

UE 的 `.uasset`、`.umap`、`.dll` 等二进制文件不应由文本工具逐个写入水印，否则会破坏资源序列化或二进制签名。对这些文件，作者信息通过其所属项目配置、插件清单和文档元数据进行可追溯标识。
