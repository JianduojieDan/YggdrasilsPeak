import unreal, json
# Runs INSIDE the editor via Tools/RoutePlanning/uepy. 2026-11-21: fits the plain knit beanie (Meshy knit_beanie, pivot
# at the bottom centre, 25 x 25 x 21 cm) to each MetaHuman face, so AYggStoryNPC::DressAs can attach it to the face's
# "head" bone (look fields hat / hat_loc / hat_rot / hat_scale). Measured on a temporary actor in the reference pose:
# the skull top = the face mesh's bounds top, the head width/depth = its bounds. The beanie's brim sits ~12 cm under
# the crown (above the eyebrows - Ji-woo's old hat covered her eyes), its crown 1.5 cm over the skull, tipped back 9 deg
# and 1 cm back, like a beanie actually worn. Writes Saved/HatFit.json (face path -> loc/rot/scale relative to "head").
ROOT = "F:/UE_project/YggdrasilsPeak"
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ML = unreal.MathLibrary
HAT = unreal.load_asset("/Game/Story/AI/knit_beanie/knit_beanie/StaticMeshes/knit_beanie")
hb = HAT.get_bounding_box(); hsz = hb.max - hb.min
SIZE = {"jiwoo": 0.79, "elena": 0.80, "pema": 0.79, "nima": 0.83, "marek": 0.86}   # x 25 cm = the beanie's outside width
FACES = {
    "jiwoo": "/Game/Story/MetaHumans/Jiwoo/Jiwoo/Face/SKM_MH_Jiwoo_Aera_FaceMesh",
    "marek": "/Game/Story/MetaHumans/Marek/Marek/Face/SKM_MH_Marek_Bruce_FaceMesh",
    "nima": "/Game/Story/MetaHumans/Nima/Nima/Face/SKM_MH_Nima_Aoi_FaceMesh",
    "elena": "/Game/Story/MetaHumans/Elena/Elena/Face/SKM_MH_Elena_Vivian_FaceMesh",
    "pema": "/Game/Story/MetaHumans/Pema/Pema/Face/SKM_MH_Pema_Lani_FaceMesh",
}
out = {}
for who, fp in FACES.items():
    face = unreal.load_asset(fp)
    if not face:
        print("NO FACE", who, fp); continue
    a = EAS.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(0, 0, -400000), unreal.Rotator(0, 0, 0))
    c = a.skeletal_mesh_component; c.set_skinned_asset_and_update(face)
    head = c.get_socket_transform("head", unreal.RelativeTransformSpace.RTS_WORLD)
    bb = face.get_bounds(); org, ext = bb.origin, bb.box_extent      # mesh-space centre / half extent (actor unrotated at the origin offset)
    base = a.get_actor_location()
    top_z = base.z + org.z + ext.z                    # the skull top (the face mesh has no hair)
    cx, cy = head.translation.x, head.translation.y + 0.5   # over the head bone, a hair forward (the skull's centre)
    width = 2 * ext.x                                 # NOTE: the face mesh's bounds include the neck/shoulder seam - not the head
    s_xy = SIZE.get(who, 0.82)                        # a real beanie on a real head: ~19.5-21 cm outside
    s_z = 0.80 * s_xy
    hat_h = hsz.z * s_z
    want = unreal.Transform(unreal.Vector(cx, cy - 1.0, top_z + 1.5 - hat_h), unreal.Rotator(-9.0, 0.0, 0.0), unreal.Vector(s_xy, s_xy, s_z))
    rel = ML.compose_transforms(want, ML.invert_transform(head))
    r = rel.rotation.rotator()
    out[fp] = {"who": who, "hat_loc": [round(rel.translation.x, 2), round(rel.translation.y, 2), round(rel.translation.z, 2)],
               "hat_rot": [round(r.pitch, 2), round(r.yaw, 2), round(r.roll, 2)],
               "hat_scale": [round(rel.scale3d.x, 3), round(rel.scale3d.y, 3), round(rel.scale3d.z, 3)],
               "head_width": round(width, 1), "crown_over_head_bone": round(top_z - head.translation.z, 1)}
    print("HAT", who, out[fp])
    EAS.destroy_actor(a)
json.dump(out, open(f"{ROOT}/Saved/HatFit.json", "w"), indent=1)
