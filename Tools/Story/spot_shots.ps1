# One-off views (2026-11-21): the camp fire at dawn and at dusk, the mess dome, a field wall -> Saved/Compare/spots/<Name>_<i>.png
param([string]$Name = "base")
$proj = "F:\UE_project\YggdrasilsPeak"
$shots = "$proj\Saved\Screenshots\WindowsEditor"
$out = "$proj\Saved\Compare\spots"
New-Item -ItemType Directory -Force $out | Out-Null
Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force
$c = @("t.MaxFPS 0", "ygg.Realm.Force none", "ygg.Realm.StartTime 0630",
  "ygg.Later 18 BugItGo 338640 2469525 -69170 -10 0 0", "ygg.Later 22 shot",
  "ygg.Later 23 BugItGo 340245 2469435 -69020 -6 0 0", "ygg.Later 27 shot",
  "ygg.Later 28 BugItGo 339390 2469290 -69060 -20 160 0", "ygg.Later 32 shot",
  "ygg.Later 33 ygg.Realm.StartTime 1830", "ygg.Later 34 BugItGo 338640 2469525 -69170 -10 0 0", "ygg.Later 40 shot",
  "ygg.Later 42 quit")
$exec = $c -join ", "
$p = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
  "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1600", "-ResY=900",
  "-nosplash", "-NoSound", "-unattended", "-YggNoSave", "-log=Spots.log", "-ExecCmds=`"$exec`"")
$p.WaitForExit(200000) | Out-Null
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; "TIMEOUT" }
$f = @(Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse | Sort-Object LastWriteTime)
for ($i = 0; $i -lt $f.Count; $i++) { Copy-Item $f[$i].FullName "$out\${Name}_$i.png" -Force }
"$($f.Count) shots"
