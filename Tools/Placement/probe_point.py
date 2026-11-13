# Runs inside the editor: what ground is under PT = (x, y)? Every tile / landscape part hit, and the ISM rope point near.
import math, unreal
exec(open('F:/UE_project/YggdrasilsPeak/Tools/Placement/ground_lib.py').read())
x, y = PT
key = (int(math.floor(x / CELL)), int(math.floor(y / CELL)))
for grid, name in ((grid_tiles, 'tile'), (grid_land, 'landscape')):
    for box, comp in grid.get(key, ()):
        if box[0] <= x <= box[2] and box[1] <= y <= box[3]:
            r = comp.line_trace_component(unreal.Vector(x, y, box[4]), unreal.Vector(x, y, box[5]), True, False, False)
            owner = comp.get_owner().get_actor_label()
            print(name, owner, comp.get_name(), 'hit z', round(r[0].z, 1) if r else None)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
hs = unreal.SystemLibrary.line_trace_multi(w, unreal.Vector(x, y, 900000), unreal.Vector(x, y, -500000), unreal.TraceTypeQuery.ECC_VISIBILITY, True, [], unreal.DrawDebugTrace.NONE, True)
for h in hs or []:
    t = h.to_tuple()
    print('world trace hit', t[9].get_actor_label() if t[9] else None, round(t[4].z, 1))
