#include "STArtifactMarker.h"

#include "FGActorRepresentationManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Resources/FGItemDescriptor.h"
#include "STRepresentationTypes.h"

void USTArtifactRepresentation::UpdateRepresentationCompassMaterial(UMaterialInstanceDynamic* compassMaterialInstance, APlayerController* ownerPlayerController) const
{
	Super::UpdateRepresentationCompassMaterial(compassMaterialInstance, ownerPlayerController);
	if (UTexture2D* Icon = GetRepresentationTexture())
	{
		compassMaterialInstance->SetTextureParameterValue(FName("Icon"), Icon);
	}
}

ASTArtifactMarker* USTArtifactRepresentation::GetMarkerActor() const
{
	return Cast<ASTArtifactMarker>(mRealActor);
}

ASTArtifactMarker::ASTArtifactMarker()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(0.1f);
}

void ASTArtifactMarker::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASTArtifactMarker, mLocation);
	DOREPLIFETIME(ASTArtifactMarker, mDescriptor);
	DOREPLIFETIME(ASTArtifactMarker, mRepresentationType);
}

void ASTArtifactMarker::InitMarker(const FVector& InLocation, TSubclassOf<UFGItemDescriptor> InDescriptor, ERepresentationType InRepresentationType)
{
	mLocation = InLocation;
	mDescriptor = InDescriptor;
	mRepresentationType = InRepresentationType;
}

void ASTArtifactMarker::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		AddAsRepresentation();
	}
}

void ASTArtifactMarker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		RemoveAsRepresentation();
	}
	Super::EndPlay(EndPlayReason);
}

bool ASTArtifactMarker::AddAsRepresentation()
{
	if (AFGActorRepresentationManager* Manager = AFGActorRepresentationManager::Get(GetWorld()))
	{
		return Manager->CreateAndAddNewRepresentation(this, false, USTArtifactRepresentation::StaticClass()) != nullptr;
	}
	return false;
}

bool ASTArtifactMarker::UpdateRepresentation()
{
	return true;
}

bool ASTArtifactMarker::RemoveAsRepresentation()
{
	if (AFGActorRepresentationManager* Manager = AFGActorRepresentationManager::Get(GetWorld()))
	{
		return Manager->RemoveRepresentationOfActor(this);
	}
	return false;
}

UMaterialInterface* ASTArtifactMarker::GetActorRepresentationCompassMaterial()
{
	// Per-artifact material instances with the Icon parameter baked at edit time - the map
	// widget re-derives dynamic instances from the parent chain, so only asset-baked parameter
	// values survive; runtime MIDs and UpdateRepresentationCompassMaterial are compass-only
	const TCHAR* Path = TEXT("/SignalTriangulation/MIC_ST_HardDrive.MIC_ST_HardDrive");
	if (mRepresentationType == STRepresentationTypes::Somersloop)
	{
		Path = TEXT("/SignalTriangulation/MIC_ST_Somersloop.MIC_ST_Somersloop");
	}
	else if (mRepresentationType == STRepresentationTypes::MercerSphere)
	{
		Path = TEXT("/SignalTriangulation/MIC_ST_MercerSphere.MIC_ST_MercerSphere");
	}
	return LoadObject<UMaterialInterface>(nullptr, Path);
}

UTexture2D* ASTArtifactMarker::GetActorRepresentationTexture()
{
	if (mDescriptor)
	{
		return UFGItemDescriptor::GetBigIcon(mDescriptor);
	}
	return nullptr;
}

FText ASTArtifactMarker::GetActorRepresentationText()
{
	if (mDescriptor)
	{
		return UFGItemDescriptor::GetItemName(mDescriptor);
	}
	return FText::GetEmpty();
}
