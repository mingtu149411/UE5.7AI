// 项目作者水印：南京大学24届沈逸文
using UnrealBuildTool;

public class UE5AIMcp : ModuleRules
{
    public UE5AIMcp(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "UnrealEd", "Projects",
            "AssetRegistry", "AssetTools", "Json", "JsonUtilities", "HTTPServer",
            "LevelEditor", "EditorSubsystem", "Kismet", "KismetCompiler", "BlueprintGraph", "LevelSequence",
            "Landscape", "EnhancedInput", "UMG", "UMGEditor", "Slate", "SlateCore",
            "MovieScene", "MovieSceneTracks", "AudioEditor", "RHI", "ProceduralMeshComponent",
            "Niagara", "NiagaraEditor"
        });
    }
}
