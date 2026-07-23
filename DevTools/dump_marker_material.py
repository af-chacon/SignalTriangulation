import unreal

for path in (
    "/Game/FactoryGame/Interface/UI/Minimap/IconMaterials/MMI_CompassIcon_Marker.MMI_CompassIcon_Marker",
    "/Game/FactoryGame/Interface/UI/Minimap/IconMaterials/MI_CompassIcon_Hub.MI_CompassIcon_Hub",
):
    mi = unreal.load_asset(path.split(".")[0])
    if not mi:
        unreal.log("[dump] MISSING " + path)
        continue
    unreal.log("[dump] === " + path)
    for prop in ("vector_parameter_values", "scalar_parameter_values", "texture_parameter_values"):
        try:
            vals = mi.get_editor_property(prop)
            for v in vals:
                info = v.get_editor_property("parameter_info")
                unreal.log("[dump] {}: {} = {}".format(prop, info.get_editor_property("name"), v.get_editor_property("parameter_value")))
        except Exception as e:
            unreal.log("[dump] {} error {}".format(prop, e))
master = unreal.load_asset("/Game/FactoryGame/Interface/UI/Minimap/IconMaterials/MM_UI_CompassIcon")
mel = unreal.MaterialEditingLibrary
unreal.log("[dump] master vector params: {}".format(list(mel.get_vector_parameter_names(master))))
unreal.log("[dump] master scalar params: {}".format(list(mel.get_scalar_parameter_names(master))))
