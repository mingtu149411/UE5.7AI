# UE5.7AI

> **项目作者：南京大学24届沈逸文**
> 面向 Unreal Engine 5.7 的本地 AI/MCP 编辑器自动化插件与工程资料。

本仓库包含 `UE5AIMcp` 编辑器插件、MCP stdio 桥接器、技术文档、开发过程报告、使用说明、功能路线图和统一测试记录。插件以本机回环接口连接支持 MCP 的 AI 客户端，并通过明确的工具白名单执行 Unreal Editor 自动化操作。

## 主要内容

- [`UE5AIMcp/`](UE5AIMcp)：UE 5.7 编辑器插件、C++ 源码、配置、Python MCP 桥接器与插件文档；
- [`UE5AIMcp/Docs/`](UE5AIMcp/Docs)：技术文档、开发过程报告、插件使用说明；
- [`tests/`](tests)：统一测试记录与相关测试资料；
- 根目录规划文档：MVP、生产内容、工程接管阶段的实施与验证计划，以及完整功能路线图；
- [`PROJECT_AUTHOR_WATERMARK.md`](PROJECT_AUTHOR_WATERMARK.md)：作者水印的可审阅声明。

## 构建与使用

目标引擎为 **Unreal Engine 5.7**。启用插件、配置 MCP 客户端和运行草地展示关卡的具体步骤见 [插件使用说明](UE5AIMcp/Docs/UE5AI_MCP_插件使用说明.md)。架构与能力边界见 [技术文档](UE5AIMcp/Docs/UE5AI_MCP_技术文档.md)。

## 版本库范围

本仓库保留可复现项目所需的源码、配置、文档、测试记录以及当前可用的插件 DLL。`build/`、`_build/`、`Intermediate/`、`Saved/`、缓存和 `.pdb` 调试符号都是可再生文件，且含有多 GB 的临时 PCH/构建输出，已通过 `.gitignore` 排除，不影响从源码重新构建插件。

二进制 Unreal 资产不被文本化改写；项目署名通过插件清单、配置、源码注释、项目元数据与文档保留，以保障 `.uasset`、`.umap` 与 DLL 的可加载性。
