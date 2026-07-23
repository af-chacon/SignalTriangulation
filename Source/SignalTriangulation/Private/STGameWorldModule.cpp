#include "STGameWorldModule.h"

#include "FGSchematic.h"
#include "STMapHooks.h"
#include "STMarkerSubsystem.h"
#include "STRepresentationTypes.h"
#include "STTreeInjector.h"
#include "SignalTriangulationModule.h"
#include "Subsystem/SubsystemActorManager.h"

USTGameWorldModule::USTGameWorldModule()
{
	bRootModule = true;
}

void USTGameWorldModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
	if (Phase == ELifecyclePhase::CONSTRUCTION)
	{
		// Register per-artifact map filter categories before any markers exist
		STRepresentationTypes::ExpandEnum();

		// Content assets are authored separately; tolerate their absence so the C++ mod still loads
		if (UClass* Schematic = LoadClass<UFGSchematic>(nullptr, TEXT("/SignalTriangulation/Schematic_SignalTriangulation.Schematic_SignalTriangulation_C")))
		{
			mSchematics.AddUnique(Schematic);
		}
		else
		{
			UE_LOG(LogSignalTriangulation, Warning, TEXT("Schematic_SignalTriangulation asset missing - MAM research not registered"));
		}

	}

	Super::DispatchLifecycleEvent(Phase);

	if (Phase == ELifecyclePhase::POST_INITIALIZATION)
	{
		// Graft our node onto the vanilla Quartz tree (runs on server and clients; the MAM UI reads the tree CDO)
		STTreeInjector::InjectIntoQuartzTree();

		// Teach the map filter sidebar the names of our custom categories
		USTMapHooks::RegisterHooks(GetWorld()->GetGameInstance());
	}

	if (Phase == ELifecyclePhase::INITIALIZATION)
	{
		if (USubsystemActorManager* SubsystemManager = GetWorld()->GetSubsystem<USubsystemActorManager>())
		{
			SubsystemManager->RegisterSubsystemActor(ASTMarkerSubsystem::StaticClass());
		}
	}
}
