#pragma once

#include "CoreMinimal.h"
#include "FGActorRepresentation.h"

/**
 * Custom ERepresentationType values registered at runtime, one per artifact kind, so each
 * gets its own filter row in the map UI. Values are appended after the last vanilla entry.
 */
namespace STRepresentationTypes
{
	extern ERepresentationType Somersloop;
	extern ERepresentationType MercerSphere;
	extern ERepresentationType HardDrive;

	/** Expands the ERepresentationType enum. Must run once, before any marker spawns. */
	void ExpandEnum();
}
