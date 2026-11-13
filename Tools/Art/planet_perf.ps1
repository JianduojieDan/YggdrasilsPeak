# Frame time while LOOKING AT each realm planet (2026-11-13, user: "whenever you look at the planet the FPS drops").
#   powershell -File Tools/Art/planet_perf.ps1 [realm list] [x y z]
# For each realm: standalone -game at a fixed spot and clock (10:00), ygg.Realm <name>, ygg.Realm.LookAt 3 holds the
# camera on the planet, 600 frames captured after warm-up with GPU pass stats; prints frame time + top GPU passes.
# "none" = the same spot and clock with no planet, camera on the sun's direction (the reference).
param([string]$Realms = "none,Midgard,Niflheim,Muspelheim,Jotunheim,Alfheim,Vanaheim,Svartalfheim,Hel,Asgard",
      [double]$X = 1013070, [double]$Y = 1516502, [double]$Z = 145100, [string]$Tag = "", [string]$Extra = "", [int]$ResX = 1920, [int]$ResY = 1080)
$proj = "F:\UE_project\YggdrasilsPeak"
$sav = "$proj\Saved\SaveGames\Expedition.sav"; $savBak = "$env:TEMP\ygg_expedition_keep.sav"
if (Test-Path $sav) { Copy-Item $sav $savBak -Force }
$csvDir = "$env:LOCALAPPDATA\UnrealEngine\5.8\Saved\Profiling\CSV"
foreach ($realm in $Realms.Split(',')) {
    $before = @(Get-ChildItem $csvDir -Filter *.csv -ErrorAction SilentlyContinue | ForEach-Object FullName)
    $look = if ($realm -eq 'none') { 1 } else { 3 }
    $cmds = "Ghost, BugItGo $X $Y $Z 0 0 0, ygg.Realm.Force $realm, ygg.Realm.LookAt $look, t.MaxFPS 0, r.VSync 0, r.GPUCsvStatsEnabled 1$Extra"
    $proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
        "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY",
        "-nosplash", "-NoSound", "-unattended", "-YggNoSave", "-log=PlanetPerf.log", "-csvCaptureFrames=600", "-csvStartFrame=900", "-ExitAfterCsvProfiling",
        "-ExecCmds=`"ygg.Realm.StartTime 1000, $cmds`"")
    $proc.WaitForExit(420000) | Out-Null
    if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force; "$realm TIMEOUT"; continue }
    $new = Get-ChildItem $csvDir -Filter *.csv | Where-Object { $before -notcontains $_.FullName } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $new) { "$realm NO CSV"; continue }
    python "$proj\Tools\Art\csv_frames.py" $new.FullName "$Tag$realm"
    python "$proj\Tools\Art\csv_gpu.py" $new.FullName "$Tag$realm" 8
}
if (Test-Path $savBak) { Copy-Item $savBak $sav -Force }
