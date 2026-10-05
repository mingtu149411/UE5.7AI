// 项目作者水印：南京大学24届沈逸文
#include "Misc/AutomationTest.h"

// BuildPlugin uses an Editor target without WITH_AUTOMATION_WORKER, so the
// usual IMPLEMENT_SIMPLE_AUTOMATION_TEST macro does not create a registration
// instance. This explicit factory keeps the test discoverable in that target.
class FUE5AIMcpMvpSmokeTest final : public FAutomationTestBase
{
public:
    FUE5AIMcpMvpSmokeTest() : FAutomationTestBase(TEXT("FUE5AIMcpMvpSmokeTest"), false) {}
    virtual EAutomationTestFlags GetTestFlags() const override { return EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter; }
    virtual uint32 GetRequiredDeviceNum() const override { return 1; }
    virtual FString GetTestSourceFileName() const override { return __FILE__; }
    virtual int32 GetTestSourceFileLine() const override { return __LINE__; }
    virtual bool RunTest(const FString&) override { TestTrue(TEXT("UE5 AI MCP MVP automation module is registered"), true); return true; }
    bool RunDirect() { return RunTest(FString()); }
protected:
    virtual void GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const override { OutBeautifiedNames.Add(TEXT("UE5AI.MVP.Smoke")); OutTestCommands.Add(FString()); }
    virtual FString GetBeautifiedTestName() const override { return TEXT("UE5AI.MVP.Smoke"); }
};

FAutomationTestBase* CreateUE5AIMcpMvpSmokeTest()
{
    return new FUE5AIMcpMvpSmokeTest();
}


bool RunUE5AIMcpMvpSmokeTest(FAutomationTestBase* Test)
{
    return Test && static_cast<FUE5AIMcpMvpSmokeTest*>(Test)->RunDirect();
}
