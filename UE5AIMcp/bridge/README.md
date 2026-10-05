# UE5 AI MCP stdio bridge

> 项目作者水印：南京大学24届沈逸文

`ue5ai_mcp_bridge.py` is a standard-MCP JSON-RPC stdio adapter for the local UE Editor plugin. It deliberately has no general URL forwarding tool: every UE operation must be named in its source-code whitelist.

Example Codex MCP configuration:

```json
{
  "mcpServers": {
    "ue5ai": {
      "command": "python",
      "args": ["D:/xuexishiyong/UE5AI/UE5AIMcp/bridge/ue5ai_mcp_bridge.py"]
    }
  }
}
```

Start the target UE project with `UE5AIMcp` enabled before calling a tool. The bridge only connects to `127.0.0.1:8175`; it does not expose UE on the LAN.
