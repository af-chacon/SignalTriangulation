import unreal

PATHS = [
    "/Game/FactoryGame/Schematics/Research/Sulfur_RS/Research_Sulfur_TurboFuel.Research_Sulfur_TurboFuel_C",
    "/Game/FactoryGame/Schematics/Research/Sulfur_RS/Research_Sulfur_RocketFuel.Research_Sulfur_RocketFuel_C",
    "/Game/FactoryGame/Schematics/Research/Sulfur_RS/Research_Sulfur_IonizedFuel.Research_Sulfur_IonizedFuel_C",
    "/Game/FactoryGame/Schematics/Research/Sulfur_RS/Research_Sulfur_compactedCoal.Research_Sulfur_compactedCoal_C",
    "/Game/FactoryGame/Schematics/Research/Quartz_RS/Research_Quartz_4.Research_Quartz_4_C",
    "/Game/FactoryGame/Schematics/Research/AlienTech_RS/Research_Alien_Somersloop.Research_Alien_Somersloop_C",
    "/Game/FactoryGame/Schematics/Research/AlienTech_RS/Research_Alien_MercerSphere.Research_Alien_MercerSphere_C",
]

for path in PATHS:
    cls = unreal.load_object(None, path)
    if not cls:
        unreal.log("[cost] MISSING " + path)
        continue
    cdo = unreal.get_default_object(cls)
    name = cdo.get_editor_property("mDisplayName")
    cost = cdo.get_editor_property("mCost")
    items = []
    for ia in cost:
        ic = ia.get_editor_property("ItemClass")
        items.append("{}x {}".format(ia.get_editor_property("Amount"), ic.get_name() if ic else "?"))
    unreal.log("[cost] {} ({}): {}".format(name, path.split("/")[-1].split(".")[0], ", ".join(items) or "FREE"))
