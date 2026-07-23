#include "STRepresentationTypes.h"

#include "SignalTriangulationModule.h"

namespace STRepresentationTypes
{
	ERepresentationType Somersloop = ERepresentationType::RT_Default;
	ERepresentationType MercerSphere = ERepresentationType::RT_Default;
	ERepresentationType HardDrive = ERepresentationType::RT_Default;

	void ExpandEnum()
	{
		// Never mutate the engine enum inside the editor process - editor serialization could
		// bake the transient values into assets. Markers degrade to RT_Default ("Other") in PIE.
		if (GIsEditor)
		{
			return;
		}

		static bool bExpanded = false;
		if (bExpanded)
		{
			return;
		}
		bExpanded = true;

		UEnum* Enum = StaticEnum<ERepresentationType>();
		int32 NumEnums = Enum->NumEnums();
		const bool bHasExistingMax = Enum->ContainsExistingMax();
		if (bHasExistingMax)
		{
			NumEnums--;
		}

		TArray<TPair<FName, int64>> AllEnums;
		int64 LargestValue = -1;
		for (int32 i = 0; i < NumEnums; ++i)
		{
			const int64 Value = Enum->GetValueByIndex(i);
			LargestValue = FMath::Max(Value, LargestValue);
			AllEnums.Emplace(Enum->GetNameByIndex(i), Value);
		}

		int64 NextValue = LargestValue + 1;
		Somersloop = static_cast<ERepresentationType>(NextValue);
		AllEnums.Emplace(FName("ERepresentationType::RT_ST_Somersloop"), NextValue++);
		MercerSphere = static_cast<ERepresentationType>(NextValue);
		AllEnums.Emplace(FName("ERepresentationType::RT_ST_MercerSphere"), NextValue++);
		HardDrive = static_cast<ERepresentationType>(NextValue);
		AllEnums.Emplace(FName("ERepresentationType::RT_ST_HardDrive"), NextValue++);

		Enum->SetEnums(AllEnums, Enum->GetCppForm(), EEnumFlags::None, bHasExistingMax);

		UE_LOG(LogSignalTriangulation, Display, TEXT("Expanded ERepresentationType: Somersloop=%d MercerSphere=%d HardDrive=%d"),
			static_cast<int32>(Somersloop), static_cast<int32>(MercerSphere), static_cast<int32>(HardDrive));
	}
}
