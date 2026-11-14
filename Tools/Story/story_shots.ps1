# Chapter 1 self-test: standalone runs that jump to a beat and take one HUD screenshot each -> Saved/Compare/story_<name>.png
#   powershell -File Tools/Story/story_shots.ps1 [-Only "prologue,room"]
param([string]$Only = "")
$proj = "F:\UE_project\YggdrasilsPeak"
$P = Get-Content "$proj\Content\Story\Chapter1_places.ygs" -Raw | ConvertFrom-Json
function At($name, $pitch = 0) { $v = $P.$name; return @($v[0], $v[1], $v[2] + 120, $pitch, $v[3]) }
$runs = [ordered]@{
    "prologue" = @{ at = (At "balcony_start" 5); cmds = "ygg.Later 2 ygg.Story.Start Chapter1"; wait = 15 }
    "title"    = @{ at = (At "balcony_start" 5); cmds = "ygg.Story.Debug 1, ygg.Later 2 ygg.Story.Jump slip"; wait = 13 }
    "room"     = @{ at = (At "room_start" -5); cmds = "ygg.Later 2 ygg.Story.Jump dawn"; wait = 15 }
    "plume"    = @{ at = (At "kneel_spot" 0); cmds = "ygg.Realm.StartTime 1735, ygg.Later 3 ygg.Story.Jump stone"; wait = 19 }
    "dolma"    = @{ at = (At "room_out" 0); cmds = "ygg.Later 2 ygg.Story.Jump dolma_calls"; wait = 9 }
    "people"   = @{ at = (At "marek_spot" 0); cmds = "ygg.Later 2 ygg.Story.Jump team, ygg.Later 4 ygg.Snow.AutoPitch -8, ygg.Later 4 ygg.Snow.AutoYaw 200"; wait = 10 }
    "plumeview"= @{ at = @(($P.kneel_spot[0]), ($P.kneel_spot[1]), ($P.kneel_spot[2] + 170), 26, -22.8); cmds = "ygg.Realm.StartTime 1735, ygg.Later 4 ygg.Story.Param plume Glow 1"; wait = 12 }
    "team"     = @{ at = @(($P.nima_spot[0] - 700), ($P.nima_spot[1]), ($P.nima_spot[2] + 130), -4, 0); cmds = "ygg.Later 2 ygg.Story.Jump team"; wait = 8 }
    "roomunlit"= @{ at = (At "room_start" -5); cmds = "ygg.Later 2 ygg.Story.Jump dawn, ygg.Later 8 viewmode unlit"; wait = 13 }
    "page"     = @{ at = (At "night_tent" 0); cmds = "ygg.Realm.StartTime 2030, ygg.Later 3 ygg.Story.Jump night"; wait = 46 }
}
foreach ($k in $runs.Keys) {
    if ($Only -and ($Only -split ',') -notcontains $k) { continue }
    $r = $runs[$k]; $a = $r.at
    & "$proj\Tools\Art\shot.ps1" -Name "story_$k" -X $a[0] -Y $a[1] -Z $a[2] -P $a[3] -W $a[4] -Extra $r.cmds -Wait $r.wait -NoGhost -UI
    Copy-Item "$proj\Saved\Logs\Shot.log" "$proj\Saved\Compare\story_$k.log" -Force
    "done $k"
}
