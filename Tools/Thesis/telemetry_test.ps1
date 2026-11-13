# A scripted play session for the thesis data log (Development_Plan.md A3): starts the game, jumps up the camps on a
# timer, quits, and charts the session it wrote (Saved/Telemetry/session_*.csv -> Saved/Review/telemetry_*.png).
#   powershell -File Tools/Thesis/telemetry_test.ps1 [-Camps "2,4,6,8"] [-Every 40] [-Extra "cmd, cmd"]
param([string]$Camps = "2,4,6,8", [int]$Every = 40, [string]$Extra = "")
$proj = "F:\UE_project\YggdrasilsPeak"
$sav = "$proj\Saved\SaveGames\Expedition.sav"; $savBak = "$env:TEMP\ygg_expedition_keep_tel.sav"
if (Test-Path $sav) { Copy-Item $sav $savBak -Force }
$before = @(Get-ChildItem "$proj\Saved\Telemetry" -Filter *.csv -ErrorAction SilentlyContinue | ForEach-Object FullName)
$t = 15; $cmds = @()
foreach ($c in ($Camps -split ',')) { $cmds += "ygg.Later $t ygg.Expedition.Go $c"; $t += $Every }
$cmds += "ygg.Later $t quit"
$exec = ($cmds -join ", ")
if ($Extra) { $exec = "$exec, $Extra" }
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1280", "-ResY=720",
    "-nosplash", "-NoSound", "-unattended", "-log=Telemetry.log", "-ExecCmds=`"$exec`"")
$proc.WaitForExit(($t + 120) * 1000) | Out-Null
if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force; "TIMEOUT (killed)" }
if (Test-Path $savBak) { Copy-Item $savBak $sav -Force }
$new = Get-ChildItem "$proj\Saved\Telemetry" -Filter *.csv | Where-Object { $before -notcontains $_.FullName } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $new) { "NO SESSION FILE"; exit 1 }
"session: $($new.FullName)"
python -I "$proj\Tools\Thesis\plot_session.py" $new.FullName
