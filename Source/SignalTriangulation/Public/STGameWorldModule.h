#pragma once

#include "CoreMinimal.h"
#include "Module/GameWorldModule.h"
#include "STGameWorldModule.generated.h"

/** Root game world module: registers the MAM research tree/schematic and the marker subsystem. */
UCLASS()
class SIGNALTRIANGULATION_API USTGameWorldModule : public UGameWorldModule
{
	GENERATED_BODY()
public:
	USTGameWorldModule();

	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
};
