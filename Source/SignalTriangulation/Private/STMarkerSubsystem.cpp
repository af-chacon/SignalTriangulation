#include "STMarkerSubsystem.h"

#include "Buildables/FGBuildableRadarTower.h"
#include "EngineUtils.h"
#include "FGSchematic.h"
#include "FGSchematicManager.h"
#include "FGScannableSubsystem.h"
#include "FGWorldScannableData.h"
#include "Resources/FGItemDescriptor.h"
#include "STArtifactMarker.h"
#include "STRepresentationTypes.h"
#include "SignalTriangulationModule.h"
#include "TimerManager.h"

namespace
{
	const TCHAR* SchematicPath = TEXT("/SignalTriangulation/Schematic_SignalTriangulation.Schematic_SignalTriangulation_C");
	const TCHAR* SomersloopClassPath = TEXT("/Game/FactoryGame/Prototype/WAT/BP_WAT1.BP_WAT1_C");
	const TCHAR* MercerSphereClassPath = TEXT("/Game/FactoryGame/Prototype/WAT/BP_WAT2.BP_WAT2_C");
	const TCHAR* SomersloopDescPath = TEXT("/Game/FactoryGame/Prototype/WAT/Desc_WAT1.Desc_WAT1_C");
	const TCHAR* MercerSphereDescPath = TEXT("/Game/FactoryGame/Prototype/WAT/Desc_WAT2.Desc_WAT2_C");
	const TCHAR* HardDriveDescPath = TEXT("/Game/FactoryGame/Resource/Environment/CrashSites/Desc_HardDrive.Desc_HardDrive_C");

	TAutoConsoleVariable<int32> CVarForceUnlock(
		TEXT("ST.ForceUnlock"), 0,
		TEXT("Signal Triangulation: treat the research as unlocked (for testing)"));
	TAutoConsoleVariable<int32> CVarShowSomersloops(
		TEXT("ST.ShowSomersloops"), 1,
		TEXT("Signal Triangulation: show Somersloop markers on the map"));
	TAutoConsoleVariable<int32> CVarShowMercerSpheres(
		TEXT("ST.ShowMercerSpheres"), 1,
		TEXT("Signal Triangulation: show Mercer Sphere markers on the map"));
	TAutoConsoleVariable<int32> CVarShowHardDrives(
		TEXT("ST.ShowHardDrives"), 1,
		TEXT("Signal Triangulation: show crash site markers on the map"));
	TAutoConsoleVariable<int32> CVarRequireRadarCoverage(
		TEXT("ST.RequireRadarCoverage"), 1,
		TEXT("Signal Triangulation: only reveal artifacts inside a powered radar tower's reveal radius (0 = whole map)"));
}

ASTMarkerSubsystem::ASTMarkerSubsystem()
{
	// Server authoritative; representations replicate to clients via the representation manager
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

void ASTMarkerSubsystem::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(mReconcileTimerHandle, this, &ASTMarkerSubsystem::Reconcile, 10.0f, true, 2.0f);
}

void ASTMarkerSubsystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(mReconcileTimerHandle);
	if (bDelegateBound)
	{
		if (AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld()))
		{
			SchematicManager->PurchasedSchematicDelegate.RemoveDynamic(this, &ASTMarkerSubsystem::OnSchematicPurchased);
		}
		bDelegateBound = false;
	}
	Super::EndPlay(EndPlayReason);
}

void ASTMarkerSubsystem::OnSchematicPurchased(TSubclassOf<UFGSchematic> PurchasedSchematic)
{
	if (mSchematic && PurchasedSchematic && PurchasedSchematic->IsChildOf(mSchematic))
	{
		Reconcile();
	}
}

bool ASTMarkerSubsystem::IsResearchUnlocked() const
{
	if (CVarForceUnlock.GetValueOnGameThread() != 0)
	{
		return true;
	}
	const AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld());
	return mSchematic && SchematicManager && SchematicManager->IsSchematicPurchased(mSchematic);
}

bool ASTMarkerSubsystem::ResolveClasses()
{
	if (mSchematic && mSomersloopClass && mMercerSphereClass && mSomersloopDesc && mMercerSphereDesc && mHardDriveDesc)
	{
		return true;
	}
	mSchematic = LoadClass<UFGSchematic>(nullptr, SchematicPath);
	mSomersloopClass = LoadClass<AActor>(nullptr, SomersloopClassPath);
	mMercerSphereClass = LoadClass<AActor>(nullptr, MercerSphereClassPath);
	mSomersloopDesc = LoadClass<UFGItemDescriptor>(nullptr, SomersloopDescPath);
	mMercerSphereDesc = LoadClass<UFGItemDescriptor>(nullptr, MercerSphereDescPath);
	mHardDriveDesc = LoadClass<UFGItemDescriptor>(nullptr, HardDriveDescPath);

	if (!mSchematic || !mSomersloopClass || !mMercerSphereClass || !mSomersloopDesc || !mMercerSphereDesc || !mHardDriveDesc)
	{
		UE_LOG(LogSignalTriangulation, Warning, TEXT("Class resolution incomplete (schematic=%d sloop=%d mercer=%d)"),
			mSchematic != nullptr, mSomersloopClass != nullptr, mMercerSphereClass != nullptr);
		return false;
	}
	return true;
}

void ASTMarkerSubsystem::Reconcile()
{
	if (!ResolveClasses())
	{
		return;
	}

	AFGScannableSubsystem* Scannables = AFGScannableSubsystem::Get(GetWorld());
	AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld());
	if (!Scannables || !SchematicManager)
	{
		return;
	}

	if (!bDelegateBound)
	{
		SchematicManager->PurchasedSchematicDelegate.AddDynamic(this, &ASTMarkerSubsystem::OnSchematicPurchased);
		bDelegateBound = true;
	}

	const bool bUnlocked = IsResearchUnlocked();
	const bool bShowSloops = bUnlocked && CVarShowSomersloops.GetValueOnGameThread() != 0;
	const bool bShowSpheres = bUnlocked && CVarShowMercerSpheres.GetValueOnGameThread() != 0;
	const bool bShowDrives = bUnlocked && CVarShowHardDrives.GetValueOnGameThread() != 0;

	// Triangulation only works inside the radar network: collect powered towers so artifacts
	// outside every reveal radius stay hidden (and get pruned if a tower loses power)
	TArray<const AFGBuildableRadarTower*> PoweredTowers;
	const bool bRequireCoverage = CVarRequireRadarCoverage.GetValueOnGameThread() != 0;
	if (bRequireCoverage && bUnlocked)
	{
		for (TActorIterator<AFGBuildableRadarTower> It(GetWorld()); It; ++It)
		{
			if (IsValid(*It) && It->HasPower())
			{
				PoweredTowers.Add(*It);
			}
		}
	}
	auto IsInCoverage = [bRequireCoverage, &PoweredTowers](const FVector& Location)
	{
		if (!bRequireCoverage)
		{
			return true;
		}
		for (const AFGBuildableRadarTower* Tower : PoweredTowers)
		{
			if (Tower->IsLocationWithinRevealRadius(Location))
			{
				return true;
			}
		}
		return false;
	};

	TSet<FGuid> WantedPickups;
	TSet<FGuid> WantedDropPods;

	for (const FWorldScannableData& Pickup : Scannables->GetAvailableItemPickups())
	{
		if (!Pickup.ActorClass || !Scannables->DoesPickupExist(Pickup.ActorGuid) || !IsInCoverage(Pickup.ActorLocation))
		{
			continue;
		}
		TSubclassOf<UFGItemDescriptor> Descriptor = nullptr;
		ERepresentationType RepType = ERepresentationType::RT_Default;
		if (bShowSloops && Pickup.ActorClass->IsChildOf(mSomersloopClass))
		{
			Descriptor = mSomersloopDesc;
			RepType = STRepresentationTypes::Somersloop;
		}
		else if (bShowSpheres && Pickup.ActorClass->IsChildOf(mMercerSphereClass))
		{
			Descriptor = mMercerSphereDesc;
			RepType = STRepresentationTypes::MercerSphere;
		}
		if (!Descriptor)
		{
			continue;
		}
		WantedPickups.Add(Pickup.ActorGuid);
		if (!mPickupMarkers.Contains(Pickup.ActorGuid))
		{
			if (ASTArtifactMarker* Marker = SpawnMarker(Pickup.ActorLocation, Descriptor, RepType))
			{
				mPickupMarkers.Add(Pickup.ActorGuid, Marker);
			}
		}
	}

	if (bShowDrives)
	{
		for (const FWorldScannableData& DropPod : Scannables->GetAvailableDropPods())
		{
			if (Scannables->HasDropPodBeenLooted(DropPod.ActorGuid) || !IsInCoverage(DropPod.ActorLocation))
			{
				continue;
			}
			WantedDropPods.Add(DropPod.ActorGuid);
			if (!mDropPodMarkers.Contains(DropPod.ActorGuid))
			{
				if (ASTArtifactMarker* Marker = SpawnMarker(DropPod.ActorLocation, mHardDriveDesc, STRepresentationTypes::HardDrive))
				{
					mDropPodMarkers.Add(DropPod.ActorGuid, Marker);
				}
			}
		}
	}

	for (auto It = mPickupMarkers.CreateIterator(); It; ++It)
	{
		if (!WantedPickups.Contains(It.Key()))
		{
			if (IsValid(It.Value()))
			{
				It.Value()->Destroy();
			}
			It.RemoveCurrent();
		}
	}
	for (auto It = mDropPodMarkers.CreateIterator(); It; ++It)
	{
		if (!WantedDropPods.Contains(It.Key()))
		{
			if (IsValid(It.Value()))
			{
				It.Value()->Destroy();
			}
			It.RemoveCurrent();
		}
	}
}

ASTArtifactMarker* ASTMarkerSubsystem::SpawnMarker(const FVector& Location, TSubclassOf<UFGItemDescriptor> Descriptor, ERepresentationType RepType)
{
	FTransform Transform(Location);
	ASTArtifactMarker* Marker = GetWorld()->SpawnActorDeferred<ASTArtifactMarker>(ASTArtifactMarker::StaticClass(), Transform);
	if (Marker)
	{
		Marker->InitMarker(Location, Descriptor, RepType);
		Marker->FinishSpawning(Transform);
	}
	return Marker;
}
