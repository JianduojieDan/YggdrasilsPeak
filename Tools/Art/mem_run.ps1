# Standalone -game memory measurement: Low Level Memory Tracker CSV (per category) + process peak RAM.
#   powershell -File Tools/Art/mem_run.ps1 <name> [frames] [extra ExecCmds]
# Spawns at the level's PlayerStart (Dingboche), runs <frames> frames (default 1500, ~20 s), then exits.
# Output: newest LLM CSV in %LOCALAPPDATA%\UnrealEngine\5.8\Saved\Profiling\LLM (UE 5.8 writes there, not the project),
# summarised by Tools/Art/llm_summary.py.
param([string]$Name, [int]$Frames = 1500, [string]$Extra = "")
$proj = "F:\UE_project\YggdrasilsPeak"
$llmDir = "$env:LOCALAPPDATA\UnrealEngine\5.8\Saved\Profiling\LLM"
$before = @(Get-ChildItem $llmDir -Filter *.csv -ErrorAction SilentlyContinue | ForEach-Object FullName)
$cmds = "t.MaxFPS 0, r.VSync 0"
if ($Extra) { $cmds = "$cmds, $Extra" }
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1920", "-ResY=1080",
    "-nosplash", "-NoSound", "-log=MemRun.log", "-llm", "-llmcsv", "-csvCaptureFrames=$Frames", "-ExitAfterCsvProfiling",
    "-ExecCmds=`"$cmds`"")
$peak = 0
while (-not $proc.HasExited) {
    try { $p = Get-Process -Id $proc.Id -ErrorAction Stop; if ($p.WorkingSet64 -gt $peak) { $peak = $p.WorkingSet64 } } catch {}
    Start-Sleep -Milliseconds 500
    if (((Get-Date) - $proc.StartTime).TotalSeconds -gt 600) { Stop-Process -Id $proc.Id -Force; "TIMEOUT"; break }
}
$new = Get-ChildItem $llmDir -Filter *.csv -ErrorAction SilentlyContinue | Where-Object { $before -notcontains $_.FullName } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
"$Name : process peak working set $([math]::Round($peak/1GB,2)) GB"
if ($new) { "LLM CSV: $($new.FullName)"; python "$proj\Tools\Art\llm_summary.py" $new.FullName } else { "NO LLM CSV" }
