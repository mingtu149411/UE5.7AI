[CmdletBinding()]
param(
    [string]$BaseUrl = 'http://127.0.0.1:8175',
    [string]$ResultPath = 'D:\xuexishiyong\UE5AI\tests\PRODUCTION_CONTENT_FINAL_UNIFIED_TEST_RESULT.json'
)

$ErrorActionPreference = 'Stop'
$result = [ordered]@{
    name = 'UE5AI production content #26-#47 unified test'
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
    $response = Invoke-WebRequest -Uri $uri -Method $Method -UseBasicParsing -TimeoutSec 60
    return ($response.Content | ConvertFrom-Json)
}

function Test-ProductionBridge {
    $bridge = 'D:\xuexishiyong\UE5AI\UE5AIMcp\bridge\ue5ai_mcp_bridge.py'
    $requests = @(
        '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-03-26","capabilities":{},"clientInfo":{"name":"ProductionContentTest","version":"1"}}}',
        '{"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}}'
    ) -join [Environment]::NewLine
    $output = $requests | & python $bridge
    $lines = @($output | Where-Object { $_ -and $_.Trim() })
    if ($LASTEXITCODE -ne 0 -or $lines.Count -ne 2) { return $false }
    $tools = ($lines[1] | ConvertFrom-Json).result.tools.name
    $missing = @('ue5_performance_snapshot', 'ue5_landscape_create', 'ue5_material_graph', 'ue5_audio_cue', 'ue5_widget_create') | Where-Object { $tools -notcontains $_ }
    return $missing.Count -eq 0
}

try {
    $health = Invoke-Ue GET '/ue5ai/health'; Add-Check 'preflight production plugin loaded' $health.ok $health.engine
    Add-Check 'MCP bridge exposes production tools' (Test-ProductionBridge) 'production tool names must be discoverable over standard MCP'
    $tools = Invoke-Ue GET '/ue5ai/tools'; Add-Check 'production tool contracts listed' ($tools.ok -and $tools.tools.Count -ge 27) ("count={0}" -f $tools.tools.Count)

    # #26 - #28
    $performance = Invoke-Ue GET '/ue5ai/performance/snapshot'; Add-Check '#26 performance snapshot' ($performance.ok -and $null -ne $performance.frame_ms -and $null -ne $performance.draw_calls -and $performance.used_physical_bytes -gt 0) ("frame={0}ms draw={1}" -f $performance.frame_ms,$performance.draw_calls)
    $cvarSet = Invoke-Ue PATCH '/ue5ai/settings/cvar' @{ name = 't.MaxFPS'; value = 61 }; Add-Check '#27 controlled CVar set' ($cvarSet.ok -and $cvarSet.current -eq '61') $cvarSet.current
    $cvarRollback = Invoke-Ue POST '/ue5ai/settings/cvar/rollback' @{ name = 't.MaxFPS' }; Add-Check '#27 controlled CVar rollback' ($cvarRollback.ok -and $cvarRollback.restored.Count -eq 1) ($cvarRollback.restored -join ',')
    $partition = Invoke-Ue GET '/ue5ai/world/partition'; Add-Check '#28 World Partition inspection' ($partition.ok -and $null -ne $partition.is_world_partition -and $null -ne $partition.level_instance_creation) $partition.world
    $levelInstance = Invoke-Ue POST '/ue5ai/world/level-instance' @{ level = '/Game/ThirdPerson/Lvl_ThirdPerson.Lvl_ThirdPerson' }; Add-Check '#28 native Level Instance binding' ($levelInstance.ok -and $levelInstance.bound_world_asset -match 'Lvl_ThirdPerson') $levelInstance.path; $cleanupActors.Add([string]$levelInstance.path)

    # #29 - #31
    $landscape = Invoke-Ue POST '/ue5ai/landscape/create' @{ quads = 7; amplitude = 32 }; Add-Check '#29 Landscape generation' ($landscape.ok -and $landscape.height_samples -eq 64) $landscape.path; $cleanupActors.Add([string]$landscape.path)
    $scatter = Invoke-Ue POST '/ue5ai/scatter/instances' @{ count = 8; seed = 20261005; extent = 250; mesh = '/Engine/BasicShapes/Cylinder.Cylinder' }; Add-Check '#30 seeded instance scatter' ($scatter.ok -and $scatter.instances -eq 8 -and $scatter.seed -eq 20261005) $scatter.path; $cleanupActors.Add([string]$scatter.path)
    $curve = Invoke-Ue POST '/ue5ai/data/create' @{ kind = 'curve'; folder = '/Game/UE5AI_AutoTmp'; name = 'Curve_Production_Final' }; Add-Check '#31 Curve data asset' $curve.ok $curve.path
    $table = Invoke-Ue POST '/ue5ai/data/create' @{ kind = 'table'; folder = '/Game/UE5AI_AutoTmp'; name = 'Table_Production_Final' }; Add-Check '#31 Data Table asset' $table.ok $table.path
    $enum = Invoke-Ue POST '/ue5ai/data/create' @{ kind = 'enum'; folder = '/Game/UE5AI_AutoTmp'; name = 'E_Production_Final' }; Add-Check '#31 User Defined Enum asset' $enum.ok $enum.path
    $struct = Invoke-Ue POST '/ue5ai/data/create' @{ kind = 'struct'; folder = '/Game/UE5AI_AutoTmp'; name = 'S_Production_Final' }; Add-Check '#31 User Defined Struct asset' $struct.ok $struct.path

    # #32 - #36
    $widget = Invoke-Ue POST '/ue5ai/ui/widget' @{ folder = '/Game/UE5AI_AutoTmp'; name = 'WBP_Production_Final'; title = 'Production Test Menu' }; Add-Check '#32 UMG base layout' ($widget.ok -and $widget.widget_count -ge 5) $widget.path
    $input = Invoke-Ue POST '/ue5ai/input/mapping' @{ folder = '/Game/UE5AI_AutoTmp'; action = 'IA_Production_Final'; context = 'IMC_Production_Final'; key = 'K' }; Add-Check '#33 Enhanced Input mapping' ($input.ok -and $input.key -eq 'K') $input.context
    $blueprint = Invoke-Ue POST '/ue5ai/blueprint/create' @{ folder = '/Game/UE5AI_AutoTmp'; name = 'BP_Production_Final'; parent = '/Script/Engine.Actor'; variable = 'TestHealth'; type = 'float' }; Add-Check '#34 safe Blueprint edit and compile' ($blueprint.ok -and $blueprint.compile_status -eq 'compiled') $blueprint.path
    $cpp = Invoke-Ue POST '/ue5ai/cpp/scaffold' @{ class = 'UE5AIProductionPreview'; write_project = 'false' }; Add-Check '#35 C++ scaffold preview' ($cpp.ok -and -not $cpp.wrote_project_source -and $cpp.status -match 'preview') $cpp.header
    $graph = Invoke-Ue GET '/ue5ai/analysis/dependencies' @{ asset = $blueprint.path }; Add-Check '#36 asset dependency graph' ($graph.ok -and $null -ne $graph.dependencies -and $null -ne $graph.referencers) $graph.asset

    # #37 - #41
    $gameplay = Invoke-Ue POST '/ue5ai/gameplay/scaffold' @{ kind = 'character'; folder = '/Game/UE5AI_AutoTmp'; name = 'BP_ProductionCharacter' }; Add-Check '#37 Gameplay Framework scaffold' $gameplay.ok $gameplay.path
    $gas = Invoke-Ue POST '/ue5ai/gas/scaffold' @{ folder = '/Game/UE5AI_AutoTmp'; name = 'GE_Production_Final' }
    $gasDetail = if ($gas.available) { $gas.path } else { $gas.status }
    Add-Check '#38 GAS capability gate' ($gas.ok -and $null -ne $gas.available) $gasDetail
    $material = Invoke-Ue POST '/ue5ai/material/graph' @{ folder = '/Game/UE5AI_AutoTmp'; name = 'M_Production_Final'; roughness = 0.42 }; Add-Check '#39 PBR Material Graph' ($material.ok -and $material.nodes -ge 2) $material.path
    $model = Invoke-Ue POST '/ue5ai/model/procedural-box' @{ size = 120 }; Add-Check '#40 procedural mesh with collision' ($model.ok -and $model.vertices -eq 8 -and $model.collision) $model.path; $cleanupActors.Add([string]$model.path)
    $dcc = Invoke-Ue POST '/ue5ai/dcc/job' @{ tool = 'Blender'; spec = 'Create a licensed low-poly validation prop with UVs and collision.' }; Add-Check '#41 external DCC manifest' ($dcc.ok -and (Test-Path -LiteralPath $dcc.job) -and $dcc.status -match 'planned') $dcc.job

    # #42 - #47
    $skeletons = Invoke-Ue GET '/ue5ai/assets/class-summary' @{ class = '/Script/Engine.Skeleton' }
    Add-Check '#42 animation test prerequisite' ($skeletons.ok -and $skeletons.count -gt 0) ("skeletons={0}" -f $skeletons.count)
    $montage = Invoke-Ue POST '/ue5ai/animation/montage' @{ skeleton = [string]$skeletons.assets[0].path; folder = '/Game/UE5AI_AutoTmp'; name = 'AM_Production_Final' }; Add-Check '#42 montage scaffold' $montage.ok $montage.path
    $animValidation = Invoke-Ue GET '/ue5ai/animation/validate' @{ asset = $montage.path }; Add-Check '#43 skeleton/animation validation' ($animValidation.ok -and $animValidation.skeleton) $animValidation.skeleton
    $sequence = Invoke-Ue POST '/ue5ai/sequence/create' @{ folder = '/Game/UE5AI_AutoTmp'; name = 'LS_Production_Final' }; Add-Check '#44 Sequencer asset create' $sequence.ok $sequence.path
    $sequenceEdit = Invoke-Ue POST '/ue5ai/sequence/add-float-track' @{ asset = $sequence.path }
    $sequenceDetail = if ($sequenceEdit.ok) { $sequenceEdit.track } else { $sequenceEdit.error }
    Add-Check '#44 Sequencer keyed track edit' ($sequenceEdit.ok -and $sequenceEdit.keyframes -eq 2) $sequenceDetail
    $niagara = Invoke-Ue POST '/ue5ai/niagara/scaffold' @{ folder = '/Game/UE5AI_AutoTmp'; name = 'NS_Production_Final' }
    $niagaraDetail = if ($niagara.available) { $niagara.path } else { $niagara.status }
    Add-Check '#45 Niagara capability gate' ($niagara.ok -and $null -ne $niagara.available) $niagaraDetail
    $audio = Invoke-Ue POST '/ue5ai/audio/cue' @{ folder = '/Game/UE5AI_AutoTmp'; name = 'SC_Production_Final' }; Add-Check '#46 Sound Cue creation' $audio.ok $audio.path
    Add-Check '#47 UMG interaction baseline' ($widget.layout -match 'Start/Settings/Quit' -and $widget.accessibility -match 'automation') $widget.accessibility

    foreach ($path in $cleanupActors) { if ($path) { [void](Invoke-Ue DELETE '/ue5ai/actors' @{ object = $path }) } }
    $result.passed = $true
}
catch {
    $result.failures.Add($_.Exception.Message)
}
finally {
    try { [void](Invoke-Ue POST '/ue5ai/settings/cvar/rollback') } catch {}
    foreach ($path in $cleanupActors) { try { if ($path) { [void](Invoke-Ue DELETE '/ue5ai/actors' @{ object = $path }) } } catch {} }
    try { [void](Invoke-Ue POST '/ue5ai/test-shutdown') } catch {}
    $result | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath $ResultPath -Encoding utf8
}

if (-not $result.passed) { exit 1 }
