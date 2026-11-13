# Runs the settings self-test (ygg.Settings.Test) in a standalone game and prints its SETTINGSTEST lines: every row of
# the settings menu stepped one notch and back, with what changed in the engine and the frame time.
#   powershell -File Tools/Art/settings_test.ps1
$proj = "F:\UE_project\YggdrasilsPeak"
$ini = "$proj\Saved\Config\WindowsEditor\GameUserSettings.ini"
Copy-Item $ini "$env:TEMP\ygg_gus_keep.ini" -Force            # the test steps every row and back; keep the file anyway
$log = "$proj\Saved\Logs\SettingsTest.log"
Remove-Item $log -ErrorAction SilentlyContinue
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "$proj\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1920", "-ResY=1080",
    "-nosplash", "-NoSound", "-unattended", "-log=SettingsTest.log", "-ExecCmds=`"ygg.Settings.Test`"")
$done = $false
for ($i = 0; $i -lt 120 -and -not $done; $i++) {
    Start-Sleep -Seconds 3
    if (Test-Path $log) { $done = [bool](Select-String -Path $log -Pattern "SETTINGSTEST done" -Quiet) }
    if ($proc.HasExited) { break }
}
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
Copy-Item "$env:TEMP\ygg_gus_keep.ini" $ini -Force
if (Test-Path $log) { Select-String -Path $log -Pattern "SETTINGSTEST|YggQuality:|Settings: tier" | ForEach-Object { $_.Line -replace '^\[[^\]]+\]\[\s*\d+\]LogTemp: Display: ', '' } } else { "NO LOG" }
