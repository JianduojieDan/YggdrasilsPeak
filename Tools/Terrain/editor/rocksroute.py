import unreal,json,glob,math
D='F:/UE_project/YggdrasilsPeak/Saved/DetailTiles/route'
MESH={'stonephoto':'/Game/Fab/Stone_-_photogrammetry/stone_photogrammetry/StaticMeshes/stone_photogrammetry',
'alaskan':'/Game/Fab/Alaskan_Cliff_Rock_1_Free/alaskan_cliff_rock_1_free/StaticMeshes/alaskan_cliff_rock_1_free',
'stone':'/Game/Fab/FREE_Stone_-_8K_Textures/stone/StaticMeshes/stone',
'rockysnowpile':'/Game/Fab/Megascans/3D/Rocky_Snow_Pile_vcytca0fa/Raw/vcytca0fa_tier_0/StaticMeshes/vcytca0fa_tier_0'}
MAT={'giant':'/Game/Environment/RockSnow/MI_RockSnow_StonePhoto','boulder':'/Game/Environment/RockSnow/MI_RockSnow_StonePhoto_Dusted',
'medium_stonephoto':'/Game/Environment/RockSnow/MI_RockSnow_StonePhoto_Dusted','medium_alaskan':'/Game/Environment/RockSnow/MI_RockSnow_alaskan_Dusted',
'medium_rockysnowpile':None,'small_stone':'/Game/Environment/RockSnow/MI_RockSnow_Stone_Dusted','small_alaskan':'/Game/Environment/RockSnow/MI_RockSnow_alaskan_Dusted'}
KEY={'giant':'stonephoto','boulder':'stonephoto','medium_stonephoto':'stonephoto','medium_alaskan':'alaskan','medium_rockysnowpile':'rockysnowpile','small_stone':'stone','small_alaskan':'alaskan'}
pl=[]
for f in glob.glob(D+'/scatter_R_*.json'): pl+=json.load(open(f))
eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_actor_label()=='Rocks_Route': eas.destroy_actor(a)
act=eas.spawn_actor_from_class(unreal.Actor,unreal.Vector(0,0,0)); act.set_actor_label('Rocks_Route'); act.set_folder_path('Environment/DetailTiles/Route')
sub=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem); root=sub.k2_gather_subobject_data_for_instance(act)[0]
comps={}; bmin={}
for k in sorted(set(p[0] for p in pl)):
    h,_=sub.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root,new_class=unreal.HierarchicalInstancedStaticMeshComponent))
    c=unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
    m=unreal.load_asset(MESH[KEY[k]]); c.set_static_mesh(m); bmin[k]=m.get_bounding_box().min.z
    if MAT[k]: c.set_material(0,unreal.load_asset(MAT[k]))
    c.set_cast_shadow(True)
    if k.startswith('small_'): c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    comps[k]=c
by={k:[] for k in comps}
for k,x,y,zb,yaw,ux,uy,sc in pl:
    up=unreal.Vector(ux,uy,1.0); up.normalize()
    rot=unreal.MathLibrary.make_rot_from_zx(up,unreal.Vector(math.cos(math.radians(yaw)),math.sin(math.radians(yaw)),0.0))
    by[k].append(unreal.Transform(unreal.Vector(x,y,zb-bmin[k]*sc*up.z),rot,unreal.Vector(sc,sc,sc)))
for k,c in comps.items():
    for i in range(0,len(by[k]),20000): c.add_instances(by[k][i:i+20000],False,True)
    print(k,c.get_instance_count())
