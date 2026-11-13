# Disk stall probe (2026-11-14). The second monitored play session froze for 2-5 s at a time while the disk counter
# read 100 % busy with almost no data moving, and the separate system monitor stalled at the same moments.
# This writes 4 KB with write-through + flush to C:, E: and F: once a second and logs every write slower than
# -SlowMs, so we learn which drive stalls (E: and F: are the ORICO NVMe, C: and D: the Samsung).
# Cost: 12 KB a second. Usage: powershell -NoProfile -File Tools/Monitor/disk_probe.ps1 [-Hours 10] [-SlowMs 50]
param([double]$Hours = 10, [int]$SlowMs = 50)

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$outDir = Join-Path $root 'Saved\Monitor'
New-Item -ItemType Directory -Force $outDir | Out-Null
$out = Join-Path $outDir ("disk_probe_{0}.csv" -f (Get-Date -Format 'yyyyMMdd_HHmmss'))
'time,drive,ms' | Out-File -Encoding ascii $out

$targets = @(
    @{ Drive = 'C'; Path = Join-Path $env:TEMP 'ygg_disk_probe.bin' },
    @{ Drive = 'E'; Path = 'E:\ygg_disk_probe.bin' },
    @{ Drive = 'F'; Path = Join-Path $outDir 'ygg_disk_probe.bin' }
)
$bytes = New-Object byte[] 4096
$end = (Get-Date).AddHours($Hours)
$worst = @{ C = 0.0; E = 0.0; F = 0.0 }
$lastSummary = Get-Date
while ((Get-Date) -lt $end) {
    foreach ($t in $targets) {
        $sw = [Diagnostics.Stopwatch]::StartNew()
        try {
            $fs = New-Object IO.FileStream($t.Path, [IO.FileMode]::Create, [IO.FileAccess]::Write, [IO.FileShare]::None, 4096, [IO.FileOptions]::WriteThrough)
            $fs.Write($bytes, 0, $bytes.Length)
            $fs.Flush($true)
            $fs.Dispose()
        } catch { }
        $ms = $sw.Elapsed.TotalMilliseconds
        if ($ms -gt $worst[$t.Drive]) { $worst[$t.Drive] = $ms }
        if ($ms -ge $SlowMs) {
            ('{0},{1},{2:N0}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss.fff'), $t.Drive, $ms) | Out-File -Append -Encoding ascii $out
        }
    }
    if (((Get-Date) - $lastSummary).TotalMinutes -ge 10) {
        ('{0},summary,worst C {1:N0} E {2:N0} F {3:N0}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $worst.C, $worst.E, $worst.F) | Out-File -Append -Encoding ascii $out
        $worst = @{ C = 0.0; E = 0.0; F = 0.0 }
        $lastSummary = Get-Date
    }
    Start-Sleep -Milliseconds 1000
}
foreach ($t in $targets) { Remove-Item -ErrorAction SilentlyContinue $t.Path }
