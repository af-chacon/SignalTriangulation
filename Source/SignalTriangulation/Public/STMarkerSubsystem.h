#pragma once

#include "CoreMinimal.h"
#include "FGActorRepresentation.h"
#include "Subsystem/ModSubsystem.h"
#include "STMarkerSubsystem.generated.h"

class UFGSchematic;
class UFGItemDescriptor;
class ASTArtifactMarker;

/**
 * Server-side subsystem that keeps map markers in sync with every uncollected Somersloop,
 * Mercer Sphere, and un-looted crash site once the Signal Triangulation research has been
 * purchased in the MAM. Per-type visibility toggles: ST.ShowSomersloops, ST.ShowMercerSpheres,
 * ST.ShowHardDrives (console variables, applied on the next reconcile pass).
 */
UCLASS()
class SIGNALTRIANGULATION_API ASTMarkerSubsystem : public AModSubsystem
{
	GENERATED_BODY()
public:
	ASTMarkerSubsystem();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnSchematicPurchased(TSubclassOf<UFGSchematic> PurchasedSchematic);

	bool IsResearchUnlocked() const;
	bool ResolveClasses();

	/** Creates/destroys markers to match research state, per-type toggles, and collection state. Idempotent. */
	void Reconcile();

	ASTArtifactMarker* SpawnMarker(const FVector& Location, TSubclassOf<UFGItemDescriptor> Descriptor, ERepresentationType RepType);

	FTimerHandle mReconcileTimerHandle;
	bool bDelegateBound = false;

	UPROPERTY()
	TSubclassOf<UFGSchematic> mSchematic;
	UPROPERTY()
	UClass* mSomersloopClass = nullptr;
	UPROPERTY()
	UClass* mMercerSphereClass = nullptr;
	UPROPERTY()
	TSubclassOf<UFGItemDescriptor> mSomersloopDesc;
	UPROPERTY()
	TSubclassOf<UFGItemDescriptor> mMercerSphereDesc;
	UPROPERTY()
	TSubclassOf<UFGItemDescriptor> mHardDriveDesc;

	/** Pickup GUID -> marker, for somersloop/mercer pickups. */
	UPROPERTY()
	TMap<FGuid, TObjectPtr<ASTArtifactMarker>> mPickupMarkers;

	/** Drop pod GUID -> marker. */
	UPROPERTY()
	TMap<FGuid, TObjectPtr<ASTArtifactMarker>> mDropPodMarkers;
};
