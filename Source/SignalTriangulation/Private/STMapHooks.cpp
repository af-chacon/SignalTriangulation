#include "STMapHooks.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Patching/BlueprintHookManager.h"
#include "Patching/BlueprintHookBlueprint.h"
#include "Patching/BlueprintHookingTypes.h"
#include "Patching/BlueprintHookTargetSpecifiers.h"
#include "Resources/FGItemDescriptor.h"
#include "STRepresentationTypes.h"
#include "SignalTriangulationModule.h"

namespace
{
	const TCHAR* FilterCategoriesWidgetPath = TEXT("/Game/FactoryGame/Interface/UI/Minimap/MapFilters/BPW_MapFilterCategories.BPW_MapFilterCategories_C");
}

FText USTMapHooks::HookGetCategoryName(ERepresentationType mRepresentationType, FText OriginalValue)
{
	const ERepresentationType Type = mRepresentationType;
	if (Type == STRepresentationTypes::Somersloop)
	{
		return NSLOCTEXT("SignalTriangulation", "Category_Somersloops", "Somersloops");
	}
	if (Type == STRepresentationTypes::MercerSphere)
	{
		return NSLOCTEXT("SignalTriangulation", "Category_MercerSpheres", "Mercer Spheres");
	}
	if (Type == STRepresentationTypes::HardDrive)
	{
		return NSLOCTEXT("SignalTriangulation", "Category_HardDrives", "Hard Drives");
	}
	return OriginalValue;
}

void USTMapHooks::RegisterHooks(UGameInstance* GameInstance)
{
	static bool bRegistered = false;
	if (bRegistered || !GameInstance || IsRunningDedicatedServer())
	{
		return;
	}
	bRegistered = true;

	UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, FilterCategoriesWidgetPath);
	if (!WidgetClass)
	{
		UE_LOG(LogSignalTriangulation, Warning, TEXT("Map hooks: filter categories widget class not found"));
		return;
	}
	UFunction* TargetFunction = WidgetClass->FindFunctionByName(TEXT("GetCategoryName"));
	UFunction* HookFunction = StaticClass()->FindFunctionByName(TEXT("HookGetCategoryName"));
	if (!TargetFunction || !HookFunction)
	{
		UE_LOG(LogSignalTriangulation, Warning, TEXT("Map hooks: functions unresolved (target=%p hook=%p)"), TargetFunction, HookFunction);
		return;
	}

	// Runtime-fabricated carrier: RegisterBlueprintHook only reads HookDescriptors and
	// MixinTargetClass off it for non-mixin hooks. Failures inside hook application are
	// logged by SML and leave the target function untouched.
	UHookBlueprintGeneratedClass* Carrier = NewObject<UHookBlueprintGeneratedClass>(GetTransientPackage(), TEXT("STMapHookCarrier"));
	if (!Carrier)
	{
		return;
	}

	FBlueprintHookDefinition Definition;
	Definition.TargetFunction = TargetFunction;
	Definition.HookFunction = HookFunction;
	Definition.Type = EBlueprintFunctionHookType::RedirectHook;
	Definition.InsertLocation = EBlueprintFunctionHookInsertLocation::ReplaceTarget;
	Definition.TargetSpecifier = NewObject<UBlueprintHookTargetSpecifier_ReturnValue>(Carrier);
	Definition.TargetSelectionMode = EBlueprintFunctionHookTargetSelectionMode::All;
	Carrier->HookDescriptors.Add(Definition);

	UBlueprintHookManager* Manager = GEngine->GetEngineSubsystem<UBlueprintHookManager>();
	if (!Manager)
	{
		return;
	}
	Manager->RegisterBlueprintHook(GameInstance, Carrier);
	UE_LOG(LogSignalTriangulation, Display, TEXT("Registered GetCategoryName hook for custom map filter categories"));
}
