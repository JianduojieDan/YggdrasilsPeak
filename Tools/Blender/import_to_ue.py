# Imports every Art/Blender/Export/*.fbx into /Game/Props/Blender via the editor's remote Python.
#   python Tools/Blender/import_to_ue.py
import sys, time
sys.path.append("E:/UE5.8/UE_5.8/Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python")
from remote_execution import RemoteExecution, MODE_EXEC_FILE

CODE = r'''
import unreal, glob, os
src = "F:/UE_project/YggdrasilsPeak/Art/Blender/Export"
tasks = []
for f in sorted(glob.glob(src + "/*.fbx")):
    t = unreal.AssetImportTask()
    t.filename = f.replace("\\", "/")
    t.destination_path = "/Game/Props/Blender"
    t.replace_existing = True
    t.automated = True
    t.save = True
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
    print("IMPORTED", t.filename, list(t.imported_object_paths))
'''

re = RemoteExecution(); re.start()
for _ in range(50):
    if re.remote_nodes: break
    time.sleep(0.2)
if not re.remote_nodes:
    sys.exit("No editor found - is UnrealEditor running?")
re.open_command_connection(re.remote_nodes[0]["node_id"])
r = re.run_command(CODE, unattended=True, exec_mode=MODE_EXEC_FILE)
for o in r.get("output", []):
    print(o["output"].strip())
print("success:", r.get("success"))
re.stop()
