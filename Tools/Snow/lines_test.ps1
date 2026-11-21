# #73 evenly spaced thin lines / wide bands on the flattened camp ground (user shots at dawn by the tents, looking down).
# Stands at the Dingboche camp (tent_out), looks down -60, dawn sun, then the same view with one suspect off at a time:
#   0 base | 1 Nanite tessellation (snow displacement) off | 2 virtual shadow maps off | 3 Lumen GI off | 4 DLSS/TSR off
# -> Saved/Compare/lines/<Name>_<i>.png      powershell -File Tools/Snow/lines_test.ps1 -X <x> -Y <y> [-Yaw 0] [-Time 630]
param([string]$Name = "base", [double]$X, [double]$Y, [double]$Yaw = 0, [double]$Time = 630, [string]$Extra = "")
$proj = "F:\UE_project\YggdrasilsPeak"
$shots = "$proj\Saved\Screenshots\WindowsEditor"
$out = "$proj\Saved\Compare\lines"
New-Item -ItemType Directory -Force $out | Out-Null
Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force
$c = @("t.MaxFPS 0", "ygg.Realm.Force none", "ygg.Realm.StartTime $Time")
function L([double]$t, [string]$cmd) { $script:c += "ygg.Later $t $cmd" }
L 18 "ygg.GroundProbe.Go $X $Y g $Yaw -60"
$t = 26.0
$tests = @(@("", ""), @("r.Nanite.AllowTessellation 0", "r.Nanite.AllowTessellation 1"),
           @("r.Shadow.Virtual.Enable 0", "r.Shadow.Virtual.Enable 1"),
           @("r.Lumen.DiffuseIndirect.Allow 0", "r.Lumen.DiffuseIndirect.Allow 1"),
           @("r.AntiAliasingMethod 0", "r.AntiAliasingMethod 4"))
foreach ($p in $tests) { if ($p[0]) { L $t $p[0] }; L ($t + 2.5) "shot"; if ($p[1]) { L ($t + 3.0) $p[1] }; $t += 5 }
L ($t + 1) "quit"
if ($Extra) { $c = @($Extra -split ',\s*') + $c }
$exec = $c -join ", "
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1600", "-ResY=900",
    "-nosplash", "-NoSound", "-unattended", "-YggNoSave", "-log=Lines.log", "-ExecCmds=`"$exec`"")
$proc.WaitForExit(240000) | Out-Null
if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force; "TIMEOUT" }
$found = @(Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTime)
for ($i = 0; $i -lt $found.Count; $i++) { Copy-Item $found[$i].FullName "$out\${Name}_$i.png" -Force }
"$Name : $($found.Count) of 5 shots"
