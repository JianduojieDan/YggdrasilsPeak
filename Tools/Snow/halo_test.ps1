# #74 dark halos + straight-edged patches around characters: chapter 1 "job" (the team follows), walk out 12 s, turn back
# to the followers on the snow, then the same view with one suspect switched off at a time:
#   0 base | 1 local exposure off | 2 Lumen screen traces off | 3 Lumen short-range AO off | 4 SSAO off | 5 Lumen GI off
# -> Saved/Compare/halo/<Name>_<i>.png   powershell -File Tools/Snow/halo_test.ps1 [-Name base] [-Extra "cmd, cmd"]
param([string]$Name = "base", [string]$Extra = "", [double]$Head = 169)
$proj = "F:\UE_project\YggdrasilsPeak"
$shots = "$proj\Saved\Screenshots\WindowsEditor"
$out = "$proj\Saved\Compare\halo"
New-Item -ItemType Directory -Force $out | Out-Null
Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force
$c = @("t.MaxFPS 0", "ygg.Story.Auto 1", "ygg.Realm.Force none")
function L([double]$t, [string]$cmd) { $script:c += "ygg.Later $t $cmd" }
L 20 "ygg.Story.Jump dawn"; L 34 "ygg.Story.Jump job"
L 60 "ygg.Snow.AutoYaw $Head"; L 60 "ygg.Snow.AutoPitch -5"; L 60 "ygg.Snow.AutoWalk 0.5"
L 72 "ygg.Snow.AutoWalk 0"; L 73 "ygg.Snow.AutoYaw $($Head + 180)"; L 73 "ygg.Snow.AutoPitch -12"
$t = 80.0
$tests = @(@("", ""), @("ShowFlag.LocalExposure 0", "ShowFlag.LocalExposure 1"),
           @("r.Lumen.ScreenProbeGather.ScreenTraces 0", "r.Lumen.ScreenProbeGather.ScreenTraces 1"),
           @("r.Lumen.ScreenProbeGather.ShortRangeAO 0", "r.Lumen.ScreenProbeGather.ShortRangeAO 1"),
           @("ShowFlag.ScreenSpaceAO 0", "ShowFlag.ScreenSpaceAO 1"),
           @("r.Lumen.DiffuseIndirect.Allow 0", "r.Lumen.DiffuseIndirect.Allow 1"))
foreach ($p in $tests) {
    if ($p[0]) { L $t $p[0] }
    L ($t + 1.5) "shot"
    if ($p[1]) { L ($t + 2.2) $p[1] }
    $t += 3.5
}
L ($t + 1) "quit"
if ($Extra) { $c = @($Extra -split ',\s*') + $c }
$exec = $c -join ", "
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1600", "-ResY=900",
    "-nosplash", "-NoSound", "-unattended", "-YggNoSave", "-log=Halo.log", "-ExecCmds=`"$exec`"")
$proc.WaitForExit(300000) | Out-Null
if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force; "TIMEOUT" }
$found = @(Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTime)
for ($i = 0; $i -lt $found.Count; $i++) { Copy-Item $found[$i].FullName "$out\${Name}_$i.png" -Force }
"$Name : $($found.Count) of 6 shots"
