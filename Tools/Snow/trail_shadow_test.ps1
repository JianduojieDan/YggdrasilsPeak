# Footprint shadow check (play-test 2026-11-14): walk 15 s through the Dingboche snow, turn round and look down at the
# trench, screenshot with the HUD. Runs once per ygg.Snow.ShadowRefresh mode -> Saved/Compare/trailshadow_<mode>.png
#   powershell -File Tools/Snow/trail_shadow_test.ps1 [-Modes "0,1"] [-Extra "cmd, cmd"]
param([string]$Modes = "0,1", [string]$Extra = "")
$proj = "F:\UE_project\YggdrasilsPeak"
foreach ($m in ($Modes -split ',')) {
    $cmds = "ygg.Snow.ShadowRefresh $m, ygg.Realm.Force none, ygg.Realm.StartTime 1500, ygg.Snow.AutoYaw -42, " +
            "ygg.Later 5 ygg.Snow.AutoWalk 0.5, ygg.Later 20 ygg.Snow.AutoWalk 0, ygg.Later 20.5 ygg.Snow.AutoYaw 138, " +
            "ygg.Later 20.5 ygg.Snow.AutoPitch -55"
    if ($Extra) { $cmds = "$cmds, $Extra" }
    & "$proj\Tools\Art\shot.ps1" -Name "trailshadow_$m" -X 336930 -Y 2472320 -Z (-69040) -P (-10) -W (-42) -Extra $cmds -Wait 27 -NoGhost -UI
    "done $m"
}
