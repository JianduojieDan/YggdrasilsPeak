# One standalone screenshot with a realm in the sky and the camera held on its planet (checks what planet_perf sees).
#   powershell -File Tools/Art/realm_shot.ps1 <realm> [x y z] [extra console commands]  -> Saved/Compare/realm_<realm>.png
param([string]$Realm = "Jotunheim", [double]$X = 1013070, [double]$Y = 1516502, [double]$Z = 145100, [string]$Extra = "", [int]$ResX = 1920, [int]$ResY = 1080)
$proj = "F:\UE_project\YggdrasilsPeak"
$shots = "$proj\Saved\Screenshots\WindowsEditor"
New-Item -ItemType Directory -Force "$proj\Saved\Compare" | Out-Null
Get-ChildItem $shots -Filter "HighresScreenshot*.png" -ErrorAction SilentlyContinue | Remove-Item -Force
$look = if ($Realm -eq 'none') { 1 } else { 3 }
$cmds = "ygg.Realm.StartTime 1000, Ghost, BugItGo $X $Y $Z 0 0 0, ygg.Realm.Force $Realm, ygg.Realm.LookAt $look, ygg.ShotAfter 12"
if ($Extra) { $cmds = "$cmds, $Extra" }
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY",
    "-nosplash", "-NoSound", "-unattended", "-YggNoSave", "-log=RealmShot.log", "-ExecCmds=`"$cmds`"")
$found = $null
for ($i = 0; $i -lt 80 -and -not $found; $i++) { Start-Sleep -Seconds 3; $found = Get-ChildItem $shots -Filter "HighresScreenshot*.png" -ErrorAction SilentlyContinue | Select-Object -First 1 }
if ($found) { Start-Sleep -Seconds 2; Copy-Item $found.FullName "$proj\Saved\Compare\realm_$Realm.png" -Force; "saved realm_$Realm.png" } else { "NO SCREENSHOT" }
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
