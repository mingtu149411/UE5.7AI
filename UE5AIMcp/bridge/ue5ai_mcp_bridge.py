#!/usr/bin/env python3
# 项目作者水印：南京大学24届沈逸文
"""Minimal stdio MCP bridge for the local UE5AIMcp editor plugin.

No sockets are opened by this process. It reads JSON-RPC messages from stdin
and forwards an explicit, auditable whitelist to 127.0.0.1:8175.
"""
from __future__ import annotations

import json
import sys
import urllib.error
import urllib.parse
import urllib.request

BASE_URL = "http://127.0.0.1:8175"

TOOLS = {
    "ue5_health": ("GET", "/ue5ai/health", "Read local UE bridge health."),
    "ue5_snapshot": ("GET", "/ue5ai/snapshot", "Read project, world, actor and Asset Registry summary."),
    "ue5_tools": ("GET", "/ue5ai/tools", "List UE bridge capability contracts."),
    "ue5_properties": ("GET", "/ue5ai/properties", "Inspect editable UObject properties."),
    "ue5_cvars": ("GET", "/ue5ai/cvars", "Search console variables."),
    "ue5_logs": ("GET", "/ue5ai/logs", "Read capped editor log buffer."),
    "ue5_diagnostics": ("GET", "/ue5ai/diagnostics", "Read editor diagnostics."),
    "ue5_capture_viewport": ("POST", "/ue5ai/viewport/screenshot", "Request a viewport screenshot."),
    "ue5_verify_viewport": ("GET", "/ue5ai/viewport/verify", "Compare two captured viewport PNGs by content digest."),
    "ue5_actor_spawn": ("POST", "/ue5ai/actors/spawn", "Create a transaction-backed static-mesh Actor."),
    "ue5_actor_transform": ("PATCH", "/ue5ai/actors/transform", "Set an Actor transform and optional tag."),
    "ue5_actor_organize": ("PATCH", "/ue5ai/actors/organize", "Set Actor folder, tag, editor layer and Data Layer binding tag."),
    "ue5_actor_delete": ("DELETE", "/ue5ai/actors", "Delete an Actor."),
    "ue5_grid": ("POST", "/ue5ai/layout/grid", "Create a static-mesh graybox grid."),
    "ue5_light": ("POST", "/ue5ai/lights/point", "Create a point light."),
    "ue5_camera_postprocess": ("POST", "/ue5ai/camera/postprocess", "Create a camera and unbound post-process volume."),
    "ue5_material_instance": ("POST", "/ue5ai/material/instance", "Create a material instance and edit a scalar parameter."),
    "ue5_import": ("POST", "/ue5ai/assets/import", "Automated import from an absolute local source path."),
    "ue5_reimport": ("POST", "/ue5ai/assets/reimport", "Reimport an asset using its recorded source."),
    "ue5_blueprint_summary": ("GET", "/ue5ai/blueprint/summary", "Read Blueprint graph names."),
    "ue5_material_summary": ("GET", "/ue5ai/material/summary", "Read Material graph nodes."),
    "ue5_sequence_create": ("POST", "/ue5ai/sequence/create", "Create a disposable Level Sequence for Sequencer integration workflows."),
    "ue5_sequence_summary": ("GET", "/ue5ai/sequence/summary", "Read Niagara, animation or Sequencer asset metadata."),
    "ue5_asset_class_summary": ("GET", "/ue5ai/assets/class-summary", "List assets for a UE asset class."),
    "ue5_validate_assets": ("GET", "/ue5ai/assets/validate", "Check invalid assets and missing package dependencies."),
    "ue5_undo": ("POST", "/ue5ai/transaction/undo", "Undo the last editor transaction."),
    "ue5_redo": ("POST", "/ue5ai/transaction/redo", "Redo the last editor transaction."),
    "ue5_save_dirty": ("POST", "/ue5ai/project/save-dirty", "Save dirty map/content packages."),
    "ue5_pie_start": ("POST", "/ue5ai/pie/start", "Start Play In Editor."),
    "ue5_pie_stop": ("POST", "/ue5ai/pie/stop", "Stop Play In Editor."),
    "ue5_automation_discover": ("GET", "/ue5ai/automation/discover", "Discover UE automation tests."),
    "ue5_automation_run": ("POST", "/ue5ai/automation/run", "Request an UE automation test run."),
    "ue5_automation_report": ("GET", "/ue5ai/automation/report", "Read captured automation result logs."),
    "ue5_performance_snapshot": ("GET", "/ue5ai/performance/snapshot", "Capture frame, draw-call and memory evidence."),
    "ue5_cvar_set": ("PATCH", "/ue5ai/settings/cvar", "Set a runtime CVar with rollback evidence."),
    "ue5_cvar_rollback": ("POST", "/ue5ai/settings/cvar/rollback", "Restore runtime CVars changed by this bridge."),
    "ue5_world_partition": ("GET", "/ue5ai/world/partition", "Read World Partition, HLOD and Level Instance capability."),
    "ue5_level_instance": ("POST", "/ue5ai/world/level-instance", "Create a Level Instance audit binding."),
    "ue5_landscape_create": ("POST", "/ue5ai/landscape/create", "Create a bounded procedural Landscape height field."),
    "ue5_scatter_instances": ("POST", "/ue5ai/scatter/instances", "Create deterministic instanced foliage/PCG-style scatter."),
    "ue5_data_create": ("POST", "/ue5ai/data/create", "Create Curve, Data Table or Data Asset scaffolding."),
    "ue5_widget_create": ("POST", "/ue5ai/ui/widget", "Create a UMG menu/widget hierarchy with automation-stable names."),
    "ue5_input_mapping": ("POST", "/ue5ai/input/mapping", "Create Enhanced Input Action and Mapping Context assets."),
    "ue5_blueprint_create": ("POST", "/ue5ai/blueprint/create", "Create a safe Blueprint class and member variable."),
    "ue5_cpp_scaffold": ("POST", "/ue5ai/cpp/scaffold", "Preview or explicitly write an audited C++ UObject scaffold."),
    "ue5_dependency_graph": ("GET", "/ue5ai/analysis/dependencies", "Read one-hop asset dependencies and referencers."),
    "ue5_gameplay_scaffold": ("POST", "/ue5ai/gameplay/scaffold", "Create a Character, Pawn or Controller foundation Blueprint."),
    "ue5_gas_scaffold": ("POST", "/ue5ai/gas/scaffold", "Create a base Gameplay Effect when GAS is enabled."),
    "ue5_material_graph": ("POST", "/ue5ai/material/graph", "Create a PBR material graph with exposed roughness."),
    "ue5_procedural_mesh": ("POST", "/ue5ai/model/procedural-box", "Create a collision-enabled procedural mesh actor."),
    "ue5_dcc_job": ("POST", "/ue5ai/dcc/job", "Write an external DCC generation/import hand-off manifest."),
    "ue5_animation_montage": ("POST", "/ue5ai/animation/montage", "Create a skeleton-bound Montage scaffold."),
    "ue5_animation_validate": ("GET", "/ue5ai/animation/validate", "Validate an animation asset skeleton binding."),
    "ue5_sequence_add_float_track": ("POST", "/ue5ai/sequence/add-float-track", "Add a keyed float track to a Level Sequence."),
    "ue5_niagara_scaffold": ("POST", "/ue5ai/niagara/scaffold", "Create an empty Niagara System when Niagara is enabled."),
    "ue5_audio_cue": ("POST", "/ue5ai/audio/cue", "Create a Sound Cue and optionally route a Sound Wave."),
    "ue5_network_replication": ("GET", "/ue5ai/network/replication", "Inspect replication topology and multiplayer regression requirements."),
    "ue5_network_session_plan": ("POST", "/ue5ai/network/session-plan", "Create an approval-gated server/client session test plan."),
    "ue5_platform_profile": ("GET", "/ue5ai/platform/profile", "Inspect platform configuration files and enabled plugins."),
    "ue5_pipeline_plan": ("POST", "/ue5ai/pipeline/plan", "Generate an auditable UBT/UAT BuildCookRun plan without executing it."),
    "ue5_budget_audit": ("GET", "/ue5ai/quality/budget", "Read asset, draw-call and memory budget evidence."),
    "ue5_compliance_audit": ("GET", "/ue5ai/compliance/audit", "Generate localization, accessibility and compliance readiness checks."),
    "ue5_source_control_status": ("GET", "/ue5ai/source-control/status", "Inspect repository/provider availability without committing changes."),
    "ue5_cicd_plan": ("POST", "/ue5ai/cicd/plan", "Write a CI/CD validation and artifact-archive plan."),
    "ue5_crash_diagnostics": ("GET", "/ue5ai/crash/diagnostics", "Inventory crash artifacts and return a reproducibility workflow."),
    "ue5_plugin_scaffold": ("POST", "/ue5ai/plugin/scaffold", "Preview or explicitly create a new editor plugin descriptor."),
    "ue5_editor_adapters": ("GET", "/ue5ai/editor/adapters", "List installed editor-domain adapter modules without loading optional modules."),
    "ue5_agent_plan": ("POST", "/ue5ai/agent/plan", "Create a persisted approval-gated long-running task state machine."),
    "ue5_agent_transition": ("PATCH", "/ue5ai/agent/transition", "Advance an approved agent task through bounded lifecycle states."),
    "ue5_scene_orchestrate": ("POST", "/ue5ai/scene/orchestrate", "Create a dry-run from-zero scene production orchestration plan."),
    "ue5_capability_coverage": ("GET", "/ue5ai/capabilities/coverage", "Read engineering takeover coverage and installed-plugin compatibility scope."),
}


def call_ue(method: str, path: str, arguments: dict) -> dict:
    query = urllib.parse.urlencode({key: str(value) for key, value in arguments.items() if value is not None})
    url = BASE_URL + path + ("?" + query if query else "")
    request = urllib.request.Request(url, method=method)
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            return json.loads(response.read().decode("utf-8"))
    except (urllib.error.URLError, json.JSONDecodeError) as exc:
        return {"ok": False, "error": f"UE bridge request failed: {exc}"}


def result(request_id, payload):
    return {"jsonrpc": "2.0", "id": request_id, "result": payload}


def error(request_id, code, message):
    return {"jsonrpc": "2.0", "id": request_id, "error": {"code": code, "message": message}}


def handle(message: dict) -> dict | None:
    method = message.get("method")
    request_id = message.get("id")
    if method == "notifications/initialized":
        return None
    if method == "initialize":
        return result(request_id, {"protocolVersion": "2025-03-26", "capabilities": {"tools": {}}, "serverInfo": {"name": "ue5ai-bridge", "version": "0.1.0"}})
    if method == "tools/list":
        tools = [{"name": name, "description": spec[2], "inputSchema": {"type": "object", "additionalProperties": True}} for name, spec in TOOLS.items()]
        return result(request_id, {"tools": tools})
    if method == "tools/call":
        params = message.get("params", {})
        name = params.get("name")
        if name not in TOOLS:
            return error(request_id, -32602, f"Unknown UE tool: {name}")
        verb, path, _ = TOOLS[name]
        payload = call_ue(verb, path, params.get("arguments", {}))
        return result(request_id, {"content": [{"type": "text", "text": json.dumps(payload, ensure_ascii=False)}], "isError": not bool(payload.get("ok", False))})
    return error(request_id, -32601, f"Unsupported method: {method}")


for line in sys.stdin:
    try:
        response = handle(json.loads(line))
        if response is not None:
            print(json.dumps(response, ensure_ascii=False), flush=True)
    except Exception as exc:
        print(json.dumps(error(None, -32603, f"Bridge internal error: {exc}")), flush=True)
