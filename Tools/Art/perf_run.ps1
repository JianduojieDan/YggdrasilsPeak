# Standalone -game frame-time measurement at a spot (same method as the 2026-10-21 baseline: ~120-127 FPS at spawn).
#   powershell -File Tools/Art/perf_run.ps1 <name> <x> <y> <z> <pitch> <yaw> [spindrift 0..1]
# Captures 600 frames after warm-up with the CSV profiler, prints median / p95 frame time. Run from PowerShell.
param([string]$Name, [double]$X, [double]$Y, [double]$Z, [double]$P, [double]$W, [double]$Spindrift = -1, [string]$Extra = "")
$proj = "F:\UE_project\YggdrasilsPeak"
$csvDir = "$env:LOCALAPPDATA\UnrealEngine\5.8\Saved\Profiling\CSV"   # UE 5.8 writes here, not the project
$before = @(Get-ChildItem $csvDir -Filter *.csv -ErrorAction SilentlyContinue | ForEach-Object FullName)
$cmds = "Ghost, BugItGo $X $Y $Z $P $W 0, ygg.Snow.Spindrift $Spindrift, t.MaxFPS 0, r.VSync 0"
if ($Extra) { $cmds = "$cmds, $Extra" }
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1920", "-ResY=1080",
    "-nosplash", "-NoSound", "-log=PerfRun.log", "-csvCaptureFrames=600", "-csvStartFrame=900", "-ExitAfterCsvProfiling",
    "-ExecCmds=`"$cmds`"")
$proc.WaitForExit(420000) | Out-Null
if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force; "TIMEOUT" }
$new = Get-ChildItem $csvDir -Filter *.csv | Where-Object { $before -notcontains $_.FullName } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $new) { "NO CSV"; exit }
python "$proj\Tools\Art\csv_frames.py" $new.FullName $Name
