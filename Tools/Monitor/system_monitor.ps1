# The computer's side of a play session (user 2026-11-13: "monitor me play the game... literally everything").
# The game records itself (Saved/Monitor/<date_time>/, UYggSessionMonitor); this watches what the game cannot see:
# the whole CPU and every core, the GPU (load, VRAM, clocks, temperature, power, why it slows down), memory and paging,
# the disk, and the other programs running at the same time (a second Unreal, a browser, a test run...).
# Started hidden by Play_Game.bat. Waits for the game, samples about once a second while it runs, then moves its file
# into the game's session folder and runs the analyser (Tools/Monitor/analyze_session.py).
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tools/Monitor/system_monitor.ps1
param([int]$WaitSeconds = 180)
$ErrorActionPreference = "SilentlyContinue"
[Threading.Thread]::CurrentThread.CurrentCulture = [Globalization.CultureInfo]::InvariantCulture
$proj = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$monDir = Join-Path $proj "Saved\Monitor"
New-Item -ItemType Directory -Force $monDir | Out-Null
$startedAt = Get-Date
$stamp = $startedAt.ToString("yyyyMMdd_HHmmss")
$out = Join-Path $monDir "system_$stamp.csv"
$smi = "C:\Windows\System32\nvidia-smi.exe"
$cores = [Environment]::ProcessorCount

function Find-Game {
    $p = Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe' OR Name LIKE 'YggdrasilsPeak%'" |
        Where-Object { $_.Name -like "YggdrasilsPeak*" -or ($_.CommandLine -match "YggdrasilsPeak" -and $_.CommandLine -match "\s-game") } |
        Select-Object -First 1
    return $p
}

# wait for the game to start
$game = $null
for ($i = 0; $i -lt $WaitSeconds -and -not $game; $i++) { $game = Find-Game; if (-not $game) { Start-Sleep 1 } }
if (-not $game) { exit 0 }
$gid = [int]$game.ProcessId
$gp = Get-Process -Id $gid

"epoch,cpu_total,cpu_core_max,cpu_cores_over90,game_cpu,game_ram_mb,game_private_mb,game_threads,ram_free_mb,commit_pct,hard_faults_s,disk_busy,disk_mb_s,gpu_util,gpu_mem_util,vram_mb,vram_total_mb,gpu_clock,mem_clock,gpu_temp,gpu_power_w,gpu_pstate,gpu_slowdown,editors,other_games,top_others" |
    Out-File -Encoding utf8 $out

$counters = @('\Processor(*)\% Processor Time', '\Memory\Available MBytes', '\Memory\% Committed Bytes In Use',
              '\Memory\Pages Input/sec', '\PhysicalDisk(_Total)\% Disk Time', '\PhysicalDisk(_Total)\Disk Bytes/sec')
$lastCpu = $gp.TotalProcessorTime.TotalSeconds
$lastT = [DateTime]::UtcNow
$prevOthers = @{}
$topOthers = ""
$n = 0
# slowdown reasons nvidia-smi reports as a bit mask
$reasons = @{ 0x1 = "idle"; 0x2 = "app-clocks"; 0x4 = "power-cap"; 0x8 = "hw-slowdown"; 0x20 = "sw-thermal"; 0x40 = "hw-thermal"; 0x80 = "power-brake"; 0x100 = "display-clocks" }

while (-not $gp.HasExited) {
    $c = (Get-Counter -Counter $counters -SampleInterval 1 -MaxSamples 1).CounterSamples
    if ($gp.HasExited) { break }
    $gp.Refresh()
    $now = [DateTime]::UtcNow
    $epoch = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds() / 1000.0
    $cpuNow = $gp.TotalProcessorTime.TotalSeconds
    $gameCpu = 100.0 * ($cpuNow - $lastCpu) / [Math]::Max(0.001, ($now - $lastT).TotalSeconds) / $cores
    $lastCpu = $cpuNow; $lastT = $now

    $coreVals = @($c | Where-Object { $_.Path -match "processor\((\d+)\)" } | ForEach-Object CookedValue)
    $total = ($c | Where-Object { $_.Path -match "processor\(_total\)" }).CookedValue
    $coreMax = if ($coreVals.Count) { ($coreVals | Measure-Object -Maximum).Maximum } else { 0 }
    $over90 = @($coreVals | Where-Object { $_ -gt 90 }).Count
    $free = ($c | Where-Object { $_.Path -match "available mbytes" }).CookedValue
    $commit = ($c | Where-Object { $_.Path -match "committed bytes in use" }).CookedValue
    $faults = ($c | Where-Object { $_.Path -match "pages input" }).CookedValue
    $disk = ($c | Where-Object { $_.Path -match "% disk time" }).CookedValue
    $diskMB = ($c | Where-Object { $_.Path -match "disk bytes/sec" }).CookedValue / 1MB

    $g = (& $smi --query-gpu=utilization.gpu,utilization.memory,memory.used,memory.total,clocks.gr,clocks.mem,temperature.gpu,power.draw,pstate,clocks_event_reasons.active --format=csv,noheader,nounits) -split ",\s*"
    $slow = ""
    if ($g.Count -ge 10) {
        $mask = [Convert]::ToInt64($g[9].Trim(), 16)
        $slow = ($reasons.Keys | Sort-Object | Where-Object { $mask -band $_ } | ForEach-Object { $reasons[$_] }) -join "+"
    }

    # every 5 s: who else uses the CPU (a second Unreal, a browser, Windows Update...) and how many editors are open
    if ($n % 5 -eq 0) {
        $procs = Get-Process | Where-Object { $_.Id -ne $gid -and $_.Id -ne $PID }
        $cur = @{}
        foreach ($p in $procs) { if ($p.TotalProcessorTime) { $cur[$p.Id] = @($p.ProcessName, $p.TotalProcessorTime.TotalSeconds) } }
        $use = foreach ($k in $cur.Keys) { if ($prevOthers.ContainsKey($k)) { [pscustomobject]@{ N = $cur[$k][0]; P = 100.0 * ($cur[$k][1] - $prevOthers[$k][1]) / 5.0 / $cores } } }
        $topOthers = (($use | Group-Object N | ForEach-Object { [pscustomobject]@{ N = $_.Name; P = ($_.Group | Measure-Object P -Sum).Sum } } |
            Sort-Object P -Descending | Select-Object -First 5 | Where-Object { $_.P -ge 1.5 } | ForEach-Object { "{0}:{1:N0}" -f $_.N, $_.P }) -join " ")
        $prevOthers = $cur
        $ue = @(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'" | Where-Object { $_.ProcessId -ne $gid })
        $editors = @($ue | Where-Object { $_.CommandLine -notmatch "\s-game" }).Count
        $otherGames = @($ue | Where-Object { $_.CommandLine -match "\s-game" }).Count
    }
    $n++

    $row = "{0:F3},{1:F1},{2:F1},{3},{4:F1},{5:F0},{6:F0},{7},{8:F0},{9:F1},{10:F1},{11:F1},{12:F1},{13},{14},{15},{16},{17},{18},{19},{20},{21},{22},{23},{24},`"{25}`"" -f `
        $epoch, $total, $coreMax, $over90, $gameCpu, ($gp.WorkingSet64 / 1MB), ($gp.PrivateMemorySize64 / 1MB), $gp.Threads.Count, $free, $commit, $faults,
        [Math]::Min(100, $disk), $diskMB, $g[0], $g[1], $g[2], $g[3], $g[4], $g[5], $g[6], $g[7], $g[8], $slow, $editors, $otherGames, $topOthers
    $row | Out-File -Encoding utf8 -Append $out
}

# the game has closed: put this file with the game's own recording and write the report
Start-Sleep 2
$session = Get-ChildItem $monDir -Directory | Where-Object { $_.CreationTime -ge $startedAt.AddSeconds(-5) } | Sort-Object CreationTime | Select-Object -Last 1
if ($session) {
    Move-Item $out (Join-Path $session.FullName "system.csv") -Force
    python -I (Join-Path $PSScriptRoot "analyze_session.py") $session.FullName *> (Join-Path $session.FullName "analyzer_output.txt")
}
