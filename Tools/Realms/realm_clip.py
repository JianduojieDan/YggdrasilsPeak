# Records a short clip of a realm from the showcase camera in the standalone game. usage: realm_clip.py <realm> [seconds] [audio device]  -> Saved/Compare/clips/<realm>.mp4
import os, subprocess, sys, time
R = sys.argv[1]; SEC = int(sys.argv[2]) if len(sys.argv) > 2 else 12; AUDIO = sys.argv[3] if len(sys.argv) > 3 else ""
ROOT = "F:/UE_project/YggdrasilsPeak"; OUT = ROOT + "/Saved/Compare/clips"; os.makedirs(OUT, exist_ok=True)
FF = "ffmpeg"; UE = "E:/UE5.8/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe"
LOG = f"clip_{R}.log"
g = subprocess.Popen([UE, ROOT + "/YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-resx=1280", "-resy=720", "-WinX=0", "-WinY=0", "-nosplash", "-ABSLOG=" + os.path.abspath(LOG),
    f"-ExecCmds=ygg.Save.LoadOnStart 0,BugItGo 809960 1469350 114400 40 12 0,ygg.Realm {R}"])
for _ in range(200):
    time.sleep(1)
    if os.path.exists(LOG) and "Bringing World" in open(LOG, errors="ignore").read(): break
time.sleep(45)   # streaming, the weather change
cmd = [FF, "-y", "-f", "gdigrab", "-framerate", "30", "-offset_x", "0", "-offset_y", "0", "-video_size", "1280x720", "-i", "desktop"]
if AUDIO: cmd += ["-f", "dshow", "-i", "audio=" + AUDIO]
cmd += ["-t", str(SEC), "-c:v", "libx264", "-preset", "veryfast", "-crf", "20", "-pix_fmt", "yuv420p", f"{OUT}/{R}.mp4"]
print(subprocess.run(cmd, capture_output=True, text=True).stderr[-200:])
g.terminate(); time.sleep(3); subprocess.run(["taskkill", "/F", "/IM", "UnrealEditor.exe"], capture_output=True) if False else None
print("clip", R)
