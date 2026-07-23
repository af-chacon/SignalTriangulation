#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "FGActorRepresentation.h"
#include "FGActorRepresentationInterface.h"
#include "STArtifactMarker.generated.h"

class UFGItemDescriptor;

/**
 * Representation that stamps the artifact icon onto the material instance the map/compass
 * widgets create from our base compass material.
 */
UCLASS()
class SIGNALTRIANGULATION_API USTArtifactRepresentation : public UFGActorRepresentation
{
	GENERATED_BODY()
public:
	virtual void UpdateRepresentationCompassMaterial(UMaterialInstanceDynamic* compassMaterialInstance, APlayerController* ownerPlayerController) const override;

	/** The marker actor behind this representation, if any. */
	class ASTArtifactMarker* GetMarkerActor() const;
};

/** Map-only marker for a single triangulated artifact (Somersloop, Mercer Sphere, or crash site hard drive). */
UCLASS()
class SIGNALTRIANGULATION_API ASTArtifactMarker : public AInfo, public IFGActorRepresentationInterface
{
	GENERATED_BODY()
public:
	ASTArtifactMarker();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Configure before the actor's BeginPlay (i.e. pass via deferred spawn). */
	void InitMarker(const FVector& InLocation, TSubclassOf<UFGItemDescriptor> InDescriptor, ERepresentationType InRepresentationType);

	// Begin IFGActorRepresentationInterface
	virtual bool AddAsRepresentation() override;
	virtual bool UpdateRepresentation() override;
	virtual bool RemoveAsRepresentation() override;
	virtual bool IsActorStatic() override { return true; }
	virtual FVector GetRealActorLocation() override { return mLocation; }
	virtual FRotator GetRealActorRotation() override { return FRotator::ZeroRotator; }
	virtual class UTexture2D* GetActorRepresentationTexture() override;
	virtual UMaterialInterface* GetActorRepresentationCompassMaterial() override;
	virtual FText GetActorRepresentationText() override;
	virtual void SetActorRepresentationText(const FText& newText) override {}
	virtual FLinearColor GetActorRepresentationColor() override { return FLinearColor::White; }
	virtual void SetActorRepresentationColor(FLinearColor newColor) override {}
	virtual ERepresentationType GetActorRepresentationType() override { return mRepresentationType; }
	virtual bool GetActorShouldShowInCompass() override { return false; }
	virtual bool GetActorShouldShowOnMap() override { return true; }
	virtual EFogOfWarRevealType GetActorFogOfWarRevealType() override { return EFogOfWarRevealType::FOWRT_None; }
	virtual float GetActorFogOfWarRevealRadius() override { return 0.0f; }
	virtual ECompassViewDistance GetActorCompassViewDistance() override { return ECompassViewDistance::CVD_Always; }
	virtual void SetActorCompassViewDistance(ECompassViewDistance compassViewDistance) override {}
	// End IFGActorRepresentationInterface

private:
	UPROPERTY(Replicated)
	FVector mLocation;

	UPROPERTY(Replicated)
	TSubclassOf<UFGItemDescriptor> mDescriptor;

	UPROPERTY(Replicated)
	ERepresentationType mRepresentationType = ERepresentationType::RT_Default;
};
