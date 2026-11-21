# Chapter 1 cast close-ups (2026-11-21): jump to "dawn" then "job" (the player is free; the team stands at its spots and
# starts to follow), then put the
# camera 1.6 m in front of each person at face height and take a shot, then a 3/4 view and a back view (hat, neck seam,
# collar, hair through the hat) -> Saved/Compare/closeups/<Name>_<who>_<view>.png
#   powershell -File Tools/Story/closeups.ps1 [-Name base] [-Only "nima,marek"]
param([string]$Name = "base", [string]$Only = "", [string]$Extra = "")
$proj = "F:\UE_project\YggdrasilsPeak"
$shots = "$proj\Saved\Screenshots\WindowsEditor"
$out = "$proj\Saved\Compare\closeups"
New-Item -ItemType Directory -Force $out | Out-Null
Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force
$P = Get-Content "$proj\Content\Story\Chapter1_places.ygs" -Raw | ConvertFrom-Json
$cast = [ordered]@{ nima = "nima_spot"; marek = "marek_spot"; elena = "elena_spot"; jiwoo = "jiwoo_spot"; pema = "aunt_spot" }
if ($Only) { $keep = $Only -split ','; $cast2 = [ordered]@{}; foreach ($k in $cast.Keys) { if ($keep -contains $k) { $cast2[$k] = $cast[$k] } }; $cast = $cast2 }
$c = @("t.MaxFPS 0", "ygg.Realm.Force none", "ygg.Realm.StartTime 0800", "ygg.Story.Auto 1", "ygg.Later 20 ygg.Story.Jump dawn", "ygg.Later 32 ygg.Story.Jump job")
$t = 44.0
$order = @()
foreach ($who in $cast.Keys) {
    $s = $P.($cast[$who])
    $yaw = [double]$s[3]
    foreach ($v in @(@("front", 0.0, 160.0), @("threeq", 40.0, 150.0), @("back", 180.0, 150.0))) {
        $a = ($yaw + $v[1]) * [math]::PI / 180.0
        $x = [double]$s[0] + [math]::Cos($a) * $v[2]; $y = [double]$s[1] + [math]::Sin($a) * $v[2]
        $z = [double]$s[2] + 150.0
        $face = $yaw + $v[1] + 180.0
        $c += "ygg.Later $t BugItGo $([math]::Round($x,1)) $([math]::Round($y,1)) $([math]::Round($z,1)) -6 $([math]::Round($face,1)) 0"
        $c += "ygg.Later $($t + 1.2) shot"
        $order += "${who}_$($v[0])"
        $t += 2.4
    }
}
$c += "ygg.Later $($t + 1) quit"
if ($Extra) { $c = @($Extra -split ',\s*') + $c }
$exec = $c -join ", "
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1600", "-ResY=900",
    "-nosplash", "-NoSound", "-unattended", "-YggNoSave", "-log=Closeups.log", "-ExecCmds=`"$exec`"")
$proc.WaitForExit(300000) | Out-Null
if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force; "TIMEOUT" }
$found = @(Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTime)
for ($i = 0; $i -lt $found.Count -and $i -lt $order.Count; $i++) { Copy-Item $found[$i].FullName "$out\${Name}_$($order[$i]).png" -Force }
"$Name : $($found.Count) of $($order.Count) shots"
