#include "STTreeInjector.h"

#include "FGResearchTree.h"
#include "FGResearchTreeNode.h"
#include "FGSchematic.h"
#include "SignalTriangulationModule.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* QuartzTreePath = TEXT("/Game/FactoryGame/Schematics/Research/BPD_ResearchTree_Quartz.BPD_ResearchTree_Quartz_C");
	const TCHAR* NodeBPPath = TEXT("/Game/FactoryGame/Schematics/Research/BPD_ResearchTreeNode.BPD_ResearchTreeNode_C");
	const TCHAR* RadarSchematicPath = TEXT("/Game/FactoryGame/Schematics/Research/Quartz_RS/Research_Quartz_4.Research_Quartz_4_C");
	const TCHAR* OurSchematicPath = TEXT("/SignalTriangulation/Schematic_SignalTriangulation.Schematic_SignalTriangulation_C");

	// UserDefinedStruct members carry GUID-mangled names ("Schematic_27_<GUID>"), so match by prefix
	FProperty* FindByPrefix(const UStruct* Struct, const TCHAR* Prefix)
	{
		for (FProperty* Prop = Struct->PropertyLink; Prop; Prop = Prop->PropertyLinkNext)
		{
			if (Prop->GetName().StartsWith(Prefix))
			{
				return Prop;
			}
		}
		return nullptr;
	}

	// Coordinate fields are ints in the cooked game but may be floats in editor data - handle both
	double GetNumeric(const FNumericProperty* Prop, const void* ValuePtr)
	{
		return Prop->IsFloatingPoint()
			? Prop->GetFloatingPointPropertyValue(ValuePtr)
			: static_cast<double>(Prop->GetSignedIntPropertyValue(ValuePtr));
	}

	void SetNumeric(FNumericProperty* Prop, void* ValuePtr, double Value)
	{
		if (Prop->IsFloatingPoint())
		{
			Prop->SetFloatingPointPropertyValue(ValuePtr, Value);
		}
		else
		{
			Prop->SetIntPropertyValue(ValuePtr, static_cast<int64>(FMath::RoundToInt(Value)));
		}
	}

	bool GetStructVector2(const FStructProperty* CoordProp, const void* CoordPtr, double& OutX, double& OutY)
	{
		const FNumericProperty* XProp = CastField<FNumericProperty>(FindByPrefix(CoordProp->Struct, TEXT("X_")));
		const FNumericProperty* YProp = CastField<FNumericProperty>(FindByPrefix(CoordProp->Struct, TEXT("Y_")));
		if (!XProp || !YProp)
		{
			return false;
		}
		OutX = GetNumeric(XProp, XProp->ContainerPtrToValuePtr<void>(CoordPtr));
		OutY = GetNumeric(YProp, YProp->ContainerPtrToValuePtr<void>(CoordPtr));
		return true;
	}

	bool SetStructVector2(const FStructProperty* CoordProp, void* CoordPtr, double X, double Y)
	{
		FNumericProperty* XProp = CastField<FNumericProperty>(FindByPrefix(CoordProp->Struct, TEXT("X_")));
		FNumericProperty* YProp = CastField<FNumericProperty>(FindByPrefix(CoordProp->Struct, TEXT("Y_")));
		if (!XProp || !YProp)
		{
			return false;
		}
		SetNumeric(XProp, XProp->ContainerPtrToValuePtr<void>(CoordPtr), X);
		SetNumeric(YProp, YProp->ContainerPtrToValuePtr<void>(CoordPtr), Y);
		return true;
	}

	struct FNodeDataAccess
	{
		FStructProperty* NodeDataProp = nullptr;
		FClassProperty* SchematicProp = nullptr;
		FStructProperty* CoordinatesProp = nullptr;
		FArrayProperty* ParentsProp = nullptr;
		FArrayProperty* UnhiddenByProp = nullptr;
		FMapProperty* ChildrenAndRoadsProp = nullptr;

		bool Resolve(const UClass* NodeClass)
		{
			NodeDataProp = CastField<FStructProperty>(NodeClass->FindPropertyByName(TEXT("mNodeDataStruct")));
			if (!NodeDataProp)
			{
				return false;
			}
			const UStruct* Data = NodeDataProp->Struct;
			SchematicProp = CastField<FClassProperty>(FindByPrefix(Data, TEXT("Schematic_")));
			CoordinatesProp = CastField<FStructProperty>(FindByPrefix(Data, TEXT("Coordinates_")));
			ParentsProp = CastField<FArrayProperty>(FindByPrefix(Data, TEXT("Parents_")));
			UnhiddenByProp = CastField<FArrayProperty>(FindByPrefix(Data, TEXT("UnhiddenBy_")));
			ChildrenAndRoadsProp = CastField<FMapProperty>(FindByPrefix(Data, TEXT("ChildrenAndRoads_")));
			return SchematicProp && CoordinatesProp && ParentsProp && UnhiddenByProp && ChildrenAndRoadsProp;
		}

		void* DataPtr(UFGResearchTreeNode* Node) const
		{
			return NodeDataProp->ContainerPtrToValuePtr<void>(Node);
		}

		UClass* GetSchematic(UFGResearchTreeNode* Node) const
		{
			return Cast<UClass>(SchematicProp->GetObjectPropertyValue(SchematicProp->ContainerPtrToValuePtr<void>(DataPtr(Node))));
		}

		bool GetCoordinates(UFGResearchTreeNode* Node, double& OutX, double& OutY) const
		{
			return GetStructVector2(CoordinatesProp, CoordinatesProp->ContainerPtrToValuePtr<void>(DataPtr(Node)), OutX, OutY);
		}
	};

	// Appends one coordinate entry to a Parents/UnhiddenBy-style array of coordinate structs
	bool AppendCoordinate(const FArrayProperty* ArrayProp, void* ArrayPtr, double X, double Y)
	{
		const FStructProperty* InnerStruct = CastField<FStructProperty>(ArrayProp->Inner);
		if (!InnerStruct)
		{
			return false;
		}
		FScriptArrayHelper Helper(ArrayProp, ArrayPtr);
		const int32 Index = Helper.AddValue();
		return SetStructVector2(InnerStruct, Helper.GetRawPtr(Index), X, Y);
	}

	// Adds ChildrenAndRoads[childCoord] = {Points: [childCoord]} on the parent node so the MAM UI draws a connecting line
	bool AddRoadToChild(const FMapProperty* MapProp, void* MapPtr, double ChildX, double ChildY)
	{
		const FStructProperty* KeyStruct = CastField<FStructProperty>(MapProp->KeyProp);
		const FStructProperty* ValueStruct = CastField<FStructProperty>(MapProp->ValueProp);
		if (!KeyStruct || !ValueStruct)
		{
			return false;
		}
		const FArrayProperty* PointsProp = CastField<FArrayProperty>(FindByPrefix(ValueStruct->Struct, TEXT("Points_")));
		if (!PointsProp)
		{
			return false;
		}

		FScriptMapHelper Helper(MapProp, MapPtr);
		const int32 Index = Helper.AddDefaultValue_Invalid_NeedsRehash();
		if (!SetStructVector2(KeyStruct, Helper.GetKeyPtr(Index), ChildX, ChildY))
		{
			return false;
		}
		const bool bOk = AppendCoordinate(PointsProp, PointsProp->ContainerPtrToValuePtr<void>(Helper.GetValuePtr(Index)), ChildX, ChildY);
		Helper.Rehash();
		return bOk;
	}
}

namespace STTreeInjector
{
	void InjectIntoQuartzTree()
	{
		UClass* TreeClass = LoadClass<UFGResearchTree>(nullptr, QuartzTreePath);
		UClass* NodeClass = LoadClass<UFGResearchTreeNode>(nullptr, NodeBPPath);
		UClass* RadarSchematic = LoadClass<UFGSchematic>(nullptr, RadarSchematicPath);
		UClass* OurSchematic = LoadClass<UFGSchematic>(nullptr, OurSchematicPath);
		if (!TreeClass || !NodeClass || !RadarSchematic || !OurSchematic)
		{
			UE_LOG(LogSignalTriangulation, Error, TEXT("Tree injection: failed to load classes (tree=%p node=%p radar=%p ours=%p)"),
				TreeClass, NodeClass, RadarSchematic, OurSchematic);
			return;
		}

		FNodeDataAccess Access;
		if (!Access.Resolve(NodeClass))
		{
			UE_LOG(LogSignalTriangulation, Error, TEXT("Tree injection: failed to resolve node data properties"));
			return;
		}

		TArray<UFGResearchTreeNode*> Nodes = UFGResearchTree::GetNodes(TreeClass);

		UFGResearchTreeNode* RadarNode = nullptr;
		TSet<FIntPoint> OccupiedSlots;
		for (UFGResearchTreeNode* Node : Nodes)
		{
			if (!Node)
			{
				continue;
			}
			UClass* NodeSchematic = Access.GetSchematic(Node);
			if (NodeSchematic == OurSchematic)
			{
				return; // already injected
			}
			double X = 0, Y = 0;
			if (Access.GetCoordinates(Node, X, Y))
			{
				OccupiedSlots.Add(FIntPoint(FMath::RoundToInt(X), FMath::RoundToInt(Y)));
			}
			if (NodeSchematic == RadarSchematic)
			{
				RadarNode = Node;
			}
		}

		if (!RadarNode)
		{
			UE_LOG(LogSignalTriangulation, Error, TEXT("Tree injection: radar node not found in Quartz tree"));
			return;
		}

		double RadarX = 0, RadarY = 0;
		Access.GetCoordinates(RadarNode, RadarX, RadarY);

		// The tree grows downward; prefer the slot directly below the radar node
		const FIntPoint Radar(FMath::RoundToInt(RadarX), FMath::RoundToInt(RadarY));
		const FIntPoint Candidates[] = {
			Radar + FIntPoint(0, 1), Radar + FIntPoint(1, 1), Radar + FIntPoint(-1, 1),
			Radar + FIntPoint(1, 0), Radar + FIntPoint(0, 2), Radar + FIntPoint(1, 2),
		};
		FIntPoint Slot = Candidates[0];
		for (const FIntPoint& Candidate : Candidates)
		{
			if (!OccupiedSlots.Contains(Candidate))
			{
				Slot = Candidate;
				break;
			}
		}

		UObject* TreeCDO = TreeClass->GetDefaultObject();
		UFGResearchTreeNode* NewNode = NewObject<UFGResearchTreeNode>(TreeCDO, NodeClass);
		void* Data = Access.DataPtr(NewNode);

		Access.SchematicProp->SetObjectPropertyValue(Access.SchematicProp->ContainerPtrToValuePtr<void>(Data), OurSchematic);
		SetStructVector2(Access.CoordinatesProp, Access.CoordinatesProp->ContainerPtrToValuePtr<void>(Data), Slot.X, Slot.Y);
		AppendCoordinate(Access.ParentsProp, Access.ParentsProp->ContainerPtrToValuePtr<void>(Data), RadarX, RadarY);
		AppendCoordinate(Access.UnhiddenByProp, Access.UnhiddenByProp->ContainerPtrToValuePtr<void>(Data), RadarX, RadarY);

		// Draw the connecting line from the radar node down to ours
		void* RadarData = Access.DataPtr(RadarNode);
		if (!AddRoadToChild(Access.ChildrenAndRoadsProp, Access.ChildrenAndRoadsProp->ContainerPtrToValuePtr<void>(RadarData), Slot.X, Slot.Y))
		{
			UE_LOG(LogSignalTriangulation, Warning, TEXT("Tree injection: could not add road entry; node will show without a connecting line"));
		}

		Nodes.Add(NewNode);
		UFGResearchTree::SetNodes(TreeClass, Nodes);

		UE_LOG(LogSignalTriangulation, Display, TEXT("Injected Signal Triangulation node into Quartz tree at (%d,%d), parent radar node at (%.0f,%.0f)"),
			Slot.X, Slot.Y, RadarX, RadarY);
	}
}
