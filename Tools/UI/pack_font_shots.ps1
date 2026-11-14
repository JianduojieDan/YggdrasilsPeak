# Pack screen in each lettering (ygg.Pack.Font 0..3), four screenshots with the UI in one standalone run, no key presses
# (start-up commands only, so nothing can reach another window). -> Saved/Compare/pack_font_<n>.png
#   powershell -File Tools/UI/pack_font_shots.ps1 [-ResX 1920 -ResY 1080] [-Modes "0,1,2,3"]
param([int]$ResX = 1920, [int]$ResY = 1080, [string]$Modes = "0,1,2,3", [int]$Start = 28)
$proj = "F:\UE_project\YggdrasilsPeak"
$shots = "$proj\Saved\Screenshots\WindowsEditor"
New-Item -ItemType Directory -Force "$proj\Saved\Compare" | Out-Null
Get-ChildItem $shots -Include "HighresScreenshot*.png","ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force
$list = $Modes -split ',' | ForEach-Object { [int]$_ }
$t = $Start
$cmds = @("ygg.Later $t ygg.Pack")
foreach ($m in $list) {
    $cmds += "ygg.Later $($t + 1) ygg.Pack.Font $m"
    $cmds += "ygg.Later $($t + 4) shot showui"
    $t += 4
}
$cmds += "ygg.Later $($t + 4) quit"
$exec = $cmds -join ', '
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY",
    "-nosplash", "-NoSound", "-unattended", "-YggNoSave", "-log=PackFont.log", "-ExecCmds=`"$exec`"")
for ($i = 0; $i -lt 60 -and -not $proc.HasExited; $i++) { Start-Sleep -Seconds 3 }
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
$found = @(Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Sort-Object Name)
for ($k = 0; $k -lt $found.Count -and $k -lt $list.Count; $k++) {
    Copy-Item $found[$k].FullName "$proj\Saved\Compare\pack_font_$($list[$k]).png" -Force
    "saved pack_font_$($list[$k]).png"
}
if ($found.Count -lt $list.Count) { "ONLY $($found.Count) OF $($list.Count) SCREENSHOTS" }
