# Runs inside the editor: displacement-related parameters of a detail tile's material (and its parents).
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
a = [x for x in eas.get_all_level_actors() if x.get_actor_label() == globals().pop('TILE', 'DetailTile_R_3_40')][0]
mat = a.static_mesh_component.get_material(0)
chain = []
m = mat
while m:
    chain.append(m)
    m = m.get_editor_property('parent') if isinstance(m, unreal.MaterialInstance) else None
print('chain', [c.get_path_name() for c in chain])
base = chain[-1]
print('base displacement_scaling', base.get_editor_property('displacement_scaling') if hasattr(base, 'get_editor_property') else None)
names = unreal.MaterialEditingLibrary.get_scalar_parameter_names(base)
for n in names:
    s = str(n)
    if any(k in s.lower() for k in ('disp', 'trail', 'rubble', 'relief', 'sink', 'depth', 'height')):
        v = unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(mat, n) if isinstance(mat, unreal.MaterialInstance) else unreal.MaterialEditingLibrary.get_material_default_scalar_parameter_value(base, n)
        print('  ', s, v)
