# TouchDesigner MCP Local Setup

Date: 2026-08-07

Installed locally from:

- Repository: https://github.com/8beeeaaat/touchdesigner-mcp
- Release: v2.0.0

Local files:

- TouchDesigner component folder:
  `C:\Users\jimmy\Documents\~ pROJECTs ~\Schrank-LED\tools\touchdesigner-mcp\touchdesigner-mcp-td-v2.0.0`
- Component to import into TouchDesigner:
  `C:\Users\jimmy\Documents\~ pROJECTs ~\Schrank-LED\tools\touchdesigner-mcp\touchdesigner-mcp-td-v2.0.0\mcp_webserver_base.tox`
- Required sibling folder:
  `C:\Users\jimmy\Documents\~ pROJECTs ~\Schrank-LED\tools\touchdesigner-mcp\touchdesigner-mcp-td-v2.0.0\modules`

Codex MCP config:

- File: `C:\Users\jimmy\.codex\config.toml`
- Backup before edit:
  `C:\Users\jimmy\.codex\config.toml.bak-touchdesigner-mcp-20260807-221749`
- Added:

```toml
[mcp_servers.touchdesigner]
command = "npx"
args = ["-y", "touchdesigner-mcp-server@latest", "--stdio"]
```

Verification already completed:

- Node.js: v24.13.0
- npm/npx: 11.6.2
- `npx -y touchdesigner-mcp-server@latest --stdio` starts successfully.
- TouchDesigner process is running.

Still required inside TouchDesigner:

1. Import `mcp_webserver_base.tox` into the current TouchDesigner project.
2. Place it at `/project1/mcp_webserver_base` if possible.
3. Keep `mcp_webserver_base.tox`, `import_modules.py`, and `modules/` together in the same folder structure.
4. Check TouchDesigner Textport for successful startup.
5. Confirm `http://127.0.0.1:9981` responds.
6. Restart Codex so the new MCP server entry is loaded.

