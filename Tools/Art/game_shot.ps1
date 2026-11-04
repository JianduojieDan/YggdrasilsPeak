# Real in-game screenshot (standalone -game: TSR, Lumen, weather, everything the player sees) from a given spot.
#   powershell -File Tools/Art/game_shot.ps1 <name> <x> <y> <z> <pitch> <yaw> [delayFrames]  -> Saved/Compare/game_<name>.png
# Run from PowerShell, not git bash (bash mangles the /Game path). Needs the editor open (it places + saves the camera).
# A fixed camera actor (GameShotCam, Tools/Art/place_shot_cam.py) is put at the spot and the game looks through it
# with the ViewActor cheat - teleporting the pawn instead is unreliable (falls, grabs faces, swings the camera).
param([string]$Name, [double]$X, [double]$Y, [double]$Z, [double]$P, [double]$W, [int]$Delay = 900)
$shots = "$env:LOCALAPPDATA\UnrealEngine\5.8\Saved\Screenshots"
$proj = "F:\UE_project\YggdrasilsPeak"
$tmp = "$env:TEMP\ygg_shotcam.py"
"SHOT = ($X, $Y, $Z, $P, $W)`nexec(open(r'$proj/Tools/Art/place_shot_cam.py').read())" | Set-Content -Encoding utf8 $tmp
$out = python -I "$proj\Tools\RoutePlanning\uepy.py" $tmp
$camName = (($out | Select-String "CAMNAME").ToString() -split " ")[1]
Get-ChildItem $shots, "$proj\Saved\Screenshots" -Recurse -Filter "HighresScreenshot*.png" -ErrorAction SilentlyContinue | Remove-Item -Force
$cmds = "Ghost, ViewActor $camName, r.HighResScreenshotDelay $Delay, HighResShot 1"
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1920", "-ResY=1080",
    "-nosplash", "-NoSound", "-log=GameShot.log", "-ExecCmds=`"$cmds`"")
$found = $null
for ($i = 0; $i -lt 100 -and -not $found; $i++) {
    Start-Sleep -Seconds 3
    $found = Get-ChildItem $shots, "$proj\Saved\Screenshots" -Recurse -Filter "HighresScreenshot*.png" -ErrorAction SilentlyContinue | Select-Object -First 1
}
if ($found) {
    Start-Sleep -Seconds 3
    # the ViewActor camera renders upside down in -game (roll 180) - turn the image upright
    python -c "from PIL import Image; Image.open(r'$($found.FullName)').rotate(180).save(r'$proj\Saved\Compare\game_$Name.png')"
    "saved game_$Name.png ($camName)"
} else { "NO SCREENSHOT" }
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
