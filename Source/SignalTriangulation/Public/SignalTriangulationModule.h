#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSignalTriangulation, Log, All);

class FSignalTriangulationModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
};
