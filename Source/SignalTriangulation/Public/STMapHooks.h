#pragma once

#include "CoreMinimal.h"
#include "FGActorRepresentation.h"
#include "STMapHooks.generated.h"

/**
 * Blueprint-script hook payloads for the map filter UI. Hook functions are static and their
 * parameter names bind to target-function members/locals by name (SML hooking convention).
 */
UCLASS()
class SIGNALTRIANGULATION_API USTMapHooks : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * Redirect hook on BPW_MapFilterCategories::GetCategoryName's return value.
	 * mRepresentationType binds to the widget's member; OriginalValue is the vanilla result.
	 */
	UFUNCTION()
	static FText HookGetCategoryName(ERepresentationType mRepresentationType, FText OriginalValue);

	/** Registers the hooks. Safe to call multiple times; failures only log. */
	static void RegisterHooks(class UGameInstance* GameInstance);
};
