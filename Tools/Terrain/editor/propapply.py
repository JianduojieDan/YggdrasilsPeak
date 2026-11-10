import unreal,json
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
al=eas.get_all_level_actors(); by={a.get_actor_label():a for a in al}
T=set(a for a in al if a.get_actor_label().startswith('DetailTile_R_'))
def tilez(x,y,ign):
    ign=list(ign)
    for _ in range(12):
        h=unreal.SystemLibrary.line_trace_single_for_objects(world,unreal.Vector(x,y,900000),unreal.Vector(x,y,-400000),[unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1],True,ign,unreal.DrawDebugTrace.NONE,True)
        if not h: return None
        t=h.to_tuple(); a=t[9]
        if a in T: return t[4].z
        if a is None or a.get_class().get_name() in ('Landscape','LandscapeStreamingProxy'): return None
        ign.append(a)
    return None
d=json.load(open('F:/UE_project/YggdrasilsPeak/Saved/DetailTiles/route/props_offsets.json'))
moved=back=0
for lab,x,y,off,zo in d['actors']:
    a=by.get(lab)
    if not a: continue
    z=tilez(x,y,[a]); l=a.get_actor_location()
    if z is not None: a.set_actor_location(unreal.Vector(l.x,l.y,z+off),False,False); moved+=1
    elif abs(l.z-zo)>1: a.set_actor_location(unreal.Vector(l.x,l.y,zo),False,False); back+=1
r=0
for a in al:
    if a.get_class().get_name()!='FixedRope': continue
    pts=a.get_editor_property('rope_points'); new=[]; ch=False
    for p in pts:
        z=tilez(p.x,p.y,[a])
        if z is not None: new.append(unreal.Vector(p.x,p.y,z+30)); ch=True
        else: new.append(p)
    if ch:
        a.set_editor_property('rope_points',new); l=a.get_actor_location(); a.set_actor_location(l+unreal.Vector(0,0,1),False,False); a.set_actor_location(l,False,False); r+=1
print('props on tiles',moved,'props restored',back,'ropes',r)
