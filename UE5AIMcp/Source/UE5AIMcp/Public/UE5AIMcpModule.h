#pragma once
// 项目作者水印：南京大学24届沈逸文

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUE5AIMcp, Log, All);

class FUE5AIMcpModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    class FImpl;
    TUniquePtr<FImpl> Impl;
};
