# Chapter 1 self-test: standalone runs that jump to a beat and take one HUD screenshot each -> Saved/Compare/story_<name>.png
#   powershell -File Tools/Story/story_shots.ps1 [-Only "prologue,room"]
param([string]$Only = "")
$proj = "F:\UE_project\YggdrasilsPeak"
$P = Get-Content "$proj\Content\Story\Chapter1_places.ygs" -Raw | ConvertFrom-Json
# parentheses around the sum: in PowerShell "," binds tighter than "+" (without them the pitch became 120)
function At($name, $pitch = 0) { $v = $P.$name; return @($v[0], $v[1], ($v[2] + 120), $pitch, $v[3]) }
$runs = [ordered]@{
    "prologue" = @{ at = (At "balcony_start" 5); cmds = "ygg.Later 2 ygg.Story.Start Chapter1"; wait = 15 }
    "title"    = @{ at = (At "balcony_start" 5); cmds = "ygg.Story.Debug 1, ygg.Later 2 ygg.Story.Jump slip"; wait = 13 }
    "room"     = @{ at = (At "room_start" -5); cmds = "ygg.Later 2 ygg.Story.Jump dawn"; wait = 15 }
    "plume"    = @{ at = (At "kneel_spot" 0); cmds = "ygg.Realm.StartTime 1735, ygg.Later 3 ygg.Story.Jump stone"; wait = 19 }
    "dolma"    = @{ at = (At "room_out" 0); cmds = "ygg.Later 2 ygg.Story.Jump dolma_calls"; wait = 9 }
    "people"   = @{ at = (At "marek_spot" 0); cmds = "ygg.Later 2 ygg.Story.Jump team, ygg.Later 4 ygg.Snow.AutoPitch -8, ygg.Later 4 ygg.Snow.AutoYaw 200"; wait = 10 }
    "plumeview"= @{ at = @(($P.kneel_spot[0]), ($P.kneel_spot[1]), ($P.kneel_spot[2] + 170), 26, -22.8); cmds = "ygg.Realm.StartTime 1735, ygg.Later 4 ygg.Story.Param plume Glow 1"; wait = 12 }
    "team"     = @{ at = @(($P.nima_spot[0] - 700), ($P.nima_spot[1]), ($P.nima_spot[2] + 130), -4, 0); cmds = "ygg.Later 2 ygg.Story.Jump team"; wait = 8 }
    "nimaclose"= @{ at = @(($P.nima_spot[0] - 230), ($P.nima_spot[1]), ($P.nima_spot[2] + 150), -6, 0); cmds = "ygg.Later 2 ygg.Story.Jump team"; wait = 11 }
    "auntclose"= @{ at = @(($P.aunt_spot[0] - 260), ($P.aunt_spot[1] + 60), ($P.aunt_spot[2] + 150), -6, -10); cmds = "ygg.Later 2 ygg.Story.Jump dolma_calls"; wait = 6 }
    "house"    = @{ at = @(341048, 2468279, -69024, -6, -104.6); cmds = "ygg.Realm.StartTime 0930"; wait = 8 }
    "housedoor"= @{ at = @(340260, 2467537, -69034, -4, -44.1); cmds = "ygg.Realm.StartTime 0930"; wait = 8 }
    "houseback"= @{ at = @(340490, 2465963, -68984, -6, 77.6); cmds = "ygg.Realm.StartTime 0930"; wait = 8 }
    "auntover" = @{ at = @(($P.aunt_spot[0] - 380), ($P.aunt_spot[1]), ($P.aunt_spot[2] + 420), -42, 0); cmds = "ygg.Later 2 ygg.Story.Jump dolma_calls"; wait = 7 }
    "c_nima"   = @{ at = @(339660, 2469134, -69122, -4, 139.5); cmds = "ygg.Story.Auto 1, ygg.Later 2 ygg.Story.Jump team"; wait = 9 }
    "c_marek"  = @{ at = @(339224, 2469625, -69084, -4, -56.3); cmds = "ygg.Story.Auto 1, ygg.Later 2 ygg.Story.Jump team"; wait = 9 }
    "c_elena"  = @{ at = @(339179, 2469520, -69084, -4, -36.9); cmds = "ygg.Story.Auto 1, ygg.Later 2 ygg.Story.Jump team"; wait = 9 }
    "c_jiwoo"  = @{ at = @(339537, 2469097, -69122, -4, 117.3); cmds = "ygg.Story.Auto 1, ygg.Later 2 ygg.Story.Jump team"; wait = 9 }
    "c_pema"   = @{ at = @(340779, 2467834, -69118, -6, -144.6); cmds = "ygg.Later 2 ygg.Story.Jump dolma_calls"; wait = 6 }
    "roomunlit"= @{ at = (At "room_start" -5); cmds = "ygg.Later 2 ygg.Story.Jump dawn, ygg.Later 8 viewmode unlit"; wait = 13 }
    "page"     = @{ at = (At "night_tent" 0); cmds = "ygg.Realm.StartTime 2030, ygg.Later 3 ygg.Story.Jump night"; wait = 46 }
}
foreach ($k in $runs.Keys) {
    if ($Only -and ($Only -split ',') -notcontains $k) { continue }
    $r = $runs[$k]; $a = $r.at
    & "$proj\Tools\Art\shot.ps1" -Name "story_$k" -X $a[0] -Y $a[1] -Z $a[2] -P $a[3] -W $a[4] -Extra ("r.Streaming.FullyLoadUsedTextures 1, " + $r.cmds) -Wait ($r.wait + 3) -NoGhost -UI
    Copy-Item "$proj\Saved\Logs\Shot.log" "$proj\Saved\Compare\story_$k.log" -Force
    "done $k"
}
