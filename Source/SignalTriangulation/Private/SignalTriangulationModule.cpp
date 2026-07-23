#include "SignalTriangulationModule.h"

DEFINE_LOG_CATEGORY(LogSignalTriangulation);

void FSignalTriangulationModule::StartupModule()
{
	UE_LOG(LogSignalTriangulation, Display, TEXT("SignalTriangulation module loaded"));
}

IMPLEMENT_MODULE(FSignalTriangulationModule, SignalTriangulation)
