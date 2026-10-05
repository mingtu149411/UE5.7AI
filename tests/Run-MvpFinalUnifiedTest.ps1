[CmdletBinding()]
param(
    [string]$BaseUrl = 'http://127.0.0.1:8175',
    [string]$ResultPath = 'D:\xuexishiyong\UE5AI\tests\MVP_FINAL_UNIFIED_TEST_RESULT.json'
)

$ErrorActionPreference = 'Stop'
$result = [ordered]@{
    name = 'UE5AI MVP #1-#25 final unified test'
    timestamp_utc = [DateTime]::UtcNow.ToString('o')
    passed = $false
    checks = [System.Collections.Generic.List[object]]::new()
    failures = [System.Collections.Generic.List[string]]::new()
}
$cleanupActors = [System.Collections.Generic.List[string]]::new()

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
    $response = Invoke-WebRequest -Uri $uri -Method $Method -UseBasicParsing -TimeoutSec 45
    return ($response.Content | ConvertFrom-Json)
}

function Wait-ForFile([string]$Path, [int]$TimeoutSeconds = 20) {
    $until = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $until) {
        if (Test-Path -LiteralPath $Path) { return $true }
        Start-Sleep -Milliseconds 500
    }
    return $false
}

function Test-Bridge {
    $bridge = 'D:\xuexishiyong\UE5AI\UE5AIMcp\bridge\ue5ai_mcp_bridge.py'
    $requests = @(
        '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-03-26","capabilities":{},"clientInfo":{"name":"MVPTest","version":"1"}}}',
        '{"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}}',
        '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"ue5_health","arguments":{}}}'
    ) -join [Environment]::NewLine
    $output = $requests | & python $bridge
    $lines = @($output | Where-Object { $_ -and $_.Trim() })
    if ($LASTEXITCODE -ne 0 -or $lines.Count -ne 3) { return $false }
    $init = $lines[0] | ConvertFrom-Json
    $tools = $lines[1] | ConvertFrom-Json
    $health = $lines[2] | ConvertFrom-Json
    return $init.result.serverInfo.name -eq 'ue5ai-bridge' -and $tools.result.tools.Count -ge 25 -and $health.result.isError -eq $false
}

try {
    # #1-#10: connection, bridge, project/editor/world, logs, screenshot, registry,
    # reflection, cvars and diagnostics.
    $health = Invoke-Ue GET '/ue5ai/health'; Add-Check '#1 local execution endpoint' ($health.ok -and $health.service -eq 'ue5-ai-mcp') ($health.engine)
    Add-Check '#1 standard MCP stdio bridge' (Test-Bridge) 'initialize, tools/list and ue5_health must all succeed'
    $tools = Invoke-Ue GET '/ue5ai/tools'; Add-Check '#1 tool contract list' ($tools.ok -and $tools.tools.Count -ge 8) 'tool contracts returned'
    $snapshot = Invoke-Ue GET '/ue5ai/snapshot'; Add-Check '#2 project identity' ($snapshot.ok -and $snapshot.project_dir -match 'AI插件测试项目') ($snapshot.project_dir)
    Add-Check '#3 editor state' ($null -ne $snapshot.is_pie -and $snapshot.world) ($snapshot.world)
    Add-Check '#6 asset registry' ($snapshot.asset_count -gt 0) ("asset_count={0}" -f $snapshot.asset_count)
    Add-Check '#7 world hierarchy' ($snapshot.actor_count -ge 0 -and $null -ne $snapshot.actors_preview) ("actor_count={0}" -f $snapshot.actor_count)
    $propertyTarget = $snapshot.actors_preview[0].path
    $properties = Invoke-Ue GET '/ue5ai/properties' @{ object = $propertyTarget }; Add-Check '#8 reflection properties' ($properties.ok -and $properties.properties.Count -gt 0) $propertyTarget
    $cvars = Invoke-Ue GET '/ue5ai/cvars' @{ filter = 'r.' }; Add-Check '#9 CVar search' ($cvars.ok -and $cvars.cvars.Count -gt 0) ("count={0}" -f $cvars.cvars.Count)
    $diagnostics = Invoke-Ue GET '/ue5ai/diagnostics'; Add-Check '#10 diagnostics' ($diagnostics.ok -and $diagnostics.has_world) $diagnostics.project
    $logs = Invoke-Ue GET '/ue5ai/logs'; Add-Check '#5 log capture' ($logs.ok -and $null -ne $logs.lines) ("lines={0}" -f $logs.lines.Count)
    $capture1 = Invoke-Ue POST '/ue5ai/viewport/screenshot'; Add-Check '#4 screenshot request' ($capture1.ok -and (Wait-ForFile $capture1.path)) $capture1.path

    # #11-#16: transactions, actor editing, organization and grayboxing.
    $actor = Invoke-Ue POST '/ue5ai/actors/spawn' @{ label = 'UE5AI_FinalTest_Actor' }; Add-Check '#12 actor create' $actor.ok $actor.path; $cleanupActors.Add($actor.path)
    $transform = Invoke-Ue PATCH '/ue5ai/actors/transform' @{ object = $actor.path; x = 120; y = 240; z = 360; pitch = 10; yaw = 20; roll = 30; sx = 1; sy = 2; sz = 3; tag = 'UE5AI_FinalTest' }; Add-Check '#13 actor transform/tag' ($transform.ok -and $transform.transform -match '120') $transform.transform
    $organize = Invoke-Ue PATCH '/ue5ai/actors/organize' @{ object = $actor.path; folder = 'UE5AI/FinalTest'; tag = 'UE5AI_Organized'; layer = 'UE5AI_EditorLayer'; data_layer = 'UE5AI_TestDataLayer' }; Add-Check '#15 folder/tag/layer binding' ($organize.ok -and $organize.folder -eq 'UE5AI/FinalTest') $organize.folder
    $undo = Invoke-Ue POST '/ue5ai/transaction/undo'; $redo = Invoke-Ue POST '/ue5ai/transaction/redo'; Add-Check '#11 undo/redo checkpoint' ($undo.ok -and $redo.ok) 'transaction endpoints succeeded'
    $light = Invoke-Ue POST '/ue5ai/lights/point' @{ intensity = 2500; x = 500; y = 0; z = 300 }; Add-Check '#14 point light' ($light.ok -and $light.intensity -eq 2500) $light.path; $cleanupActors.Add($light.path)
    $camera = Invoke-Ue POST '/ue5ai/camera/postprocess' @{ x = 0; y = -500; z = 250; fov = 80; exposure = 1.25 }; Add-Check '#14 camera/post-process' ($camera.ok -and $camera.fov -eq 80) $camera.camera; $cleanupActors.Add($camera.camera); $cleanupActors.Add($camera.postprocess)
    $grid = Invoke-Ue POST '/ue5ai/layout/grid' @{ rows = 2; cols = 3; spacing = 150; folder = 'UE5AI/FinalGrid' }; Add-Check '#16 static-mesh graybox grid' ($grid.ok -and $grid.count -eq 6) ("count={0}" -f $grid.count); foreach ($path in $grid.paths) { $cleanupActors.Add([string]$path) }

    # #17-#22: content/asset workflow and readback.
    $material = Invoke-Ue POST '/ue5ai/material/instance' @{ parent = '/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial'; folder = '/Game/UE5AI_AutoTmp'; name = 'MI_UE5AI_Final'; scalar = 'UE5AI_TestScalar'; value = 0.75 }; Add-Check '#17 material instance' $material.ok $material.path
    $fixture = 'D:\xuexishiyong\UE5AI\tests\fixtures\UE5AI_Triangle.obj'
    $import = Invoke-Ue POST '/ue5ai/assets/import' @{ file = $fixture; destination = '/Game/UE5AI_AutoTmp' }; Add-Check '#18 asset import' ($import.ok -and $import.imported.Count -gt 0) $import.file
    $reimport = Invoke-Ue POST '/ue5ai/assets/reimport' @{ asset = [string]$import.imported[0] }; Add-Check '#18 asset reimport' $reimport.ok $reimport.asset
    $bpClass = Invoke-Ue GET '/ue5ai/assets/class-summary' @{ class = '/Script/Engine.Blueprint' }; Add-Check '#21 asset-class summary' ($bpClass.ok -and $bpClass.count -gt 0) ("blueprints={0}" -f $bpClass.count)
    $blueprint = Invoke-Ue GET '/ue5ai/blueprint/summary' @{ asset = [string]$bpClass.assets[0].path }; Add-Check '#19 Blueprint graph export' $blueprint.ok $blueprint.path
    $matGraph = Invoke-Ue GET '/ue5ai/material/summary' @{ asset = '/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial' }; Add-Check '#20 Material Graph export' ($matGraph.ok -and $null -ne $matGraph.nodes) $matGraph.path
    $sequence = Invoke-Ue POST '/ue5ai/sequence/create' @{ folder = '/Game/UE5AI_AutoTmp'; name = 'LS_UE5AI_Final' }; Add-Check '#21 Sequencer asset create' $sequence.ok $sequence.path
    $sequenceSummary = Invoke-Ue GET '/ue5ai/sequence/summary' @{ asset = $sequence.path }; Add-Check '#21 Sequencer/animation/Niagara summary protocol' ($sequenceSummary.ok -and $sequenceSummary.class -match 'LevelSequence') $sequenceSummary.class
    $validation = Invoke-Ue GET '/ue5ai/assets/validate'; Add-Check '#22 asset integrity validation' $validation.ok ("invalid={0}; missing={1}; issues={2}" -f $validation.invalid_count,$validation.missing_dependency_count,($validation.issues -join ' | '))

    # #23-#25: visual proof, PIE and plugin automation.
    $capture2 = Invoke-Ue POST '/ue5ai/viewport/screenshot'; Add-Check '#23 candidate screenshot' ($capture2.ok -and (Wait-ForFile $capture2.path)) $capture2.path
    $visual = Invoke-Ue GET '/ue5ai/viewport/verify' @{ baseline = $capture1.path; candidate = $capture2.path }; Add-Check '#23 visual verification evidence' ($visual.ok -and $visual.baseline_sha1 -and $visual.candidate_sha1) 'screenshot hashes returned'
    $pieStart = Invoke-Ue POST '/ue5ai/pie/start'; Add-Check '#24 PIE start request' $pieStart.ok $pieStart.status
    Start-Sleep -Seconds 3
    $pieSnapshot = Invoke-Ue GET '/ue5ai/snapshot'; Add-Check '#24 PIE observable state' ($pieSnapshot.is_pie -eq $true) 'snapshot must report PIE while running'
    $pieStop = Invoke-Ue POST '/ue5ai/pie/stop'; Add-Check '#24 PIE stop request' $pieStop.ok $pieStop.status
    Start-Sleep -Seconds 2
    $discover = Invoke-Ue GET '/ue5ai/automation/discover' @{ filter = 'UE5AI.MVP' }; Add-Check '#25 automation discovery' ($discover.ok -and $discover.count -ge 1) ("count={0}" -f $discover.count)
    $run = Invoke-Ue POST '/ue5ai/automation/run' @{ filter = 'UE5AI.MVP.Smoke' }; Add-Check '#25 automation execution request' $run.ok $run.status
    Start-Sleep -Seconds 5
    $report = Invoke-Ue GET '/ue5ai/automation/report'; Add-Check '#25 automation report' ($report.ok -and $report.passed_signal) 'captured successful automation signal required'

    foreach ($path in $cleanupActors) { if ($path) { [void](Invoke-Ue DELETE '/ue5ai/actors' @{ object = $path }) } }
    $save = Invoke-Ue POST '/ue5ai/project/save-dirty'; Add-Check '#11 save dirty packages after cleanup' $save.ok 'project save accepted'
    $result.passed = $true
}
catch {
    $result.failures.Add($_.Exception.Message)
}
finally {
    try { [void](Invoke-Ue POST '/ue5ai/pie/stop') } catch {}
    foreach ($path in $cleanupActors) { try { if ($path) { [void](Invoke-Ue DELETE '/ue5ai/actors' @{ object = $path }) } } catch {} }
    try { [void](Invoke-Ue POST '/ue5ai/project/save-dirty') } catch {}
    try { [void](Invoke-Ue POST '/ue5ai/test-shutdown') } catch {}
    $result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $ResultPath -Encoding utf8
}

if (-not $result.passed) { exit 1 }
