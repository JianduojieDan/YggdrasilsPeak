# Runs INSIDE the editor. Imports Saved/Story (meshes, sounds, photos) for chapter 1 and wires their materials. Re-runnable.
import unreal, glob, os
at = unreal.AssetToolsHelpers.get_asset_tools(); EAL = unreal.EditorAssetLibrary; mel = unreal.MaterialEditingLibrary
S = "F:/UE_project/YggdrasilsPeak/Saved/Story"
MD = "/Game/Story/Materials"

def imp(files, dest):
    tasks = []
    for f in files:
        t = unreal.AssetImportTask(); t.filename = f; t.destination_path = dest; t.automated = True; t.replace_existing = True; t.save = True
        tasks.append(t)
    at.import_asset_tasks(tasks)
    return [p for t in tasks for p in t.imported_object_paths]

def find_mesh(name):
    for p in EAL.list_assets("/Game/Story/Meshes", recursive=True):
        a = p.split(".")[0]
        if a.endswith("/" + name):
            o = unreal.load_asset(a)
            if isinstance(o, unreal.StaticMesh): return o
    return None

imp(sorted(glob.glob(f"{S}/*.glb")), "/Game/Story/Meshes")
SLOTS = {"SM_Tent_Dome": ["TentYellow", "Pole", "Cord"], "SM_Tent_Mess": ["TentBlue", "Pole", "Cord"], "SM_Khata": ["Silk"]}
for name, mats in SLOTS.items():
    m = find_mesh(name)
    if not m: print("missing mesh", name); continue
    for i, k in enumerate(mats):
        m.set_material(i, unreal.load_asset(f"{MD}/MI_Story_{k}"))
    EAL.save_asset(m.get_path_name().split(".")[0], False)
    print("mesh", m.get_path_name())

waves = imp(sorted(glob.glob(f"{S}/Audio/*.wav")), "/Game/Audio/Story")
for p in waves:
    w = unreal.load_asset(p.split(".")[0])
    if w and "Loop" in w.get_name():
        w.set_editor_property("looping", True); EAL.save_asset(p.split(".")[0], False)
print("sounds", len(waves))

photos = imp(sorted(glob.glob(f"{S}/Photos/*.png")), "/Game/Story/Photos")
parent = unreal.load_asset(f"{MD}/M_StoryProp")
for p in photos:
    tex = unreal.load_asset(p.split(".")[0])
    n = tex.get_name().replace("T_Photo_", "")
    path = f"{MD}/MI_Story_Photo_{n}"
    mi = unreal.load_asset(path) or at.create_asset(f"MI_Story_Photo_{n}", MD, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    mel.set_material_instance_scalar_parameter_value(mi, "Kind", 8.0)
    mel.set_material_instance_texture_parameter_value(mi, "Photo", tex)
    EAL.save_loaded_asset(mi, False)
print("photos", len(photos))
