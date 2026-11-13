# One standalone screenshot from a spot, with settings swapped for this run only.
#   powershell -File Tools/Art/shot.ps1 <name> <x> <y> <z> <pitch> <yaw> [-Extra "cmd, cmd"] [-Set "Graphics=0;Upscaling=4"]
#     [-Sg 0..3] [-Wait 14] [-ResX 1920 -ResY 1080]  ->  Saved/Compare/shot_<name>.png
# -Set writes [YggdrasilsPeak.Display] keys, -Sg the engine scalability levels (sg.*), both restored afterwards.
# The picture is taken ygg.ShotAfter seconds into play (after the arrival fade and Nanite streaming).
param([string]$Name, [double]$X, [double]$Y, [double]$Z, [double]$P, [double]$W, [string]$Extra = "", [string]$Set = "",
      [int]$Sg = -1, [int]$Wait = 14, [int]$ResX = 1920, [int]$ResY = 1080, [switch]$NoGhost, [switch]$UI)
$proj = "F:\UE_project\YggdrasilsPeak"
$ini = "$proj\Saved\Config\WindowsEditor\GameUserSettings.ini"
$keep = "$env:TEMP\ygg_gus_shot_keep.ini"
Copy-Item $ini $keep -Force
$c = [IO.File]::ReadAllText($ini)
foreach ($kv in ($Set -split ';' | Where-Object { $_ })) {
    $k, $v = $kv -split '=', 2
    if ($c -match "(?m)^$k=") { $c = [regex]::Replace($c, "(?m)^$k=.*$", "$k=$v") } else { $c = $c.Replace("[YggdrasilsPeak.Display]", "[YggdrasilsPeak.Display]`r`n$k=$v") }
}
if ($Sg -ge 0) { $c = [regex]::Replace($c, "(?m)^(sg\.(?!ResolutionQuality)\w+)=\d+", "`${1}=$Sg") }
[IO.File]::WriteAllText($ini, $c, (New-Object Text.UTF8Encoding $false))
$shots = "$proj\Saved\Screenshots\WindowsEditor"
New-Item -ItemType Directory -Force "$proj\Saved\Compare" | Out-Null
Get-ChildItem $shots -Include "HighresScreenshot*.png","ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force
$ghost = if ($NoGhost) { "" } else { "Ghost, " }          # -NoGhost: the climber walks (snow kick, footprints)
$cmds = "${ghost}BugItGo $X $Y $Z $P $W 0, ygg.ShotAfter $Wait"
if ($UI) { $cmds = "ygg.ShotUI 1, $cmds" }                 # -UI: a normal screenshot with the HUD / menus (HighResShot has none)
if ($Extra) { $cmds = "$cmds, $Extra" }
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY",
    "-nosplash", "-NoSound", "-unattended", "-log=Shot.log", "-ExecCmds=`"$cmds`"")
$found = $null
for ($i = 0; $i -lt 80 -and -not $found -and -not $proc.HasExited; $i++) { Start-Sleep -Seconds 3; $found = Get-ChildItem $shots -Include "HighresScreenshot*.png","ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1 }
if ($found) { Start-Sleep -Seconds 2; Copy-Item $found.FullName "$proj\Saved\Compare\shot_$Name.png" -Force; "saved shot_$Name.png" } else { "NO SCREENSHOT" }
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
Copy-Item $keep $ini -Force
