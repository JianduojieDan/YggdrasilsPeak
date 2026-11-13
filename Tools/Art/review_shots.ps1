# The round-2 review pictures, one standalone run each (Tools/Art/shot.ps1), then contact sheets in Saved/Review.
#   powershell -File Tools/Art/review_shots.ps1 [names...]
param([string[]]$Only = @())
$proj = "F:\UE_project\YggdrasilsPeak"
Set-Location $proj
$Only = @($Only | ForEach-Object { $_ -split ',' } | Where-Object { $_ })   # "a,b" arrives as one string through -File
$def = "Graphics=-1;Upscaling=2;Upscaler=0;RayTracing=1;Hud=0"
$shots = @(
    # name, x, y, z, pitch, yaw, -Set, -Sg, extra, wait
    @("snow_old",   338700, 2468650, -69000, -32, 200, "Graphics=0;Upscaling=4;Upscaler=1", 0, "", 14),
    @("snow_new",   338700, 2468650, -69000, -32, 200, $def, 2, "", 14),
    @("hud_watch",  338533, 2468724, -68990, -6, 200, $def, 2, "", 14),
    @("hud_storm",  338533, 2468724, -68990, -6, 200, $def, 2, "ygg.Realm.StartTime 1000, ygg.Realm.Force Jotunheim", 14),
    @("hud_quiet",  338533, 2468724, -68990, -6, 200, "$def;Hud=1", 2, "", 14),
    @("pack",       338533, 2468724, -68990, -6, 200, $def, 2, "ygg.Later 8 ygg.Pack", 13),
    @("trees_4800", 154111, 2041970, -19700, 0, 120, $def, 2, "", 28),
    @("trees_6000", 779675, 1422934, 102760, 0, 40, $def, 2, "", 28),
    @("trees_valley", 338533, 2468724, -68500, -8, 230, $def, 2, "", 28),
    @("clumps",     154111, 2041970, -19700, -40, 120, $def, 2, "ygg.Later 3 ygg.Snow.AutoWalk 0.5, ygg.Realm.LookAt 4", 9),
    @("prints",     154111, 2041970, -19700, -40, 120, $def, 2, "ygg.Later 2 ygg.Snow.AutoWalk 0.5, ygg.Later 8 ygg.Snow.AutoYaw 300, ygg.Realm.LookAt 4", 14)
)
foreach ($s in $shots) {
    if ($Only.Count -and $Only -notcontains $s[0]) { continue }
    # an empty string argument is dropped by powershell -File: pass -Extra only when there is one
    # numbers as -Name:value: a bare "-19700" is read by powershell -File as a parameter NAME and everything after it is
    # lost (2026-11-13: every shot and perf run with a negative height ran at z = 0, hundreds of metres up)
    $a = @("-File", "Tools/Art/shot.ps1", "-Name:$($s[0])", "-X:$($s[1])", "-Y:$($s[2])", "-Z:$($s[3])", "-P:$($s[4])", "-W:$($s[5])", "-Set", $s[6], "-Sg:$($s[7])", "-Wait:$($s[9])")
    if ($s[8]) { $a += @("-Extra", $s[8]) }
    if ($s[0] -eq "clumps" -or $s[0] -eq "prints") { $a += "-NoGhost" }
    if ($s[0] -like "hud*" -or $s[0] -eq "pack") { $a += "-UI" }   # HighResShot leaves the HUD and menus out
    $o = & powershell @a
    "$($s[0]): $o"
}
