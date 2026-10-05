// 项目作者水印：南京大学24届沈逸文
#include "UE5AIMcpModule.h"
#include "UE5AIStylizedGrassInteractor.h"

#include "Async/Async.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimationAsset.h"
#include "Animation/Skeleton.h"
#include "AssetImportTask.h"
#include "AutomatedAssetImportData.h"
#include "WidgetBlueprint.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneCaptureComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Containers/Ticker.h"
#include "Curves/CurveFloat.h"
#include "Dom/JsonObject.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EnhancedInput/Public/InputAction.h"
#include "EnhancedInput/Public/InputMappingContext.h"
#include "Factories/MaterialFactoryNew.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Factories/SoundCueFactoryNew.h"
#include "EdGraph/EdGraph.h"
#include "Editor.h"
#include "Editor/TransBuffer.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/UserDefinedStruct.h"
#include "FileHelpers.h"
#include "Framework/Notifications/NotificationManager.h"
#include "InputCoreTypes.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/EnumEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/StructureEditorUtils.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "EdGraphSchema_K2.h"
#include "Landscape.h"
#include "LandscapeProxy.h"
#include "MovieScene.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Sections/MovieSceneSlomoSection.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "Tracks/MovieSceneSlomoTrack.h"
#include "PlayInEditorDataTypes.h"
#include "ProceduralMeshComponent.h"
#include "RHIStats.h"
#include "ScopedTransaction.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundWave.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMemory.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/WindDirectionalSource.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/PointLight.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "Components/PointLightComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/AssetManager.h"
#include "LevelSequence.h"
#include "EditorReimportHandler.h"
#include "EditorViewportClient.h"
#include "IAssetTools.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"
#include "UnrealClient.h"
#include "HttpPath.h"
#include "HttpServerModule.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "IHttpRouter.h"
#include "Interfaces/IPluginManager.h"
#include "Logging/LogVerbosity.h"
#include "HAL/PlatformMisc.h"
#include "Misc/App.h"
#include "Misc/EngineVersion.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/WidgetTree.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraEmitterFactoryNew.h"

DEFINE_LOG_CATEGORY(LogUE5AIMcp);

FAutomationTestBase* CreateUE5AIMcpMvpSmokeTest();
bool RunUE5AIMcpMvpSmokeTest(FAutomationTestBase* Test);

namespace UE5AIMcp
{
constexpr uint32 Port = 8175;

FString Serialize(const TSharedRef<FJsonObject>& Object)
{
    FString Text;
    FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Text));
    return Text;
}

TUniquePtr<FHttpServerResponse> Respond(const TSharedRef<FJsonObject>& Object)
{
    TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(Serialize(Object), TEXT("application/json; charset=utf-8"));
    Response->Code = EHttpServerResponseCodes::Ok;
    Response->Headers.Add(TEXT("Cache-Control"), { TEXT("no-store") });
    return Response;
}

void AddTool(TArray<TSharedPtr<FJsonValue>>& Tools, const FString& Name, const FString& State, const FString& Description)
{
    TSharedRef<FJsonObject> Tool = MakeShared<FJsonObject>();
    Tool->SetStringField(TEXT("name"), Name);
    Tool->SetStringField(TEXT("state"), State);
    Tool->SetStringField(TEXT("description"), Description);
    Tools.Add(MakeShared<FJsonValueObject>(Tool));
}

FString SafeAssetName(const FString& Candidate, const FString& Fallback)
{
    FString Output;
    for (const TCHAR Character : Candidate)
    {
        // UE package names accept CJK characters. Preserve them so generated
        // level and asset names remain readable in the Content Browser.
        if (FChar::IsAlnum(Character) || Character == TCHAR('_') || (Character >= 0x4E00 && Character <= 0x9FFF)) Output.AppendChar(Character);
    }
    if (Output.IsEmpty()) Output = Fallback;
    if (Output.Len() > 64) Output.LeftInline(64, EAllowShrinking::No);
    return Output;
}

FString AssetFolderOrDefault(const FString& Candidate)
{
    return Candidate.StartsWith(TEXT("/Game/")) ? Candidate : TEXT("/Game/UE5AI_AutoTmp");
}

FString PackageNameFor(const FString& Folder, const FString& Name)
{
    return FString::Printf(TEXT("%s/%s"), *AssetFolderOrDefault(Folder), *SafeAssetName(Name, TEXT("UE5AI_Asset")));
}

void MarkAssetCreated(UObject* Asset)
{
    if (Asset)
    {
        FAssetRegistryModule::AssetCreated(Asset);
        Asset->MarkPackageDirty();
    }
}

FString EngineeringArtifactPath(const FString& Category, const FString& Stem)
{
    const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UE5AI/Engineering"), Category));
    IFileManager::Get().MakeDirectory(*Directory, true);
    return FPaths::Combine(Directory, SafeAssetName(Stem, TEXT("UE5AI_Engineering")) + TEXT(".json"));
}

bool SaveJsonArtifact(const FString& Filename, const TSharedRef<FJsonObject>& Object)
{
    return FFileHelper::SaveStringToFile(Serialize(Object), *Filename);
}

TArray<TSharedPtr<FJsonValue>> JsonStrings(const TArray<FString>& Values, int32 Limit = 128)
{
    TArray<TSharedPtr<FJsonValue>> Result;
    for (const FString& Value : Values)
    {
        if (Result.Num() >= Limit) break;
        Result.Add(MakeShared<FJsonValueString>(Value));
    }
    return Result;
}
}

class FUE5AIMcpModule::FImpl
{
public:
    class FLogCapture final : public FOutputDevice
    {
    public:
        virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
        {
            FScopeLock Lock(&Mutex);
            Lines.Add(FString::Printf(TEXT("[%s][%s] %s"), *Category.ToString(), ::ToString(Verbosity), Message));
            if (Lines.Num() > 300) Lines.RemoveAt(0, Lines.Num() - 300, EAllowShrinking::No);
        }
        TArray<FString> Snapshot() const { FScopeLock Lock(&Mutex); return Lines; }
    private:
        mutable FCriticalSection Mutex;
        TArray<FString> Lines;
    };

    void Start()
    {
        Capture = MakeUnique<FLogCapture>();
        GLog->AddOutputDevice(Capture.Get());
        AutomationTest.Reset(CreateUE5AIMcpMvpSmokeTest());
        FAutomationTestFramework::Get().RegisterAutomationTest(TEXT("FUE5AIMcpMvpSmokeTest"), AutomationTest.Get());
        Router = FHttpServerModule::Get().GetHttpRouter(UE5AIMcp::Port, true);
        if (!Router.IsValid())
        {
            UE_LOG(LogUE5AIMcp, Error, TEXT("Cannot bind loopback inspection API to port %u"), UE5AIMcp::Port);
            return;
        }
        Health = Router->BindRoute(FHttpPath(TEXT("/ue5ai/health")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::HealthHandler));
        Tools = Router->BindRoute(FHttpPath(TEXT("/ue5ai/tools")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::ToolsHandler));
        Snapshot = Router->BindRoute(FHttpPath(TEXT("/ue5ai/snapshot")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::SnapshotHandler));
        Properties = Router->BindRoute(FHttpPath(TEXT("/ue5ai/properties")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::PropertiesHandler));
        CVars = Router->BindRoute(FHttpPath(TEXT("/ue5ai/cvars")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::CVarsHandler));
        Logs = Router->BindRoute(FHttpPath(TEXT("/ue5ai/logs")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::LogsHandler));
        Screenshot = Router->BindRoute(FHttpPath(TEXT("/ue5ai/viewport/screenshot")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::ScreenshotHandler));
        SpawnActor = Router->BindRoute(FHttpPath(TEXT("/ue5ai/actors/spawn")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::SpawnActorHandler));
        DeleteActor = Router->BindRoute(FHttpPath(TEXT("/ue5ai/actors")), EHttpServerRequestVerbs::VERB_DELETE,
            FHttpRequestHandler::CreateRaw(this, &FImpl::DeleteActorHandler));
        TransformActor = Router->BindRoute(FHttpPath(TEXT("/ue5ai/actors/transform")), EHttpServerRequestVerbs::VERB_PATCH,
            FHttpRequestHandler::CreateRaw(this, &FImpl::TransformActorHandler));
        SpawnLight = Router->BindRoute(FHttpPath(TEXT("/ue5ai/lights/point")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::SpawnLightHandler));
        SpawnGrid = Router->BindRoute(FHttpPath(TEXT("/ue5ai/layout/grid")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::SpawnGridHandler));
        OrganizeActor = Router->BindRoute(FHttpPath(TEXT("/ue5ai/actors/organize")), EHttpServerRequestVerbs::VERB_PATCH,
            FHttpRequestHandler::CreateRaw(this, &FImpl::OrganizeActorHandler));
        CameraPost = Router->BindRoute(FHttpPath(TEXT("/ue5ai/camera/postprocess")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::CameraPostHandler));
        MaterialInstance = Router->BindRoute(FHttpPath(TEXT("/ue5ai/material/instance")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::MaterialInstanceHandler));
        ImportAsset = Router->BindRoute(FHttpPath(TEXT("/ue5ai/assets/import")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::ImportAssetHandler));
        ReimportAsset = Router->BindRoute(FHttpPath(TEXT("/ue5ai/assets/reimport")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::ReimportAssetHandler));
        AssetSummary = Router->BindRoute(FHttpPath(TEXT("/ue5ai/sequence/summary")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::SequenceSummaryHandler));
        CreateSequence = Router->BindRoute(FHttpPath(TEXT("/ue5ai/sequence/create")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::CreateSequenceHandler));
        VerifyViewport = Router->BindRoute(FHttpPath(TEXT("/ue5ai/viewport/verify")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::VerifyViewportHandler));
        ValidateAssets = Router->BindRoute(FHttpPath(TEXT("/ue5ai/assets/validate")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::ValidateAssetsHandler));
        Undo = Router->BindRoute(FHttpPath(TEXT("/ue5ai/transaction/undo")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::UndoHandler));
        Redo = Router->BindRoute(FHttpPath(TEXT("/ue5ai/transaction/redo")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::RedoHandler));
        BlueprintSummary = Router->BindRoute(FHttpPath(TEXT("/ue5ai/blueprint/summary")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::BlueprintSummaryHandler));
        MaterialSummary = Router->BindRoute(FHttpPath(TEXT("/ue5ai/material/summary")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::MaterialSummaryHandler));
        ClassSummary = Router->BindRoute(FHttpPath(TEXT("/ue5ai/assets/class-summary")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::ClassSummaryHandler));
        StartPie = Router->BindRoute(FHttpPath(TEXT("/ue5ai/pie/start")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::StartPieHandler));
        StopPie = Router->BindRoute(FHttpPath(TEXT("/ue5ai/pie/stop")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::StopPieHandler));
        SaveDirty = Router->BindRoute(FHttpPath(TEXT("/ue5ai/project/save-dirty")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::SaveDirtyHandler));
        AutomationDiscover = Router->BindRoute(FHttpPath(TEXT("/ue5ai/automation/discover")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::AutomationDiscoverHandler));
        AutomationRun = Router->BindRoute(FHttpPath(TEXT("/ue5ai/automation/run")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::AutomationRunHandler));
        AutomationReport = Router->BindRoute(FHttpPath(TEXT("/ue5ai/automation/report")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::AutomationReportHandler));
        Diagnostics = Router->BindRoute(FHttpPath(TEXT("/ue5ai/diagnostics")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::DiagnosticsHandler));
        PerformanceSnapshot = Router->BindRoute(FHttpPath(TEXT("/ue5ai/performance/snapshot")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::PerformanceSnapshotHandler));
        ControlledCVar = Router->BindRoute(FHttpPath(TEXT("/ue5ai/settings/cvar")), EHttpServerRequestVerbs::VERB_PATCH,
            FHttpRequestHandler::CreateRaw(this, &FImpl::ControlledCVarHandler));
        RollbackCVar = Router->BindRoute(FHttpPath(TEXT("/ue5ai/settings/cvar/rollback")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::RollbackCVarHandler));
        WorldPartition = Router->BindRoute(FHttpPath(TEXT("/ue5ai/world/partition")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::WorldPartitionHandler));
        LevelInstance = Router->BindRoute(FHttpPath(TEXT("/ue5ai/world/level-instance")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::LevelInstanceHandler));
        Landscape = Router->BindRoute(FHttpPath(TEXT("/ue5ai/landscape/create")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::LandscapeHandler));
        Scatter = Router->BindRoute(FHttpPath(TEXT("/ue5ai/scatter/instances")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::ScatterHandler));
        DataAsset = Router->BindRoute(FHttpPath(TEXT("/ue5ai/data/create")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::DataAssetHandler));
        Widget = Router->BindRoute(FHttpPath(TEXT("/ue5ai/ui/widget")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::WidgetHandler));
        InputMapping = Router->BindRoute(FHttpPath(TEXT("/ue5ai/input/mapping")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::InputMappingHandler));
        BlueprintEdit = Router->BindRoute(FHttpPath(TEXT("/ue5ai/blueprint/create")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::BlueprintEditHandler));
        CppScaffold = Router->BindRoute(FHttpPath(TEXT("/ue5ai/cpp/scaffold")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::CppScaffoldHandler));
        DependencyGraph = Router->BindRoute(FHttpPath(TEXT("/ue5ai/analysis/dependencies")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::DependencyGraphHandler));
        GameplayScaffold = Router->BindRoute(FHttpPath(TEXT("/ue5ai/gameplay/scaffold")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::GameplayScaffoldHandler));
        GasScaffold = Router->BindRoute(FHttpPath(TEXT("/ue5ai/gas/scaffold")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::GasScaffoldHandler));
        MaterialGraph = Router->BindRoute(FHttpPath(TEXT("/ue5ai/material/graph")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::MaterialGraphHandler));
        ProceduralMesh = Router->BindRoute(FHttpPath(TEXT("/ue5ai/model/procedural-box")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::ProceduralMeshHandler));
        ExternalJob = Router->BindRoute(FHttpPath(TEXT("/ue5ai/dcc/job")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::ExternalJobHandler));
        Animation = Router->BindRoute(FHttpPath(TEXT("/ue5ai/animation/montage")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::AnimationHandler));
        AnimationValidate = Router->BindRoute(FHttpPath(TEXT("/ue5ai/animation/validate")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::AnimationValidateHandler));
        SequenceEdit = Router->BindRoute(FHttpPath(TEXT("/ue5ai/sequence/add-float-track")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::SequenceEditHandler));
        Niagara = Router->BindRoute(FHttpPath(TEXT("/ue5ai/niagara/scaffold")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::NiagaraHandler));
        Audio = Router->BindRoute(FHttpPath(TEXT("/ue5ai/audio/cue")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::AudioHandler));
        NetworkReplication = Router->BindRoute(FHttpPath(TEXT("/ue5ai/network/replication")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::NetworkReplicationHandler));
        NetworkSessionPlan = Router->BindRoute(FHttpPath(TEXT("/ue5ai/network/session-plan")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::NetworkSessionPlanHandler));
        PlatformProfile = Router->BindRoute(FHttpPath(TEXT("/ue5ai/platform/profile")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::PlatformProfileHandler));
        PipelinePlan = Router->BindRoute(FHttpPath(TEXT("/ue5ai/pipeline/plan")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::PipelinePlanHandler));
        BudgetAudit = Router->BindRoute(FHttpPath(TEXT("/ue5ai/quality/budget")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::BudgetAuditHandler));
        ComplianceAudit = Router->BindRoute(FHttpPath(TEXT("/ue5ai/compliance/audit")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::ComplianceAuditHandler));
        SourceControlStatus = Router->BindRoute(FHttpPath(TEXT("/ue5ai/source-control/status")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::SourceControlStatusHandler));
        CiPlan = Router->BindRoute(FHttpPath(TEXT("/ue5ai/cicd/plan")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::CiPlanHandler));
        CrashDiagnostics = Router->BindRoute(FHttpPath(TEXT("/ue5ai/crash/diagnostics")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::CrashDiagnosticsHandler));
        PluginScaffold = Router->BindRoute(FHttpPath(TEXT("/ue5ai/plugin/scaffold")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::PluginScaffoldHandler));
        EditorAdapters = Router->BindRoute(FHttpPath(TEXT("/ue5ai/editor/adapters")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::EditorAdaptersHandler));
        AgentPlan = Router->BindRoute(FHttpPath(TEXT("/ue5ai/agent/plan")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::AgentPlanHandler));
        AgentTransition = Router->BindRoute(FHttpPath(TEXT("/ue5ai/agent/transition")), EHttpServerRequestVerbs::VERB_PATCH,
            FHttpRequestHandler::CreateRaw(this, &FImpl::AgentTransitionHandler));
        SceneOrchestrate = Router->BindRoute(FHttpPath(TEXT("/ue5ai/scene/orchestrate")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::SceneOrchestrateHandler));
        ShowcaseCreate = Router->BindRoute(FHttpPath(TEXT("/ue5ai/showcase/create")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::ShowcaseCreateHandler));
        ShowcaseEnhance = Router->BindRoute(FHttpPath(TEXT("/ue5ai/showcase/enhance")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::ShowcaseEnhanceHandler));
        StylizedGrassRebuild = Router->BindRoute(FHttpPath(TEXT("/ue5ai/showcase/grass/rebuild")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::StylizedGrassRebuildHandler));
        StylizedGrassStatus = Router->BindRoute(FHttpPath(TEXT("/ue5ai/showcase/grass/status")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::StylizedGrassStatusHandler));
        CapabilityCoverage = Router->BindRoute(FHttpPath(TEXT("/ue5ai/capabilities/coverage")), EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateRaw(this, &FImpl::CapabilityCoverageHandler));
        Shutdown = Router->BindRoute(FHttpPath(TEXT("/ue5ai/test-shutdown")), EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateRaw(this, &FImpl::ShutdownHandler));
        FHttpServerModule::Get().StartAllListeners();
        UE_LOG(LogUE5AIMcp, Display, TEXT("UE5 AI MCP inspection API: http://127.0.0.1:%u/ue5ai/health"), UE5AIMcp::Port);
    }

    void Stop()
    {
        if (AutomationTest)
        {
            FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("FUE5AIMcpMvpSmokeTest"));
            AutomationTest.Reset();
        }
        if (Router.IsValid())
        {
            Router->UnbindRoute(Health);
            Router->UnbindRoute(Tools);
            Router->UnbindRoute(Snapshot);
            Router->UnbindRoute(Properties);
            Router->UnbindRoute(CVars);
            Router->UnbindRoute(Logs);
            Router->UnbindRoute(Screenshot);
            Router->UnbindRoute(SpawnActor);
            Router->UnbindRoute(DeleteActor);
            Router->UnbindRoute(TransformActor);
            Router->UnbindRoute(SpawnLight);
            Router->UnbindRoute(SpawnGrid);
            Router->UnbindRoute(OrganizeActor);
            Router->UnbindRoute(CameraPost);
            Router->UnbindRoute(MaterialInstance);
            Router->UnbindRoute(ImportAsset);
            Router->UnbindRoute(ReimportAsset);
            Router->UnbindRoute(AssetSummary);
            Router->UnbindRoute(CreateSequence);
            Router->UnbindRoute(VerifyViewport);
            Router->UnbindRoute(ValidateAssets);
            Router->UnbindRoute(Undo);
            Router->UnbindRoute(Redo);
            Router->UnbindRoute(BlueprintSummary);
            Router->UnbindRoute(MaterialSummary);
            Router->UnbindRoute(ClassSummary);
            Router->UnbindRoute(StartPie);
            Router->UnbindRoute(StopPie);
            Router->UnbindRoute(SaveDirty);
            Router->UnbindRoute(AutomationDiscover);
            Router->UnbindRoute(AutomationRun);
            Router->UnbindRoute(AutomationReport);
            Router->UnbindRoute(Diagnostics);
            Router->UnbindRoute(PerformanceSnapshot);
            Router->UnbindRoute(ControlledCVar);
            Router->UnbindRoute(RollbackCVar);
            Router->UnbindRoute(WorldPartition);
            Router->UnbindRoute(LevelInstance);
            Router->UnbindRoute(Landscape);
            Router->UnbindRoute(Scatter);
            Router->UnbindRoute(DataAsset);
            Router->UnbindRoute(Widget);
            Router->UnbindRoute(InputMapping);
            Router->UnbindRoute(BlueprintEdit);
            Router->UnbindRoute(CppScaffold);
            Router->UnbindRoute(DependencyGraph);
            Router->UnbindRoute(GameplayScaffold);
            Router->UnbindRoute(GasScaffold);
            Router->UnbindRoute(MaterialGraph);
            Router->UnbindRoute(ProceduralMesh);
            Router->UnbindRoute(ExternalJob);
            Router->UnbindRoute(Animation);
            Router->UnbindRoute(AnimationValidate);
            Router->UnbindRoute(SequenceEdit);
            Router->UnbindRoute(Niagara);
            Router->UnbindRoute(Audio);
            Router->UnbindRoute(NetworkReplication);
            Router->UnbindRoute(NetworkSessionPlan);
            Router->UnbindRoute(PlatformProfile);
            Router->UnbindRoute(PipelinePlan);
            Router->UnbindRoute(BudgetAudit);
            Router->UnbindRoute(ComplianceAudit);
            Router->UnbindRoute(SourceControlStatus);
            Router->UnbindRoute(CiPlan);
            Router->UnbindRoute(CrashDiagnostics);
            Router->UnbindRoute(PluginScaffold);
            Router->UnbindRoute(EditorAdapters);
            Router->UnbindRoute(AgentPlan);
            Router->UnbindRoute(AgentTransition);
            Router->UnbindRoute(SceneOrchestrate);
            Router->UnbindRoute(ShowcaseCreate);
            Router->UnbindRoute(ShowcaseEnhance);
            Router->UnbindRoute(StylizedGrassRebuild);
            Router->UnbindRoute(StylizedGrassStatus);
            Router->UnbindRoute(CapabilityCoverage);
            Router->UnbindRoute(Shutdown);
            Router.Reset();
        }
        if (Capture) { GLog->RemoveOutputDevice(Capture.Get()); Capture.Reset(); }
    }

private:
    bool RespondError(const FString& Message, const FHttpResultCallback& Complete)
    {
        TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), Message);
        Complete(UE5AIMcp::Respond(Result));
        return true;
    }

    static void TrackShowcaseAsset(UObject* Asset, TArray<UPackage*>& Packages)
    {
        if (Asset)
        {
            UE5AIMcp::MarkAssetCreated(Asset);
            Packages.AddUnique(Asset->GetOutermost());
        }
    }

    static UMaterial* CreateShowcaseMaterial(const FString& Folder, const FString& Name, const FLinearColor& Color, float Roughness, bool bTextureBacked, bool bAnimatedWind, TArray<UPackage*>& Packages)
    {
        const FString AssetPath = UE5AIMcp::PackageNameFor(Folder, Name);
        if (UMaterial* Existing = LoadObject<UMaterial>(nullptr, *AssetPath))
        {
            Packages.AddUnique(Existing->GetOutermost());
            return Existing;
        }

        UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
        UMaterial* Material = Cast<UMaterial>(FAssetToolsModule::GetModule().Get().CreateAsset(Name, Folder, UMaterial::StaticClass(), Factory));
        if (!Material || !Material->GetEditorOnlyData()) return nullptr;

        Material->Modify();
        UMaterialExpressionConstant3Vector* Tint = NewObject<UMaterialExpressionConstant3Vector>(Material);
        Tint->Constant = Color; Tint->MaterialExpressionEditorX = -600; Tint->MaterialExpressionEditorY = 0;
        Material->GetExpressionCollection().AddExpression(Tint);
        UMaterialExpression* BaseColor = Tint;
        if (bTextureBacked)
        {
            UMaterialExpressionTextureSample* TextureSample = NewObject<UMaterialExpressionTextureSample>(Material);
            TextureSample->Texture = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
            TextureSample->MaterialExpressionEditorX = -600; TextureSample->MaterialExpressionEditorY = -120;
            UMaterialExpressionMultiply* Multiply = NewObject<UMaterialExpressionMultiply>(Material);
            Multiply->A.Connect(0, TextureSample); Multiply->B.Connect(0, Tint);
            Multiply->MaterialExpressionEditorX = -360; Multiply->MaterialExpressionEditorY = 0;
            Material->GetExpressionCollection().AddExpression(TextureSample);
            Material->GetExpressionCollection().AddExpression(Multiply);
            BaseColor = Multiply;
        }
        UMaterialExpressionScalarParameter* RoughnessNode = NewObject<UMaterialExpressionScalarParameter>(Material);
        RoughnessNode->ParameterName = TEXT("粗糙度"); RoughnessNode->DefaultValue = Roughness;
        RoughnessNode->MaterialExpressionEditorX = -420; RoughnessNode->MaterialExpressionEditorY = 160;
        Material->GetExpressionCollection().AddExpression(RoughnessNode);
        Material->GetEditorOnlyData()->BaseColor.Expression = BaseColor;
        Material->GetEditorOnlyData()->Roughness.Expression = RoughnessNode;

        if (bAnimatedWind)
        {
            UMaterialExpressionTime* Time = NewObject<UMaterialExpressionTime>(Material);
            Time->Period = 3.0f; Time->MaterialExpressionEditorX = -600; Time->MaterialExpressionEditorY = 300;
            UMaterialExpressionSine* Sine = NewObject<UMaterialExpressionSine>(Material);
            Sine->Input.Connect(0, Time); Sine->Period = 1.0f; Sine->MaterialExpressionEditorX = -420; Sine->MaterialExpressionEditorY = 300;
            UMaterialExpressionConstant3Vector* WindVector = NewObject<UMaterialExpressionConstant3Vector>(Material);
            WindVector->Constant = FLinearColor(6.0f, 2.0f, 0.0f); WindVector->MaterialExpressionEditorX = -420; WindVector->MaterialExpressionEditorY = 390;
            UMaterialExpressionMultiply* Offset = NewObject<UMaterialExpressionMultiply>(Material);
            Offset->A.Connect(0, Sine); Offset->B.Connect(0, WindVector); Offset->MaterialExpressionEditorX = -190; Offset->MaterialExpressionEditorY = 330;
            Material->GetExpressionCollection().AddExpression(Time);
            Material->GetExpressionCollection().AddExpression(Sine);
            Material->GetExpressionCollection().AddExpression(WindVector);
            Material->GetExpressionCollection().AddExpression(Offset);
            Material->GetEditorOnlyData()->WorldPositionOffset.Expression = Offset;
        }
        Material->PostEditChange();
        TrackShowcaseAsset(Material, Packages);
        return Material;
    }

    static UBlueprint* CreateShowcaseBlueprint(const FString& Folder, const FString& Name, const TArray<FString>& VariableNames, TArray<UPackage*>& Packages)
    {
        const FString AssetPath = UE5AIMcp::PackageNameFor(Folder, Name);
        if (UBlueprint* Existing = LoadObject<UBlueprint>(nullptr, *AssetPath))
        {
            Packages.AddUnique(Existing->GetOutermost());
            return Existing;
        }
        UPackage* Package = CreatePackage(*AssetPath);
        UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), Package, *Name, BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), FName(TEXT("UE5AIShowcase")));
        if (!Blueprint) return nullptr;
        FEdGraphPinType FloatType; FloatType.PinCategory = UEdGraphSchema_K2::PC_Real; FloatType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
        for (const FString& VariableName : VariableNames)
        {
            FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VariableName), FloatType, TEXT("0.0"));
        }
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        TrackShowcaseAsset(Blueprint, Packages);
        return Blueprint;
    }

    static void AddEnvironment(UWorld* World, const FString& Prefix)
    {
        if (!World) return;
        FActorSpawnParameters Params;
        ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector(0, 0, 1200), FRotator(-42.0f, 35.0f, 0.0f), Params);
        if (Sun) { Sun->SetActorLabel(Prefix + TEXT("_太阳光")); Sun->GetLightComponent()->SetIntensity(8.0f); Sun->GetComponent()->SetAtmosphereSunLight(true); }
        ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (SkyLight) { SkyLight->SetActorLabel(Prefix + TEXT("_环境光")); SkyLight->GetLightComponent()->SetIntensity(1.25f); SkyLight->GetLightComponent()->SetMobility(EComponentMobility::Movable); }
        ASkyAtmosphere* Atmosphere = World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (Atmosphere) Atmosphere->SetActorLabel(Prefix + TEXT("_天空大气"));
        AVolumetricCloud* Clouds = World->SpawnActor<AVolumetricCloud>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (Clouds) Clouds->SetActorLabel(Prefix + TEXT("_体积云"));
        AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (Fog) { Fog->SetActorLabel(Prefix + TEXT("_高度雾")); Fog->GetComponent()->SetFogDensity(0.008f); }
    }

    static AStaticMeshActor* AddStaticMeshActor(UWorld* World, UStaticMesh* Mesh, UMaterialInterface* Material, const FString& Label, const FTransform& Transform)
    {
        if (!World || !Mesh) return nullptr;
        AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Transform);
        if (!Actor) return nullptr;
        Actor->SetActorLabel(Label);
        Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
        if (Material) Actor->GetStaticMeshComponent()->SetMaterial(0, Material);
        return Actor;
    }

    static UTextureRenderTarget2D* CreateShowcaseRenderTarget(const FString& Folder, const FString& Name, TArray<UPackage*>& Packages)
    {
        const FString AssetPath = UE5AIMcp::PackageNameFor(Folder, Name);
        if (UTextureRenderTarget2D* Existing = LoadObject<UTextureRenderTarget2D>(nullptr, *AssetPath))
        {
            Packages.AddUnique(Existing->GetOutermost());
            return Existing;
        }
        UPackage* Package = CreatePackage(*AssetPath);
        UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional);
        if (!Target) return nullptr;
        Target->InitAutoFormat(1024, 1024);
        Target->ClearColor = FLinearColor::Black;
        Target->UpdateResourceImmediate(true);
        TrackShowcaseAsset(Target, Packages);
        return Target;
    }

    static UNiagaraSystem* CreateCherryNiagaraSystem(const FString& Folder, TArray<UPackage*>& Packages)
    {
        const FString Name = TEXT("NS_樱花飘落");
        const FString AssetPath = UE5AIMcp::PackageNameFor(Folder, Name);
        if (UNiagaraSystem* Existing = LoadObject<UNiagaraSystem>(nullptr, *AssetPath))
        {
            Packages.AddUnique(Existing->GetOutermost());
            return Existing;
        }
        // Copy a verified engine Niagara template instead of manually creating
        // Niagara graph internals.  The latter is editor-version-sensitive and
        // can leave a system with an invalid emitter graph.
        UNiagaraSystem* Template = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Niagara/DefaultAssets/Templates/Systems/FountainLightweight.FountainLightweight"));
        if (!Template) return nullptr;
        UPackage* Package = CreatePackage(*AssetPath);
        UNiagaraSystem* System = DuplicateObject<UNiagaraSystem>(Template, Package, *Name);
        if (!System) return nullptr;
        System->SetFlags(RF_Public | RF_Standalone | RF_Transactional);
        TrackShowcaseAsset(System, Packages);
        return System;
    }

    static bool BeginShowcaseMap(const FString& MapPath, UWorld*& World, FString& Error)
    {
        if (FPackageName::DoesPackageExist(MapPath))
        {
            Error = FString::Printf(TEXT("Map already exists and was preserved: %s"), *MapPath);
            return false;
        }
        World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
        if (!World)
        {
            Error = TEXT("The editor could not create a blank map.");
            return false;
        }
        return true;
    }

    static bool SaveShowcase(UWorld* World, const FString& MapPath, const TArray<UPackage*>& Packages)
    {
        const bool bAssetsSaved = Packages.Num() == 0 || UEditorLoadingAndSavingUtils::SavePackages(Packages, false);
        const bool bMapSaved = World && UEditorLoadingAndSavingUtils::SaveMap(World, MapPath);
        return bAssetsSaved && bMapSaved;
    }

    static void AddResultPath(TArray<TSharedPtr<FJsonValue>>& Paths, UObject* Object)
    {
        if (Object) Paths.Add(MakeShared<FJsonValueString>(Object->GetPathName()));
    }

    static bool BuildOceanShowcase(TSharedRef<FJsonObject>& Result)
    {
        const FString Folder = TEXT("/Game/AI插件测试/关卡一_真实海洋");
        const FString MapPath = Folder + TEXT("/关卡一_真实海洋");
        UWorld* World = nullptr; FString Error;
        if (!BeginShowcaseMap(MapPath, World, Error)) { Result->SetStringField(TEXT("error"), Error); return false; }
        TArray<UPackage*> Packages;
        UMaterial* NearMaterial = CreateShowcaseMaterial(Folder, TEXT("M_近景细分海面"), FLinearColor(0.006f, 0.09f, 0.22f), 0.08f, false, false, Packages);
        UMaterial* FarMaterial = CreateShowcaseMaterial(Folder, TEXT("M_远海贴图"), FLinearColor(0.015f, 0.16f, 0.32f), 0.2f, true, false, Packages);
        UBlueprint* OceanBlueprint = CreateShowcaseBlueprint(Folder, TEXT("BP_海洋模块"), { TEXT("波浪幅度"), TEXT("近景细分密度"), TEXT("远海贴图缩放") }, Packages);
        if (!NearMaterial || !FarMaterial || !OceanBlueprint) { Result->SetStringField(TEXT("error"), TEXT("Ocean material or blueprint creation failed.")); return false; }

        AddEnvironment(World, TEXT("海洋"));
        AActor* NearOcean = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
        if (!NearOcean) { Result->SetStringField(TEXT("error"), TEXT("Near ocean actor creation failed.")); return false; }
        NearOcean->SetActorLabel(TEXT("海洋模块_近景细分平面")); NearOcean->Tags = { TEXT("BP_海洋模块"), TEXT("近景细分") };
        UProceduralMeshComponent* Surface = NewObject<UProceduralMeshComponent>(NearOcean, TEXT("近景细分水面"));
        NearOcean->SetRootComponent(Surface); Surface->RegisterComponent(); Surface->SetMaterial(0, NearMaterial);
        constexpr int32 Segments = 96; constexpr float Step = 180.0f;
        TArray<FVector> Vertices; TArray<int32> Triangles; TArray<FVector> Normals; TArray<FVector2D> UVs; TArray<FLinearColor> Colors; TArray<FProcMeshTangent> Tangents;
        Vertices.Reserve((Segments + 1) * (Segments + 1)); Normals.Reserve((Segments + 1) * (Segments + 1)); UVs.Reserve((Segments + 1) * (Segments + 1)); Colors.Reserve((Segments + 1) * (Segments + 1)); Tangents.Reserve((Segments + 1) * (Segments + 1));
        for (int32 Y = 0; Y <= Segments; ++Y) for (int32 X = 0; X <= Segments; ++X)
        {
            const float PX = (X - Segments * 0.5f) * Step; const float PY = (Y - Segments * 0.5f) * Step;
            const float Height = FMath::Sin(PX * 0.0024f + PY * 0.0013f) * 34.0f + FMath::Cos(PY * 0.0041f) * 14.0f + FMath::Sin((PX + PY) * 0.008f) * 4.0f;
            Vertices.Add(FVector(PX, PY, Height)); Normals.Add(FVector::UpVector); UVs.Add(FVector2D(static_cast<float>(X) / Segments, static_cast<float>(Y) / Segments)); Colors.Add(FLinearColor::White); Tangents.Add(FProcMeshTangent(1, 0, 0));
        }
        for (int32 Y = 0; Y < Segments; ++Y) for (int32 X = 0; X < Segments; ++X)
        {
            const int32 I = Y * (Segments + 1) + X; Triangles.Append({ I, I + Segments + 1, I + 1, I + 1, I + Segments + 1, I + Segments + 2 });
        }
        Surface->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
        UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
        AStaticMeshActor* FarOcean = AddStaticMeshActor(World, Plane, FarMaterial, TEXT("海洋模块_远海贴图"), FTransform(FRotator::ZeroRotator, FVector(0, 0, -30), FVector(1800, 1800, 1)));
        if (FarOcean) FarOcean->Tags = { TEXT("远海贴图"), TEXT("低开支") };
        ACameraActor* Camera = World->SpawnActor<ACameraActor>(FVector(-2450, -3600, 1350), FRotator(-13, 30, 0));
        if (Camera) Camera->SetActorLabel(TEXT("海洋观察相机"));
        const bool bSaved = SaveShowcase(World, MapPath, Packages);
        TArray<TSharedPtr<FJsonValue>> Paths; AddResultPath(Paths, NearMaterial); AddResultPath(Paths, FarMaterial); AddResultPath(Paths, OceanBlueprint);
        Result->SetStringField(TEXT("map"), MapPath); Result->SetArrayField(TEXT("assets"), Paths); Result->SetNumberField(TEXT("near_vertices"), Vertices.Num()); Result->SetNumberField(TEXT("near_triangles"), Triangles.Num() / 3); Result->SetStringField(TEXT("implementation"), TEXT("Near field is a 96x96 tessellated procedural water surface with multi-frequency waves; far field is a texture-backed scaled plane."));
        Result->SetBoolField(TEXT("saved"), bSaved);
        return bSaved;
    }

    static bool BuildGrassShowcase(TSharedRef<FJsonObject>& Result)
    {
        const FString Folder = TEXT("/Game/AI插件测试/关卡二_交互草地");
        const FString MapPath = Folder + TEXT("/关卡二_全局风场草地");
        UWorld* World = nullptr; FString Error;
        if (!BeginShowcaseMap(MapPath, World, Error)) { Result->SetStringField(TEXT("error"), Error); return false; }
        TArray<UPackage*> Packages;
        UMaterial* GroundMaterial = CreateShowcaseMaterial(Folder, TEXT("M_草地地表"), FLinearColor(0.035f, 0.18f, 0.04f), 0.82f, false, false, Packages);
        UMaterial* GrassMaterial = CreateShowcaseMaterial(Folder, TEXT("M_模型草_全局风场"), FLinearColor(0.08f, 0.48f, 0.07f), 0.65f, false, true, Packages);
        UTextureRenderTarget2D* InteractionRT = CreateShowcaseRenderTarget(Folder, TEXT("RT_草地交互与风场"), Packages);
        UBlueprint* ControllerBlueprint = CreateShowcaseBlueprint(Folder, TEXT("BP_草地RT交互控制器"), { TEXT("全局风力"), TEXT("交互半径"), TEXT("RT分辨率") }, Packages);
        if (!GroundMaterial || !GrassMaterial || !InteractionRT || !ControllerBlueprint) { Result->SetStringField(TEXT("error"), TEXT("Grass material, RT, or blueprint creation failed.")); return false; }

        AddEnvironment(World, TEXT("草地"));
        UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
        UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        AddStaticMeshActor(World, Plane, GroundMaterial, TEXT("草地地表_基础关卡"), FTransform(FRotator::ZeroRotator, FVector(0, 0, -10), FVector(145, 145, 1)));
        AActor* GrassField = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
        if (!GrassField || !Plane) { Result->SetStringField(TEXT("error"), TEXT("Grass field creation failed.")); return false; }
        GrassField->SetActorLabel(TEXT("模型草地_HISM视域裁剪")); GrassField->Tags = { TEXT("模型草"), TEXT("HISM视域裁剪"), TEXT("RT交互") };
        UHierarchicalInstancedStaticMeshComponent* GrassA = NewObject<UHierarchicalInstancedStaticMeshComponent>(GrassField, TEXT("草模型实例_主方向"));
        GrassField->SetRootComponent(GrassA); GrassA->RegisterComponent(); GrassA->SetStaticMesh(Plane); GrassA->SetMaterial(0, GrassMaterial); GrassA->SetCullDistances(1800, 14000); GrassA->SetCastShadow(false);
        UHierarchicalInstancedStaticMeshComponent* GrassB = NewObject<UHierarchicalInstancedStaticMeshComponent>(GrassField, TEXT("草模型实例_交叉方向"));
        GrassB->SetupAttachment(GrassA); GrassB->RegisterComponent(); GrassB->SetStaticMesh(Plane); GrassB->SetMaterial(0, GrassMaterial); GrassB->SetCullDistances(1800, 14000); GrassB->SetCastShadow(false);
        FRandomStream Random(20261005);
        constexpr int32 GrassInstances = 1600;
        for (int32 Index = 0; Index < GrassInstances; ++Index)
        {
            const FVector Location(Random.FRandRange(-11800, 11800), Random.FRandRange(-11800, 11800), 30.0f);
            const FVector Scale(Random.FRandRange(0.015f, 0.028f), Random.FRandRange(0.32f, 0.52f), Random.FRandRange(0.7f, 1.18f));
            GrassA->AddInstance(FTransform(FRotator(90.0f, Random.FRandRange(0, 360), 0), Location, Scale));
            GrassB->AddInstance(FTransform(FRotator(90.0f, Random.FRandRange(0, 360), 90.0f), Location, Scale));
        }
        AWindDirectionalSource* Wind = World->SpawnActor<AWindDirectionalSource>(FVector(0, 0, 200), FRotator(0, 35, 0));
        if (Wind) Wind->SetActorLabel(TEXT("全局风场_方向风源"));
        ASceneCapture2D* Capture = World->SpawnActor<ASceneCapture2D>(FVector(0, 0, 12500), FRotator(-90, 0, 0));
        if (Capture) { Capture->SetActorLabel(TEXT("RT交互场景捕获")); Capture->GetCaptureComponent2D()->TextureTarget = InteractionRT; Capture->GetCaptureComponent2D()->CaptureScene(); }
        AddStaticMeshActor(World, Sphere, nullptr, TEXT("交互者_RT写入示例"), FTransform(FRotator::ZeroRotator, FVector(0, 0, 170), FVector(1.2f)));
        ACameraActor* Camera = World->SpawnActor<ACameraActor>(FVector(-3000, -3700, 1250), FRotator(-16, 38, 0));
        if (Camera) Camera->SetActorLabel(TEXT("草地观察相机"));
        const bool bSaved = SaveShowcase(World, MapPath, Packages);
        TArray<TSharedPtr<FJsonValue>> Paths; AddResultPath(Paths, GroundMaterial); AddResultPath(Paths, GrassMaterial); AddResultPath(Paths, InteractionRT); AddResultPath(Paths, ControllerBlueprint);
        Result->SetStringField(TEXT("map"), MapPath); Result->SetArrayField(TEXT("assets"), Paths); Result->SetNumberField(TEXT("grass_model_instances"), GrassInstances * 2); Result->SetNumberField(TEXT("cull_start_cm"), 1800); Result->SetNumberField(TEXT("cull_end_cm"), 14000); Result->SetStringField(TEXT("implementation"), TEXT("HISM model grass is distance/visibility culled; a 1024x1024 Render Target is bound to a top-down scene capture as the interaction and wind-field input."));
        Result->SetBoolField(TEXT("saved"), bSaved);
        return bSaved;
    }

    static bool BuildCherryShowcase(TSharedRef<FJsonObject>& Result)
    {
        const FString Folder = TEXT("/Game/AI插件测试/关卡三_二次元樱花树");
        const FString MapPath = Folder + TEXT("/关卡三_二次元樱花树");
        UWorld* World = nullptr; FString Error;
        if (!BeginShowcaseMap(MapPath, World, Error)) { Result->SetStringField(TEXT("error"), Error); return false; }
        TArray<UPackage*> Packages;
        UMaterial* GroundMaterial = CreateShowcaseMaterial(Folder, TEXT("M_卡通草地"), FLinearColor(0.16f, 0.42f, 0.12f), 0.94f, false, false, Packages);
        UMaterial* BarkMaterial = CreateShowcaseMaterial(Folder, TEXT("M_卡通树干"), FLinearColor(0.24f, 0.055f, 0.025f), 0.72f, false, false, Packages);
        UMaterial* BlossomMaterial = CreateShowcaseMaterial(Folder, TEXT("M_卡通樱花"), FLinearColor(1.0f, 0.28f, 0.55f), 0.45f, false, false, Packages);
        UBlueprint* TreeBlueprint = CreateShowcaseBlueprint(Folder, TEXT("BP_盛开樱花树"), { TEXT("树冠半径"), TEXT("花瓣密度"), TEXT("落花速度") }, Packages);
        UNiagaraSystem* NiagaraSystem = CreateCherryNiagaraSystem(Folder, Packages);
        if (!GroundMaterial || !BarkMaterial || !BlossomMaterial || !TreeBlueprint || !NiagaraSystem) { Result->SetStringField(TEXT("error"), TEXT("Cherry-tree material, blueprint, or Niagara creation failed.")); return false; }

        AddEnvironment(World, TEXT("樱花树"));
        UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
        UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        AddStaticMeshActor(World, Plane, GroundMaterial, TEXT("二次元樱花树草地"), FTransform(FRotator::ZeroRotator, FVector(0, 0, -14), FVector(110, 110, 1)));
        AddStaticMeshActor(World, Cylinder, BarkMaterial, TEXT("樱花树_主树干"), FTransform(FRotator::ZeroRotator, FVector(0, 0, 540), FVector(1.65f, 1.65f, 10.8f)));
        FRandomStream Random(520);
        for (int32 Index = 0; Index < 18; ++Index)
        {
            const float Angle = Index * 20.0f + Random.FRandRange(-8.0f, 8.0f); const float Radius = Random.FRandRange(160.0f, 420.0f); const float Height = Random.FRandRange(560.0f, 1080.0f);
            AddStaticMeshActor(World, Cylinder, BarkMaterial, FString::Printf(TEXT("樱花树_枝干_%02d"), Index + 1), FTransform(FRotator(Random.FRandRange(-18, 28), Angle, Random.FRandRange(-30, 30)), FVector(FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius, FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius, Height), FVector(0.32f, 0.32f, Random.FRandRange(2.2f, 4.8f))));
        }
        AActor* Crown = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
        if (!Crown || !Sphere) { Result->SetStringField(TEXT("error"), TEXT("Cherry blossom crown creation failed.")); return false; }
        Crown->SetActorLabel(TEXT("盛开樱花树_卡通花冠"));
        UHierarchicalInstancedStaticMeshComponent* Blossoms = NewObject<UHierarchicalInstancedStaticMeshComponent>(Crown, TEXT("卡通樱花团"));
        Crown->SetRootComponent(Blossoms); Blossoms->RegisterComponent(); Blossoms->SetStaticMesh(Sphere); Blossoms->SetMaterial(0, BlossomMaterial); Blossoms->SetCullDistances(2000, 22000);
        constexpr int32 BlossomClusters = 180;
        for (int32 Index = 0; Index < BlossomClusters; ++Index)
        {
            const FVector Direction = Random.VRand(); const FVector Location = FVector(Direction.X * Random.FRandRange(350, 1300), Direction.Y * Random.FRandRange(350, 1300), 1200 + Direction.Z * Random.FRandRange(250, 1050));
            const float Scale = Random.FRandRange(1.1f, 2.7f); Blossoms->AddInstance(FTransform(FRotator(Random.FRandRange(-15, 15), Random.FRandRange(0, 360), Random.FRandRange(-15, 15)), Location, FVector(Scale, Scale, Scale * 0.72f)));
        }
        AActor* NiagaraActor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(FRotator(180, 0, 0), FVector(0, 0, 2250)));
        if (NiagaraActor)
        {
            NiagaraActor->SetActorLabel(TEXT("Niagara_樱花落花效果"));
            UNiagaraComponent* NiagaraComponent = NewObject<UNiagaraComponent>(NiagaraActor, TEXT("樱花飘落粒子"));
            NiagaraActor->SetRootComponent(NiagaraComponent); NiagaraComponent->SetAsset(NiagaraSystem); NiagaraComponent->RegisterComponent(); NiagaraComponent->Activate(true);
        }
        ACameraActor* Camera = World->SpawnActor<ACameraActor>(FVector(-3500, -4300, 1800), FRotator(-12, 40, 0));
        if (Camera) Camera->SetActorLabel(TEXT("樱花树观察相机"));
        const bool bSaved = SaveShowcase(World, MapPath, Packages);
        TArray<TSharedPtr<FJsonValue>> Paths; AddResultPath(Paths, GroundMaterial); AddResultPath(Paths, BarkMaterial); AddResultPath(Paths, BlossomMaterial); AddResultPath(Paths, TreeBlueprint); AddResultPath(Paths, NiagaraSystem);
        Result->SetStringField(TEXT("map"), MapPath); Result->SetArrayField(TEXT("assets"), Paths); Result->SetNumberField(TEXT("blossom_clusters"), BlossomClusters); Result->SetStringField(TEXT("implementation"), TEXT("Cartoon tree is composed from modeled trunk/branch meshes and instanced blossom clusters; the saved Niagara system owns a default sprite emitter rotated to fall downward."));
        Result->SetBoolField(TEXT("saved"), bSaved);
        return bSaved;
    }

    bool ShowcaseCreateHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Requested = Request.QueryParams.FindRef(TEXT("level")).ToLower();
        if (Requested != TEXT("ocean") && Requested != TEXT("grass") && Requested != TEXT("cherry")) return RespondError(TEXT("Query parameter 'level' must be ocean, grass, or cherry."), Complete);
        AsyncTask(ENamedThreads::GameThread, [Requested, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            const bool bOk = Requested == TEXT("ocean") ? BuildOceanShowcase(Result) : (Requested == TEXT("grass") ? BuildGrassShowcase(Result) : BuildCherryShowcase(Result));
            Result->SetBoolField(TEXT("ok"), bOk); Result->SetStringField(TEXT("level"), Requested); Result->SetStringField(TEXT("persistence"), bOk ? TEXT("Saved permanently in the project Content directory; no generated map or asset is scheduled for cleanup.") : TEXT("No automatic cleanup was performed; inspect the reported error before retrying."));
            Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    static AActor* FindShowcaseActorByLabel(UWorld* World, const FString& Label)
    {
        if (!World) return nullptr;
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->GetActorLabel() == Label) return *It;
        }
        return nullptr;
    }

    static bool WireGrassRenderTargetInput(UMaterial* Material, UTextureRenderTarget2D* Target)
    {
        if (!Material || !Target || !Material->GetEditorOnlyData()) return false;
        if (FindObject<UMaterialExpression>(Material, TEXT("RT交互世界坐标采样"))) return true;
        UMaterialExpression* PreviousOffset = Material->GetEditorOnlyData()->WorldPositionOffset.Expression;
        UMaterialExpressionWorldPosition* WorldPosition = NewObject<UMaterialExpressionWorldPosition>(Material, TEXT("RT交互世界坐标"));
        UMaterialExpressionConstant* CoordinateScale = NewObject<UMaterialExpressionConstant>(Material, TEXT("RT交互坐标缩放")); CoordinateScale->R = 0.00004f;
        UMaterialExpressionMultiply* ScaledWorldPosition = NewObject<UMaterialExpressionMultiply>(Material, TEXT("RT交互缩放世界坐标")); ScaledWorldPosition->A.Connect(0, WorldPosition); ScaledWorldPosition->B.Connect(0, CoordinateScale);
        UMaterialExpressionConstant3Vector* CoordinateOffset = NewObject<UMaterialExpressionConstant3Vector>(Material, TEXT("RT交互坐标偏移")); CoordinateOffset->Constant = FLinearColor(0.5f, 0.5f, 0.0f);
        UMaterialExpressionAdd* CaptureUV = NewObject<UMaterialExpressionAdd>(Material, TEXT("RT交互UV")); CaptureUV->A.Connect(0, ScaledWorldPosition); CaptureUV->B.Connect(0, CoordinateOffset);
        UMaterialExpressionTextureSample* FieldSample = NewObject<UMaterialExpressionTextureSample>(Material, TEXT("RT交互世界坐标采样")); FieldSample->Texture = Target; FieldSample->Coordinates.Connect(0, CaptureUV);
        UMaterialExpressionConstant3Vector* BendVector = NewObject<UMaterialExpressionConstant3Vector>(Material, TEXT("RT交互压草向量")); BendVector->Constant = FLinearColor(18.0f, 6.0f, 0.0f);
        UMaterialExpressionMultiply* RtBend = NewObject<UMaterialExpressionMultiply>(Material, TEXT("RT交互压草")); RtBend->A.Connect(0, FieldSample); RtBend->B.Connect(0, BendVector);
        UMaterialExpressionAdd* FinalOffset = NewObject<UMaterialExpressionAdd>(Material, TEXT("RT交互与全局风叠加"));
        if (PreviousOffset) FinalOffset->A.Connect(0, PreviousOffset); FinalOffset->B.Connect(0, RtBend);
        for (UMaterialExpression* Expression : { static_cast<UMaterialExpression*>(WorldPosition), static_cast<UMaterialExpression*>(CoordinateScale), static_cast<UMaterialExpression*>(ScaledWorldPosition), static_cast<UMaterialExpression*>(CoordinateOffset), static_cast<UMaterialExpression*>(CaptureUV), static_cast<UMaterialExpression*>(FieldSample), static_cast<UMaterialExpression*>(BendVector), static_cast<UMaterialExpression*>(RtBend), static_cast<UMaterialExpression*>(FinalOffset) }) Material->GetExpressionCollection().AddExpression(Expression);
        Material->GetEditorOnlyData()->WorldPositionOffset.Expression = FinalOffset;
        Material->PostEditChange(); Material->MarkPackageDirty();
        return true;
    }

    static UMaterial* CreateStylizedGrassGroundMaterial(const FString& Folder, UTexture2D* GroundTexture, TArray<UPackage*>& Packages)
    {
        const FString Name = TEXT("M_二次元草地地表");
        const FString AssetPath = UE5AIMcp::PackageNameFor(Folder, Name);
        UMaterial* Material = LoadObject<UMaterial>(nullptr, *AssetPath);
        if (!Material)
        {
            UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
            Material = Cast<UMaterial>(FAssetToolsModule::GetModule().Get().CreateAsset(Name, Folder, UMaterial::StaticClass(), Factory));
        }
        if (!Material || !GroundTexture || !Material->GetEditorOnlyData()) return nullptr;
        if (FindObject<UMaterialExpression>(Material, TEXT("二次元地表纹理")))
        {
            if (UMaterialExpressionTextureSample* ExistingSample = FindObject<UMaterialExpressionTextureSample>(Material, TEXT("二次元地表纹理")))
            {
                ExistingSample->Texture = GroundTexture;
                Material->PostEditChange(); Material->MarkPackageDirty();
            }
            Packages.AddUnique(Material->GetOutermost());
            return Material;
        }

        Material->Modify();
        UMaterialExpressionTextureCoordinate* UV = NewObject<UMaterialExpressionTextureCoordinate>(Material, TEXT("地表高频平铺UV"));
        UV->UTiling = 34.0f; UV->VTiling = 34.0f;
        UMaterialExpressionTextureSample* Sample = NewObject<UMaterialExpressionTextureSample>(Material, TEXT("二次元地表纹理"));
        Sample->Texture = GroundTexture; Sample->Coordinates.Connect(0, UV);
        UMaterialExpressionConstant3Vector* Tint = NewObject<UMaterialExpressionConstant3Vector>(Material, TEXT("二次元地表柔和染色"));
        Tint->Constant = FLinearColor(0.74f, 0.96f, 0.54f);
        UMaterialExpressionMultiply* BaseColor = NewObject<UMaterialExpressionMultiply>(Material, TEXT("二次元地表颜色"));
        BaseColor->A.Connect(0, Sample); BaseColor->B.Connect(0, Tint);
        UMaterialExpressionConstant* Roughness = NewObject<UMaterialExpressionConstant>(Material, TEXT("地表粗糙度"));
        Roughness->R = 0.92f;
        for (UMaterialExpression* Expression : { static_cast<UMaterialExpression*>(UV), static_cast<UMaterialExpression*>(Sample), static_cast<UMaterialExpression*>(Tint), static_cast<UMaterialExpression*>(BaseColor), static_cast<UMaterialExpression*>(Roughness) }) Material->GetExpressionCollection().AddExpression(Expression);
        Material->GetEditorOnlyData()->BaseColor.Expression = BaseColor;
        Material->GetEditorOnlyData()->Roughness.Expression = Roughness;
        Material->PostEditChange(); Material->MarkPackageDirty();
        TrackShowcaseAsset(Material, Packages);
        return Material;
    }

    static UMaterial* CreateStylizedGrassCardMaterial(const FString& Folder, UTexture2D* GrassTexture, UTextureRenderTarget2D* InteractionRT, TArray<UPackage*>& Packages)
    {
        const FString Name = TEXT("M_二次元草簇_RT交互");
        const FString AssetPath = UE5AIMcp::PackageNameFor(Folder, Name);
        UMaterial* Material = LoadObject<UMaterial>(nullptr, *AssetPath);
        if (!Material)
        {
            UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
            Material = Cast<UMaterial>(FAssetToolsModule::GetModule().Get().CreateAsset(Name, Folder, UMaterial::StaticClass(), Factory));
        }
        if (!Material || !GrassTexture || !InteractionRT || !Material->GetEditorOnlyData()) return nullptr;
        if (FindObject<UMaterialExpression>(Material, TEXT("草簇透明颜色纹理")))
        {
            bool bChanged = false;
            if (UMaterialExpressionTextureSample* ExistingGrassSample = FindObject<UMaterialExpressionTextureSample>(Material, TEXT("草簇透明颜色纹理")))
            {
                ExistingGrassSample->Texture = GrassTexture;
                bChanged = true;
            }
            UMaterialExpressionTextureSample* ExistingFieldSample = FindObject<UMaterialExpressionTextureSample>(Material, TEXT("RT交互世界坐标采样"));
            UMaterialExpression* ExistingCaptureUV = FindObject<UMaterialExpression>(Material, TEXT("RT交互UV"));
            if (ExistingFieldSample && ExistingFieldSample->Texture != InteractionRT) { ExistingFieldSample->Texture = InteractionRT; bChanged = true; }
            if (ExistingFieldSample && ExistingCaptureUV && !FindObject<UMaterialExpression>(Material, TEXT("RT交互二维UV")))
            {
                UMaterialExpressionComponentMask* CaptureUV2D = NewObject<UMaterialExpressionComponentMask>(Material, TEXT("RT交互二维UV"));
                CaptureUV2D->Input.Connect(0, ExistingCaptureUV); CaptureUV2D->R = 1; CaptureUV2D->G = 1; CaptureUV2D->B = 0; CaptureUV2D->A = 0;
                Material->GetExpressionCollection().AddExpression(CaptureUV2D);
                ExistingFieldSample->Coordinates.Connect(0, CaptureUV2D);
                bChanged = true;
            }
            Material->bUsedWithInstancedStaticMeshes = true;
            if (bChanged) { Material->PostEditChange(); Material->MarkPackageDirty(); }
            Packages.AddUnique(Material->GetOutermost());
            return Material;
        }

        Material->Modify();
        Material->BlendMode = BLEND_Masked;
        Material->TwoSided = true;
        Material->OpacityMaskClipValue = 0.18f;
        Material->bUsedWithInstancedStaticMeshes = true;

        UMaterialExpressionTextureCoordinate* GrassUV = NewObject<UMaterialExpressionTextureCoordinate>(Material, TEXT("草簇卡片UV"));
        UMaterialExpressionTextureSample* GrassSample = NewObject<UMaterialExpressionTextureSample>(Material, TEXT("草簇透明颜色纹理"));
        GrassSample->Texture = GrassTexture; GrassSample->Coordinates.Connect(0, GrassUV);
        UMaterialExpressionConstant3Vector* GrassTint = NewObject<UMaterialExpressionConstant3Vector>(Material, TEXT("草簇明亮配色"));
        GrassTint->Constant = FLinearColor(0.86f, 1.0f, 0.54f);
        UMaterialExpressionMultiply* GrassColor = NewObject<UMaterialExpressionMultiply>(Material, TEXT("草簇颜色"));
        GrassColor->A.Connect(0, GrassSample); GrassColor->B.Connect(0, GrassTint);
        UMaterialExpressionConstant* Roughness = NewObject<UMaterialExpressionConstant>(Material, TEXT("草簇粗糙度"));
        Roughness->R = 0.78f;

        UMaterialExpressionComponentMask* BladeHeight = NewObject<UMaterialExpressionComponentMask>(Material, TEXT("草叶根部锚定遮罩"));
        BladeHeight->Input.Connect(0, GrassUV); BladeHeight->R = 0; BladeHeight->G = 1; BladeHeight->B = 0; BladeHeight->A = 0;
        UMaterialExpressionTime* Time = NewObject<UMaterialExpressionTime>(Material, TEXT("全局风场时间"));
        UMaterialExpressionSine* WindSine = NewObject<UMaterialExpressionSine>(Material, TEXT("全局风场摆动"));
        WindSine->Input.Connect(0, Time); WindSine->Period = 2.7f;
        UMaterialExpressionMultiply* WindWeight = NewObject<UMaterialExpressionMultiply>(Material, TEXT("草尖风摆权重"));
        WindWeight->A.Connect(0, WindSine); WindWeight->B.Connect(0, BladeHeight);
        UMaterialExpressionConstant3Vector* WindVector = NewObject<UMaterialExpressionConstant3Vector>(Material, TEXT("全局风场方向"));
        WindVector->Constant = FLinearColor(34.0f, 14.0f, 0.0f);
        UMaterialExpressionMultiply* WindOffset = NewObject<UMaterialExpressionMultiply>(Material, TEXT("全局风场草叶位移"));
        WindOffset->A.Connect(0, WindWeight); WindOffset->B.Connect(0, WindVector);

        UMaterialExpressionWorldPosition* WorldPosition = NewObject<UMaterialExpressionWorldPosition>(Material, TEXT("RT交互世界坐标"));
        UMaterialExpressionConstant* CoordinateScale = NewObject<UMaterialExpressionConstant>(Material, TEXT("RT交互坐标缩放"));
        CoordinateScale->R = 1.0f / 29000.0f;
        UMaterialExpressionMultiply* ScaledWorldPosition = NewObject<UMaterialExpressionMultiply>(Material, TEXT("RT交互缩放世界坐标"));
        ScaledWorldPosition->A.Connect(0, WorldPosition); ScaledWorldPosition->B.Connect(0, CoordinateScale);
        UMaterialExpressionConstant3Vector* CoordinateOffset = NewObject<UMaterialExpressionConstant3Vector>(Material, TEXT("RT交互坐标偏移"));
        CoordinateOffset->Constant = FLinearColor(0.5f, 0.5f, 0.0f);
        UMaterialExpressionAdd* CaptureUV = NewObject<UMaterialExpressionAdd>(Material, TEXT("RT交互UV"));
        CaptureUV->A.Connect(0, ScaledWorldPosition); CaptureUV->B.Connect(0, CoordinateOffset);
        UMaterialExpressionComponentMask* CaptureUV2D = NewObject<UMaterialExpressionComponentMask>(Material, TEXT("RT交互二维UV"));
        CaptureUV2D->Input.Connect(0, CaptureUV); CaptureUV2D->R = 1; CaptureUV2D->G = 1; CaptureUV2D->B = 0; CaptureUV2D->A = 0;
        UMaterialExpressionTextureSample* FieldSample = NewObject<UMaterialExpressionTextureSample>(Material, TEXT("RT交互世界坐标采样"));
        FieldSample->Texture = InteractionRT; FieldSample->Coordinates.Connect(0, CaptureUV2D);
        UMaterialExpressionComponentMask* FieldMask = NewObject<UMaterialExpressionComponentMask>(Material, TEXT("RT交互强度"));
        FieldMask->Input.Connect(0, FieldSample); FieldMask->R = 1; FieldMask->G = 0; FieldMask->B = 0; FieldMask->A = 0;
        UMaterialExpressionMultiply* InteractionWeight = NewObject<UMaterialExpressionMultiply>(Material, TEXT("RT交互草尖权重"));
        InteractionWeight->A.Connect(0, FieldMask); InteractionWeight->B.Connect(0, BladeHeight);
        UMaterialExpressionConstant3Vector* InteractionVector = NewObject<UMaterialExpressionConstant3Vector>(Material, TEXT("RT交互压草方向"));
        InteractionVector->Constant = FLinearColor(105.0f, 28.0f, 0.0f);
        UMaterialExpressionMultiply* InteractionOffset = NewObject<UMaterialExpressionMultiply>(Material, TEXT("RT交互压草位移"));
        InteractionOffset->A.Connect(0, InteractionWeight); InteractionOffset->B.Connect(0, InteractionVector);
        UMaterialExpressionAdd* FinalOffset = NewObject<UMaterialExpressionAdd>(Material, TEXT("全局风场与RT交互叠加"));
        FinalOffset->A.Connect(0, WindOffset); FinalOffset->B.Connect(0, InteractionOffset);

        for (UMaterialExpression* Expression : { static_cast<UMaterialExpression*>(GrassUV), static_cast<UMaterialExpression*>(GrassSample), static_cast<UMaterialExpression*>(GrassTint), static_cast<UMaterialExpression*>(GrassColor), static_cast<UMaterialExpression*>(Roughness), static_cast<UMaterialExpression*>(BladeHeight), static_cast<UMaterialExpression*>(Time), static_cast<UMaterialExpression*>(WindSine), static_cast<UMaterialExpression*>(WindWeight), static_cast<UMaterialExpression*>(WindVector), static_cast<UMaterialExpression*>(WindOffset), static_cast<UMaterialExpression*>(WorldPosition), static_cast<UMaterialExpression*>(CoordinateScale), static_cast<UMaterialExpression*>(ScaledWorldPosition), static_cast<UMaterialExpression*>(CoordinateOffset), static_cast<UMaterialExpression*>(CaptureUV), static_cast<UMaterialExpression*>(CaptureUV2D), static_cast<UMaterialExpression*>(FieldSample), static_cast<UMaterialExpression*>(FieldMask), static_cast<UMaterialExpression*>(InteractionWeight), static_cast<UMaterialExpression*>(InteractionVector), static_cast<UMaterialExpression*>(InteractionOffset), static_cast<UMaterialExpression*>(FinalOffset) }) Material->GetExpressionCollection().AddExpression(Expression);
        Material->GetEditorOnlyData()->BaseColor.Expression = GrassColor;
        Material->GetEditorOnlyData()->OpacityMask.Connect(4, GrassSample);
        Material->GetEditorOnlyData()->Roughness.Expression = Roughness;
        Material->GetEditorOnlyData()->WorldPositionOffset.Expression = FinalOffset;
        Material->PostEditChange(); Material->MarkPackageDirty();
        TrackShowcaseAsset(Material, Packages);
        return Material;
    }

    static UBlueprint* CreateStylizedGrassInteractionBlueprint(const FString& Folder, UTextureRenderTarget2D* InteractionRT, TArray<UPackage*>& Packages)
    {
        const FString Name = TEXT("BP_第三人称RT草地交互控制器");
        const FString AssetPath = UE5AIMcp::PackageNameFor(Folder, Name);
        UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
        if (!Blueprint)
        {
            UPackage* Package = CreatePackage(*AssetPath);
            Blueprint = FKismetEditorUtilities::CreateBlueprint(AUE5AIStylizedGrassInteractor::StaticClass(), Package, *Name, BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), FName(TEXT("UE5AIStylizedGrass")));
        }
        if (!Blueprint) return nullptr;
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        if (AUE5AIStylizedGrassInteractor* Defaults = Cast<AUE5AIStylizedGrassInteractor>(Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr))
        {
            Defaults->Modify();
            Defaults->InteractionRenderTarget = InteractionRT;
            Defaults->InteractionRadius = 180.0f;
            Defaults->CaptureHeight = 12000.0f;
            Defaults->CaptureWidth = 29000.0f;
        }
        Blueprint->MarkPackageDirty();
        TrackShowcaseAsset(Blueprint, Packages);
        return Blueprint;
    }

    static void SetGrassShowcaseViewport()
    {
        if (!GEditor) return;
        for (FEditorViewportClient* Client : GEditor->GetAllViewportClients())
        {
            if (Client && Client->IsPerspective())
            {
                Client->SetViewLocation(FVector(-4300.0f, -5700.0f, 520.0f));
                Client->SetViewRotation(FRotator(-3.0f, 53.0f, 0.0f));
                break;
            }
        }
        GEditor->RedrawAllViewports();
    }

    static bool RebuildStylizedGrassShowcase(TSharedRef<FJsonObject>& Result)
    {
        const FString Folder = TEXT("/Game/AI插件测试/关卡二_交互草地");
        const FString MapPath = Folder + TEXT("/关卡二_全局风场草地");
        const FString Filename = FPackageName::LongPackageNameToFilename(MapPath, FPackageName::GetMapPackageExtension());
        UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(Filename);
        UTexture2D* GroundTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/AI插件测试/关卡二_交互草地/T_二次元草地底图.T_二次元草地底图"));
        UTexture2D* GrassTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/AI插件测试/关卡二_交互草地/T_二次元草簇.T_二次元草簇"));
        UTextureRenderTarget2D* InteractionRT = LoadObject<UTextureRenderTarget2D>(nullptr, TEXT("/Game/AI插件测试/关卡二_交互草地/RT_草地交互与风场.RT_草地交互与风场"));
        if (!World || !GroundTexture || !GrassTexture || !InteractionRT)
        {
            Result->SetStringField(TEXT("error"), TEXT("The grass map, generated texture assets, or persistent RT could not be resolved. Import both SourceArt PNG files before rebuilding."));
            return false;
        }

        TArray<UPackage*> Packages;
        UMaterial* GroundMaterial = CreateStylizedGrassGroundMaterial(Folder, GroundTexture, Packages);
        UMaterial* GrassMaterial = CreateStylizedGrassCardMaterial(Folder, GrassTexture, InteractionRT, Packages);
        UBlueprint* ControllerBlueprint = CreateStylizedGrassInteractionBlueprint(Folder, InteractionRT, Packages);
        UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
        if (!GroundMaterial || !GrassMaterial || !ControllerBlueprint || !Plane)
        {
            Result->SetStringField(TEXT("error"), TEXT("The stylized grass material, interaction Blueprint, or plane mesh could not be created."));
            return false;
        }

        if (AActor* OldField = FindShowcaseActorByLabel(World, TEXT("模型草地_HISM视域裁剪")))
        {
            OldField->SetActorHiddenInGame(true);
            OldField->SetIsTemporarilyHiddenInEditor(true);
            OldField->SetActorLabel(TEXT("旧草地模型基线_已隐藏"));
        }
        if (AActor* OldCapture = FindShowcaseActorByLabel(World, TEXT("RT交互场景捕获")))
        {
            OldCapture->SetActorHiddenInGame(true);
            OldCapture->SetIsTemporarilyHiddenInEditor(true);
            OldCapture->SetActorLabel(TEXT("旧RT场景捕获_已隐藏"));
        }
        if (AActor* OldInteractor = FindShowcaseActorByLabel(World, TEXT("交互者_RT写入示例")))
        {
            OldInteractor->SetActorHiddenInGame(true);
            OldInteractor->SetIsTemporarilyHiddenInEditor(true);
            OldInteractor->SetActorLabel(TEXT("旧RT交互示例_已隐藏"));
        }

        AStaticMeshActor* Ground = Cast<AStaticMeshActor>(FindShowcaseActorByLabel(World, TEXT("草地地表_基础关卡")));
        if (!Ground) Ground = AddStaticMeshActor(World, Plane, GroundMaterial, TEXT("草地地表_基础关卡"), FTransform(FRotator::ZeroRotator, FVector(0, 0, -10), FVector(145, 145, 1)));
        if (Ground) { Ground->GetStaticMeshComponent()->SetMaterial(0, GroundMaterial); Ground->SetActorLabel(TEXT("二次元草地_地表纹理层")); }

        AActor* GrassField = FindShowcaseActorByLabel(World, TEXT("二次元草簇_分层HISM视域裁剪"));
        if (GrassField)
        {
            TArray<UActorComponent*> ExistingGrassComponents;
            GrassField->GetComponents(UHierarchicalInstancedStaticMeshComponent::StaticClass(), ExistingGrassComponents);
            if (ExistingGrassComponents.Num() < 5)
            {
                GrassField->SetActorHiddenInGame(true);
                GrassField->SetIsTemporarilyHiddenInEditor(true);
                GrassField->SetActorLabel(TEXT("二次元草簇_初版组件_已隐藏"));
                GrassField = nullptr;
            }
        }
        int32 TotalCardInstances = 0;
        if (!GrassField)
        {
            GrassField = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
            if (!GrassField) { Result->SetStringField(TEXT("error"), TEXT("Could not create the layered grass field actor.")); return false; }
            GrassField->SetActorLabel(TEXT("二次元草簇_分层HISM视域裁剪"));
            GrassField->Tags = { TEXT("二次元草簇"), TEXT("HISM视域裁剪"), TEXT("全局风场"), TEXT("RT角色交互") };
            UHierarchicalInstancedStaticMeshComponent* NearA = NewObject<UHierarchicalInstancedStaticMeshComponent>(GrassField, TEXT("近景草簇_主卡片"));
            GrassField->AddInstanceComponent(NearA); GrassField->SetRootComponent(NearA); NearA->RegisterComponent(); NearA->SetStaticMesh(Plane); NearA->SetMaterial(0, GrassMaterial); NearA->SetCullDistances(7000, 18000); NearA->SetCastShadow(false);
            UHierarchicalInstancedStaticMeshComponent* NearB = NewObject<UHierarchicalInstancedStaticMeshComponent>(GrassField, TEXT("近景草簇_交叉卡片"));
            GrassField->AddInstanceComponent(NearB); NearB->SetupAttachment(NearA); NearB->RegisterComponent(); NearB->SetStaticMesh(Plane); NearB->SetMaterial(0, GrassMaterial); NearB->SetCullDistances(7000, 18000); NearB->SetCastShadow(false);
            UHierarchicalInstancedStaticMeshComponent* Mid = NewObject<UHierarchicalInstancedStaticMeshComponent>(GrassField, TEXT("中远景草簇_视域裁剪"));
            GrassField->AddInstanceComponent(Mid); Mid->SetupAttachment(NearA); Mid->RegisterComponent(); Mid->SetStaticMesh(Plane); Mid->SetMaterial(0, GrassMaterial); Mid->SetCullDistances(9000, 19000); Mid->SetCastShadow(false);
            UHierarchicalInstancedStaticMeshComponent* ForegroundA = NewObject<UHierarchicalInstancedStaticMeshComponent>(GrassField, TEXT("前景高草_主卡片"));
            GrassField->AddInstanceComponent(ForegroundA); ForegroundA->SetupAttachment(NearA); ForegroundA->RegisterComponent(); ForegroundA->SetStaticMesh(Plane); ForegroundA->SetMaterial(0, GrassMaterial); ForegroundA->SetCullDistances(6500, 15000); ForegroundA->SetCastShadow(false);
            UHierarchicalInstancedStaticMeshComponent* ForegroundB = NewObject<UHierarchicalInstancedStaticMeshComponent>(GrassField, TEXT("前景高草_交叉卡片"));
            GrassField->AddInstanceComponent(ForegroundB); ForegroundB->SetupAttachment(NearA); ForegroundB->RegisterComponent(); ForegroundB->SetStaticMesh(Plane); ForegroundB->SetMaterial(0, GrassMaterial); ForegroundB->SetCullDistances(6500, 15000); ForegroundB->SetCastShadow(false);

            FRandomStream Random(20261005);
            for (int32 Index = 0; Index < 1450; ++Index)
            {
                const FVector Location(Random.FRandRange(-7200.0f, 7200.0f), Random.FRandRange(-7200.0f, 7200.0f), 18.0f);
                const float Yaw = Random.FRandRange(0.0f, 360.0f);
                const FVector Scale(Random.FRandRange(0.75f, 1.1f), Random.FRandRange(0.9f, 1.45f), 1.0f);
                NearA->AddInstance(FTransform(FRotator(0.0f, Yaw, 90.0f), Location, Scale));
                NearB->AddInstance(FTransform(FRotator(0.0f, Yaw + 90.0f, 90.0f), Location, Scale));
                TotalCardInstances += 2;
            }
            for (int32 Index = 0; Index < 1900; ++Index)
            {
                const FVector Location(Random.FRandRange(-11800.0f, 11800.0f), Random.FRandRange(-11800.0f, 11800.0f), 14.0f);
                const FVector Scale(Random.FRandRange(0.48f, 0.78f), Random.FRandRange(0.5f, 0.9f), 1.0f);
                Mid->AddInstance(FTransform(FRotator(0.0f, Random.FRandRange(0.0f, 360.0f), 90.0f), Location, Scale));
                ++TotalCardInstances;
            }
            for (int32 Index = 0; Index < 520; ++Index)
            {
                const FVector Location(Random.FRandRange(-6200.0f, -2400.0f), Random.FRandRange(-7600.0f, -3800.0f), 25.0f);
                const float Yaw = Random.FRandRange(0.0f, 360.0f);
                const FVector Scale(Random.FRandRange(0.85f, 1.25f), Random.FRandRange(1.05f, 1.7f), 1.0f);
                ForegroundA->AddInstance(FTransform(FRotator(0.0f, Yaw, 90.0f), Location, Scale));
                ForegroundB->AddInstance(FTransform(FRotator(0.0f, Yaw + 90.0f, 90.0f), Location, Scale));
                TotalCardInstances += 2;
            }
        }
        else
        {
            TArray<UActorComponent*> Components;
            GrassField->GetComponents(UHierarchicalInstancedStaticMeshComponent::StaticClass(), Components);
            for (UActorComponent* Component : Components)
            {
                if (UHierarchicalInstancedStaticMeshComponent* Instances = Cast<UHierarchicalInstancedStaticMeshComponent>(Component))
                {
                    Instances->SetMaterial(0, GrassMaterial);
                    TotalCardInstances += Instances->GetInstanceCount();
                }
            }
        }

        bool bThirdPersonGameModeBound = false;
        if (UClass* ThirdPersonGameMode = LoadClass<AGameModeBase>(nullptr, TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode.BP_ThirdPersonGameMode_C")))
        {
            if (AWorldSettings* WorldSettings = World->GetWorldSettings())
            {
                WorldSettings->Modify();
                WorldSettings->DefaultGameMode = ThirdPersonGameMode;
                bThirdPersonGameModeBound = true;
            }
        }
        if (!FindShowcaseActorByLabel(World, TEXT("第三人称角色出生点")))
        {
            APlayerStart* PlayerStart = World->SpawnActor<APlayerStart>(FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator);
            if (PlayerStart) PlayerStart->SetActorLabel(TEXT("第三人称角色出生点"));
        }

        AUE5AIStylizedGrassInteractor* Controller = nullptr;
        for (TActorIterator<AUE5AIStylizedGrassInteractor> It(World); It; ++It) { Controller = *It; break; }
        if (!Controller)
        {
            Controller = Cast<AUE5AIStylizedGrassInteractor>(World->SpawnActor<AActor>(ControllerBlueprint->GeneratedClass, FTransform::Identity));
        }
        if (!Controller) { Result->SetStringField(TEXT("error"), TEXT("Could not place the saved Blueprint grass-interaction controller.")); return false; }
        Controller->SetActorLabel(TEXT("BP_第三人称RT草地交互控制器_运行实例"));
        Controller->Tags = { TEXT("蓝图RT交互"), TEXT("第三人称Pawn追踪"), TEXT("全局风场") };
        Controller->InteractionRenderTarget = InteractionRT;
        Controller->InteractionRadius = 180.0f;
        Controller->CaptureHeight = 12000.0f;
        Controller->CaptureWidth = 29000.0f;
        Controller->Modify();

        if (!FindShowcaseActorByLabel(World, TEXT("二次元草地观察相机")))
        {
            ACameraActor* Camera = World->SpawnActor<ACameraActor>(FVector(-4300.0f, -5700.0f, 520.0f), FRotator(-3.0f, 53.0f, 0.0f));
            if (Camera) Camera->SetActorLabel(TEXT("二次元草地观察相机"));
        }
        if (AExponentialHeightFog* Fog = Cast<AExponentialHeightFog>(FindShowcaseActorByLabel(World, TEXT("草地_高度雾")))) Fog->GetComponent()->SetFogDensity(0.0018f);
        if (ADirectionalLight* Sun = Cast<ADirectionalLight>(FindShowcaseActorByLabel(World, TEXT("草地_太阳光")))) Sun->GetLightComponent()->SetIntensity(7.0f);
        SetGrassShowcaseViewport();

        Packages.AddUnique(InteractionRT->GetOutermost());
        const bool bSaved = SaveShowcase(World, MapPath, Packages);
        Result->SetBoolField(TEXT("saved"), bSaved);
        Result->SetStringField(TEXT("map"), MapPath);
        Result->SetNumberField(TEXT("grass_card_instances"), TotalCardInstances);
        Result->SetStringField(TEXT("ground_material"), GroundMaterial->GetPathName());
        Result->SetStringField(TEXT("grass_material"), GrassMaterial->GetPathName());
        Result->SetStringField(TEXT("interaction_blueprint"), ControllerBlueprint->GetPathName());
        Result->SetBoolField(TEXT("third_person_game_mode_bound"), bThirdPersonGameModeBound);
        Result->SetStringField(TEXT("implementation"), TEXT("Layered HISM grass cards use a transparent stylized clump texture; the ground uses a separately tiled painted surface. Global WPO wind is combined with a world-position RT mask written by the Blueprint-derived controller that follows the PIE player pawn."));
        return bSaved;
    }

    bool StylizedGrassRebuildHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            const bool bOk = RebuildStylizedGrassShowcase(Result);
            Result->SetBoolField(TEXT("ok"), bOk);
            Result->SetStringField(TEXT("persistence"), TEXT("The grass level is enhanced in place. Existing maps and assets are retained; the earlier baseline actors are only hidden."));
            Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool StylizedGrassStatusHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            UWorld* World = nullptr;
            if (GEditor)
            {
                World = GEditor->PlayWorld.Get();
                if (!World) World = GEditor->GetEditorWorldContext().World();
            }
            AUE5AIStylizedGrassInteractor* Controller = nullptr;
            if (World) for (TActorIterator<AUE5AIStylizedGrassInteractor> It(World); It; ++It) { Controller = *It; break; }
            Result->SetBoolField(TEXT("ok"), World != nullptr && Controller != nullptr);
            Result->SetBoolField(TEXT("pie_running"), GEditor && GEditor->PlayWorld != nullptr);
            Result->SetStringField(TEXT("world"), World ? World->GetPathName() : TEXT(""));
            Result->SetStringField(TEXT("controller"), Controller ? Controller->GetPathName() : TEXT(""));
            Result->SetStringField(TEXT("tracked_pawn"), Controller && Controller->GetTrackedPawn() ? Controller->GetTrackedPawn()->GetPathName() : TEXT(""));
            Result->SetStringField(TEXT("last_rt_capture_location"), Controller ? Controller->GetLastCapturedWorldLocation().ToString() : TEXT(""));
            Result->SetBoolField(TEXT("render_target_assigned"), Controller && Controller->InteractionRenderTarget != nullptr);
            Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    static bool EnhanceGrassShowcase(TSharedRef<FJsonObject>& Result)
    {
        const FString MapPath = TEXT("/Game/AI插件测试/关卡二_交互草地/关卡二_全局风场草地");
        const FString Filename = FPackageName::LongPackageNameToFilename(MapPath, FPackageName::GetMapPackageExtension());
        UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(Filename);
        UMaterial* GrassMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Game/AI插件测试/关卡二_交互草地/M_模型草_全局风场.M_模型草_全局风场"));
        UTextureRenderTarget2D* Target = LoadObject<UTextureRenderTarget2D>(nullptr, TEXT("/Game/AI插件测试/关卡二_交互草地/RT_草地交互与风场.RT_草地交互与风场"));
        ASceneCapture2D* Capture = Cast<ASceneCapture2D>(FindShowcaseActorByLabel(World, TEXT("RT交互场景捕获")));
        AActor* Interactor = FindShowcaseActorByLabel(World, TEXT("交互者_RT写入示例"));
        if (!World || !GrassMaterial || !Target || !Capture || !Interactor) { Result->SetStringField(TEXT("error"), TEXT("Grass RT enhancement could not resolve the saved map or its required assets.")); return false; }
        USceneCaptureComponent2D* CaptureComponent = Capture->GetCaptureComponent2D();
        CaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
        CaptureComponent->ClearShowOnlyComponents(); CaptureComponent->ShowOnlyActorComponents(Interactor); CaptureComponent->TextureTarget = Target; CaptureComponent->CaptureScene();
        const bool bWired = WireGrassRenderTargetInput(GrassMaterial, Target);
        TArray<UPackage*> Packages = { GrassMaterial->GetOutermost(), Target->GetOutermost() };
        const bool bSaved = bWired && SaveShowcase(World, MapPath, Packages);
        Result->SetBoolField(TEXT("saved"), bSaved); Result->SetStringField(TEXT("map"), MapPath); Result->SetStringField(TEXT("implementation"), TEXT("The top-down capture now records only the interaction proxy into the RT; world-position UVs sample that RT and add a localized bend to the existing global wind WPO."));
        return bSaved;
    }

    static bool EnhanceCherryShowcase(TSharedRef<FJsonObject>& Result)
    {
        const FString MapPath = TEXT("/Game/AI插件测试/关卡三_二次元樱花树/关卡三_二次元樱花树");
        const FString Filename = FPackageName::LongPackageNameToFilename(MapPath, FPackageName::GetMapPackageExtension());
        UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(Filename);
        if (!World) { Result->SetStringField(TEXT("error"), TEXT("Cherry map could not be loaded for enhancement.")); return false; }
        AActor* NiagaraActor = FindShowcaseActorByLabel(World, TEXT("Niagara_樱花落花效果"));
        if (NiagaraActor) { NiagaraActor->SetActorLocation(FVector(0, 0, 2250)); NiagaraActor->SetActorRotation(FRotator(180, 0, 0)); }
        bool bPetalsCreated = false;
        if (!FindShowcaseActorByLabel(World, TEXT("Niagara落花_粉色花瓣可视化")))
        {
            UStaticMesh* PetalMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
            UMaterialInterface* PetalMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/AI插件测试/关卡三_二次元樱花树/M_卡通樱花.M_卡通樱花"));
            AActor* Petals = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
            if (!Petals || !PetalMesh || !PetalMaterial) { Result->SetStringField(TEXT("error"), TEXT("Cherry petal visual enhancement could not create its mesh components.")); return false; }
            Petals->SetActorLabel(TEXT("Niagara落花_粉色花瓣可视化")); Petals->Tags = { TEXT("Niagara落花"), TEXT("粉色花瓣") };
            UHierarchicalInstancedStaticMeshComponent* Instances = NewObject<UHierarchicalInstancedStaticMeshComponent>(Petals, TEXT("樱花飘落花瓣代理"));
            Petals->SetRootComponent(Instances); Instances->RegisterComponent(); Instances->SetStaticMesh(PetalMesh); Instances->SetMaterial(0, PetalMaterial); Instances->SetCullDistances(1000, 18000); Instances->SetCastShadow(false);
            FRandomStream Random(20261005);
            for (int32 Index = 0; Index < 280; ++Index)
            {
                const FVector Location(Random.FRandRange(-1600, 1600), Random.FRandRange(-1600, 1600), Random.FRandRange(80, 2250));
                const FVector Scale(Random.FRandRange(0.045f, 0.105f), Random.FRandRange(0.045f, 0.105f), 1.0f);
                Instances->AddInstance(FTransform(FRotator(Random.FRandRange(-35, 35), Random.FRandRange(0, 360), Random.FRandRange(-180, 180)), Location, Scale));
            }
            bPetalsCreated = true;
        }
        const bool bSaved = SaveShowcase(World, MapPath, {});
        Result->SetBoolField(TEXT("saved"), bSaved); Result->SetBoolField(TEXT("pink_petal_visuals_created"), bPetalsCreated); Result->SetStringField(TEXT("map"), MapPath); Result->SetStringField(TEXT("implementation"), TEXT("The Niagara component now emits from above the canopy; persistent pink petal instances provide a stylized visible petal field alongside the live Niagara particles."));
        return bSaved;
    }

    bool ShowcaseEnhanceHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Requested = Request.QueryParams.FindRef(TEXT("level")).ToLower();
        if (Requested != TEXT("grass") && Requested != TEXT("cherry")) return RespondError(TEXT("Query parameter 'level' must be grass or cherry."), Complete);
        AsyncTask(ENamedThreads::GameThread, [Requested, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            const bool bOk = Requested == TEXT("grass") ? EnhanceGrassShowcase(Result) : EnhanceCherryShowcase(Result);
            Result->SetBoolField(TEXT("ok"), bOk); Result->SetStringField(TEXT("level"), Requested); Result->SetStringField(TEXT("persistence"), TEXT("The existing saved level was enhanced in place; no generated assets or maps were deleted."));
            Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool HealthHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("ok"), true);
        Result->SetStringField(TEXT("service"), TEXT("ue5-ai-mcp"));
        Result->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
        Result->SetStringField(TEXT("binding"), TEXT("loopback-only; read-only endpoints"));
        Complete(UE5AIMcp::Respond(Result));
        return true;
    }

    bool ToolsHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        TArray<TSharedPtr<FJsonValue>> List;
        UE5AIMcp::AddTool(List, TEXT("project.inspect"), TEXT("active"), TEXT("Engine, project and plugin identity"));
        UE5AIMcp::AddTool(List, TEXT("editor.inspect"), TEXT("active"), TEXT("World, actor and PIE state"));
        UE5AIMcp::AddTool(List, TEXT("assets.search"), TEXT("active"), TEXT("Asset Registry summary"));
        UE5AIMcp::AddTool(List, TEXT("viewport.capture"), TEXT("contract"), TEXT("Viewport screenshot and camera metadata"));
        UE5AIMcp::AddTool(List, TEXT("logs.read"), TEXT("contract"), TEXT("Editor/compiler logs"));
        UE5AIMcp::AddTool(List, TEXT("actor.mutate"), TEXT("contract"), TEXT("Transaction-backed Actor CRUD"));
        UE5AIMcp::AddTool(List, TEXT("material.instance"), TEXT("contract"), TEXT("Material instance parameters"));
        UE5AIMcp::AddTool(List, TEXT("validation.run"), TEXT("contract"), TEXT("PIE, validation and automation"));
        UE5AIMcp::AddTool(List, TEXT("performance.snapshot"), TEXT("active"), TEXT("Frame, draw-call and memory evidence"));
        UE5AIMcp::AddTool(List, TEXT("settings.cvar"), TEXT("active"), TEXT("Reversible runtime CVar changes"));
        UE5AIMcp::AddTool(List, TEXT("world.partition"), TEXT("active"), TEXT("World Partition status and Level Instance capability"));
        UE5AIMcp::AddTool(List, TEXT("landscape.create"), TEXT("active"), TEXT("Bounded procedural Landscape creation"));
        UE5AIMcp::AddTool(List, TEXT("scatter.instances"), TEXT("active"), TEXT("Seeded instanced foliage/PCG-style scatter"));
        UE5AIMcp::AddTool(List, TEXT("data.create"), TEXT("active"), TEXT("Data asset, table and curve authoring"));
        UE5AIMcp::AddTool(List, TEXT("ui.widget"), TEXT("active"), TEXT("UMG widget and menu scaffolding"));
        UE5AIMcp::AddTool(List, TEXT("input.mapping"), TEXT("active"), TEXT("Enhanced Input asset creation"));
        UE5AIMcp::AddTool(List, TEXT("blueprint.create"), TEXT("active"), TEXT("Safe Blueprint variable and class editing"));
        UE5AIMcp::AddTool(List, TEXT("cpp.scaffold"), TEXT("active"), TEXT("Audited C++ source generation plan"));
        UE5AIMcp::AddTool(List, TEXT("gameplay.scaffold"), TEXT("active"), TEXT("Gameplay Framework base class scaffold"));
        UE5AIMcp::AddTool(List, TEXT("gas.scaffold"), TEXT("conditional"), TEXT("Gameplay Ability System assets when the engine plugin is enabled"));
        UE5AIMcp::AddTool(List, TEXT("material.graph"), TEXT("active"), TEXT("PBR material graph generation"));
        UE5AIMcp::AddTool(List, TEXT("model.procedural"), TEXT("active"), TEXT("Collision-enabled procedural mesh generation"));
        UE5AIMcp::AddTool(List, TEXT("dcc.job"), TEXT("active"), TEXT("External DCC/import hand-off manifest"));
        UE5AIMcp::AddTool(List, TEXT("animation.montage"), TEXT("active"), TEXT("Skeleton-bound animation montage scaffold"));
        UE5AIMcp::AddTool(List, TEXT("sequence.edit"), TEXT("active"), TEXT("Sequencer float track and key authoring"));
        UE5AIMcp::AddTool(List, TEXT("niagara.scaffold"), TEXT("conditional"), TEXT("Niagara system assets when the plugin is enabled"));
        UE5AIMcp::AddTool(List, TEXT("audio.cue"), TEXT("active"), TEXT("Sound Cue creation and source routing"));
        UE5AIMcp::AddTool(List, TEXT("network.replication"), TEXT("active"), TEXT("Replication topology inspection and multi-process session plans"));
        UE5AIMcp::AddTool(List, TEXT("platform.profile"), TEXT("active"), TEXT("Platform, plugin and SDK configuration inventory"));
        UE5AIMcp::AddTool(List, TEXT("pipeline.plan"), TEXT("active"), TEXT("Auditable UBT/UAT cook-package-deploy command plan"));
        UE5AIMcp::AddTool(List, TEXT("quality.budget"), TEXT("active"), TEXT("Asset, draw-call and memory budget evidence"));
        UE5AIMcp::AddTool(List, TEXT("compliance.audit"), TEXT("active"), TEXT("Localization and accessibility readiness audit"));
        UE5AIMcp::AddTool(List, TEXT("source-control.status"), TEXT("active"), TEXT("Repository/provider availability and safe collaboration status"));
        UE5AIMcp::AddTool(List, TEXT("cicd.plan"), TEXT("active"), TEXT("CI/CD validation and artifact archival plan"));
        UE5AIMcp::AddTool(List, TEXT("crash.diagnostics"), TEXT("active"), TEXT("Crash archive inventory and reproducibility workflow"));
        UE5AIMcp::AddTool(List, TEXT("showcase.create"), TEXT("active"), TEXT("Persistent Chinese-named ocean, RT grass, and cherry-tree demonstration levels"));
        UE5AIMcp::AddTool(List, TEXT("plugin.scaffold"), TEXT("active"), TEXT("Preview or explicitly write an editor plugin scaffold"));
        UE5AIMcp::AddTool(List, TEXT("editor.adapters"), TEXT("active"), TEXT("Installed editor-domain adapter inventory"));
        UE5AIMcp::AddTool(List, TEXT("agent.plan"), TEXT("active"), TEXT("Recoverable autonomous-task state machine with approval gates"));
        UE5AIMcp::AddTool(List, TEXT("scene.orchestrate"), TEXT("active"), TEXT("From-zero scene generation orchestration plan"));
        UE5AIMcp::AddTool(List, TEXT("capabilities.coverage"), TEXT("active"), TEXT("Engine/plugin compatibility and regression coverage matrix"));
        TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("ok"), true);
        Result->SetArrayField(TEXT("tools"), List);
        Complete(UE5AIMcp::Respond(Result));
        return true;
    }

    bool SnapshotHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        // The listener runs off the editor thread. Defer all UObject access to GameThread.
        AsyncTask(ENamedThreads::GameThread, [this, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetBoolField(TEXT("ok"), true);
            Result->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
            Result->SetStringField(TEXT("project_dir"), FPaths::ProjectDir());
            Result->SetBoolField(TEXT("is_pie"), GEditor && GEditor->PlayWorld != nullptr);
            UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
            if (World)
            {
                Result->SetStringField(TEXT("world"), World->GetPathName());
                TArray<TSharedPtr<FJsonValue>> Actors;
                int32 Count = 0;
                for (TActorIterator<AActor> It(World); It; ++It)
                {
                    ++Count;
                    if (Actors.Num() < 100)
                    {
                        const AActor* Actor = *It;
                        TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
                        Entry->SetStringField(TEXT("name"), Actor->GetName());
                        Entry->SetStringField(TEXT("path"), Actor->GetPathName());
                        Entry->SetStringField(TEXT("class"), Actor->GetClass()->GetPathName());
                        Entry->SetStringField(TEXT("label"), Actor->GetActorLabel());
                        Entry->SetStringField(TEXT("transform"), Actor->GetActorTransform().ToHumanReadableString());
                        Actors.Add(MakeShared<FJsonValueObject>(Entry));
                    }
                }
                Result->SetNumberField(TEXT("actor_count"), Count);
                Result->SetArrayField(TEXT("actors_preview"), Actors);
            }
            FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
            TArray<FAssetData> Assets;
            AssetRegistry.Get().GetAllAssets(Assets, true);
            Result->SetNumberField(TEXT("asset_count"), Assets.Num());
            Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool PropertiesHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString* ObjectPath = Request.QueryParams.Find(TEXT("object"));
        if (!ObjectPath || ObjectPath->IsEmpty())
        {
            TSharedRef<FJsonObject> Error = MakeShared<FJsonObject>();
            Error->SetBoolField(TEXT("ok"), false);
            Error->SetStringField(TEXT("error"), TEXT("Query parameter 'object' is required."));
            Complete(UE5AIMcp::Respond(Error)); return true;
        }
        const FString Path = *ObjectPath;
        AsyncTask(ENamedThreads::GameThread, [Path, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            UObject* Object = StaticFindObject(UObject::StaticClass(), nullptr, *Path);
            if (!Object) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Object not found.")); Complete(UE5AIMcp::Respond(Result)); return; }
            Result->SetBoolField(TEXT("ok"), true);
            Result->SetStringField(TEXT("object"), Object->GetPathName());
            Result->SetStringField(TEXT("class"), Object->GetClass()->GetPathName());
            TArray<TSharedPtr<FJsonValue>> Fields;
            for (TFieldIterator<FProperty> It(Object->GetClass()); It && Fields.Num() < 200; ++It)
            {
                FProperty* Field = *It;
                if (!Field->HasAnyPropertyFlags(CPF_Edit)) continue;
                TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
                Entry->SetStringField(TEXT("name"), Field->GetName());
                Entry->SetStringField(TEXT("type"), Field->GetClass()->GetName());
                Entry->SetBoolField(TEXT("editable"), Field->HasAnyPropertyFlags(CPF_Edit));
                Fields.Add(MakeShared<FJsonValueObject>(Entry));
            }
            Result->SetArrayField(TEXT("properties"), Fields);
            Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool CVarsHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Filter = Request.QueryParams.FindRef(TEXT("filter"));
        TArray<TSharedPtr<FJsonValue>> Values;
        IConsoleManager::Get().ForEachConsoleObjectThatContains(FConsoleObjectVisitor::CreateLambda([&Values](const TCHAR* Name, IConsoleObject* Object)
        {
            if (Values.Num() >= 200) return;
            TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
            Entry->SetStringField(TEXT("name"), Name);
            Entry->SetStringField(TEXT("help"), Object->GetHelp());
            if (IConsoleVariable* Variable = Object->AsVariable()) Entry->SetStringField(TEXT("value"), Variable->GetString());
            Values.Add(MakeShared<FJsonValueObject>(Entry));
        }), *Filter);
        TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("filter"), Filter); Result->SetArrayField(TEXT("cvars"), Values);
        Complete(UE5AIMcp::Respond(Result)); return true;
    }

    bool LogsHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        TArray<TSharedPtr<FJsonValue>> Values;
        for (const FString& Line : Capture->Snapshot()) Values.Add(MakeShared<FJsonValueString>(Line));
        TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("ok"), true); Result->SetArrayField(TEXT("lines"), Values);
        Complete(UE5AIMcp::Respond(Result)); return true;
    }

    bool ScreenshotHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [this, Complete]()
        {
            const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UE5AI"));
            IFileManager::Get().MakeDirectory(*Directory, true);
            const FString Filename = FPaths::ConvertRelativePathToFull(FPaths::Combine(Directory, FString::Printf(TEXT("viewport_%lld.png"), FDateTime::UtcNow().GetTicks())));
            FScreenshotRequest::RequestScreenshot(Filename, false, false);
            // Hidden/unattended editor instances may not redraw again until a
            // tool asks for it; force a render frame so every request writes.
            if (GEditor) GEditor->RedrawAllViewports();
            FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, Filename, Complete, Attempts = 0](float) mutable
            {
                const bool bWritten = IFileManager::Get().FileExists(*Filename);
                if (!bWritten && ++Attempts < 60) return true;
                const bool bFallback = !bWritten && !LastScreenshotPath.IsEmpty() && IFileManager::Get().FileExists(*LastScreenshotPath);
                const FString DeliveredPath = bWritten ? Filename : (bFallback ? LastScreenshotPath : Filename);
                if (bWritten) LastScreenshotPath = Filename;
                TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
                Result->SetBoolField(TEXT("ok"), bWritten || bFallback);
                Result->SetStringField(TEXT("status"), bWritten ? TEXT("captured") : (bFallback ? TEXT("fallback-last-frame") : TEXT("capture-timeout")));
                Result->SetBoolField(TEXT("fresh"), bWritten);
                Result->SetStringField(TEXT("path"), DeliveredPath);
                Result->SetStringField(TEXT("note"), bWritten ? TEXT("Viewport screenshot written.") : TEXT("No new render frame arrived; returned the most recent valid viewport screenshot."));
                Complete(UE5AIMcp::Respond(Result));
                return false;
            }), 0.1f);
        });
        return true;
    }

    bool SpawnActorHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Label = Request.QueryParams.FindRef(TEXT("label")).IsEmpty() ? TEXT("UE5AI_TestActor") : Request.QueryParams.FindRef(TEXT("label"));
        AsyncTask(ENamedThreads::GameThread, [Label, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
            if (!World) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("No editor world.")); Complete(UE5AIMcp::Respond(Result)); return; }
            const FScopedTransaction Transaction(NSLOCTEXT("UE5AIMcp", "SpawnActor", "UE5 AI: Spawn Actor"));
            AActor* Actor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform::Identity);
            if (!Actor) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Spawn failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            Actor->Modify(); Actor->SetActorLabel(Label); World->MarkPackageDirty();
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("path"), Actor->GetPathName()); Result->SetStringField(TEXT("label"), Actor->GetActorLabel());
            Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool DeleteActorHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Path = Request.QueryParams.FindRef(TEXT("object"));
        if (Path.IsEmpty()) { TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Query parameter 'object' is required.")); Complete(UE5AIMcp::Respond(Result)); return true; }
        AsyncTask(ENamedThreads::GameThread, [Path, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            AActor* Actor = FindObject<AActor>(nullptr, *Path);
            if (!Actor || !GEditor) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Actor not found.")); Complete(UE5AIMcp::Respond(Result)); return; }
            const FScopedTransaction Transaction(NSLOCTEXT("UE5AIMcp", "DeleteActor", "UE5 AI: Delete Actor"));
            const bool bDeleted = GEditor->GetEditorWorldContext().World()->EditorDestroyActor(Actor, true);
            Result->SetBoolField(TEXT("ok"), bDeleted); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool TransformActorHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Path = Request.QueryParams.FindRef(TEXT("object"));
        if (Path.IsEmpty()) { TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Query parameter 'object' is required.")); Complete(UE5AIMcp::Respond(Result)); return true; }
        const FVector Location(FCString::Atof(*Request.QueryParams.FindRef(TEXT("x"))), FCString::Atof(*Request.QueryParams.FindRef(TEXT("y"))), FCString::Atof(*Request.QueryParams.FindRef(TEXT("z"))));
        const FRotator Rotation(FCString::Atof(*Request.QueryParams.FindRef(TEXT("pitch"))), FCString::Atof(*Request.QueryParams.FindRef(TEXT("yaw"))), FCString::Atof(*Request.QueryParams.FindRef(TEXT("roll"))));
        const FVector Scale(FCString::Atof(*Request.QueryParams.FindRef(TEXT("sx"))), FCString::Atof(*Request.QueryParams.FindRef(TEXT("sy"))), FCString::Atof(*Request.QueryParams.FindRef(TEXT("sz"))));
        const FString Tag = Request.QueryParams.FindRef(TEXT("tag"));
        AsyncTask(ENamedThreads::GameThread, [Path, Location, Rotation, Scale, Tag, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); AActor* Actor = FindObject<AActor>(nullptr, *Path);
            if (!Actor) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Actor not found.")); Complete(UE5AIMcp::Respond(Result)); return; }
            const FScopedTransaction Transaction(NSLOCTEXT("UE5AIMcp", "TransformActor", "UE5 AI: Transform Actor"));
            Actor->Modify(); Actor->SetActorLocationAndRotation(Location, Rotation); Actor->SetActorScale3D(Scale);
            if (!Tag.IsEmpty()) Actor->Tags.AddUnique(FName(*Tag));
            Actor->MarkPackageDirty(); Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("transform"), Actor->GetActorTransform().ToHumanReadableString()); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool SpawnLightHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const float Intensity = FMath::Max(0.0f, FCString::Atof(*Request.QueryParams.FindRef(TEXT("intensity"))));
        const FVector Location(FCString::Atof(*Request.QueryParams.FindRef(TEXT("x"))), FCString::Atof(*Request.QueryParams.FindRef(TEXT("y"))), FCString::Atof(*Request.QueryParams.FindRef(TEXT("z"))));
        AsyncTask(ENamedThreads::GameThread, [Intensity, Location, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
            if (!World) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("No editor world.")); Complete(UE5AIMcp::Respond(Result)); return; }
            const FScopedTransaction Transaction(NSLOCTEXT("UE5AIMcp", "SpawnPointLight", "UE5 AI: Spawn Point Light"));
            APointLight* Light = World->SpawnActor<APointLight>(APointLight::StaticClass(), FTransform(Location));
            if (!Light) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Light spawn failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            UPointLightComponent* Component = CastChecked<UPointLightComponent>(Light->GetLightComponent());
            Light->Modify(); Light->SetActorLabel(TEXT("UE5AI_PointLight")); Component->SetIntensity(Intensity > 0 ? Intensity : 1000.0f); Light->MarkPackageDirty();
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("path"), Light->GetPathName()); Result->SetNumberField(TEXT("intensity"), Component->Intensity); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool SpawnGridHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const int32 Rows = FMath::Clamp(FCString::Atoi(*Request.QueryParams.FindRef(TEXT("rows"))), 1, 20);
        const int32 Cols = FMath::Clamp(FCString::Atoi(*Request.QueryParams.FindRef(TEXT("cols"))), 1, 20);
        const float Spacing = FMath::Max(1.0f, FCString::Atof(*Request.QueryParams.FindRef(TEXT("spacing"))));
        const FString Folder = Request.QueryParams.FindRef(TEXT("folder")).IsEmpty() ? TEXT("UE5AI/Grid") : Request.QueryParams.FindRef(TEXT("folder"));
        AsyncTask(ENamedThreads::GameThread, [Rows, Cols, Spacing, Folder, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
            UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
            if (!World || !Mesh) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Editor world or Cube mesh unavailable.")); Complete(UE5AIMcp::Respond(Result)); return; }
            const FScopedTransaction Transaction(NSLOCTEXT("UE5AIMcp", "SpawnGrid", "UE5 AI: Spawn Grid")); TArray<TSharedPtr<FJsonValue>> Paths;
            for (int32 Row=0; Row<Rows; ++Row) for (int32 Col=0; Col<Cols; ++Col)
            {
                AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(FVector(Row*Spacing, Col*Spacing, 0)));
                if (!Actor) continue; Actor->Modify(); Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh); Actor->SetActorLabel(FString::Printf(TEXT("UE5AI_Grid_%d_%d"), Row, Col)); Actor->SetFolderPath(FName(*Folder)); Actor->Tags.AddUnique(TEXT("UE5AI_Grid")); Actor->MarkPackageDirty(); Paths.Add(MakeShared<FJsonValueString>(Actor->GetPathName()));
            }
            Result->SetBoolField(TEXT("ok"), Paths.Num() == Rows*Cols); Result->SetNumberField(TEXT("count"), Paths.Num()); Result->SetArrayField(TEXT("paths"), Paths); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    // Organizes actors with stable editor metadata.  The Layer field is the
    // editor's classic layer system; the data_layer tag gives World Partition
    // aware callers a deterministic hand-off without silently creating a
    // runtime Data Layer asset in a user map.
    bool OrganizeActorHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Path = Request.QueryParams.FindRef(TEXT("object"));
        const FString Folder = Request.QueryParams.FindRef(TEXT("folder"));
        const FString Tag = Request.QueryParams.FindRef(TEXT("tag"));
        const FString Layer = Request.QueryParams.FindRef(TEXT("layer"));
        const FString DataLayer = Request.QueryParams.FindRef(TEXT("data_layer"));
        if (Path.IsEmpty()) { return RespondError(TEXT("Query parameter 'object' is required."), Complete); }
        AsyncTask(ENamedThreads::GameThread, [Path, Folder, Tag, Layer, DataLayer, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            AActor* Actor = FindObject<AActor>(nullptr, *Path);
            if (!Actor) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Actor not found.")); Complete(UE5AIMcp::Respond(Result)); return; }
            const FScopedTransaction Transaction(NSLOCTEXT("UE5AIMcp", "OrganizeActor", "UE5 AI: Organize Actor"));
            Actor->Modify();
            if (!Folder.IsEmpty()) Actor->SetFolderPath(FName(*Folder));
            if (!Tag.IsEmpty()) Actor->Tags.AddUnique(FName(*Tag));
            if (!Layer.IsEmpty()) Actor->Layers.AddUnique(FName(*Layer));
            if (!DataLayer.IsEmpty()) Actor->Tags.AddUnique(FName(*FString::Printf(TEXT("UE5AI_DataLayer:%s"), *DataLayer)));
            Actor->MarkPackageDirty();
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("folder"), Actor->GetFolderPath().ToString());
            Result->SetStringField(TEXT("data_layer_binding"), DataLayer.IsEmpty() ? TEXT("") : DataLayer);
            Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool CameraPostHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FVector Location(FCString::Atof(*Request.QueryParams.FindRef(TEXT("x"))), FCString::Atof(*Request.QueryParams.FindRef(TEXT("y"))), FCString::Atof(*Request.QueryParams.FindRef(TEXT("z"))));
        const float Fov = FMath::Clamp(FCString::Atof(*Request.QueryParams.FindRef(TEXT("fov"))), 5.0f, 170.0f);
        const float Exposure = FMath::Clamp(FCString::Atof(*Request.QueryParams.FindRef(TEXT("exposure"))), -10.0f, 10.0f);
        AsyncTask(ENamedThreads::GameThread, [Location, Fov, Exposure, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
            if (!World) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("No editor world.")); Complete(UE5AIMcp::Respond(Result)); return; }
            const FScopedTransaction Transaction(NSLOCTEXT("UE5AIMcp", "CreateCameraPost", "UE5 AI: Create Camera and Post Process"));
            ACameraActor* Camera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform(Location));
            APostProcessVolume* Volume = World->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FTransform(Location));
            if (!Camera || !Volume) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Camera or post process volume spawn failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            Camera->Modify(); Volume->Modify(); Camera->SetActorLabel(TEXT("UE5AI_Camera")); Volume->SetActorLabel(TEXT("UE5AI_PostProcess"));
            Camera->GetCameraComponent()->SetFieldOfView(Fov > 5.0f ? Fov : 90.0f);
            Volume->bUnbound = true; Volume->Settings.bOverride_AutoExposureBias = true; Volume->Settings.AutoExposureBias = Exposure;
            Camera->MarkPackageDirty(); Volume->MarkPackageDirty();
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("camera"), Camera->GetPathName()); Result->SetStringField(TEXT("postprocess"), Volume->GetPathName()); Result->SetNumberField(TEXT("fov"), Camera->GetCameraComponent()->FieldOfView); Result->SetNumberField(TEXT("exposure"), Volume->Settings.AutoExposureBias);
            Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool MaterialInstanceHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString ParentPath = Request.QueryParams.FindRef(TEXT("parent"));
        const FString Folder = Request.QueryParams.FindRef(TEXT("folder")).IsEmpty() ? TEXT("/Game/UE5AI_AutoTmp") : Request.QueryParams.FindRef(TEXT("folder"));
        const FString Name = Request.QueryParams.FindRef(TEXT("name")).IsEmpty() ? TEXT("MI_UE5AI_Test") : Request.QueryParams.FindRef(TEXT("name"));
        const FString Scalar = Request.QueryParams.FindRef(TEXT("scalar"));
        const float Value = FCString::Atof(*Request.QueryParams.FindRef(TEXT("value")));
        AsyncTask(ENamedThreads::GameThread, [ParentPath, Folder, Name, Scalar, Value, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, *ParentPath);
            if (!Parent) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Parent material is required and must resolve.")); Complete(UE5AIMcp::Respond(Result)); return; }
            UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>(); Factory->InitialParent = Parent;
            UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(FAssetToolsModule::GetModule().Get().CreateAsset(Name, Folder, UMaterialInstanceConstant::StaticClass(), Factory));
            if (!Instance) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Material instance creation failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            if (!Scalar.IsEmpty()) Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(*Scalar), Value);
            Instance->PostEditChange(); Instance->MarkPackageDirty();
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("path"), Instance->GetPathName()); Result->SetStringField(TEXT("parent"), Parent->GetPathName()); Result->SetStringField(TEXT("scalar"), Scalar); Result->SetNumberField(TEXT("value"), Value);
            Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool ImportAssetHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Filename = Request.QueryParams.FindRef(TEXT("file"));
        const FString Destination = Request.QueryParams.FindRef(TEXT("destination")).IsEmpty() ? TEXT("/Game/UE5AI_AutoTmp") : Request.QueryParams.FindRef(TEXT("destination"));
        if (Filename.IsEmpty() || !FPaths::FileExists(Filename)) { return RespondError(TEXT("An existing absolute 'file' path is required."), Complete); }
        // Interchange import spins the task graph.  It must not be invoked
        // from the HTTP request's GameThread continuation (that creates a
        // nested TaskGraph queue in UE 5.7).  Defer one editor tick instead.
        AsyncTask(ENamedThreads::GameThread, [Filename, Destination, Complete]()
        {
            FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Filename, Destination, Complete](float)
            {
                TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
                UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
                ImportData->Filenames.Add(Filename); ImportData->DestinationPath = Destination; ImportData->bReplaceExisting = true; ImportData->bSkipReadOnly = true;
                const TArray<UObject*> ImportedObjects = FAssetToolsModule::GetModule().Get().ImportAssetsAutomated(ImportData);
                TArray<TSharedPtr<FJsonValue>> Imported;
                TArray<UPackage*> ImportedPackages;
                for (UObject* Object : ImportedObjects)
                {
                    if (!Object) continue;
                    Object->Modify();
                    Object->MarkPackageDirty();
                    ImportedPackages.AddUnique(Object->GetOutermost());
                    Imported.Add(MakeShared<FJsonValueString>(Object->GetPathName()));
                }
                const bool bSaved = ImportedPackages.Num() > 0 && UEditorLoadingAndSavingUtils::SavePackages(ImportedPackages, false);
                Result->SetBoolField(TEXT("ok"), Imported.Num() > 0 && bSaved); Result->SetBoolField(TEXT("saved"), bSaved); Result->SetStringField(TEXT("file"), Filename); Result->SetStringField(TEXT("destination"), Destination); Result->SetArrayField(TEXT("imported"), Imported); Complete(UE5AIMcp::Respond(Result));
                return false;
            }), 0.0f);
        });
        return true;
    }

    bool ReimportAssetHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Path = Request.QueryParams.FindRef(TEXT("asset"));
        if (Path.IsEmpty()) { return RespondError(TEXT("Query parameter 'asset' is required."), Complete); }
        // Reimport uses the same Interchange task graph path as import.  Queue
        // it on the following editor tick rather than nesting it in the HTTP
        // continuation's task graph.
        AsyncTask(ENamedThreads::GameThread, [Path, Complete]()
        {
            FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Path, Complete](float)
            {
                TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); UObject* Asset = LoadObject<UObject>(nullptr, *Path);
                if (!Asset) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Asset not found.")); Complete(UE5AIMcp::Respond(Result)); return false; }
                const bool bReimported = FReimportManager::Instance()->Reimport(Asset, false, true);
                Result->SetBoolField(TEXT("ok"), bReimported); Result->SetStringField(TEXT("asset"), Asset->GetPathName()); Complete(UE5AIMcp::Respond(Result)); return false;
            }), 0.0f);
        });
        return true;
    }

    bool ValidateAssetsHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [Complete]()
        {
            IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
            TArray<FAssetData> Assets; Registry.GetAllAssets(Assets, true);
            int32 Invalid = 0; int32 MissingDependencies = 0; TArray<TSharedPtr<FJsonValue>> Samples;
            for (const FAssetData& Asset : Assets)
            {
                if (!Asset.IsValid()) { ++Invalid; if (Samples.Num() < 50) Samples.Add(MakeShared<FJsonValueString>(Asset.GetObjectPathString())); continue; }
                TArray<FName> Dependencies; Registry.GetDependencies(Asset.PackageName, Dependencies, UE::AssetRegistry::EDependencyCategory::Package);
                for (const FName& Dependency : Dependencies)
                {
                    const FString DependencyPath = Dependency.ToString();
                    // Script and engine-plugin package references are virtual
                    // in the Asset Registry, so file existence is only a
                    // reliable integrity signal for project content packages.
                    if (Dependency.IsNone() || !DependencyPath.StartsWith(TEXT("/Game/")) || FindPackage(nullptr, *DependencyPath) || FPackageName::DoesPackageExist(DependencyPath)) continue;
                    ++MissingDependencies; if (Samples.Num() < 50) Samples.Add(MakeShared<FJsonValueString>(FString::Printf(TEXT("%s -> %s"), *Asset.PackageName.ToString(), *Dependency.ToString())));
                }
            }
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), true); Result->SetBoolField(TEXT("integrity_ok"), Invalid == 0 && MissingDependencies == 0); Result->SetNumberField(TEXT("asset_count"), Assets.Num()); Result->SetNumberField(TEXT("invalid_count"), Invalid); Result->SetNumberField(TEXT("missing_dependency_count"), MissingDependencies); Result->SetArrayField(TEXT("issues"), Samples); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool SequenceSummaryHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Path = Request.QueryParams.FindRef(TEXT("asset"));
        if (Path.IsEmpty()) { return RespondError(TEXT("Query parameter 'asset' is required."), Complete); }
        AsyncTask(ENamedThreads::GameThread, [Path, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); UObject* Asset = LoadObject<UObject>(nullptr, *Path);
            if (!Asset) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Asset not found.")); Complete(UE5AIMcp::Respond(Result)); return; }
            TArray<TSharedPtr<FJsonValue>> Properties;
            for (TFieldIterator<FProperty> It(Asset->GetClass()); It && Properties.Num() < 100; ++It)
            {
                FProperty* Property = *It; if (!Property->HasAnyPropertyFlags(CPF_Edit)) continue;
                TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>(); Entry->SetStringField(TEXT("name"), Property->GetName()); Entry->SetStringField(TEXT("type"), Property->GetClass()->GetName()); Properties.Add(MakeShared<FJsonValueObject>(Entry));
            }
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("path"), Asset->GetPathName()); Result->SetStringField(TEXT("class"), Asset->GetClass()->GetPathName()); Result->SetStringField(TEXT("kind"), TEXT("Generic asset summary: supports Niagara systems, animation assets and Level Sequences without loading their editor modules.")); Result->SetArrayField(TEXT("editable_properties"), Properties); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool CreateSequenceHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Folder = Request.QueryParams.FindRef(TEXT("folder")).IsEmpty() ? TEXT("/Game/UE5AI_AutoTmp") : Request.QueryParams.FindRef(TEXT("folder"));
        const FString Name = Request.QueryParams.FindRef(TEXT("name")).IsEmpty() ? TEXT("LS_UE5AI_Test") : Request.QueryParams.FindRef(TEXT("name"));
        AsyncTask(ENamedThreads::GameThread, [Folder, Name, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            const FString PackageName = FString::Printf(TEXT("%s/%s"), *Folder, *Name);
            UPackage* Package = CreatePackage(*PackageName);
            ULevelSequence* Sequence = Package ? NewObject<ULevelSequence>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional) : nullptr;
            if (!Sequence) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Level Sequence creation failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            Sequence->Initialize();
            FAssetRegistryModule::AssetCreated(Sequence); Sequence->MarkPackageDirty(); Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("path"), Sequence->GetPathName()); Result->SetStringField(TEXT("class"), Sequence->GetClass()->GetPathName()); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    static FString FileSha1(const FString& Filename)
    {
        TArray<uint8> Bytes; if (!FFileHelper::LoadFileToArray(Bytes, *Filename)) return TEXT("");
        uint8 Hash[FSHA1::DigestSize]; FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Hash); return BytesToHex(Hash, FSHA1::DigestSize);
    }

    bool VerifyViewportHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Baseline = Request.QueryParams.FindRef(TEXT("baseline")); const FString Candidate = Request.QueryParams.FindRef(TEXT("candidate"));
        TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        if (!FPaths::FileExists(Baseline) || !FPaths::FileExists(Candidate)) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Both baseline and candidate screenshot paths must exist.")); Complete(UE5AIMcp::Respond(Result)); return true; }
        const FString BaselineHash = FileSha1(Baseline); const FString CandidateHash = FileSha1(Candidate);
        Result->SetBoolField(TEXT("ok"), !BaselineHash.IsEmpty() && !CandidateHash.IsEmpty()); Result->SetStringField(TEXT("baseline"), Baseline); Result->SetStringField(TEXT("candidate"), Candidate); Result->SetStringField(TEXT("baseline_sha1"), BaselineHash); Result->SetStringField(TEXT("candidate_sha1"), CandidateHash); Result->SetBoolField(TEXT("pixels_changed"), BaselineHash != CandidateHash); Result->SetStringField(TEXT("next_step"), TEXT("Provide both paths to the AI client for semantic visual review; this endpoint provides deterministic image-change evidence.")); Complete(UE5AIMcp::Respond(Result)); return true;
    }

    bool UndoHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete) { return HandleTransaction(false, Complete); }
    bool RedoHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete) { return HandleTransaction(true, Complete); }
    bool HandleTransaction(const bool bRedo, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [bRedo, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            const bool bSuccess = GEditor && GEditor->Trans && (bRedo ? GEditor->Trans->Redo() : GEditor->Trans->Undo());
            Result->SetBoolField(TEXT("ok"), bSuccess); Result->SetStringField(TEXT("operation"), bRedo ? TEXT("redo") : TEXT("undo")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }
    bool BlueprintSummaryHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Path = Request.QueryParams.FindRef(TEXT("asset"));
        AsyncTask(ENamedThreads::GameThread, [Path, Complete]() { TSharedRef<FJsonObject> Result=MakeShared<FJsonObject>(); UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*Path); if(!BP){Result->SetBoolField(TEXT("ok"),false);Result->SetStringField(TEXT("error"),TEXT("Blueprint not found."));Complete(UE5AIMcp::Respond(Result));return;} TArray<TSharedPtr<FJsonValue>> Graphs; for(UEdGraph* Graph:BP->UbergraphPages){if(Graph) Graphs.Add(MakeShared<FJsonValueString>(Graph->GetName()));} for(UEdGraph* Graph:BP->FunctionGraphs){if(Graph) Graphs.Add(MakeShared<FJsonValueString>(Graph->GetName()));} Result->SetBoolField(TEXT("ok"),true);Result->SetStringField(TEXT("path"),BP->GetPathName());Result->SetArrayField(TEXT("graphs"),Graphs);Complete(UE5AIMcp::Respond(Result)); }); return true;
    }
    bool MaterialSummaryHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Path=Request.QueryParams.FindRef(TEXT("asset")); AsyncTask(ENamedThreads::GameThread,[Path,Complete](){TSharedRef<FJsonObject> Result=MakeShared<FJsonObject>(); UMaterial* Material=LoadObject<UMaterial>(nullptr,*Path); if(!Material){Result->SetBoolField(TEXT("ok"),false);Result->SetStringField(TEXT("error"),TEXT("Material not found."));Complete(UE5AIMcp::Respond(Result));return;} TArray<TSharedPtr<FJsonValue>> Nodes; for(UMaterialExpression* Node:Material->GetExpressions()){if(Node){TSharedRef<FJsonObject> Entry=MakeShared<FJsonObject>();Entry->SetStringField(TEXT("name"),Node->GetName());Entry->SetStringField(TEXT("class"),Node->GetClass()->GetPathName());Nodes.Add(MakeShared<FJsonValueObject>(Entry));}} Result->SetBoolField(TEXT("ok"),true);Result->SetStringField(TEXT("path"),Material->GetPathName());Result->SetArrayField(TEXT("nodes"),Nodes);Complete(UE5AIMcp::Respond(Result));});return true;
    }
    bool ClassSummaryHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString ClassPath=Request.QueryParams.FindRef(TEXT("class")); AsyncTask(ENamedThreads::GameThread,[ClassPath,Complete](){TSharedRef<FJsonObject> Result=MakeShared<FJsonObject>(); FTopLevelAssetPath ClassName(ClassPath); if(ClassPath.IsEmpty()){Result->SetBoolField(TEXT("ok"),false);Result->SetStringField(TEXT("error"),TEXT("Query parameter 'class' is required."));Complete(UE5AIMcp::Respond(Result));return;} FARFilter Filter;Filter.ClassPaths.Add(ClassName);Filter.bRecursiveClasses=true;TArray<FAssetData> Assets;FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssets(Filter,Assets);TArray<TSharedPtr<FJsonValue>> Entries;for(const FAssetData& Asset:Assets){if(Entries.Num()>=250)break;TSharedRef<FJsonObject> E=MakeShared<FJsonObject>();E->SetStringField(TEXT("path"),Asset.GetObjectPathString());E->SetStringField(TEXT("name"),Asset.AssetName.ToString());E->SetStringField(TEXT("class"),Asset.AssetClassPath.ToString());Entries.Add(MakeShared<FJsonValueObject>(E));}Result->SetBoolField(TEXT("ok"),true);Result->SetStringField(TEXT("class"),ClassPath);Result->SetNumberField(TEXT("count"),Assets.Num());Result->SetArrayField(TEXT("assets"),Entries);Complete(UE5AIMcp::Respond(Result));});return true;
    }
    bool StartPieHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread,[Complete](){TSharedRef<FJsonObject> Result=MakeShared<FJsonObject>();if(!GEditor){Result->SetBoolField(TEXT("ok"),false);Complete(UE5AIMcp::Respond(Result));return;}FRequestPlaySessionParams Params;GEditor->RequestPlaySession(Params);Result->SetBoolField(TEXT("ok"),true);Result->SetStringField(TEXT("status"),TEXT("PIE requested"));Complete(UE5AIMcp::Respond(Result));});return true;
    }
    bool StopPieHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread,[Complete](){TSharedRef<FJsonObject> Result=MakeShared<FJsonObject>();if(!GEditor){Result->SetBoolField(TEXT("ok"),false);Complete(UE5AIMcp::Respond(Result));return;}GEditor->RequestEndPlayMap();Result->SetBoolField(TEXT("ok"),true);Result->SetStringField(TEXT("status"),TEXT("PIE stop requested"));Complete(UE5AIMcp::Respond(Result));});return true;
    }
    bool SaveDirtyHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread,[Complete](){TSharedRef<FJsonObject> Result=MakeShared<FJsonObject>();bool bNeeded=false;const bool bSaved=FEditorFileUtils::SaveDirtyPackages(false,true,true,true,false,false,&bNeeded);Result->SetBoolField(TEXT("ok"),bSaved);Result->SetBoolField(TEXT("packages_were_dirty"),bNeeded);Complete(UE5AIMcp::Respond(Result));});return true;
    }

    bool AutomationDiscoverHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Filter = Request.QueryParams.FindRef(TEXT("filter"));
        AsyncTask(ENamedThreads::GameThread, [this, Filter, Complete]()
        {
            TArray<FAutomationTestInfo> Tests; FAutomationTestFramework::Get().GetValidTestNames(Tests);
            TArray<TSharedPtr<FJsonValue>> Matches; for (const FAutomationTestInfo& Test : Tests) { const FString Name = Test.GetFullTestPath(); if (Filter.IsEmpty() || Name.Contains(Filter, ESearchCase::IgnoreCase)) Matches.Add(MakeShared<FJsonValueString>(Name)); }
            // Precompiled editor plugins are not Automation Worker modules.
            // Surface this plugin's explicitly owned smoke test alongside
            // framework tests so discovery is stable in packaged plugin builds.
            const FString PluginSmokeName = TEXT("UE5AI.MVP.Smoke");
            if ((Filter.IsEmpty() || PluginSmokeName.Contains(Filter, ESearchCase::IgnoreCase)) && !Matches.ContainsByPredicate([](const TSharedPtr<FJsonValue>& Value) { return Value.IsValid() && Value->AsString() == TEXT("UE5AI.MVP.Smoke"); })) Matches.Add(MakeShared<FJsonValueString>(PluginSmokeName));
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("filter"), Filter); Result->SetNumberField(TEXT("count"), Matches.Num()); Result->SetArrayField(TEXT("tests"), Matches); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool AutomationRunHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Filter = Request.QueryParams.FindRef(TEXT("filter")).IsEmpty() ? TEXT("UE5AI.MVP") : Request.QueryParams.FindRef(TEXT("filter"));
        AsyncTask(ENamedThreads::GameThread, [this, Filter, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
            if (Filter.Contains(TEXT("UE5AI.MVP.Smoke"), ESearchCase::IgnoreCase) && AutomationTest)
            {
                const bool bPassed = RunUE5AIMcpMvpSmokeTest(AutomationTest.Get());
                bPluginSmokeRan = true;
                bPluginSmokePassed = bPassed;
                UE_LOG(LogUE5AIMcp, Display, TEXT("UE5AI.MVP.Smoke automation %s"), bPassed ? TEXT("Success") : TEXT("Failed"));
                Result->SetBoolField(TEXT("ok"), bPassed); Result->SetStringField(TEXT("filter"), Filter); Result->SetStringField(TEXT("status"), bPassed ? TEXT("completed; poll automation/report for captured completion log") : TEXT("failed")); Complete(UE5AIMcp::Respond(Result)); return;
            }
            const bool bRequested = GEditor && GEditor->Exec(World, *FString::Printf(TEXT("Automation RunTests %s"), *Filter));
            Result->SetBoolField(TEXT("ok"), bRequested); Result->SetStringField(TEXT("filter"), Filter); Result->SetStringField(TEXT("status"), bRequested ? TEXT("requested; poll automation/report for captured completion log") : TEXT("Editor rejected Automation RunTests command")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool AutomationReportHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [this, Complete]()
        {
            TArray<TSharedPtr<FJsonValue>> Lines; bool bPass = bPluginSmokeRan && bPluginSmokePassed; bool bFail = bPluginSmokeRan && !bPluginSmokePassed;
            if (bPluginSmokeRan) Lines.Add(MakeShared<FJsonValueString>(FString::Printf(TEXT("UE5AI.MVP.Smoke automation %s"), bPluginSmokePassed ? TEXT("Success") : TEXT("Failed"))));
            for (const FString& Line : Capture->Snapshot())
            {
                if (!Line.Contains(TEXT("UE5AI.MVP"), ESearchCase::IgnoreCase) && !Line.Contains(TEXT("Automation"), ESearchCase::IgnoreCase)) continue;
                Lines.Add(MakeShared<FJsonValueString>(Line));
                // Only this plugin's named smoke test may alter its result.
                // Other engine automation diagnostics remain visible as context.
                if (Line.Contains(TEXT("UE5AI.MVP.Smoke"), ESearchCase::IgnoreCase))
                {
                    bPass |= Line.Contains(TEXT("Success"), ESearchCase::IgnoreCase) || Line.Contains(TEXT("Passed"), ESearchCase::IgnoreCase);
                    bFail |= Line.Contains(TEXT("Failed"), ESearchCase::IgnoreCase);
                }
            }
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), !bFail); Result->SetBoolField(TEXT("passed_signal"), bPass); Result->SetBoolField(TEXT("failed_signal"), bFail); Result->SetArrayField(TEXT("lines"), Lines); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool PerformanceSnapshotHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [Complete]()
        {
            const FPlatformMemoryStats Memory = FPlatformMemory::GetStats();
            const double DeltaSeconds = FApp::GetDeltaTime();
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetBoolField(TEXT("ok"), true);
            Result->SetNumberField(TEXT("frame_ms"), DeltaSeconds * 1000.0);
            Result->SetNumberField(TEXT("fps_estimate"), DeltaSeconds > SMALL_NUMBER ? 1.0 / DeltaSeconds : 0.0);
            Result->SetNumberField(TEXT("draw_calls"), GNumDrawCallsRHI[0]);
            Result->SetNumberField(TEXT("used_physical_bytes"), static_cast<double>(Memory.UsedPhysical));
            Result->SetNumberField(TEXT("peak_used_physical_bytes"), static_cast<double>(Memory.PeakUsedPhysical));
            Result->SetNumberField(TEXT("available_physical_bytes"), static_cast<double>(Memory.AvailablePhysical));
            Result->SetStringField(TEXT("gpu_frame_ms"), TEXT("unavailable without a live GPU profiler capture"));
            Result->SetStringField(TEXT("bottleneck_hint"), GNumDrawCallsRHI[0] > 2000 ? TEXT("draw-call pressure: consider instancing, HLOD or Nanite") : TEXT("no draw-call threshold exceeded in this sample"));
            Result->SetStringField(TEXT("capture_scope"), TEXT("single editor frame; use Unreal Insights/CSV Profiler for multi-frame traces"));
            Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool ControlledCVarHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Name = Request.QueryParams.FindRef(TEXT("name"));
        const FString Value = Request.QueryParams.FindRef(TEXT("value"));
        if (Name.IsEmpty() || Value.IsEmpty()) return RespondError(TEXT("Query parameters 'name' and 'value' are required."), Complete);
        AsyncTask(ENamedThreads::GameThread, [this, Name, Value, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*Name);
            if (!Variable) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("CVar not found.")); Complete(UE5AIMcp::Respond(Result)); return; }
            const FString Previous = Variable->GetString();
            ControlledCVarOriginals.FindOrAdd(Name) = Previous;
            Variable->Set(*Value, ECVF_SetByCode);
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("name"), Name); Result->SetStringField(TEXT("previous"), Previous); Result->SetStringField(TEXT("current"), Variable->GetString()); Result->SetBoolField(TEXT("rollback_available"), true);
            Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool RollbackCVarHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Name = Request.QueryParams.FindRef(TEXT("name"));
        AsyncTask(ENamedThreads::GameThread, [this, Name, Complete]()
        {
            TArray<TSharedPtr<FJsonValue>> Restored;
            TArray<FString> Names;
            if (Name.IsEmpty()) ControlledCVarOriginals.GetKeys(Names); else Names.Add(Name);
            for (const FString& Entry : Names)
            {
                if (const FString* Original = ControlledCVarOriginals.Find(Entry))
                {
                    if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*Entry))
                    {
                        Variable->Set(**Original, ECVF_SetByCode);
                        Restored.Add(MakeShared<FJsonValueString>(Entry));
                    }
                    ControlledCVarOriginals.Remove(Entry);
                }
            }
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), Name.IsEmpty() || Restored.Num() > 0); Result->SetArrayField(TEXT("restored"), Restored); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool WorldPartitionHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [Complete]()
        {
            UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetBoolField(TEXT("ok"), World != nullptr);
            if (!World) { Result->SetStringField(TEXT("error"), TEXT("No editor world.")); Complete(UE5AIMcp::Respond(Result)); return; }
            Result->SetStringField(TEXT("world"), World->GetPathName());
            Result->SetBoolField(TEXT("is_world_partition"), World->IsPartitionedWorld());
            Result->SetBoolField(TEXT("level_instance_creation"), LoadClass<AActor>(nullptr, TEXT("/Script/Engine.LevelInstance")) != nullptr);
            Result->SetStringField(TEXT("hlod"), TEXT("HLOD build remains an explicit editor build step; this endpoint reports topology before scheduling it."));
            Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool LevelInstanceHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString LevelPath = Request.QueryParams.FindRef(TEXT("level"));
        if (LevelPath.IsEmpty()) return RespondError(TEXT("Query parameter 'level' must name an existing world asset."), Complete);
        AsyncTask(ENamedThreads::GameThread, [LevelPath, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
            UWorld* SourceLevel = LoadObject<UWorld>(nullptr, *LevelPath);
            if (!World || !SourceLevel) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Editor world or source level is unavailable.")); Complete(UE5AIMcp::Respond(Result)); return; }
            const FScopedTransaction Transaction(NSLOCTEXT("UE5AIMcp", "CreateLevelInstance", "UE5 AI: Create Level Instance"));
            ALevelInstance* Instance = World->SpawnActor<ALevelInstance>(ALevelInstance::StaticClass(), FTransform::Identity);
            if (!Instance) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Level Instance actor spawn failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            Instance->Modify(); const bool bBound = Instance->SetWorldAsset(TSoftObjectPtr<UWorld>(SourceLevel)); Instance->SetActorLabel(TEXT("UE5AI_LevelInstance")); Instance->Tags.AddUnique(FName(*FString::Printf(TEXT("UE5AI_SourceLevel:%s"), *SourceLevel->GetPathName()))); Instance->MarkPackageDirty();
            Result->SetBoolField(TEXT("ok"), bBound); Result->SetStringField(TEXT("path"), Instance->GetPathName()); Result->SetStringField(TEXT("source_level"), SourceLevel->GetPathName()); Result->SetStringField(TEXT("bound_world_asset"), Instance->GetWorldAsset().ToSoftObjectPath().ToString()); Result->SetStringField(TEXT("note"), TEXT("Native ALevelInstance WorldAsset binding created; packed-level and HLOD builds remain explicit editor build steps.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool LandscapeHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const int32 Quads = FMath::Clamp(FCString::Atoi(*Request.QueryParams.FindRef(TEXT("quads"))), 7, 127);
        const float Amplitude = FMath::Clamp(FCString::Atof(*Request.QueryParams.FindRef(TEXT("amplitude"))), 0.0f, 16000.0f);
        AsyncTask(ENamedThreads::GameThread, [Quads, Amplitude, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
            if (!World) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("No editor world.")); Complete(UE5AIMcp::Respond(Result)); return; }
            const FScopedTransaction Transaction(NSLOCTEXT("UE5AIMcp", "CreateLandscape", "UE5 AI: Create Landscape"));
            ALandscape* NewLandscape = World->SpawnActor<ALandscape>(ALandscape::StaticClass(), FTransform::Identity);
            if (!NewLandscape) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Landscape actor spawn failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            const FGuid LandscapeGuid = FGuid::NewGuid();
            // ALandscape::Import always reads the base edit layer from the
            // default (zero) GUID.  This intentionally differs from the
            // landscape actor's own GUID; it mirrors the UE editor's new
            // landscape workflow exactly.
            const FGuid BaseEditLayerGuid;
            TMap<FGuid, TArray<uint16>> Heights;
            TArray<uint16>& HeightData = Heights.Add(BaseEditLayerGuid);
            HeightData.Reserve((Quads + 1) * (Quads + 1));
            for (int32 Y = 0; Y <= Quads; ++Y) for (int32 X = 0; X <= Quads; ++X)
            {
                const float Wave = FMath::Sin(static_cast<float>(X) * 0.21f) * FMath::Cos(static_cast<float>(Y) * 0.17f);
                HeightData.Add(static_cast<uint16>(FMath::Clamp(32768.0f + Wave * Amplitude, 0.0f, 65535.0f)));
            }
            // Landscape::Import requires matching entries for the same base
            // edit layer in both maps, including for height-only terrain.
            TMap<FGuid, TArray<FLandscapeImportLayerInfo>> Layers;
            Layers.Add(BaseEditLayerGuid, TArray<FLandscapeImportLayerInfo>());
            TArray<FLandscapeLayer> EditLayers;
            NewLandscape->Import(LandscapeGuid, 0, 0, Quads, Quads, 1, Quads, Heights, nullptr, Layers, ELandscapeImportAlphamapType::Additive, EditLayers);
            NewLandscape->Modify(); NewLandscape->SetActorLabel(TEXT("UE5AI_Landscape")); NewLandscape->Tags.AddUnique(TEXT("UE5AI_Landscape")); NewLandscape->CreateLandscapeInfo(); NewLandscape->MarkPackageDirty();
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("path"), NewLandscape->GetPathName()); Result->SetNumberField(TEXT("quads"), Quads); Result->SetNumberField(TEXT("height_samples"), HeightData.Num()); Result->SetStringField(TEXT("mode"), TEXT("procedural sine/cosine height field; use heightmap import for authored terrain")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool ScatterHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const int32 Count = FMath::Clamp(FCString::Atoi(*Request.QueryParams.FindRef(TEXT("count"))), 1, 1000);
        const int32 Seed = FCString::Atoi(*Request.QueryParams.FindRef(TEXT("seed")));
        const float Extent = FMath::Clamp(FCString::Atof(*Request.QueryParams.FindRef(TEXT("extent"))), 10.0f, 100000.0f);
        const FString MeshPath = Request.QueryParams.FindRef(TEXT("mesh")).IsEmpty() ? TEXT("/Engine/BasicShapes/Cylinder.Cylinder") : Request.QueryParams.FindRef(TEXT("mesh"));
        AsyncTask(ENamedThreads::GameThread, [Count, Seed, Extent, MeshPath, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr; UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
            if (!World || !Mesh) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Editor world or static mesh is unavailable.")); Complete(UE5AIMcp::Respond(Result)); return; }
            const FScopedTransaction Transaction(NSLOCTEXT("UE5AIMcp", "ScatterInstances", "UE5 AI: Scatter Instances"));
            AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
            UInstancedStaticMeshComponent* Instances = NewObject<UInstancedStaticMeshComponent>(Owner, TEXT("UE5AI_ScatterInstances"));
            Owner->SetRootComponent(Instances); Instances->SetStaticMesh(Mesh); Instances->RegisterComponent();
            FRandomStream Random(Seed); for (int32 Index = 0; Index < Count; ++Index) { const FVector Location(Random.FRandRange(-Extent, Extent), Random.FRandRange(-Extent, Extent), 0.0f); const float Scale = Random.FRandRange(0.75f, 1.25f); Instances->AddInstance(FTransform(FRotator(0, Random.FRandRange(0.0f, 360.0f), 0), Location, FVector(Scale))); }
            Owner->Modify(); Owner->SetActorLabel(TEXT("UE5AI_SeededScatter")); Owner->Tags.AddUnique(TEXT("UE5AI_PCGScatter")); Owner->MarkPackageDirty();
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("path"), Owner->GetPathName()); Result->SetNumberField(TEXT("instances"), Instances->GetInstanceCount()); Result->SetNumberField(TEXT("seed"), Seed); Result->SetStringField(TEXT("technique"), TEXT("Instanced Static Mesh scatter; deterministic PCG-compatible fallback")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool DataAssetHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Kind = Request.QueryParams.FindRef(TEXT("kind")).IsEmpty() ? TEXT("curve") : Request.QueryParams.FindRef(TEXT("kind"));
        const FString Folder = UE5AIMcp::AssetFolderOrDefault(Request.QueryParams.FindRef(TEXT("folder")));
        const FString Name = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("name")), TEXT("DA_UE5AI"));
        AsyncTask(ENamedThreads::GameThread, [Kind, Folder, Name, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); const FString PackageName = UE5AIMcp::PackageNameFor(Folder, Name); UPackage* Package = CreatePackage(*PackageName); UObject* Asset = nullptr;
            if (Kind.Equals(TEXT("curve"), ESearchCase::IgnoreCase))
            {
                UCurveFloat* Curve = NewObject<UCurveFloat>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional); Curve->FloatCurve.AddKey(0.0f, 0.0f); Curve->FloatCurve.AddKey(1.0f, 1.0f); Asset = Curve;
            }
            else if (Kind.Equals(TEXT("table"), ESearchCase::IgnoreCase))
            {
                UDataTable* Table = NewObject<UDataTable>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional); Table->RowStruct = FTableRowBase::StaticStruct(); Table->AddRow(TEXT("Default"), FTableRowBase()); Asset = Table;
            }
            else if (Kind.Equals(TEXT("asset"), ESearchCase::IgnoreCase))
            {
                Asset = NewObject<UDataAsset>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional);
            }
            else if (Kind.Equals(TEXT("enum"), ESearchCase::IgnoreCase))
            {
                UUserDefinedEnum* Enum = Cast<UUserDefinedEnum>(FEnumEditorUtils::CreateUserDefinedEnum(Package, *Name, RF_Public | RF_Standalone | RF_Transactional));
                if (Enum) { FEnumEditorUtils::AddNewEnumeratorForUserDefinedEnum(Enum); FEnumEditorUtils::AddNewEnumeratorForUserDefinedEnum(Enum); }
                Asset = Enum;
            }
            else if (Kind.Equals(TEXT("struct"), ESearchCase::IgnoreCase))
            {
                UUserDefinedStruct* Struct = FStructureEditorUtils::CreateUserDefinedStruct(Package, *Name, RF_Public | RF_Standalone | RF_Transactional);
                if (Struct)
                {
                    FEdGraphPinType FloatType;
                    FloatType.PinCategory = UEdGraphSchema_K2::PC_Real;
                    FloatType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
                    FStructureEditorUtils::AddVariable(Struct, FloatType);
                }
                Asset = Struct;
            }
            if (!Asset) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Supported data kinds are curve, table, asset, enum and struct.")); Complete(UE5AIMcp::Respond(Result)); return; }
            UE5AIMcp::MarkAssetCreated(Asset); Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("kind"), Kind); Result->SetStringField(TEXT("path"), Asset->GetPathName()); Result->SetStringField(TEXT("schema_note"), Kind.Equals(TEXT("table"), ESearchCase::IgnoreCase) ? TEXT("Generic FTableRowBase row; production schemas should use a project USTRUCT.") : TEXT("Asset created with editable native properties.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool WidgetHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Folder = UE5AIMcp::AssetFolderOrDefault(Request.QueryParams.FindRef(TEXT("folder")));
        const FString Name = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("name")), TEXT("WBP_UE5AI_Menu"));
        const FString Title = Request.QueryParams.FindRef(TEXT("title")).IsEmpty() ? TEXT("UE5 AI Menu") : Request.QueryParams.FindRef(TEXT("title"));
        AsyncTask(ENamedThreads::GameThread, [Folder, Name, Title, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>(); Factory->ParentClass = UUserWidget::StaticClass();
            UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(FAssetToolsModule::GetModule().Get().CreateAsset(Name, Folder, UWidgetBlueprint::StaticClass(), Factory));
            if (!Blueprint) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Widget Blueprint creation failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            UWidgetTree* Tree = Blueprint->WidgetTree;
            if (!Tree) { Tree = NewObject<UWidgetTree>(Blueprint, TEXT("WidgetTree"), RF_Transactional); Blueprint->WidgetTree = Tree; }
            UVerticalBox* Root = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootMenu"));
            UTextBlock* Header = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title")); Header->SetText(FText::FromString(Title)); Root->AddChildToVerticalBox(Header);
            for (const TCHAR* ButtonName : { TEXT("StartButton"), TEXT("SettingsButton"), TEXT("QuitButton") }) { UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), FName(ButtonName)); Button->SetIsEnabled(true); Root->AddChildToVerticalBox(Button); }
            Tree->RootWidget = Root; Blueprint->Modify(); Blueprint->MarkPackageDirty(); FKismetEditorUtilities::CompileBlueprint(Blueprint);
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("path"), Blueprint->GetPathName()); Result->SetNumberField(TEXT("widget_count"), 5); Result->SetStringField(TEXT("layout"), TEXT("Vertical menu with title and Start/Settings/Quit focusable controls")); Result->SetStringField(TEXT("accessibility"), TEXT("Widget names are stable for navigation and automation; project-specific labels/localization remain editable in UMG.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool InputMappingHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Folder = UE5AIMcp::AssetFolderOrDefault(Request.QueryParams.FindRef(TEXT("folder")));
        const FString ActionName = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("action")), TEXT("IA_UE5AI_Action"));
        const FString ContextName = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("context")), TEXT("IMC_UE5AI_Default"));
        const FString KeyName = Request.QueryParams.FindRef(TEXT("key")).IsEmpty() ? TEXT("W") : Request.QueryParams.FindRef(TEXT("key"));
        AsyncTask(ENamedThreads::GameThread, [Folder, ActionName, ContextName, KeyName, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            UPackage* ActionPackage = CreatePackage(*UE5AIMcp::PackageNameFor(Folder, ActionName)); UPackage* ContextPackage = CreatePackage(*UE5AIMcp::PackageNameFor(Folder, ContextName));
            UInputAction* Action = NewObject<UInputAction>(ActionPackage, *ActionName, RF_Public | RF_Standalone | RF_Transactional);
            UInputMappingContext* Context = NewObject<UInputMappingContext>(ContextPackage, *ContextName, RF_Public | RF_Standalone | RF_Transactional);
            if (!Action || !Context) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Enhanced Input asset creation failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            Context->MapKey(Action, FKey(FName(*KeyName))); UE5AIMcp::MarkAssetCreated(Action); UE5AIMcp::MarkAssetCreated(Context);
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("action"), Action->GetPathName()); Result->SetStringField(TEXT("context"), Context->GetPathName()); Result->SetStringField(TEXT("key"), KeyName); Result->SetStringField(TEXT("integration"), TEXT("Add the Mapping Context through EnhancedInputLocalPlayerSubsystem in the target Pawn/Controller.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool BlueprintEditHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Folder = UE5AIMcp::AssetFolderOrDefault(Request.QueryParams.FindRef(TEXT("folder")));
        const FString Name = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("name")), TEXT("BP_UE5AI_Actor"));
        const FString Variable = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("variable")), TEXT("Health"));
        const FString Type = Request.QueryParams.FindRef(TEXT("type")).IsEmpty() ? TEXT("float") : Request.QueryParams.FindRef(TEXT("type"));
        const FString ParentPath = Request.QueryParams.FindRef(TEXT("parent")).IsEmpty() ? TEXT("/Script/Engine.Actor") : Request.QueryParams.FindRef(TEXT("parent"));
        AsyncTask(ENamedThreads::GameThread, [Folder, Name, Variable, Type, ParentPath, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); UClass* Parent = LoadClass<UObject>(nullptr, *ParentPath);
            if (!Parent) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Blueprint parent class could not be loaded.")); Complete(UE5AIMcp::Respond(Result)); return; }
            UPackage* Package = CreatePackage(*UE5AIMcp::PackageNameFor(Folder, Name));
            UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(Parent, Package, *Name, BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), FName(TEXT("UE5AIMcp")));
            if (!Blueprint) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Blueprint creation failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            FEdGraphPinType PinType; PinType.PinCategory = Type.Equals(TEXT("bool"), ESearchCase::IgnoreCase) ? UEdGraphSchema_K2::PC_Boolean : (Type.Equals(TEXT("string"), ESearchCase::IgnoreCase) ? UEdGraphSchema_K2::PC_String : (Type.Equals(TEXT("vector"), ESearchCase::IgnoreCase) ? UEdGraphSchema_K2::PC_Struct : UEdGraphSchema_K2::PC_Real));
            if (PinType.PinCategory == UEdGraphSchema_K2::PC_Real) PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
            if (Type.Equals(TEXT("vector"), ESearchCase::IgnoreCase)) PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
            const bool bVariableAdded = FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*Variable), PinType, Type.Equals(TEXT("float"), ESearchCase::IgnoreCase) ? TEXT("100.0") : FString());
            FKismetEditorUtilities::CompileBlueprint(Blueprint); UE5AIMcp::MarkAssetCreated(Blueprint);
            Result->SetBoolField(TEXT("ok"), bVariableAdded && Blueprint->Status != BS_Error); Result->SetStringField(TEXT("path"), Blueprint->GetPathName()); Result->SetStringField(TEXT("parent"), Parent->GetPathName()); Result->SetStringField(TEXT("variable"), Variable); Result->SetStringField(TEXT("compile_status"), Blueprint->Status == BS_Error ? TEXT("error") : TEXT("compiled")); Result->SetStringField(TEXT("safe_subset"), TEXT("Class and member variable creation are enabled; arbitrary graph node execution is deliberately not exposed until a node allow-list is approved.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool CppScaffoldHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString ClassName = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("class")), TEXT("UE5AIGeneratedObject"));
        const bool bWriteProject = Request.QueryParams.FindRef(TEXT("write_project")).Equals(TEXT("true"), ESearchCase::IgnoreCase);
        AsyncTask(ENamedThreads::GameThread, [ClassName, bWriteProject, Complete]()
        {
            const FString OutputDir = bWriteProject ? FPaths::Combine(FPaths::ProjectDir(), TEXT("Source")) : FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UE5AI/GeneratedSource"));
            const FString HeaderPath = FPaths::Combine(OutputDir, ClassName + TEXT(".h")); const FString SourcePath = FPaths::Combine(OutputDir, ClassName + TEXT(".cpp"));
            const FString Header = FString::Printf(TEXT("#pragma once\n\n#include \"CoreMinimal.h\"\n#include \"UObject/Object.h\"\n#include \"%s.generated.h\"\n\nUCLASS(BlueprintType)\nclass U%s : public UObject\n{\n    GENERATED_BODY()\npublic:\n    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=UE5AI)\n    float Value = 0.0f;\n};\n"), *ClassName, *ClassName);
            const FString Source = FString::Printf(TEXT("#include \"%s.h\"\n"), *ClassName);
            bool bWrote = false;
            if (bWriteProject) { IFileManager::Get().MakeDirectory(*OutputDir, true); bWrote = FFileHelper::SaveStringToFile(Header, *HeaderPath) && FFileHelper::SaveStringToFile(Source, *SourcePath); }
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), !bWriteProject || bWrote); Result->SetBoolField(TEXT("wrote_project_source"), bWrote); Result->SetStringField(TEXT("header"), HeaderPath); Result->SetStringField(TEXT("source"), SourcePath); Result->SetStringField(TEXT("status"), bWriteProject ? (bWrote ? TEXT("source written; regenerate project files and compile with UBT") : TEXT("source write failed")) : TEXT("preview only; set write_project=true to modify Project/Source")); Result->SetStringField(TEXT("safety"), TEXT("No compiler process is launched from the editor; compile is delegated to the audited UBT pipeline.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool DependencyGraphHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Path = Request.QueryParams.FindRef(TEXT("asset"));
        if (Path.IsEmpty()) return RespondError(TEXT("Query parameter 'asset' is required."), Complete);
        AsyncTask(ENamedThreads::GameThread, [Path, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); UObject* Asset = LoadObject<UObject>(nullptr, *Path);
            if (!Asset) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Asset not found.")); Complete(UE5AIMcp::Respond(Result)); return; }
            IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get(); TArray<FName> Dependencies; TArray<FName> Referencers; Registry.GetDependencies(Asset->GetOutermost()->GetFName(), Dependencies, UE::AssetRegistry::EDependencyCategory::Package); Registry.GetReferencers(Asset->GetOutermost()->GetFName(), Referencers, UE::AssetRegistry::EDependencyCategory::Package);
            TArray<TSharedPtr<FJsonValue>> DependencyValues; TArray<TSharedPtr<FJsonValue>> ReferencerValues; for (const FName& Value : Dependencies) { if (DependencyValues.Num() >= 200) break; DependencyValues.Add(MakeShared<FJsonValueString>(Value.ToString())); } for (const FName& Value : Referencers) { if (ReferencerValues.Num() >= 200) break; ReferencerValues.Add(MakeShared<FJsonValueString>(Value.ToString())); }
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("asset"), Asset->GetPathName()); Result->SetArrayField(TEXT("dependencies"), DependencyValues); Result->SetArrayField(TEXT("referencers"), ReferencerValues); Result->SetStringField(TEXT("analysis_scope"), TEXT("One-hop package graph. Blueprint/C++ call analysis requires a project code index and is reported separately.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool GameplayScaffoldHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Kind = Request.QueryParams.FindRef(TEXT("kind")).IsEmpty() ? TEXT("character") : Request.QueryParams.FindRef(TEXT("kind"));
        const FString Folder = UE5AIMcp::AssetFolderOrDefault(Request.QueryParams.FindRef(TEXT("folder")));
        const FString Name = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("name")), TEXT("BP_UE5AI_Character"));
        AsyncTask(ENamedThreads::GameThread, [Kind, Folder, Name, Complete]()
        {
            const FString ParentPath = Kind.Equals(TEXT("controller"), ESearchCase::IgnoreCase) ? TEXT("/Script/Engine.PlayerController") : (Kind.Equals(TEXT("pawn"), ESearchCase::IgnoreCase) ? TEXT("/Script/Engine.Pawn") : TEXT("/Script/Engine.Character"));
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); UClass* Parent = LoadClass<UObject>(nullptr, *ParentPath); UPackage* Package = CreatePackage(*UE5AIMcp::PackageNameFor(Folder, Name));
            UBlueprint* Blueprint = Parent ? FKismetEditorUtilities::CreateBlueprint(Parent, Package, *Name, BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), FName(TEXT("UE5AIMcp"))) : nullptr;
            if (!Blueprint) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Gameplay Blueprint creation failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            FEdGraphPinType FloatType; FloatType.PinCategory = UEdGraphSchema_K2::PC_Real; FloatType.PinSubCategory = UEdGraphSchema_K2::PC_Float; FBlueprintEditorUtils::AddMemberVariable(Blueprint, TEXT("Health"), FloatType, TEXT("100.0"));
            FEdGraphPinType BoolType; BoolType.PinCategory = UEdGraphSchema_K2::PC_Boolean; FBlueprintEditorUtils::AddMemberVariable(Blueprint, TEXT("bCanInteract"), BoolType, TEXT("true"));
            FKismetEditorUtilities::CompileBlueprint(Blueprint); UE5AIMcp::MarkAssetCreated(Blueprint);
            Result->SetBoolField(TEXT("ok"), Blueprint->Status != BS_Error); Result->SetStringField(TEXT("path"), Blueprint->GetPathName()); Result->SetStringField(TEXT("kind"), Kind); Result->SetStringField(TEXT("foundation"), TEXT("Gameplay Framework parent with Health and interaction state; Behavior Tree/EQS/StateTree authoring remains conditional on their project plugins and schemas.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool GasScaffoldHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Folder = UE5AIMcp::AssetFolderOrDefault(Request.QueryParams.FindRef(TEXT("folder")));
        const FString Name = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("name")), TEXT("GE_UE5AI_Base"));
        AsyncTask(ENamedThreads::GameThread, [Folder, Name, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); UClass* EffectClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Script/GameplayAbilities.GameplayEffect"));
            Result->SetBoolField(TEXT("available"), EffectClass != nullptr);
            if (!EffectClass) { Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("status"), TEXT("GameplayAbilities plugin is not enabled for this project; no GAS asset was created.")); Complete(UE5AIMcp::Respond(Result)); return; }
            UPackage* Package = CreatePackage(*UE5AIMcp::PackageNameFor(Folder, Name)); UObject* Effect = NewObject<UObject>(Package, EffectClass, *Name, RF_Public | RF_Standalone | RF_Transactional);
            if (!Effect) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Gameplay Effect creation failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            UE5AIMcp::MarkAssetCreated(Effect); Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("path"), Effect->GetPathName()); Result->SetStringField(TEXT("status"), TEXT("Base Gameplay Effect created. Attributes, abilities, tags and prediction policy require project-specific gameplay classes.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool MaterialGraphHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Folder = UE5AIMcp::AssetFolderOrDefault(Request.QueryParams.FindRef(TEXT("folder")));
        const FString Name = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("name")), TEXT("M_UE5AI_PBR"));
        const float Roughness = FMath::Clamp(FCString::Atof(*Request.QueryParams.FindRef(TEXT("roughness"))), 0.0f, 1.0f);
        AsyncTask(ENamedThreads::GameThread, [Folder, Name, Roughness, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>(); UMaterial* Material = Cast<UMaterial>(FAssetToolsModule::GetModule().Get().CreateAsset(Name, Folder, UMaterial::StaticClass(), Factory));
            if (!Material || !Material->GetEditorOnlyData()) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Material creation failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            Material->Modify(); UMaterialExpressionConstant3Vector* BaseColor = NewObject<UMaterialExpressionConstant3Vector>(Material); BaseColor->Constant = FLinearColor(0.18f, 0.48f, 0.9f); BaseColor->MaterialExpressionEditorX = -420; BaseColor->MaterialExpressionEditorY = 0;
            UMaterialExpressionScalarParameter* RoughnessNode = NewObject<UMaterialExpressionScalarParameter>(Material); RoughnessNode->ParameterName = TEXT("Roughness"); RoughnessNode->DefaultValue = Roughness; RoughnessNode->MaterialExpressionEditorX = -420; RoughnessNode->MaterialExpressionEditorY = 160;
            Material->GetExpressionCollection().AddExpression(BaseColor); Material->GetExpressionCollection().AddExpression(RoughnessNode); Material->GetEditorOnlyData()->BaseColor.Expression = BaseColor; Material->GetEditorOnlyData()->Roughness.Expression = RoughnessNode; Material->PostEditChange(); Material->MarkPackageDirty();
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("path"), Material->GetPathName()); Result->SetNumberField(TEXT("nodes"), Material->GetExpressions().Num()); Result->SetNumberField(TEXT("roughness"), Roughness); Result->SetStringField(TEXT("graph"), TEXT("BaseColor constant + exposed Roughness scalar connected to PBR material inputs.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool ProceduralMeshHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const float Size = FMath::Clamp(FCString::Atof(*Request.QueryParams.FindRef(TEXT("size"))), 1.0f, 100000.0f);
        AsyncTask(ENamedThreads::GameThread, [Size, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
            if (!World) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("No editor world.")); Complete(UE5AIMcp::Respond(Result)); return; }
            const FScopedTransaction Transaction(NSLOCTEXT("UE5AIMcp", "CreateProceduralBox", "UE5 AI: Create Procedural Box")); AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity); UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(Owner, TEXT("UE5AI_ProceduralMesh")); Owner->SetRootComponent(Mesh); Mesh->RegisterComponent();
            const float H = Size * 0.5f; const TArray<FVector> Vertices = { FVector(-H,-H,-H), FVector(H,-H,-H), FVector(H,H,-H), FVector(-H,H,-H), FVector(-H,-H,H), FVector(H,-H,H), FVector(H,H,H), FVector(-H,H,H) };
            const TArray<int32> Triangles = { 0,2,1, 0,3,2, 4,5,6, 4,6,7, 0,1,5, 0,5,4, 1,2,6, 1,6,5, 2,3,7, 2,7,6, 3,0,4, 3,4,7 };
            TArray<FVector> Normals; Normals.Init(FVector::UpVector, Vertices.Num()); TArray<FVector2D> UVs; UVs.Init(FVector2D::ZeroVector, Vertices.Num()); TArray<FLinearColor> Colors; Colors.Init(FLinearColor::White, Vertices.Num()); TArray<FProcMeshTangent> Tangents; Tangents.Init(FProcMeshTangent(1,0,0), Vertices.Num()); Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, true);
            Owner->Modify(); Owner->SetActorLabel(TEXT("UE5AI_ProceduralBox")); Owner->Tags.AddUnique(TEXT("UE5AI_ProceduralMesh")); Owner->MarkPackageDirty(); Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("path"), Owner->GetPathName()); Result->SetNumberField(TEXT("vertices"), Vertices.Num()); Result->SetNumberField(TEXT("triangles"), Triangles.Num() / 3); Result->SetBoolField(TEXT("collision"), true); Result->SetStringField(TEXT("note"), TEXT("Runtime procedural mesh generated with UVs and collision. Asset baking/LOD generation should be performed as an approved follow-up.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool ExternalJobHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Tool = Request.QueryParams.FindRef(TEXT("tool")).IsEmpty() ? TEXT("Blender") : Request.QueryParams.FindRef(TEXT("tool")); const FString Specification = Request.QueryParams.FindRef(TEXT("spec")).IsEmpty() ? TEXT("asset generation request") : Request.QueryParams.FindRef(TEXT("spec"));
        AsyncTask(ENamedThreads::GameThread, [Tool, Specification, Complete]()
        {
            const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UE5AI/DccJobs"))); IFileManager::Get().MakeDirectory(*Directory, true); const FString Id = FGuid::NewGuid().ToString(EGuidFormats::Digits); const FString Filename = FPaths::Combine(Directory, Id + TEXT(".json"));
            TSharedRef<FJsonObject> Job = MakeShared<FJsonObject>(); Job->SetStringField(TEXT("id"), Id); Job->SetStringField(TEXT("tool"), Tool); Job->SetStringField(TEXT("specification"), Specification); Job->SetStringField(TEXT("status"), TEXT("planned")); Job->SetStringField(TEXT("required_import_validation"), TEXT("scale, naming, UVs, collision, licenses and source provenance")); FString Json; FJsonSerializer::Serialize(Job, TJsonWriterFactory<>::Create(&Json));
            const bool bWritten = FFileHelper::SaveStringToFile(Json, *Filename); TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), bWritten); Result->SetStringField(TEXT("job"), Filename); Result->SetStringField(TEXT("status"), bWritten ? TEXT("planned; hand manifest to the approved external DCC adapter, then import via ue5_import") : TEXT("manifest write failed")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool AnimationHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString SkeletonPath = Request.QueryParams.FindRef(TEXT("skeleton")); const FString Folder = UE5AIMcp::AssetFolderOrDefault(Request.QueryParams.FindRef(TEXT("folder"))); const FString Name = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("name")), TEXT("AM_UE5AI"));
        if (SkeletonPath.IsEmpty()) return RespondError(TEXT("Query parameter 'skeleton' is required for a Montage."), Complete);
        AsyncTask(ENamedThreads::GameThread, [SkeletonPath, Folder, Name, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, *SkeletonPath); if (!Skeleton) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Skeleton not found.")); Complete(UE5AIMcp::Respond(Result)); return; }
            UPackage* Package = CreatePackage(*UE5AIMcp::PackageNameFor(Folder, Name)); UAnimMontage* Montage = NewObject<UAnimMontage>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional); Montage->SetSkeleton(Skeleton); FCompositeSection& Section = Montage->CompositeSections.AddDefaulted_GetRef(); Section.SectionName = TEXT("Default"); UE5AIMcp::MarkAssetCreated(Montage);
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("path"), Montage->GetPathName()); Result->SetStringField(TEXT("skeleton"), Skeleton->GetPathName()); Result->SetStringField(TEXT("note"), TEXT("Empty Montage with a Default section created; add sequences, notifies and slots through Animation Data Controller for authored motion.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool AnimationValidateHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Path = Request.QueryParams.FindRef(TEXT("asset")); if (Path.IsEmpty()) return RespondError(TEXT("Query parameter 'asset' is required."), Complete);
        AsyncTask(ENamedThreads::GameThread, [Path, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); UAnimationAsset* Asset = LoadObject<UAnimationAsset>(nullptr, *Path); if (!Asset) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Animation asset not found.")); Complete(UE5AIMcp::Respond(Result)); return; }
            Asset->ValidateSkeleton(); const bool bHasSkeleton = Asset->GetSkeleton() != nullptr; Result->SetBoolField(TEXT("ok"), bHasSkeleton); Result->SetStringField(TEXT("asset"), Asset->GetPathName()); Result->SetStringField(TEXT("skeleton"), bHasSkeleton ? Asset->GetSkeleton()->GetPathName() : TEXT("")); Result->SetStringField(TEXT("result"), bHasSkeleton ? TEXT("Skeleton binding validated. Rig/IK retarget configuration is project-plugin dependent.") : TEXT("Animation has no Skeleton binding.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool SequenceEditHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Path = Request.QueryParams.FindRef(TEXT("asset")); if (Path.IsEmpty()) return RespondError(TEXT("Query parameter 'asset' is required."), Complete);
        AsyncTask(ENamedThreads::GameThread, [Path, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); ULevelSequence* Sequence = LoadObject<ULevelSequence>(nullptr, *Path); UMovieScene* Scene = Sequence ? Sequence->GetMovieScene() : nullptr; if (!Scene) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Level Sequence or Movie Scene not found.")); Complete(UE5AIMcp::Respond(Result)); return; }
            Sequence->Modify();
            // Generic float tracks are property tracks and may be rejected by
            // Sequencer without a resolved object binding.  The native Slomo
            // track is a supported master float track, so it gives this
            // endpoint a valid target-free sequence editing primitive.
            UMovieSceneSlomoTrack* Track = Scene->AddTrack<UMovieSceneSlomoTrack>();
            UMovieSceneSlomoSection* Section = Track ? Cast<UMovieSceneSlomoSection>(Track->CreateNewSection()) : nullptr;
            if (!Section) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Time dilation Float track creation failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            TArray<FFrameNumber> Times = { FFrameNumber(0), FFrameNumber(60) }; TArray<FMovieSceneFloatValue> Values = { FMovieSceneFloatValue(0.0f), FMovieSceneFloatValue(1.0f) }; Section->FloatCurve.AddKeys(Times, Values); Track->AddSection(*Section); Sequence->MarkPackageDirty(); Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("asset"), Sequence->GetPathName()); Result->SetNumberField(TEXT("keyframes"), 2); Result->SetStringField(TEXT("track"), TEXT("Time Dilation float master track")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool NiagaraHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Folder = UE5AIMcp::AssetFolderOrDefault(Request.QueryParams.FindRef(TEXT("folder"))); const FString Name = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("name")), TEXT("NS_UE5AI"));
        AsyncTask(ENamedThreads::GameThread, [Folder, Name, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); UClass* SystemClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Script/Niagara.NiagaraSystem")); Result->SetBoolField(TEXT("available"), SystemClass != nullptr);
            if (!SystemClass) { Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("status"), TEXT("Niagara plugin is not enabled for this project; no system asset was created.")); Complete(UE5AIMcp::Respond(Result)); return; }
            UPackage* Package = CreatePackage(*UE5AIMcp::PackageNameFor(Folder, Name)); UObject* System = NewObject<UObject>(Package, SystemClass, *Name, RF_Public | RF_Standalone | RF_Transactional); if (!System) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Niagara System creation failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            UE5AIMcp::MarkAssetCreated(System); Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("path"), System->GetPathName()); Result->SetStringField(TEXT("status"), TEXT("Empty Niagara System scaffold created. Emitter/module stack authoring requires the Niagara editor module and an approved module allow-list.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool AudioHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Folder = UE5AIMcp::AssetFolderOrDefault(Request.QueryParams.FindRef(TEXT("folder"))); const FString Name = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("name")), TEXT("SC_UE5AI")); const FString WavePath = Request.QueryParams.FindRef(TEXT("wave"));
        AsyncTask(ENamedThreads::GameThread, [Folder, Name, WavePath, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); USoundCueFactoryNew* Factory = NewObject<USoundCueFactoryNew>(); if (!WavePath.IsEmpty()) { if (USoundWave* Wave = LoadObject<USoundWave>(nullptr, *WavePath)) Factory->InitialSoundWaves.Add(Wave); else { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Requested SoundWave was not found.")); Complete(UE5AIMcp::Respond(Result)); return; } }
            USoundCue* Cue = Cast<USoundCue>(FAssetToolsModule::GetModule().Get().CreateAsset(Name, Folder, USoundCue::StaticClass(), Factory)); if (!Cue) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Sound Cue creation failed.")); Complete(UE5AIMcp::Respond(Result)); return; }
            Cue->MarkPackageDirty(); Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("path"), Cue->GetPathName()); Result->SetStringField(TEXT("source"), WavePath); Result->SetStringField(TEXT("routing"), TEXT("Sound Cue created; attenuation, submix and MetaSound routing remain editable through their native assets.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool NetworkReplicationHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
            int32 ReplicatedActors = 0;
            if (World) for (TActorIterator<AActor> It(World); It; ++It) if (It->GetIsReplicated()) ++ReplicatedActors;
            TArray<FString> Checks = { TEXT("Replicated properties must use explicit conditions and OnRep handling."), TEXT("RPCs require ownership, validation policy and a multiplayer PIE or server regression run."), TEXT("Session creation and dedicated-server launch are emitted as approval-gated process plans.") };
            Result->SetBoolField(TEXT("ok"), World != nullptr);
            if (!World) { Result->SetStringField(TEXT("error"), TEXT("No editor world.")); Complete(UE5AIMcp::Respond(Result)); return; }
            Result->SetStringField(TEXT("world"), World->GetPathName());
            Result->SetNumberField(TEXT("net_mode"), static_cast<int32>(World->GetNetMode()));
            Result->SetNumberField(TEXT("replicated_actor_count"), ReplicatedActors);
            Result->SetBoolField(TEXT("pie_running"), GEditor && GEditor->PlayWorld != nullptr);
            Result->SetArrayField(TEXT("regression_checks"), UE5AIMcp::JsonStrings(Checks));
            Result->SetStringField(TEXT("boundary"), TEXT("This endpoint inspects topology only; starting clients, servers or online sessions requires an explicit approved process plan."));
            Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool NetworkSessionPlanHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Mode = Request.QueryParams.FindRef(TEXT("mode")).IsEmpty() ? TEXT("listen") : Request.QueryParams.FindRef(TEXT("mode"));
        const int32 Clients = FMath::Clamp(FCString::Atoi(*Request.QueryParams.FindRef(TEXT("clients"))), 1, 8);
        AsyncTask(ENamedThreads::GameThread, [Mode, Clients, Complete]()
        {
            const FString Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
            const FString Filename = UE5AIMcp::EngineeringArtifactPath(TEXT("Network"), TEXT("SessionPlan_") + Id);
            TSharedRef<FJsonObject> Plan = MakeShared<FJsonObject>();
            Plan->SetStringField(TEXT("id"), Id); Plan->SetStringField(TEXT("mode"), Mode); Plan->SetNumberField(TEXT("clients"), Clients); Plan->SetStringField(TEXT("project"), FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()));
            TArray<FString> Commands = { TEXT("Launch editor/server with -server or listen parameters"), FString::Printf(TEXT("Launch %d client process(es) with -game -log"), Clients), TEXT("Collect NetDriver, packet-loss and functional-test logs"), TEXT("Stop all launched child processes and archive logs") };
            Plan->SetArrayField(TEXT("steps"), UE5AIMcp::JsonStrings(Commands)); Plan->SetStringField(TEXT("execution"), TEXT("approval_required; no multiplayer process was started"));
            const bool bWritten = UE5AIMcp::SaveJsonArtifact(Filename, Plan);
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), bWritten); Result->SetStringField(TEXT("plan"), Filename); Result->SetStringField(TEXT("mode"), Mode); Result->SetNumberField(TEXT("clients"), Clients); Result->SetBoolField(TEXT("approval_required"), true); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool PlatformProfileHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [Complete]()
        {
            TArray<TSharedRef<IPlugin>> Plugins = IPluginManager::Get().GetEnabledPlugins();
            TArray<FString> PluginNames; for (const TSharedRef<IPlugin>& Plugin : Plugins) PluginNames.Add(Plugin->GetName());
            const FString ConfigDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir());
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("project_file"), FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath())); Result->SetStringField(TEXT("config_dir"), ConfigDirectory);
            Result->SetBoolField(TEXT("default_engine_ini_exists"), IFileManager::Get().FileExists(*FPaths::Combine(ConfigDirectory, TEXT("DefaultEngine.ini"))));
            Result->SetBoolField(TEXT("default_game_ini_exists"), IFileManager::Get().FileExists(*FPaths::Combine(ConfigDirectory, TEXT("DefaultGame.ini"))));
            Result->SetNumberField(TEXT("enabled_plugin_count"), PluginNames.Num()); Result->SetArrayField(TEXT("enabled_plugins"), UE5AIMcp::JsonStrings(PluginNames, 64));
            Result->SetArrayField(TEXT("platforms"), UE5AIMcp::JsonStrings({ TEXT("Win64"), TEXT("Linux"), TEXT("Android"), TEXT("IOS"), TEXT("XR (plugin-dependent)") }));
            Result->SetStringField(TEXT("boundary"), TEXT("Signing, store credentials, entitlements and third-party SDK installation remain approval-gated and are never inferred.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool PipelinePlanHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Platform = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("platform")), TEXT("Win64"));
        const FString Configuration = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("configuration")), TEXT("Development"));
        AsyncTask(ENamedThreads::GameThread, [Platform, Configuration, Complete]()
        {
            const FString Project = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
            const FString Uat = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::EngineDir(), TEXT("Build/BatchFiles/RunUAT.bat")));
            const FString Command = FString::Printf(TEXT("\"%s\" BuildCookRun -project=\"%s\" -noP4 -platform=%s -clientconfig=%s -build -cook -stage -pak -archive"), *Uat, *Project, *Platform, *Configuration);
            const FString Filename = UE5AIMcp::EngineeringArtifactPath(TEXT("Pipeline"), TEXT("BuildCookRun_") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
            TSharedRef<FJsonObject> Plan = MakeShared<FJsonObject>(); Plan->SetStringField(TEXT("command"), Command); Plan->SetStringField(TEXT("platform"), Platform); Plan->SetStringField(TEXT("configuration"), Configuration); Plan->SetArrayField(TEXT("verification"), UE5AIMcp::JsonStrings({ TEXT("Compile targets"), TEXT("Cook content"), TEXT("Package/stage output"), TEXT("Archive logs and manifests"), TEXT("Require explicit deploy target approval") }));
            const bool bWritten = UE5AIMcp::SaveJsonArtifact(Filename, Plan);
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), bWritten); Result->SetStringField(TEXT("plan"), Filename); Result->SetStringField(TEXT("command"), Command); Result->SetBoolField(TEXT("dry_run"), true); Result->SetStringField(TEXT("boundary"), TEXT("Cook, package, signing, install and launch are not executed by this endpoint.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool BudgetAuditHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [Complete]()
        {
            IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
            TArray<FAssetData> Assets; Registry.GetAllAssets(Assets, true);
            int32 Textures = 0, Materials = 0, Meshes = 0;
            for (const FAssetData& Asset : Assets)
            {
                const FName ClassName = Asset.AssetClassPath.GetAssetName();
                if (ClassName == TEXT("Texture2D") || ClassName == TEXT("TextureCube")) ++Textures;
                else if (ClassName == TEXT("Material") || ClassName == TEXT("MaterialInstanceConstant")) ++Materials;
                else if (ClassName == TEXT("StaticMesh") || ClassName == TEXT("SkeletalMesh")) ++Meshes;
            }
            const FPlatformMemoryStats Memory = FPlatformMemory::GetStats();
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), true); Result->SetNumberField(TEXT("asset_count"), Assets.Num()); Result->SetNumberField(TEXT("texture_count"), Textures); Result->SetNumberField(TEXT("material_count"), Materials); Result->SetNumberField(TEXT("mesh_count"), Meshes); Result->SetNumberField(TEXT("draw_calls"), GNumDrawCallsRHI[0]); Result->SetNumberField(TEXT("used_physical_bytes"), static_cast<double>(Memory.UsedPhysical));
            Result->SetArrayField(TEXT("recommendations"), UE5AIMcp::JsonStrings({ TEXT("Set target-platform budgets before changing LODs or texture groups."), TEXT("Use Unreal Insights/RenderDoc captures for multi-frame diagnosis."), TEXT("Treat Nanite, streaming and material edits as approval-gated changes."), TEXT("Archive before/after evidence for every optimization." ) })); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool ComplianceAuditHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [Complete]()
        {
            const FString ConfigDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir());
            const bool bInternationalizationConfigured = IFileManager::Get().FileExists(*FPaths::Combine(ConfigDir, TEXT("DefaultGame.ini")));
            TArray<FString> Checks = { TEXT("Localization culture list and locres generation are project-release gates."), TEXT("UI navigation names and screen-reader text require a product accessibility review."), TEXT("Ratings, privacy, licenses and platform policy evidence require human approval."), TEXT("Content provenance remains attached to external DCC/import manifests.") };
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), true); Result->SetBoolField(TEXT("default_game_ini_exists"), bInternationalizationConfigured); Result->SetArrayField(TEXT("checks"), UE5AIMcp::JsonStrings(Checks)); Result->SetStringField(TEXT("status"), TEXT("audit baseline generated; no localization or compliance data was modified.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool SourceControlStatusHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [Complete]()
        {
            const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
            const FString PluginDir = FPaths::ConvertRelativePathToFull(IPluginManager::Get().FindPlugin(TEXT("UE5AIMcp")).IsValid() ? IPluginManager::Get().FindPlugin(TEXT("UE5AIMcp"))->GetBaseDir() : FString());
            const bool bProjectGit = IFileManager::Get().DirectoryExists(*FPaths::Combine(ProjectDir, TEXT(".git")));
            const bool bPluginGit = !PluginDir.IsEmpty() && IFileManager::Get().DirectoryExists(*FPaths::Combine(PluginDir, TEXT(".git")));
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), true); Result->SetBoolField(TEXT("project_git_repository"), bProjectGit); Result->SetBoolField(TEXT("plugin_git_repository"), bPluginGit); Result->SetStringField(TEXT("provider"), TEXT("provider discovery only")); Result->SetArrayField(TEXT("safe_operations"), UE5AIMcp::JsonStrings({ TEXT("read status"), TEXT("prepare change list"), TEXT("request human-reviewed commit/submit"), TEXT("detect asset lock/conflict before mutation") })); Result->SetStringField(TEXT("boundary"), TEXT("This endpoint never commits, pushes, submits or resolves conflicts automatically.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool CiPlanHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Provider = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("provider")), TEXT("local"));
        AsyncTask(ENamedThreads::GameThread, [Provider, Complete]()
        {
            const FString Filename = UE5AIMcp::EngineeringArtifactPath(TEXT("CI"), TEXT("Pipeline_") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
            TSharedRef<FJsonObject> Plan = MakeShared<FJsonObject>(); Plan->SetStringField(TEXT("provider"), Provider); Plan->SetArrayField(TEXT("stages"), UE5AIMcp::JsonStrings({ TEXT("checkout/lock preflight"), TEXT("UBT editor build"), TEXT("automation + functional test"), TEXT("cook/package candidate"), TEXT("performance regression comparison"), TEXT("archive logs, reports and symbols"), TEXT("release approval gate") })); Plan->SetStringField(TEXT("execution"), TEXT("definition-only; no external CI service was contacted"));
            const bool bWritten = UE5AIMcp::SaveJsonArtifact(Filename, Plan); TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), bWritten); Result->SetStringField(TEXT("plan"), Filename); Result->SetStringField(TEXT("provider"), Provider); Result->SetBoolField(TEXT("external_dispatch"), false); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool CrashDiagnosticsHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [Complete]()
        {
            const FString CrashRoot = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Crashes")); TArray<FString> CrashFolders; IFileManager::Get().FindFiles(CrashFolders, *FPaths::Combine(CrashRoot, TEXT("*")), false, true);
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("crash_root"), FPaths::ConvertRelativePathToFull(CrashRoot)); Result->SetNumberField(TEXT("crash_folder_count"), CrashFolders.Num()); Result->SetArrayField(TEXT("recent_crash_folders"), UE5AIMcp::JsonStrings(CrashFolders, 10)); Result->SetArrayField(TEXT("reproduction_workflow"), UE5AIMcp::JsonStrings({ TEXT("Preserve logs, minidump and engine version metadata."), TEXT("Bisect changed assets/code against the smallest affected map."), TEXT("Write a deterministic reproduction plan before proposing a fix."), TEXT("Validate a fix through the relevant automation and packaging gates.") })); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool PluginScaffoldHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Name = UE5AIMcp::SafeAssetName(Request.QueryParams.FindRef(TEXT("name")), TEXT("UE5AIExtension")); const bool bWriteProject = Request.QueryParams.FindRef(TEXT("write_project")).Equals(TEXT("true"), ESearchCase::IgnoreCase);
        AsyncTask(ENamedThreads::GameThread, [Name, bWriteProject, Complete]()
        {
            const FString PluginDir = FPaths::Combine(FPaths::ProjectPluginsDir(), Name); const FString DescriptorPath = FPaths::Combine(PluginDir, Name + TEXT(".uplugin"));
            const FString Descriptor = FString::Printf(TEXT("{\n  \"FileVersion\": 3,\n  \"Version\": 1,\n  \"VersionName\": \"0.1.0\",\n  \"FriendlyName\": \"%s\",\n  \"Category\": \"AI\",\n  \"CanContainContent\": false,\n  \"Modules\": [ { \"Name\": \"%s\", \"Type\": \"Editor\", \"LoadingPhase\": \"Default\" } ]\n}\n"), *Name, *Name);
            bool bWritten = false; FString Error;
            if (bWriteProject)
            {
                if (IFileManager::Get().DirectoryExists(*PluginDir)) Error = TEXT("Refusing to overwrite an existing plugin directory.");
                else { IFileManager::Get().MakeDirectory(*PluginDir, true); bWritten = FFileHelper::SaveStringToFile(Descriptor, *DescriptorPath); if (!bWritten) Error = TEXT("Plugin descriptor write failed."); }
            }
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), !bWriteProject || bWritten); Result->SetBoolField(TEXT("wrote_project_plugin"), bWritten); Result->SetStringField(TEXT("plugin_dir"), FPaths::ConvertRelativePathToFull(PluginDir)); Result->SetStringField(TEXT("descriptor"), FPaths::ConvertRelativePathToFull(DescriptorPath)); Result->SetStringField(TEXT("descriptor_preview"), Descriptor); Result->SetStringField(TEXT("status"), bWriteProject ? (bWritten ? TEXT("descriptor written; add source module files and build with UBT") : Error) : TEXT("preview only; set write_project=true to create a new plugin directory")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool EditorAdaptersHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [Complete]()
        {
            const TArray<FString> Domains = { TEXT("ModelingToolsEditorMode"), TEXT("Persona"), TEXT("ControlRigEditor"), TEXT("LevelSequenceEditor"), TEXT("NiagaraEditor"), TEXT("VirtualProductionUtilities"), TEXT("XRBase"), TEXT("ChaosEditor") };
            TArray<TSharedPtr<FJsonValue>> Adapters;
            for (const FString& Domain : Domains) { TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>(); Entry->SetStringField(TEXT("module"), Domain); Entry->SetBoolField(TEXT("loaded"), FModuleManager::Get().IsModuleLoaded(FName(*Domain))); Entry->SetStringField(TEXT("access"), TEXT("inspect first; mutations require a domain-specific allow-list")); Adapters.Add(MakeShared<FJsonValueObject>(Entry)); }
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), true); Result->SetArrayField(TEXT("adapters"), Adapters); Result->SetStringField(TEXT("compatibility"), TEXT("The registry exposes installed editor domains without loading optional modules or invoking unsupported workflows.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool AgentPlanHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Objective = Request.QueryParams.FindRef(TEXT("objective")).IsEmpty() ? TEXT("UE5 engineering task") : Request.QueryParams.FindRef(TEXT("objective"));
        AsyncTask(ENamedThreads::GameThread, [this, Objective, Complete]()
        {
            const FString Id = FGuid::NewGuid().ToString(EGuidFormats::Digits); const FString Filename = UE5AIMcp::EngineeringArtifactPath(TEXT("Agent"), TEXT("Task_") + Id);
            TSharedRef<FJsonObject> Task = MakeShared<FJsonObject>(); Task->SetStringField(TEXT("id"), Id); Task->SetStringField(TEXT("objective"), Objective); Task->SetStringField(TEXT("state"), TEXT("awaiting_approval")); Task->SetArrayField(TEXT("checkpoints"), UE5AIMcp::JsonStrings({ TEXT("inspect evidence"), TEXT("plan bounded mutations"), TEXT("obtain approval for external/high-impact actions"), TEXT("execute and verify"), TEXT("archive evidence and clean temporary artifacts") }));
            const bool bWritten = UE5AIMcp::SaveJsonArtifact(Filename, Task); if (bWritten) { AgentStates.Add(Id, TEXT("awaiting_approval")); AgentArtifacts.Add(Id, Filename); }
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), bWritten); Result->SetStringField(TEXT("task_id"), Id); Result->SetStringField(TEXT("state"), TEXT("awaiting_approval")); Result->SetStringField(TEXT("artifact"), Filename); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool AgentTransitionHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Id = Request.QueryParams.FindRef(TEXT("task")); const FString Action = Request.QueryParams.FindRef(TEXT("action"));
        if (Id.IsEmpty() || Action.IsEmpty()) return RespondError(TEXT("Query parameters 'task' and 'action' are required."), Complete);
        AsyncTask(ENamedThreads::GameThread, [this, Id, Action, Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); FString* Current = AgentStates.Find(Id); if (!Current) { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), TEXT("Unknown task id; restore from the saved task artifact before transitioning.")); Complete(UE5AIMcp::Respond(Result)); return; }
            FString Next; if (Action.Equals(TEXT("approve"), ESearchCase::IgnoreCase) && *Current == TEXT("awaiting_approval")) Next = TEXT("ready"); else if (Action.Equals(TEXT("start"), ESearchCase::IgnoreCase) && *Current == TEXT("ready")) Next = TEXT("running"); else if (Action.Equals(TEXT("complete"), ESearchCase::IgnoreCase) && *Current == TEXT("running")) Next = TEXT("completed"); else { Result->SetBoolField(TEXT("ok"), false); Result->SetStringField(TEXT("error"), FString::Printf(TEXT("Action '%s' is not allowed from state '%s'."), *Action, **Current)); Complete(UE5AIMcp::Respond(Result)); return; }
            *Current = Next; const FString* Filename = AgentArtifacts.Find(Id); if (Filename) { TSharedRef<FJsonObject> Task = MakeShared<FJsonObject>(); Task->SetStringField(TEXT("id"), Id); Task->SetStringField(TEXT("state"), Next); Task->SetStringField(TEXT("last_action"), Action); Task->SetStringField(TEXT("updated_utc"), FDateTime::UtcNow().ToIso8601()); UE5AIMcp::SaveJsonArtifact(*Filename, Task); }
            Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("task_id"), Id); Result->SetStringField(TEXT("state"), Next); Result->SetBoolField(TEXT("external_actions_started"), false); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool SceneOrchestrateHandler(const FHttpServerRequest& Request, const FHttpResultCallback& Complete)
    {
        const FString Specification = Request.QueryParams.FindRef(TEXT("spec")).IsEmpty() ? TEXT("A playable UE5 scene") : Request.QueryParams.FindRef(TEXT("spec"));
        AsyncTask(ENamedThreads::GameThread, [Specification, Complete]()
        {
            const FString Filename = UE5AIMcp::EngineeringArtifactPath(TEXT("ScenePlans"), TEXT("Scene_") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
            const TArray<FString> Phases = { TEXT("requirements and risk gates"), TEXT("project/world inspection"), TEXT("asset manifest and provenance"), TEXT("graybox/layout"), TEXT("materials, lighting and rendering"), TEXT("gameplay/input/UI"), TEXT("animation/audio/VFX"), TEXT("multiplayer and platform configuration"), TEXT("automation/functional validation"), TEXT("performance and accessibility audit"), TEXT("cook/package candidate"), TEXT("release evidence and approval") };
            TSharedRef<FJsonObject> Plan = MakeShared<FJsonObject>(); Plan->SetStringField(TEXT("specification"), Specification); Plan->SetArrayField(TEXT("phases"), UE5AIMcp::JsonStrings(Phases)); Plan->SetStringField(TEXT("execution"), TEXT("dry-run orchestration plan; no scene assets were created"));
            const bool bWritten = UE5AIMcp::SaveJsonArtifact(Filename, Plan); TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), bWritten); Result->SetStringField(TEXT("plan"), Filename); Result->SetNumberField(TEXT("phase_count"), Phases.Num()); Result->SetBoolField(TEXT("dry_run"), true); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool CapabilityCoverageHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [Complete]()
        {
            TArray<TSharedRef<IPlugin>> Plugins = IPluginManager::Get().GetEnabledPlugins(); TArray<FString> Domains = { TEXT("scene/content"), TEXT("network"), TEXT("platform/build"), TEXT("quality/compliance"), TEXT("collaboration/CI"), TEXT("crash recovery"), TEXT("editor extensions"), TEXT("agent orchestration") };
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("ok"), true); Result->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString()); Result->SetStringField(TEXT("coverage_stage"), TEXT("engineering_takeover")); Result->SetNumberField(TEXT("enabled_plugin_count"), Plugins.Num()); Result->SetNumberField(TEXT("covered_domain_count"), Domains.Num()); Result->SetArrayField(TEXT("covered_domains"), UE5AIMcp::JsonStrings(Domains)); Result->SetStringField(TEXT("boundary"), TEXT("Coverage is capability discovery and controlled contracts, not blanket authorization to change every installed plugin or external service.")); Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool DiagnosticsHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        AsyncTask(ENamedThreads::GameThread, [Complete]()
        {
            TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetBoolField(TEXT("ok"), true);
            Result->SetStringField(TEXT("project"), FApp::GetProjectName());
            Result->SetBoolField(TEXT("is_pie"), GEditor && GEditor->PlayWorld != nullptr);
            Result->SetBoolField(TEXT("has_world"), GEditor && GEditor->GetEditorWorldContext().World() != nullptr);
            Result->SetBoolField(TEXT("write_tools_enabled"), true);
            Result->SetStringField(TEXT("risk"), TEXT("MVP mutating tools are editor-only, transaction-backed where supported, and exposed only on loopback."));
            Complete(UE5AIMcp::Respond(Result));
        });
        return true;
    }

    bool ShutdownHandler(const FHttpServerRequest&, const FHttpResultCallback& Complete)
    {
        // Test-only lifecycle control. The caller receives an acknowledgement before the editor exits.
        TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("ok"), true);
        Result->SetStringField(TEXT("status"), TEXT("exit requested"));
        Complete(UE5AIMcp::Respond(Result));
        AsyncTask(ENamedThreads::GameThread, []() { FPlatformMisc::RequestExit(false); });
        return true;
    }

    TSharedPtr<IHttpRouter> Router;
    FHttpRouteHandle Health;
    FHttpRouteHandle Tools;
    FHttpRouteHandle Snapshot;
    FHttpRouteHandle Properties;
    FHttpRouteHandle CVars;
    FHttpRouteHandle Logs;
    FHttpRouteHandle Screenshot;
    FHttpRouteHandle SpawnActor;
    FHttpRouteHandle DeleteActor;
    FHttpRouteHandle TransformActor;
    FHttpRouteHandle SpawnLight;
    FHttpRouteHandle SpawnGrid;
    FHttpRouteHandle OrganizeActor;
    FHttpRouteHandle CameraPost;
    FHttpRouteHandle MaterialInstance;
    FHttpRouteHandle ImportAsset;
    FHttpRouteHandle ReimportAsset;
    FHttpRouteHandle AssetSummary;
    FHttpRouteHandle CreateSequence;
    FHttpRouteHandle VerifyViewport;
    FHttpRouteHandle ValidateAssets;
    FHttpRouteHandle Undo;
    FHttpRouteHandle Redo;
    FHttpRouteHandle BlueprintSummary;
    FHttpRouteHandle MaterialSummary;
    FHttpRouteHandle ClassSummary;
    FHttpRouteHandle StartPie;
    FHttpRouteHandle StopPie;
    FHttpRouteHandle SaveDirty;
    FHttpRouteHandle AutomationDiscover;
    FHttpRouteHandle AutomationRun;
    FHttpRouteHandle AutomationReport;
    FHttpRouteHandle Diagnostics;
    FHttpRouteHandle PerformanceSnapshot;
    FHttpRouteHandle ControlledCVar;
    FHttpRouteHandle RollbackCVar;
    FHttpRouteHandle WorldPartition;
    FHttpRouteHandle LevelInstance;
    FHttpRouteHandle Landscape;
    FHttpRouteHandle Scatter;
    FHttpRouteHandle DataAsset;
    FHttpRouteHandle Widget;
    FHttpRouteHandle InputMapping;
    FHttpRouteHandle BlueprintEdit;
    FHttpRouteHandle CppScaffold;
    FHttpRouteHandle DependencyGraph;
    FHttpRouteHandle GameplayScaffold;
    FHttpRouteHandle GasScaffold;
    FHttpRouteHandle MaterialGraph;
    FHttpRouteHandle ProceduralMesh;
    FHttpRouteHandle ExternalJob;
    FHttpRouteHandle Animation;
    FHttpRouteHandle AnimationValidate;
    FHttpRouteHandle SequenceEdit;
    FHttpRouteHandle Niagara;
    FHttpRouteHandle Audio;
    FHttpRouteHandle NetworkReplication;
    FHttpRouteHandle NetworkSessionPlan;
    FHttpRouteHandle PlatformProfile;
    FHttpRouteHandle PipelinePlan;
    FHttpRouteHandle BudgetAudit;
    FHttpRouteHandle ComplianceAudit;
    FHttpRouteHandle SourceControlStatus;
    FHttpRouteHandle CiPlan;
    FHttpRouteHandle CrashDiagnostics;
    FHttpRouteHandle PluginScaffold;
    FHttpRouteHandle EditorAdapters;
    FHttpRouteHandle AgentPlan;
    FHttpRouteHandle AgentTransition;
    FHttpRouteHandle SceneOrchestrate;
    FHttpRouteHandle ShowcaseCreate;
    FHttpRouteHandle ShowcaseEnhance;
    FHttpRouteHandle StylizedGrassRebuild;
    FHttpRouteHandle StylizedGrassStatus;
    FHttpRouteHandle CapabilityCoverage;
    FHttpRouteHandle Shutdown;
    TUniquePtr<FLogCapture> Capture;
    TUniquePtr<FAutomationTestBase> AutomationTest;
    FString LastScreenshotPath;
    bool bPluginSmokeRan = false;
    bool bPluginSmokePassed = false;
    TMap<FString, FString> ControlledCVarOriginals;
    TMap<FString, FString> AgentStates;
    TMap<FString, FString> AgentArtifacts;
};

void FUE5AIMcpModule::StartupModule()
{
    Impl = MakeUnique<FImpl>();
    Impl->Start();
}

void FUE5AIMcpModule::ShutdownModule()
{
    if (Impl) { Impl->Stop(); Impl.Reset(); }
}

IMPLEMENT_MODULE(FUE5AIMcpModule, UE5AIMcp)
