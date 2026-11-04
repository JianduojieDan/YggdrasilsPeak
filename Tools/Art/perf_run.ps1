# Standalone -game frame-time measurement at a spot (same method as the 2026-10-21 baseline: ~120-127 FPS at spawn).
#   powershell -File Tools/Art/perf_run.ps1 <name> <x> <y> <z> <pitch> <yaw> [spindrift 0..1]
# Captures 600 frames after warm-up with the CSV profiler, prints median / p95 frame time. Run from PowerShell.
param([string]$Name, [double]$X, [double]$Y, [double]$Z, [double]$P, [double]$W, [double]$Spindrift = -1)
$proj = "F:\UE_project\YggdrasilsPeak"
$csvDir = "$proj\Saved\Profiling\CSV"
$before = @(Get-ChildItem $csvDir -Filter *.csv -ErrorAction SilentlyContinue | ForEach-Object FullName)
$cmds = "Ghost, BugItGo $X $Y $Z $P $W 0, ygg.Snow.Spindrift $Spindrift, t.MaxFPS 0, r.VSync 0"
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1920", "-ResY=1080",
    "-nosplash", "-NoSound", "-log=PerfRun.log", "-csvCaptureFrames=600", "-csvStartFrame=900", "-ExitAfterCsvProfiling",
    "-ExecCmds=`"$cmds`"")
$proc.WaitForExit(420000) | Out-Null
if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force; "TIMEOUT" }
$new = Get-ChildItem $csvDir -Filter *.csv | Where-Object { $before -notcontains $_.FullName } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $new) { "NO CSV"; exit }
$rows = Import-Csv $new.FullName | Where-Object { $_.FrameTime -match '^[0-9.]+$' }
$ft = $rows | ForEach-Object { [double]$_.FrameTime } | Sort-Object
$gpu = $rows | ForEach-Object { [double]$_.GPUTime } | Sort-Object
$med = $ft[[int]($ft.Count / 2)]; $p95 = $ft[[int]($ft.Count * 0.95)]
"$Name : frames $($ft.Count)  median $([math]::Round($med,2)) ms ($([math]::Round(1000/$med)) fps)  p95 $([math]::Round($p95,2)) ms  GPU median $([math]::Round($gpu[[int]($gpu.Count/2)],2)) ms"
