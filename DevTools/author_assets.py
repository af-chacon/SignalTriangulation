"""Author the SignalTriangulation MAM research tree + schematic assets.

Run headlessly:
  UnrealEditor-Cmd FactoryGame.uproject -run=pythonscript -script=<this file> -nullrhi -unattended
"""
import unreal

MOD_ROOT = "/SignalTriangulation"
SCHEMATIC_NAME = "Schematic_SignalTriangulation"
TREE_NAME = "ResearchTree_SignalTriangulation"

RADAR_RESEARCH = "/Game/FactoryGame/Schematics/Research/Quartz_RS/Research_Quartz_4.Research_Quartz_4_C"
# The node is thematically the overlap of the Quartz and Alien Technology trees: it also
# requires the vanilla artifact-analysis researches that teach radar towers to detect them.
SOMERSLOOP_RESEARCH = "/Game/FactoryGame/Schematics/Research/AlienTech_RS/Research_Alien_Somersloop.Research_Alien_Somersloop_C"
MERCER_RESEARCH = "/Game/FactoryGame/Schematics/Research/AlienTech_RS/Research_Alien_MercerSphere.Research_Alien_MercerSphere_C"
NODE_BP_CLASS = "/Game/FactoryGame/Schematics/Research/BPD_ResearchTreeNode.BPD_ResearchTreeNode_C"
# Cost shape follows the fuel researches (1x hard drive + materials): pay with the artifacts
# themselves plus Superposition Oscillators (internally "QuantumOscillator") as the late-game anchor.
COST_ITEMS = [
    ("/Game/FactoryGame/Resource/Environment/CrashSites/Desc_HardDrive.Desc_HardDrive_C", 1),
    ("/Game/FactoryGame/Prototype/WAT/Desc_WAT2.Desc_WAT2_C", 3),
    ("/Game/FactoryGame/Prototype/WAT/Desc_WAT1.Desc_WAT1_C", 1),
    ("/Game/FactoryGame/Resource/Parts/QuantumOscillator/Desc_QuantumOscillator.Desc_QuantumOscillator_C", 10),
]
# Icon: reuse the hard drive descriptor's icon texture (resolved from the placeholder CDO below)
ICON_DESC = "/Game/FactoryGame/Resource/Environment/CrashSites/Desc_HardDrive.Desc_HardDrive_C"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def log(msg):
    unreal.log("[SignalTriangulation] " + msg)


def load_class(path):
    cls = unreal.load_object(None, path)
    if not cls:
        raise RuntimeError("Failed to load class: " + path)
    return cls


unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([MOD_ROOT], force_rescan=True)


def get_or_create_blueprint(name, parent_class):
    asset_path = "{}/{}".format(MOD_ROOT, name)
    existing = unreal.load_asset(asset_path) if eal.does_asset_exist(asset_path) else unreal.load_asset(asset_path)
    if existing:
        log("Reusing existing asset " + asset_path)
        return existing
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    bp = asset_tools.create_asset(name, MOD_ROOT, unreal.Blueprint, factory)
    if not bp:
        raise RuntimeError("Failed to create blueprint " + asset_path)
    return bp


def cdo_of(bp):
    gen = bp.generated_class()
    return unreal.get_default_object(gen)


def find_icon_texture():
    try:
        desc_cls = load_class(ICON_DESC)
        desc_cdo = unreal.get_default_object(desc_cls)
        for prop in ("mPersistentBigIcon", "mSmallIcon"):
            try:
                tex = desc_cdo.get_editor_property(prop)
                if tex:
                    log("Using icon from {}.{}".format(ICON_DESC, prop))
                    return tex
            except Exception:
                pass
    except Exception as e:
        log("Icon lookup failed: {}".format(e))
    return None


def make_brush(texture):
    brush = unreal.SlateBrush()
    if texture:
        brush.set_editor_property("resource_object", texture)
    return brush


PURCHASED_DEP_BP = "/Game/FactoryGame/AvailabilityDependencies/BP_SchematicPurchasedDependency.BP_SchematicPurchasedDependency_C"


def make_purchased_dependency(outer, schematic_classes):
    dep = unreal.new_object(load_class(PURCHASED_DEP_BP), outer=outer)
    dep.set_editor_property("mSchematics", schematic_classes)
    dep.set_editor_property("mRequireAllSchematicsToBePurchased", True)
    return dep


# ---------------------------------------------------------------- schematic
schematic_bp = get_or_create_blueprint(SCHEMATIC_NAME, unreal.FGSchematic)
sch_cdo = cdo_of(schematic_bp)

sch_cdo.set_editor_property("mDisplayName", "Signal Triangulation")
sch_cdo.set_editor_property(
    "mDescription",
    "Cross-references radar tower return signals to triangulate the precise map position of every "
    "Somersloop, Mercer Sphere, and FICSIT crash site on the planet.",
)
sch_cdo.set_editor_property("mType", unreal.SchematicType.EST_MAM)
sch_cdo.set_editor_property("mTechTier", 3)
sch_cdo.set_editor_property("mTimeToComplete", 3.0)

cost = []
for path, amount in COST_ITEMS:
    ia = unreal.ItemAmount()
    ia.set_editor_property("ItemClass", load_class(path))
    ia.set_editor_property("Amount", amount)
    cost.append(ia)
sch_cdo.set_editor_property("mCost", cost)

icon_tex = find_icon_texture()
if icon_tex:
    sch_cdo.set_editor_property("mSchematicIcon", make_brush(icon_tex))
    try:
        sch_cdo.set_editor_property("mSmallSchematicIcon", icon_tex)
    except Exception as e:
        log("mSmallSchematicIcon skipped: {}".format(e))

radar_cls = load_class(RADAR_RESEARCH)
dependency_classes = [radar_cls, load_class(SOMERSLOOP_RESEARCH), load_class(MERCER_RESEARCH)]
sch_cdo.set_editor_property("mSchematicDependencies", [make_purchased_dependency(sch_cdo, dependency_classes)])

log("Schematic configured")

# ---------------------------------------------------------------- research tree
# NOTE: unused since the node is now injected into the vanilla Quartz tree at runtime
# (see STTreeInjector.cpp); kept behind this flag in case a standalone tree is wanted again.
AUTHOR_STANDALONE_TREE = False

tree_bp = None
if AUTHOR_STANDALONE_TREE:
    tree_bp = get_or_create_blueprint(TREE_NAME, unreal.FGResearchTree)
    tree_cdo = cdo_of(tree_bp)

    tree_cdo.set_editor_property("mDisplayName", "Signal Triangulation")
    tree_cdo.set_editor_property("mPreUnlockDisplayName", "Anomalous Radar Returns")
    tree_cdo.set_editor_property(
        "mPreUnlockDescription",
        "Radar towers are picking up anomalous returns. Further research required.",
    )
    tree_cdo.set_editor_property(
        "mPostUnlockDescription",
        "Triangulates exact locations of alien artifacts and FICSIT crash sites from radar tower data.",
    )
    if icon_tex:
        tree_cdo.set_editor_property("mResearchTreeIcon", make_brush(icon_tex))

    tree_cdo.set_editor_property("mUnlockDependencies", [make_purchased_dependency(tree_cdo, [radar_cls])])

    # The MAM node data is a UserDefinedStruct; fields must be addressed by their internal mangled names
    F_SCHEMATIC = "Schematic_27_3663A42446FDB4387D0C81AFC23E225B"
    F_COORDINATES = "Coordinates_23_5A3DE6C040C7026EDEA49E9CE8612292"
    F_COORD_X = "X_2_3FF107F84D30EB52DD50898C7D2CAD67"
    F_COORD_Y = "Y_4_F18C5B824136D7759146338CAA496F0A"

    node_cls = load_class(NODE_BP_CLASS)
    node = unreal.new_object(node_cls, outer=tree_cdo)
    node_data = node.get_editor_property("mNodeDataStruct")
    node_data.set_editor_property(F_SCHEMATIC, schematic_bp.generated_class())
    coords = node_data.get_editor_property(F_COORDINATES)
    coords.set_editor_property(F_COORD_X, 2.0)
    coords.set_editor_property(F_COORD_Y, 2.0)
    node_data.set_editor_property(F_COORDINATES, coords)
    node.set_editor_property("mNodeDataStruct", node_data)
    tree_cdo.set_editor_property("mNodes", [node])

    log("Research tree configured")

# ---------------------------------------------------------------- save
for bp in (schematic_bp, tree_bp) if tree_bp else (schematic_bp,):
    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception as e:
        log("Compile skipped for {}: {}".format(bp.get_name(), e))
    if not eal.save_loaded_asset(bp):
        raise RuntimeError("Failed to save " + bp.get_name())

log("DONE - assets saved")
