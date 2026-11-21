# #82 coat hem flap: chapter 1 "job" (the team follows), walk out 12 s, turn back and film the followers walking up to
# you: a burst of shots while they approach -> Saved/Compare/hem/<Name>_<i>.png
#   powershell -File Tools/Story/hem_test.ps1 [-Name base] [-Extra "cmd, cmd"] [-Count 8]
param([string]$Name = "base", [string]$Extra = "", [double]$Head = 169, [int]$Count = 8)
$proj = "F:\UE_project\YggdrasilsPeak"
$shots = "$proj\Saved\Screenshots\WindowsEditor"
$out = "$proj\Saved\Compare\hem"
New-Item -ItemType Directory -Force $out | Out-Null
Get-ChildItem $shots -Include "ScreenShot*.png", "HighresScreenshot*.png" -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force
$c = @("t.MaxFPS 0", "ygg.Story.Auto 1", "ygg.Realm.Force none")
function L([double]$t, [string]$cmd) { $script:c += "ygg.Later $t $cmd" }
L 20 "ygg.Story.Jump dawn"; L 34 "ygg.Story.Jump job"
L 60 "ygg.Snow.AutoYaw $Head"; L 60 "ygg.Snow.AutoPitch -5"; L 60 "ygg.Snow.AutoWalk 0.5"
L 72 "ygg.Snow.AutoWalk 0"; L 72.2 "ygg.Snow.AutoYaw $($Head + 180)"; L 72.2 "ygg.Snow.AutoPitch -14"
$t = 72.8
for ($i = 0; $i -lt $Count; $i++) { L $t "shot"; $t += 0.7 }
# then face forward again (anyone walking ahead, e.g. Nima)
L $t "ygg.Snow.AutoYaw $Head"; L $t "ygg.Snow.AutoPitch -8"; $t += 0.8
for ($i = 0; $i -lt 4; $i++) { L $t "shot"; $t += 0.7 }
L ($t + 1) "quit"
if ($Extra) { $c = @($Extra -split ',\s*') + $c }
$exec = $c -join ", "
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1600", "-ResY=900",
    "-nosplash", "-NoSound", "-unattended", "-YggNoSave", "-log=Hem.log", "-ExecCmds=`"$exec`"")
$proc.WaitForExit(300000) | Out-Null
if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force; "TIMEOUT" }
$found = @(Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTime)
for ($i = 0; $i -lt $found.Count; $i++) { Copy-Item $found[$i].FullName "$out\${Name}_$i.png" -Force }
"$Name : $($found.Count) of $($Count + 4) shots"
