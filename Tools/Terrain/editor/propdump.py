import unreal,json
eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
out={'actors':[],'ropes':[]}
for a in eas.get_all_level_actors():
    n=a.get_class().get_name(); lab=a.get_actor_label()
    if lab.startswith('DetailTile_') or 'andscape' in n or lab.startswith('Rocks_') or lab=='Scatter_C4': continue
    if n=='FixedRope':
        out['ropes'].append([lab,[[p.x,p.y,p.z] for p in a.get_editor_property('rope_points')]]); continue
    o,e=a.get_actor_bounds(False)
    if max(e.x,e.y)<3000 and e.x>0:
        l=a.get_actor_location(); out['actors'].append([lab,l.x,l.y,l.z])
json.dump(out,open('F:/UE_project/YggdrasilsPeak/Saved/DetailTiles/route/props_now.json','w')); print(len(out['actors']),len(out['ropes']))
