# Headlamp / compass / tent-light views (2026-11-21, user: "tent too shiny, compass looks like the flashlight, headlamp too
# strong"): the prologue at night with the headlamp on, the compass held up there, then the tent at dawn lying and sitting,
# and the compass held up in the tent -> Saved/Compare/lights/<Name>_<i>.png
#   powershell -File Tools/Story/light_shots.ps1 [-Name base] [-Extra "cmd, cmd"]
param([string]$Name = "base", [string]$Extra = "")
$proj = "F:\UE_project\YggdrasilsPeak"
$shots = "$proj\Saved\Screenshots\WindowsEditor"
$out = "$proj\Saved\Compare\lights"
New-Item -ItemType Directory -Force $out | Out-Null
Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force
$c = @("t.MaxFPS 0", "ygg.Story.Auto 1", "ygg.Realm.Force none",
  "ygg.Later 24 shot", "ygg.Later 25 ygg.Compass.Show 1", "ygg.Later 28.5 shot", "ygg.Later 29 ygg.Compass.Show 0",
  "ygg.Later 30 ygg.Story.Jump dawn", "ygg.Later 46 shot", "ygg.Later 47 ygg.Compass.Show 1", "ygg.Later 50.5 shot",
  "ygg.Later 51 ygg.Compass.Show 0", "ygg.Later 53 quit")
if ($Extra) { $c = @($Extra -split ',\s*') + $c }
$exec = $c -join ", "
$p = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
  "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1600", "-ResY=900",
  "-nosplash", "-NoSound", "-unattended", "-YggNoSave", "-log=LightShots.log", "-ExecCmds=`"$exec`"")
$p.WaitForExit(200000) | Out-Null
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; "TIMEOUT" }
$f = @(Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse | Sort-Object LastWriteTime)
for ($i = 0; $i -lt $f.Count; $i++) { Copy-Item $f[$i].FullName "$out\${Name}_$i.png" -Force }
"$($f.Count) of 4 shots"
