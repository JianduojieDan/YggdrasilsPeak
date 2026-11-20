# #68 blocky footprint / own shadow test: open snow near Dingboche, low sun, walk out, turn, walk back along the own
# trench looking down (the 2026-11-14 view). 6 shots -> Saved/Compare/t68/<Name>_i.png
#   powershell -File Tools/Snow/own_shadow_test.ps1 -Name base -Yaw -70 [-ResX 2560 -ResY 1600] [-Extra "cmd, cmd"]
param([string]$Name = "base", [string]$Extra = "", [double]$X = 335000, [double]$Y = 2474000, [double]$Yaw = -42,
      [double]$Time = 1600, [int]$ResX = 1600, [int]$ResY = 1000)
$proj = "F:\UE_project\YggdrasilsPeak"
$shots = "$proj\Saved\Screenshots\WindowsEditor"
$out = "$proj\Saved\Compare\t68"
New-Item -ItemType Directory -Force $out | Out-Null
Get-ChildItem $shots -Include "ScreenShot*.png", "HighresScreenshot*.png" -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force
$c = @("t.MaxFPS 0", "ygg.Realm.Force none", "ygg.Realm.StartTime $Time", "ygg.ShotUI 1")
$t = 18.0
$c += "ygg.Later $t ygg.GroundProbe.Go $X $Y g $Yaw -30"; $t += 4
$c += "ygg.Later $t ygg.Snow.AutoYaw $Yaw"; $c += "ygg.Later $t ygg.Snow.AutoPitch -35"; $c += "ygg.Later $t ygg.Snow.AutoWalk 0.5"
$t += 12; $c += "ygg.Later $t ygg.Snow.AutoWalk 0"
# turn round and walk back in the trench, sun behind, looking down at the feet (the user's 11-14 view)
$back = $Yaw + 180
$t += 1; $c += "ygg.Later $t ygg.Snow.AutoYaw $back"; $c += "ygg.Later $t ygg.Snow.AutoPitch -58"
$t += 1.5; $c += "ygg.Later $t shot"
$c += "ygg.Later $t ygg.Snow.AutoWalk 0.5"
foreach ($k in 1..4) { $t += 1.5; $c += "ygg.Later $t shot" }
$t += 0.5; $c += "ygg.Later $t ygg.Snow.AutoWalk 0"
$t += 0.3; $c += "ygg.Later $t shot"
$t += 1; $c += "ygg.Later $t quit"
if ($Extra) { $c = @($Extra -split ',\s*') + $c }
$exec = $c -join ", "
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY",
    "-nosplash", "-NoSound", "-unattended", "-YggNoSave", "-log=T68.log", "-ExecCmds=`"$exec`"")
$proc.WaitForExit(([int]$t + 120) * 1000) | Out-Null
if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force; "TIMEOUT" }
$found = @(Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTime)
for ($i = 0; $i -lt $found.Count; $i++) { Copy-Item $found[$i].FullName "$out\${Name}_$i.png" -Force }
"$Name : $($found.Count) of 6 shots"
