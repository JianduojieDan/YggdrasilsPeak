# Runs OUTSIDE the editor (scratch helpers mcp + uepy): for each realm start PIE at its route landmark, look up the route,
# take frames over time, build Saved/Compare/realms_<tag>.jpg. usage: realm_shots.py <tag> [realm ...]
import json, os, subprocess, sys, time, glob, shutil, math
ROOT = "F:/UE_project/YggdrasilsPeak"; UEPY = ROOT + "/Tools/RoutePlanning/uepy.py"; SHOTS = ROOT + "/Saved/Screenshots/WindowsEditor"
L = [(340208,2469643,-69458),(292144,1501054,13269),(487512,1289826,24898),(657236,1343477,67536),(809960,1469350,114212),(1001409,1510839,142583),(1169446,1593845,202261),(1287207,1585072,283188)]
SPOTS = {k:4 for k in ["Midgard","Niflheim","Muspelheim","Jotunheim","Alfheim","Vanaheim","Svartalfheim","Hel","Asgard"]}   # one showcase camera for all: direct comparison
SUMMIT = (1240508,1412617)
def ue(code):
    open("_c.txt","w").write(code); return subprocess.run([sys.executable,"-I",UEPY,"_c.txt"],capture_output=True,text=True).stdout
def mcp(tool, args):
    json.dump({"toolset_name":"EditorToolset.EditorAppToolset","tool_name":tool,"arguments":args},open("_m.json","w"))
    subprocess.run([sys.executable,"-I","mcp.py","call_tool","_m.json"],capture_output=True)
PRE = "import unreal\nw=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()\n"
tag = sys.argv[1]; names = sys.argv[2:] or list(SPOTS)
ue("import unreal\ntry:\n    unreal.get_default_object(unreal.EditorPerformanceSettings).set_editor_property('throttle_cpu_when_not_foreground',False)\nexcept Exception as e:\n    print(e)\nw=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()\nunreal.SystemLibrary.execute_console_command(w,'ygg.Save.LoadOnStart 0')\nif w.get_name()!='base_map': unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/LevelPrototyping/base_map')\n")
time.sleep(45)   # map load + landscape collision
out = f"{ROOT}/Saved/Compare/{tag}"; os.makedirs(out, exist_ok=True)
for n in names:
    x,y,z = L[SPOTS[n]]; yaw = 12.3
    r=ue("import unreal\nw=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()\n"+f"h=unreal.SystemLibrary.line_trace_single(w,unreal.Vector({x},{y},900000),unreal.Vector({x},{y},-400000),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)\nprint('Z=',h.to_tuple()[4].z)\n")
    z=float(r.split("Z=")[1].split()[0])-3000+150   # a real fall would kill the player and fade to black
    mcp("StartPIE",{"options":{"bSimulate":False,"playMode":"PlayMode_InViewPort","startTransform":{"location":{"x":x,"y":y,"z":z+3000},"rotation":{"pitch":0,"yaw":yaw,"roll":0},"scale":{"x":1,"y":1,"z":1}}}})
    time.sleep(9)
    ue(PRE+f"pc=unreal.GameplayStatics.get_player_controller(w,0)\npc.set_control_rotation(unreal.Rotator(roll=0,pitch=40,yaw={yaw}))\nunreal.SystemLibrary.execute_console_command(w,'ygg.Realm {n}')\n")
    time.sleep(26)   # the ~20 s weather change must have arrived
    for i in range(3):
        for f in glob.glob(SHOTS+"/Highres*"): os.remove(f)
        ue(PRE+"unreal.SystemLibrary.execute_console_command(w,'HighResShot 1')\n")
        time.sleep(3.5)
        fs = glob.glob(SHOTS+"/Highres*")
        if fs: shutil.move(fs[0], f"{out}/{n}_{i}.png")
    mcp("StopPIE",{}); time.sleep(4)
ue("import unreal\nunreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'ygg.Save.LoadOnStart 1')\n")
from PIL import Image, ImageDraw
W,H = 640,292; cols = 3; fs = [f"{out}/{n}_0.png" for n in names]
sheet = Image.new("RGB",(W*cols,H*math.ceil(len(fs)/cols)))
for i,f in enumerate(fs):
    if os.path.exists(f):
        im = Image.open(f).convert("RGB").resize((W,H)); ImageDraw.Draw(im).text((8,8),names[i],fill=(255,255,0)); sheet.paste(im,(i%cols*W,i//cols*H))
sheet.save(f"{ROOT}/Saved/Compare/realms_{tag}.jpg",quality=88); print("sheet", tag)
