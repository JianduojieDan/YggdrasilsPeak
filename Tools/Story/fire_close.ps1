# Close views of the Dingboche camp fire (2026-11-21): 2.4 m away looking down, at dawn and at dusk -> Saved/Compare/spots/<Name>_close<i>.png
param([string]$Name = "fire")
$proj = "F:\UE_project\YggdrasilsPeak"
$shots = "$proj\Saved\Screenshots\WindowsEditor"
$out = "$proj\Saved\Compare\spots"
New-Item -ItemType Directory -Force $out | Out-Null
Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force
$c = @("t.MaxFPS 0", "ygg.Realm.Force none", "ygg.Realm.StartTime 0630",
  "ygg.Later 18 BugItGo 338800 2469470 -69100 -22 13 0", "ygg.Later 22 shot",
  "ygg.Later 24 BugItGo 339037 2469300 -69100 -20 90 0", "ygg.Later 28 shot",
  "ygg.Later 29 ygg.Realm.StartTime 1900", "ygg.Later 30 BugItGo 338800 2469470 -69100 -22 13 0", "ygg.Later 37 shot",
  "ygg.Later 39 quit")
$exec = $c -join ", "
$p = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
  "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1600", "-ResY=900",
  "-nosplash", "-NoSound", "-unattended", "-YggNoSave", "-log=FireClose.log", "-ExecCmds=`"$exec`"")
$p.WaitForExit(200000) | Out-Null
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; "TIMEOUT" }
$f = @(Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse | Sort-Object LastWriteTime)
for ($i = 0; $i -lt $f.Count; $i++) { Copy-Item $f[$i].FullName "$out\${Name}_close$i.png" -Force }
"$($f.Count) shots"
