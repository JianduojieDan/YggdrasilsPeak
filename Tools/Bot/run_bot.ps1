# The robot player: the standalone game plays a chapter by itself (ygg.Bot.Run, Source/YggdrasilsPeak/Survival/YggPlayBot.*)
# - story auto-answers, walks to the next story place, camera stress, stuck teleports - then writes the report and quits.
#   powershell -File Tools/Bot/run_bot.ps1 [-Chapter chapter1] [-Fast] [-TimeoutMin 95] [-Extra "ygg.Bot.CapMin 30, ygg.Bot.Camera 0"]
#       [-ResX 1280 -ResY 720] [-Delay 3] [-UseSave] [-NoMonitor] [-NoCamRec] [-Sound]
#   -> Saved/Bot/<yyyyMMdd_HHmmss>/report.md (progress.md every 60 s while it runs, done.txt at the end)
# -Fast: t.MaxFPS 0 and the walk at sprint. -Extra: console commands run at the start (comma separated), e.g. ygg.Bot.* cvars.
# Test runs neither load nor write the player's save (-YggNoSave; -UseSave drops it) and keep the session monitor on
# (-YggMonitor; -NoMonitor turns it off) and the camera flight recorder on (-YggCamRec: -YggNoSave would switch it off;
# its incidents are counted in the report; -NoCamRec leaves it off).
# One heavy process: close the editor first. The run takes up to ygg.Bot.CapMin (75) minutes of game time plus the start.
# -Chapter summit: the death-zone run instead (ygg.Bot.Summit, YggSummitBot.*): camp jump to the South Col, route leg 7 to the
#   summit, the summit beat, then down; -SummitArgs passes its options, e.g. "noo2 stay 15" or "down 300 cap 60".
#   -> Saved/Bot/summit_<time>_<o2|noo2>[_fast]/report.md, samples.csv; pictures Saved/Review/A5/.
# -Chapter route: the route walk (ygg.Bot.Route, YggRouteBot.*): camp jump to the first leg's camp, walks route.json legs
#   (default 0..3 = Dingboche -> Camp 1), checks every 200 m; -RouteArgs e.g. "from 2 to 3 speed 3" or "at 5200".
#   -> Saved/Bot/route_<time>_L<a>-<b>/report.md, samples.csv, points.csv; pictures Saved/Review/A4/.
param([string]$Chapter = "chapter1", [switch]$Fast, [int]$TimeoutMin = 95, [string]$Extra = "", [int]$ResX = 1280, [int]$ResY = 720,
      [double]$Delay = 3, [switch]$UseSave, [switch]$NoMonitor, [switch]$NoCamRec, [switch]$Sound, [string]$SummitArgs = "", [string]$RouteArgs = "")
$proj = "F:\UE_project\YggdrasilsPeak"
$exe = "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$inv = [Globalization.CultureInfo]::InvariantCulture
function Num([double]$v) { $v.ToString($inv) }     # "2.5", never "2,5", whatever the machine's language

# the game writes into the project's Saved/; UE 5.8 sends some -game output to the user's AppData instead, so look there too
$roots = @("$proj\Saved\Bot", "$env:LOCALAPPDATA\UnrealEngine\5.8\Saved\Bot")
$before = @{}
foreach ($r in $roots) { Get-ChildItem $r -Directory -ErrorAction SilentlyContinue | ForEach-Object { $before[$_.FullName] = 1 } }
# keep a copy of the save anyway, like the other run scripts
$sav = "$proj\Saved\SaveGames\Expedition.sav"; $savBak = "$env:TEMP\ygg_expedition_keep_bot.sav"
if (Test-Path $sav) { Copy-Item $sav $savBak -Force }

$run = if ($Chapter -eq "summit") { "ygg.Bot.Summit $SummitArgs".Trim() } elseif ($Chapter -eq "route") { "ygg.Bot.Route $RouteArgs".Trim() } else { "ygg.Bot.Run $Chapter" }
if ($Fast) { $run = "$run fast" }
$cmds = @()
if ($Extra) { $cmds += @($Extra -split ',\s*' | Where-Object { $_ }) }
$cmds += "ygg.Later $(Num $Delay) $run"             # -ExecCmds run before the world and the player exist
$exec = $cmds -join ", "
$gameArgs = @("$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY",
    "-nosplash", "-unattended", "-log=Bot.log", "-ExecCmds=`"$exec`"")
if (-not $Sound) { $gameArgs += "-NoSound" }
if (-not $UseSave) { $gameArgs += "-YggNoSave" }
if (-not $NoMonitor) { $gameArgs += "-YggMonitor" }
if (-not $NoCamRec) { $gameArgs += "-YggCamRec" }

$t0 = Get-Date
"robot run: $Chapter$(if ($Fast) { ' (fast)' }), timeout $TimeoutMin min"
"exec: $exec"
$proc = Start-Process -FilePath $exe -PassThru -ArgumentList $gameArgs
$limit = [Math]::Max(5, $TimeoutMin) * 60
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
    if ($dir -and $sec - $lastNote -ge 60 -and (Test-Path "$dir\progress.md")) {
        $lastNote = $sec
        $line = Get-Content "$dir\progress.md" -Encoding UTF8 -ErrorAction SilentlyContinue | Where-Object { $_ -like "- Now:*" } | Select-Object -First 1
        "$([int]($sec / 60)) min: $line"
    }
}
if ($done) { $proc.WaitForExit(60000) | Out-Null }      # it quits by itself once done.txt is written (ygg.Bot.Quit 1)
if (-not $proc.HasExited) {
    if (-not $done) { "TIMEOUT after $TimeoutMin min: stopping the game (no final report; progress.md has the last minute)" }
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
}
if (Test-Path $savBak) { Copy-Item $savBak $sav -Force }
$mins = [Math]::Round(((Get-Date) - $t0).TotalMinutes, 1)
if (-not $dir) {
    "NO RUN FOLDER: did the game start, and does the build have ygg.Bot.Run? See $proj\Saved\Logs\Bot.log or $env:LOCALAPPDATA\UnrealEngine\5.8\Saved\Logs\Bot.log ($mins min)"
    exit 1
}
$report = if (Test-Path "$dir\report.md") { "$dir\report.md" } else { "$dir\progress.md" }
""
"report: $report ($mins min)"
if (Test-Path $report) {
    $lines = @(Get-Content $report -Encoding UTF8)
    # the summary and the beat table, then the end of the report
    $cut = [Array]::FindIndex([string[]]$lines, [Predicate[string]] { param($l) $l -like "## Stuck*" })
    if ($cut -lt 0) { $cut = [Math]::Min(45, $lines.Count) }
    $lines[0..([Math]::Max(0, $cut - 1))]
    "..."
    $lines | Select-Object -Last 30
}
if ($done) {
    $why = (Get-Content "$dir\done.txt" -Encoding UTF8 | Select-Object -First 1)
    "result: $why"
    if ($why -like "*finished*") { exit 0 }
}
exit 1
