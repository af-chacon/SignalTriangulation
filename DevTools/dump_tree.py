import unreal

TREE = "/Game/FactoryGame/Schematics/Research/BPD_ResearchTree_Quartz.BPD_ResearchTree_Quartz_C"

F_SCHEMATIC = "Schematic_27_3663A42446FDB4387D0C81AFC23E225B"
F_COORDINATES = "Coordinates_23_5A3DE6C040C7026EDEA49E9CE8612292"
F_CHILDREN = "ChildrenAndRoads_34_758C9E0D4F09DAF4BBAD309358952A0A"
F_X = "X_2_3FF107F84D30EB52DD50898C7D2CAD67"
F_Y = "Y_4_F18C5B824136D7759146338CAA496F0A"


def coord(c):
    return (c.get_editor_property(F_X), c.get_editor_property(F_Y))


tree_cls = unreal.load_object(None, TREE)
cdo = unreal.get_default_object(tree_cls)
nodes = cdo.get_editor_property("mNodes")
unreal.log("[dump] {} nodes".format(len(nodes)))
for node in nodes:
    nd = node.get_editor_property("mNodeDataStruct")
    sch = nd.get_editor_property(F_SCHEMATIC)
    xy = coord(nd.get_editor_property(F_COORDINATES))
    children = nd.get_editor_property(F_CHILDREN)
    unreal.log("[dump] node {} at {}".format(sch.get_name() if sch else "None", xy))
    try:
        for child_coord, road in children.items():
            pts = [coord(p) for p in road.get_editor_property("Points_10_9533B9104470D8E053E7ACA5C4C9F865")]
            unreal.log("[dump]    child {} road {}".format(coord(child_coord), pts))
    except Exception as e:
        unreal.log("[dump]    children read error: {}".format(e))
