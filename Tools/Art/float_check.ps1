# Re-checks A6 "character floats above the snow": one game run, ygg.GroundProbe.Go to each A6 spot, wait, ygg.GroundProbe A6re_<spot>.
# Prints the new rows of Saved/Review/ground_probe.csv next to the old A6 row of the same spot, and PASS/FAIL per spot.
#   powershell -File Tools/Art/float_check.ps1 [-Wait:16] [-Hold:6]
# A6 thresholds (Development_Plan 2026-11-20 entry): walkable spots: boots over visible snow -8..+8 cm (A6 saw 0-7), mode Walking;
# sliding spots (>46 deg): boots -8..+25 cm (A6 saw 0 / +24 at 61 deg), not the 58-138 cm airborne pose;
# all spots: visible minus collision under the boots within +-5 cm (A6 saw 1-5).
param([int]$Wait = 16, [int]$Hold = 6)
$proj = "F:\UE_project\YggdrasilsPeak"
$csv = "$proj\Saved\Review\ground_probe.csv"
$sav = "$proj\Saved\SaveGames\Expedition.sav"; $savBak = "$env:TEMP\ygg_expedition_keep_float.sav"
if (Test-Path $sav) { Copy-Item $sav $savBak -Force }
# spot: name, x, y, z ('g' = on the ground there), kind, old A6 label
$spots = @(
    @("w36_r7_30", 1291108, 1508317, "g", "walk", "r7_30_stand"),
    @("w36_r6_40", 1263948, 1586655, "g", "walk", "r6_40_stand"),
    @("w33_r7_54", 1252108, 1470317, "g", "walk", "r7_54_stand"),
    @("w29_r2_84", 661014, 1326409, "g", "walk", "r2_84_stand"),
    @("f56_5302m", 533014, 1285409, "g", "walk", "e18"),
    @("s61_r6_3", 1173804, 1587771, "g", "slide", "r6_3_stand"),
    @("s56_r6_3w3", 1172977, 1588412, "g", "slide", "r6_3_walk3")
)
$before = if (Test-Path $csv) { @(Get-Content $csv).Count } else { 0 }
$t = $Wait; $cmds = @("t.MaxFPS 0", "r.VSync 0")
foreach ($s in $spots) {
    $cmds += "ygg.Later $t ygg.GroundProbe.Go $($s[1]) $($s[2]) $($s[3]) 0 0"
    $cmds += "ygg.Later $($t + 4) ygg.GroundProbe A6re_$($s[0])"
    if ($s[4] -eq "slide") { $cmds += "ygg.Later $($t + 5.5) ygg.GroundProbe A6re_$($s[0])_b" }
    $t += $Hold
}
$cmds += "ygg.Later $($t + 2) quit"
$exec = $cmds -join ", "
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1280", "-ResY=720",
    "-nosplash", "-NoSound", "-unattended", "-YggNoSave", "-log=Float.log", "-ExecCmds=`"$exec`"")
$proc.WaitForExit(($t + 90) * 1000) | Out-Null
if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force }
if (Test-Path $savBak) { Copy-Item $savBak $sav -Force }

$all = @(Import-Csv $csv)
$old = $all[0..([Math]::Max($before - 2, -1))]   # rows that existed before the run (header is line 1)
if ($before -le 1) { $old = @() }
$new = @($all | Select-Object -Skip ([Math]::Max($before - 1, 0)) | Where-Object { $_.label -like "A6re_*" })
$f = { param($v) "{0,7}" -f $v }
"{0,-16} {1,5} {2,-8} {3,17} {4,17} {5,17}" -f "spot", "slope", "mode", "boots over vis L/R", "vis-col L/R", "(old boots L/R)"
$allPass = $true
foreach ($s in $spots) {
    $rows = @($new | Where-Object { $_.label -eq "A6re_$($s[0])" -or $_.label -eq "A6re_$($s[0])_b" })
    $o = $old | Where-Object { $_.label -eq $s[5] } | Select-Object -Last 1
    if ($rows.Count -eq 0) { "{0,-16} NO ROW (teleport failed or run crashed): FAIL" -f $s[0]; $allPass = $false; continue }
    $pass = $true
    foreach ($r in $rows) {
        $l = [double]$r.sole_over_vis_l; $rr = [double]$r.sole_over_vis_r
        $vl = [double]$r.vis_minus_col_l; $vr = [double]$r.vis_minus_col_r
        $hi = if ($s[4] -eq "slide") { 25 } else { 8 }
        $ok = ($l -ge -8 -and $l -le $hi -and $rr -ge -8 -and $rr -le $hi -and [Math]::Abs($vl) -le 5 -and [Math]::Abs($vr) -le 5)
        if ($s[4] -eq "walk" -and $r.mode -ne "MOVE_Walking") { $ok = $false }
        if (-not $ok) { $pass = $false }
        $oldTxt = if ($o) { "($($o.sole_over_vis_l)/$($o.sole_over_vis_r), $($o.mode))" } else { "(no old row)" }
        "{0,-16} {1,5} {2,-8} {3,8}/{4,-8} {5,8}/{6,-8} {7}  {8}" -f $r.label.Substring(5), $r.slope, $r.mode.Replace("MOVE_", ""), $l, $rr, $vl, $vr, $oldTxt, $(if ($ok) { "ok" } else { "bad" })
    }
    "  -> $($s[0]): $(if ($pass) { 'PASS' } else { 'FAIL' })"
    if (-not $pass) { $allPass = $false }
}
"OVERALL: $(if ($allPass) { 'PASS' } else { 'FAIL' })   (log: Saved/Logs/Float.log)"
