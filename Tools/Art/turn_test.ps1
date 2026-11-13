# Turning hitches (user 2026-11-13: "when you turn, a flash, the FPS drops to 20-30 and comes back"): stands at a spot,
# turns the view in small steps (15 degrees every 0.1 s, two full turns, twice), records every frame time with the CSV
# profiler and lists the spikes, plus the log lines around shader/PSO compiles and streaming.
#   powershell -File Tools/Art/turn_test.ps1 -Name:dingboche -X:338533 -Y:2468724 -Z:-68990 [-Pitch:5] [-Extra "cmd, cmd"]
param([string]$Name = "turn", [double]$X, [double]$Y, [double]$Z, [double]$Pitch = 5, [string]$Extra = "", [int]$ResX = 1920, [int]$ResY = 1080)
$proj = "F:\UE_project\YggdrasilsPeak"
$csvDir = "$env:LOCALAPPDATA\UnrealEngine\5.8\Saved\Profiling\CSV"
$before = @(Get-ChildItem $csvDir -Filter *.csv -ErrorAction SilentlyContinue | ForEach-Object FullName)
$cmds = @("t.MaxFPS 0", "r.VSync 0", "BugItGo $X $Y $Z $Pitch 0 0", "ygg.Later 12 csvprofile start")
$t = 13.0
for ($rep = 0; $rep -lt 2; $rep++) {
    for ($k = 0; $k -le 48; $k++) { $cmds += "ygg.Later $([math]::Round($t, 2)) BugItGo $X $Y $Z $Pitch $($k * 15) 0"; $t += 0.1 }
    $t += 1.0
}
$cmds += "ygg.Later $([math]::Round($t + 1, 2)) csvprofile stop"
$cmds += "ygg.Later $([math]::Round($t + 4, 2)) quit"
if ($Extra) { $cmds = @($Extra -split ',\s*') + $cmds }
$exec = $cmds -join ", "
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY",
    "-nosplash", "-NoSound", "-unattended", "-YggNoSave", "-log=Turn.log", "-ExecCmds=`"$exec`"")
$proc.WaitForExit(([int]$t + 120) * 1000) | Out-Null
if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force; "TIMEOUT" }
Start-Sleep 2
$new = Get-ChildItem $csvDir -Filter *.csv | Where-Object { $before -notcontains $_.FullName } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $new) { "NO CSV"; exit 1 }
python -I -c "
import csv, sys
csv.field_size_limit(10**9)
rows = list(csv.reader(open(r'$($new.FullName)', newline='', encoding='utf-8', errors='replace')))
h = rows[0]; d = [r for r in rows[1:] if len(r) == len(h)]
fi = h.index('FrameTime'); gi = h.index('GameThreadTime') if 'GameThreadTime' in h else None; ri = h.index('RenderThreadTime') if 'RenderThreadTime' in h else None
gp = h.index('GPUTime') if 'GPUTime' in h else None
ft = [float(r[fi]) for r in d]
s = sorted(ft); n = len(s)
print(f'$Name : {n} frames  median {s[n//2]:.2f} ms  p99 {s[int(n*0.99)]:.2f} ms  max {s[-1]:.2f} ms  frames over 33 ms: {sum(1 for v in ft if v > 33.3)}  over 50 ms: {sum(1 for v in ft if v > 50)}')
spikes = sorted(range(n), key=lambda i: -ft[i])[:8]
for i in sorted(spikes):
    g = float(d[i][gi]) if gi is not None else 0; r = float(d[i][ri]) if ri is not None else 0; p = float(d[i][gp]) if gp is not None else 0
    print(f'   frame {i:5d}: {ft[i]:7.1f} ms   game {g:6.1f}  render {r:6.1f}  gpu {p:6.1f}')
"
Select-String -Path "$proj\Saved\Logs\Turn.log" -Pattern "PSO|Hitch|hitch|ShaderCompil|Compiling shader|streaming pool|Streaming|over budget" | Select-Object -First 12 | ForEach-Object { $_.Line.Substring(0, [Math]::Min(170, $_.Line.Length)) }
