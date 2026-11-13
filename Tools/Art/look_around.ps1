# One game run, several views: stands at a spot and screenshots each (pitch,yaw) with `stat unit` on, to find what is
# where and what it costs. Saved/Compare/look_<name>_<i>.png, plus a contact sheet Saved/Review/look_<name>.png.
#   powershell -File Tools/Art/look_around.ps1 -Name:sky -X:338533 -Y:2468724 -Z:-68990 -Views "35,0;35,45;35,90" [-Extra "cmd, cmd"] [-Wait:16]
param([string]$Name = "look", [double]$X, [double]$Y, [double]$Z, [string]$Views = "35,0;35,90;35,180;35,270", [string]$Extra = "", [int]$Wait = 16, [switch]$Ghost)
$proj = "F:\UE_project\YggdrasilsPeak"
$shots = "$proj\Saved\Screenshots\WindowsEditor"
$sav = "$proj\Saved\SaveGames\Expedition.sav"; $savBak = "$env:TEMP\ygg_expedition_keep_look.sav"
if (Test-Path $sav) { Copy-Item $sav $savBak -Force }
Get-ChildItem $shots -Include "ScreenShot*.png", "HighresScreenshot*.png" -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force
$list = @($Views -split ';' | Where-Object { $_ })
$t = $Wait; $cmds = @("stat unit", "t.MaxFPS 0", "r.VSync 0")
if ($Ghost) { $cmds += "Ghost" }
foreach ($v in $list) {
    $p, $w = $v -split ','
    $cmds += "ygg.Later $t BugItGo $X $Y $Z $p $w 0"
    $cmds += "ygg.Later $($t + 3) shot showui"
    $t += 5
}
$cmds += "ygg.Later $($t + 2) quit"
if ($Extra) { $cmds = @($Extra -split ',\s*') + $cmds }
$exec = $cmds -join ", "
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1920", "-ResY=1080",
    "-nosplash", "-NoSound", "-unattended", "-log=Look.log", "-ExecCmds=`"$exec`"")
$proc.WaitForExit(($t + 90) * 1000) | Out-Null
if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force }
if (Test-Path $savBak) { Copy-Item $savBak $sav -Force }
$found = @(Get-ChildItem $shots -Include "ScreenShot*.png" -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTime)
New-Item -ItemType Directory -Force "$proj\Saved\Compare" | Out-Null
for ($i = 0; $i -lt $found.Count; $i++) { Copy-Item $found[$i].FullName "$proj\Saved\Compare\look_${Name}_$i.png" -Force }
"$($found.Count) of $($list.Count) views saved as Saved/Compare/look_${Name}_*.png"
python -I -c "
from PIL import Image, ImageDraw
import glob
fs = sorted(glob.glob(r'$proj\Saved\Compare\look_${Name}_*.png'), key=lambda f: int(f.rsplit('_', 1)[1][:-4]))
if fs:
    n = len(fs); cols = 2; rows = (n + 1) // 2
    s = Image.new('RGB', (cols * 960, rows * 540))
    for i, f in enumerate(fs):
        s.paste(Image.open(f).convert('RGB').resize((960, 540)), ((i % cols) * 960, (i // cols) * 540))
    s.save(r'$proj\Saved\Review\look_${Name}.png')
"
