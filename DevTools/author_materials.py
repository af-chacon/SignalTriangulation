"""Author per-artifact compass-icon material instances (parent MM_UI_CompassIcon, Icon param baked).

Run headlessly:
  UnrealEditor-Cmd FactoryGame.uproject -run=pythonscript -script=<this file> -nullrhi -unattended
"""
import unreal

MOD_ROOT = "/SignalTriangulation"
MASTER_MATERIAL = "/Game/FactoryGame/Interface/UI/Minimap/IconMaterials/MM_UI_CompassIcon.MM_UI_CompassIcon"

# (asset name, descriptor class whose big icon becomes the Icon parameter)
INSTANCES = [
    ("MIC_ST_Somersloop", "/Game/FactoryGame/Prototype/WAT/Desc_WAT1.Desc_WAT1_C"),
    ("MIC_ST_MercerSphere", "/Game/FactoryGame/Prototype/WAT/Desc_WAT2.Desc_WAT2_C"),
    ("MIC_ST_HardDrive", "/Game/FactoryGame/Resource/Environment/CrashSites/Desc_HardDrive.Desc_HardDrive_C"),
]

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary

unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([MOD_ROOT], force_rescan=True)


def log(msg):
    unreal.log("[SignalTriangulation] " + msg)


def get_icon_texture(descriptor_path):
    cls = unreal.load_object(None, descriptor_path)
    if not cls:
        raise RuntimeError("Descriptor not found: " + descriptor_path)
    cdo = unreal.get_default_object(cls)
    for prop in ("mPersistentBigIcon", "mSmallIcon"):
        try:
            tex = cdo.get_editor_property(prop)
            if tex:
                log("{} {} -> {} ({})".format(descriptor_path, prop, tex, type(tex).__name__))
                if isinstance(tex, unreal.SoftObjectPath):
                    tex = unreal.load_asset(str(tex.export_text()).split("'")[0])
                if isinstance(tex, unreal.Texture):
                    return tex
        except Exception:
            pass
    raise RuntimeError("No icon texture on " + descriptor_path)


master = unreal.load_asset(MASTER_MATERIAL)
if not master:
    raise RuntimeError("Master material not found")
log("Master texture params: {}".format(list(mel.get_texture_parameter_names(master))))

for name, descriptor_path in INSTANCES:
    asset_path = "{}/{}".format(MOD_ROOT, name)
    mic = unreal.load_asset(asset_path)
    if not mic:
        factory = unreal.MaterialInstanceConstantFactoryNew()
        mic = asset_tools.create_asset(name, MOD_ROOT, unreal.MaterialInstanceConstant, factory)
        if not mic:
            raise RuntimeError("Failed to create " + asset_path)
    mic.set_editor_property("parent", master)
    tex = get_icon_texture(descriptor_path)
    pv = unreal.TextureParameterValue()
    pv.set_editor_property("parameter_info", unreal.MaterialParameterInfo(name="Icon"))
    pv.set_editor_property("parameter_value", tex)
    mic.set_editor_property("texture_parameter_values", [pv])
    # White diamond background, like the vanilla marker instance (master default is orange)
    cv = unreal.VectorParameterValue()
    cv.set_editor_property("parameter_info", unreal.MaterialParameterInfo(name="ColorPrimary"))
    cv.set_editor_property("parameter_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    mic.set_editor_property("vector_parameter_values", [cv])
    mel.update_material_instance(mic)
    if not eal.save_loaded_asset(mic):
        raise RuntimeError("Failed to save " + name)
    log("Authored {} with icon {}".format(name, tex.get_name()))

log("DONE - material instances saved")
