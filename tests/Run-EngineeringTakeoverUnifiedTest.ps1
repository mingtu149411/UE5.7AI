[CmdletBinding()]
param(
    [string]$BaseUrl = 'http://127.0.0.1:8175',
    [string]$ResultPath = 'D:\xuexishiyong\UE5AI\tests\ENGINEERING_TAKEOVER_FINAL_UNIFIED_TEST_RESULT.json'
)

$ErrorActionPreference = 'Stop'
$result = [ordered]@{
    name = 'UE5AI engineering takeover #48-#60 unified test'
    timestamp_utc = [DateTime]::UtcNow.ToString('o')
    passed = $false
    checks = [System.Collections.Generic.List[object]]::new()
    failures = [System.Collections.Generic.List[string]]::new()
}

function Add-Check([string]$Name, [bool]$Passed, [string]$Detail) {
    $script:result.checks.Add([ordered]@{ name = $Name; passed = $Passed; detail = $Detail })
    if (-not $Passed) { throw "${Name}: $Detail" }
}

function Invoke-Ue([string]$Method, [string]$Route, [hashtable]$Query = @{}) {
    $parts = foreach ($entry in $Query.GetEnumerator()) {
        '{0}={1}' -f [Uri]::EscapeDataString([string]$entry.Key), [Uri]::EscapeDataString([string]$entry.Value)
    }
    $uri = "$BaseUrl$Route"
    if ($parts.Count -gt 0) { $uri += '?' + ($parts -join '&') }
    $response = Invoke-WebRequest -Uri $uri -Method $Method -UseBasicParsing -TimeoutSec 60
    return ($response.Content | ConvertFrom-Json)
}

function Test-EngineeringBridge {
    $bridge = 'D:\xuexishiyong\UE5AI\UE5AIMcp\bridge\ue5ai_mcp_bridge.py'
    $requests = @(
        '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-03-26","capabilities":{},"clientInfo":{"name":"EngineeringTakeoverTest","version":"1"}}}',
        '{"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}}'
    ) -join [Environment]::NewLine
    $output = $requests | & python $bridge
    $lines = @($output | Where-Object { $_ -and $_.Trim() })
    if ($LASTEXITCODE -ne 0 -or $lines.Count -ne 2) { return $false }
    $tools = ($lines[1] | ConvertFrom-Json).result.tools.name
    $required = @('ue5_network_replication', 'ue5_pipeline_plan', 'ue5_budget_audit', 'ue5_agent_plan', 'ue5_scene_orchestrate', 'ue5_capability_coverage')
    return (@($required | Where-Object { $tools -notcontains $_ }).Count -eq 0)
}

try {
    $health = Invoke-Ue GET '/ue5ai/health'; Add-Check 'preflight engineering plugin loaded' $health.ok $health.engine
    Add-Check 'MCP bridge exposes engineering tools' (Test-EngineeringBridge) 'engineering tool names must be discoverable over standard MCP'
    $tools = Invoke-Ue GET '/ue5ai/tools'; Add-Check 'engineering tool contracts listed' ($tools.ok -and $tools.tools.Count -ge 40) ("count={0}" -f $tools.tools.Count)

    # #48
    $network = Invoke-Ue GET '/ue5ai/network/replication'; Add-Check '#48 replication topology' ($network.ok -and $null -ne $network.net_mode -and $null -ne $network.replicated_actor_count) $network.world
    $sessionPlan = Invoke-Ue POST '/ue5ai/network/session-plan' @{ mode = 'listen'; clients = 2 }; Add-Check '#48 multiplayer session plan' ($sessionPlan.ok -and $sessionPlan.approval_required -and (Test-Path -LiteralPath $sessionPlan.plan) -and $sessionPlan.clients -eq 2) $sessionPlan.plan

    # #49 - #52
    $platform = Invoke-Ue GET '/ue5ai/platform/profile'; Add-Check '#49 platform/plugin profile' ($platform.ok -and $platform.enabled_plugin_count -gt 0 -and $platform.platforms.Count -ge 4) $platform.config_dir
    $pipeline = Invoke-Ue POST '/ue5ai/pipeline/plan' @{ platform = 'Win64'; configuration = 'Development' }; Add-Check '#50 cook/package/deploy plan' ($pipeline.ok -and $pipeline.dry_run -and (Test-Path -LiteralPath $pipeline.plan) -and $pipeline.command -match 'BuildCookRun') $pipeline.plan
    $budget = Invoke-Ue GET '/ue5ai/quality/budget'; Add-Check '#51 quality and memory budget audit' ($budget.ok -and $budget.asset_count -gt 0 -and $null -ne $budget.draw_calls -and $budget.recommendations.Count -ge 4) ("assets={0}, draw={1}" -f $budget.asset_count,$budget.draw_calls)
    $compliance = Invoke-Ue GET '/ue5ai/compliance/audit'; Add-Check '#52 localization/accessibility/compliance baseline' ($compliance.ok -and $compliance.checks.Count -ge 4 -and $compliance.status -match 'no localization') $compliance.status

    # #53 - #57
    $sourceControl = Invoke-Ue GET '/ue5ai/source-control/status'; Add-Check '#53 source-control collaboration status' ($sourceControl.ok -and $sourceControl.safe_operations.Count -ge 4 -and $sourceControl.boundary -match 'never commits') $sourceControl.provider
    $ci = Invoke-Ue POST '/ue5ai/cicd/plan' @{ provider = 'local' }; Add-Check '#54 CI/CD plan' ($ci.ok -and -not $ci.external_dispatch -and (Test-Path -LiteralPath $ci.plan)) $ci.plan
    $crashes = Invoke-Ue GET '/ue5ai/crash/diagnostics'; Add-Check '#55 crash diagnostics workflow' ($crashes.ok -and $null -ne $crashes.crash_folder_count -and $crashes.reproduction_workflow.Count -ge 4) $crashes.crash_root
    $plugin = Invoke-Ue POST '/ue5ai/plugin/scaffold' @{ name = 'UE5AITakeoverPreview'; write_project = 'false' }; Add-Check '#56 plugin scaffold preview' ($plugin.ok -and -not $plugin.wrote_project_plugin -and $plugin.descriptor_preview -match 'UE5AITakeoverPreview') $plugin.status
    $adapters = Invoke-Ue GET '/ue5ai/editor/adapters'; Add-Check '#57 editor-domain adapters' ($adapters.ok -and $adapters.adapters.Count -ge 8 -and $adapters.compatibility -match 'without loading') ("adapters={0}" -f $adapters.adapters.Count)

    # #58 - #60
    $agent = Invoke-Ue POST '/ue5ai/agent/plan' @{ objective = 'Validate approval-gated engineering takeover lifecycle.' }; Add-Check '#58 recoverable autonomous agent plan' ($agent.ok -and $agent.state -eq 'awaiting_approval' -and (Test-Path -LiteralPath $agent.artifact)) $agent.artifact
    $agentTransition = Invoke-Ue PATCH '/ue5ai/agent/transition' @{ task = $agent.task_id; action = 'approve' }; Add-Check '#58 approval-gated task transition' ($agentTransition.ok -and $agentTransition.state -eq 'ready' -and -not $agentTransition.external_actions_started) $agentTransition.state
    $scene = Invoke-Ue POST '/ue5ai/scene/orchestrate' @{ spec = 'A small network-ready accessible third-person test scene.' }; Add-Check '#59 from-zero scene orchestration' ($scene.ok -and $scene.dry_run -and $scene.phase_count -ge 12 -and (Test-Path -LiteralPath $scene.plan)) $scene.plan
    $coverage = Invoke-Ue GET '/ue5ai/capabilities/coverage'; Add-Check '#60 capability coverage matrix' ($coverage.ok -and $coverage.coverage_stage -eq 'engineering_takeover' -and $coverage.covered_domain_count -ge 8 -and $coverage.enabled_plugin_count -gt 0) $coverage.engine

    $result.passed = $true
}
catch {
    $result.failures.Add($_.Exception.Message)
}
finally {
    try { [void](Invoke-Ue POST '/ue5ai/test-shutdown') } catch {}
    $result | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath $ResultPath -Encoding utf8
}

if (-not $result.passed) { exit 1 }
