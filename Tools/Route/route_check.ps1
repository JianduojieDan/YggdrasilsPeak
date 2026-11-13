# The route check without walking: the standalone game jumps along the climbing route every -Spacing metres and checks
# the ground, every object within 40 m and the guidance ahead, with a picture (ygg.RouteCheck,
# Source/YggdrasilsPeak/Survival/YggRouteCheck.*). Then Tools/Route/analyze_route_check.py writes report.md and sheets.
#   powershell -File Tools/Route/route_check.ps1 [-Spacing:200] [-FirstLeg:0] [-LastLeg:7] [-Shots:1] [-Settle:3]
#       [-Time:1100] [-Radius:40] [-Extra "cmd, cmd"] [-ResX:1280 -ResY:720] [-TimeoutMin:0] [-NoAnalyse] [-Sound]
#   -> Saved/RouteCheck/<yyyyMMdd_HHmmss>/  points.csv objects.csv shots/ log.txt done.txt report.md sheet_NN.jpg
# Shots: 0 none, 1 along the route, 2 also looking back. Time: the time of day held at every point (0..2400).
# Negative numbers need the colon form (powershell -File reads "-1" as a parameter name): -Time:-1 lets the clock run.
# Whole route at 200 m: ~140 points, ~15-20 minutes. One heavy process: close the editor first.
param([double]$Spacing = 200, [int]$FirstLeg = 0, [int]$LastLeg = 99, [int]$Shots = 1, [double]$Settle = 3, [double]$Time = 1100,
      [double]$Radius = 40, [string]$Extra = "", [int]$ResX = 1280, [int]$ResY = 720, [int]$TimeoutMin = 0, [switch]$NoAnalyse, [switch]$Sound)
$proj = "F:\UE_project\YggdrasilsPeak"
$exe = "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$inv = [Globalization.CultureInfo]::InvariantCulture
function Num([double]$v) { $v.ToString($inv) }     # "2.5", never "2,5", whatever the machine's language

# the game writes into the project's Saved/; UE 5.8 sends some -game output to the user's AppData instead, so look there too
$roots = @("$proj\Saved\RouteCheck", "$env:LOCALAPPDATA\UnrealEngine\5.8\Saved\RouteCheck")
$before = @{}
foreach ($r in $roots) { Get-ChildItem $r -Directory -ErrorAction SilentlyContinue | ForEach-Object { $before[$_.FullName] = 1 } }
# test runs neither load nor write the save (-YggNoSave); keep a copy anyway, like the other run scripts
$sav = "$proj\Saved\SaveGames\Expedition.sav"; $savBak = "$env:TEMP\ygg_expedition_keep_route.sav"
if (Test-Path $sav) { Copy-Item $sav $savBak -Force }

$cmds = @(
    "ygg.Realm.Force none",                       # ordinary weather: no realm storm or white-out in the pictures
    "ygg.RouteCheck.QuitWhenDone 1",
    "ygg.RouteCheck.Settle $(Num $Settle)",
    "ygg.RouteCheck.Time $(Num $Time)",
    "ygg.RouteCheck.Radius $(Num $Radius)")
if ($Extra) { $cmds += @($Extra -split ',\s*' | Where-Object { $_ }) }
$cmds += "ygg.RouteCheck $(Num $Spacing) $FirstLeg $LastLeg $Shots"
$exec = $cmds -join ", "
$gameArgs = @("$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY",
    "-nosplash", "-unattended", "-YggNoSave", "-log=RouteCheck.log", "-ExecCmds=`"$exec`"")
if (-not $Sound) { $gameArgs += "-NoSound" }

$t0 = Get-Date
"route check: spacing $Spacing m, legs $FirstLeg..$LastLeg, shots $Shots"
"exec: $exec"
$proc = Start-Process -FilePath $exe -PassThru -ArgumentList $gameArgs
# ~28 km of route; a point takes Settle..3x Settle (streaming) plus ~2 s of objects and ~1-2 s per picture; the game needs
# 1-2 minutes to start. The limit follows -Settle, so a longer settle is not cut off near the end of the route.
$points = [Math]::Ceiling(28000.0 / [Math]::Max($Spacing, 10)) + 10
$limit = if ($TimeoutMin -gt 0) { $TimeoutMin * 60 } else { 300 + $points * (3 * [Math]::Max($Settle, 0.5) + 4 + 2 * $Shots) }
$dir = $null; $done = $false; $lastNote = 0
while (((Get-Date) - $t0).TotalSeconds -lt $limit) {
    Start-Sleep -Seconds 5
    if (-not $dir) {
        foreach ($r in $roots) {
            $new = Get-ChildItem $r -Directory -ErrorAction SilentlyContinue | Where-Object { -not $before.ContainsKey($_.FullName) } |
                Sort-Object LastWriteTime -Descending | Select-Object -First 1
            if ($new) { $dir = $new.FullName; "run folder: $dir"; break }
        }
    }
    if ($dir -and (Test-Path "$dir\done.txt")) { $done = $true; break }
    if ($proc.HasExited) { break }
    $sec = [int]((Get-Date) - $t0).TotalSeconds
    if ($dir -and $sec - $lastNote -ge 60) {
        $lastNote = $sec
        $n = 0
        if (Test-Path "$dir\points.csv") { $n = [Math]::Max(0, @(Get-Content "$dir\points.csv").Count - 1) }
        "$([int]($sec / 60)) min: $n points checked"
    }
}
if ($done) { $proc.WaitForExit(60000) | Out-Null }      # it quits by itself once done.txt is written
if (-not $proc.HasExited) {
    if (-not $done) { "TIMEOUT after $limit s: stopping the game" }
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
}
if (Test-Path $savBak) { Copy-Item $savBak $sav -Force }
$mins = [Math]::Round(((Get-Date) - $t0).TotalMinutes, 1)
if (-not $dir) { "NO RUN FOLDER: did the game start? See $env:LOCALAPPDATA\UnrealEngine\5.8\Saved\Logs\RouteCheck.log ($mins min)"; exit 1 }
if ($done) { Get-Content "$dir\done.txt" } else { "no done.txt: the run did not finish (see $dir\log.txt)" }
"$mins min"
if (-not $NoAnalyse) { python -I "$proj\Tools\Route\analyze_route_check.py" "$dir" }
