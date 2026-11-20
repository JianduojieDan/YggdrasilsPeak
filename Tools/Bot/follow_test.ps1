# NPC follow check (#8): chapter 1 from the "job" beat (Nima, Elena, Jiwoo, Marek follow), then the player walks out of
# Dingboche toward ev_climb with stops, a bend and a turn back through the file. NPCDbg lines -> Saved/Logs/Follow.log,
# look-back shots -> Saved/Compare/follow/<Name>_i.png, summary printed by follow_report.py.
#   powershell -File Tools/Bot/follow_test.ps1 [-Name base] [-Extra "cmd, cmd"]
param([string]$Name = "base", [string]$Extra = "", [double]$Head = 169, [int]$ResX = 1600, [int]$ResY = 900)
$proj = "F:\UE_project\YggdrasilsPeak"
$shots = "$proj\Saved\Screenshots\WindowsEditor"
$out = "$proj\Saved\Compare\follow"
New-Item -ItemType Directory -Force $out | Out-Null
Get-ChildItem $shots -Include "ScreenShot*.png", "HighresScreenshot*.png" -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force
$c = @("t.MaxFPS 0", "ygg.Story.Auto 1", "ygg.Story.NPCDebug 1", "ygg.Realm.Force none")
function L([double]$t, [string]$cmd) { $script:c += "ygg.Later $t $cmd" }
function Walk([double]$t, [double]$yaw) { L $t "ygg.Snow.AutoYaw $yaw"; L $t "ygg.Snow.AutoPitch -5"; L $t "ygg.Snow.AutoWalk 0.5" }
function Stop([double]$t) { L $t "ygg.Snow.AutoWalk 0" }
function LookBack([double]$t, [double]$yaw) { L $t "ygg.Snow.AutoYaw $yaw"; L $t "ygg.Snow.AutoPitch -8"; L ($t + 1.5) "shot" }
L 20 "ygg.Story.Jump dawn"
L 34 "ygg.Story.Jump job"
L 74 "echo FOLLOWTEST walk1"
Walk 75 $Head;                Stop 95;  LookBack 96.5 ($Head + 180)      # 20 s straight, stop
L 100.5 "echo FOLLOWTEST walk2"
Walk 101 $Head;               Walk 107 ($Head - 50); Stop 115; LookBack 116.5 ($Head + 130)   # a bend
L 120.5 "echo FOLLOWTEST back"
Walk 121 ($Head + 130);       Stop 133; LookBack 134.5 ($Head - 50)      # turn back through the file
L 138.5 "echo FOLLOWTEST walk3"
Walk 139 ($Head - 50);        Stop 151; LookBack 152.5 ($Head + 130)
L 156 "quit"
if ($Extra) { $c = @($Extra -split ',\s*') + $c }
$exec = $c -join ", "
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY",
    "-nosplash", "-NoSound", "-unattended", "-YggNoSave", "-log=Follow.log", "-ExecCmds=`"$exec`"")
$proc.WaitForExit(300000) | Out-Null
if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force; "TIMEOUT" }
$found = @(Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTime)
for ($i = 0; $i -lt $found.Count; $i++) { Copy-Item $found[$i].FullName "$out\${Name}_$i.png" -Force }
"$Name : $($found.Count) of 4 shots"
python -I "$proj\Tools\Bot\follow_report.py" "$proj\Saved\Logs\Follow.log"
