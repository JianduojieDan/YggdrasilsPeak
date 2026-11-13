# Screens of the menus in the real game (standalone -game, windowed): starts the game on a map, then runs steps and
# saves OS screen grabs of the game window (the UI included, which HighResShot leaves out).
#   powershell -File Tools/UI/menu_shots.ps1 <map> "<step>;<step>;..."
# Steps: wait:<s>  shot:<name> (-> Saved/Compare/menu_<name>.png)  key:<SendKeys text, e.g. {ENTER} {DOWN} e>
#        con:<console command> (opens the console with the ` key, types, Enter)
# Example: menu_shots.ps1 /Game/UI/Menu/L_MainMenu "wait:30;shot:splash;key:{ENTER};wait:3;shot:main"
param([string]$Map, [string]$Steps, [int]$W = 1600, [int]$H = 900, [string]$Exe = "", [string]$More = "")
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System; using System.Runtime.InteropServices;
public class GW { [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  public struct RECT { public int L, T, R, B; } public struct POINT { public int X, Y; } }
"@
# -More "<args>": extra launch arguments (e.g. -YggMonitor). Keys are only ever sent while the game window is in front:
# 2026-11-14 a broken launch left $hwnd null, the old "-eq 0" check let it through, and the keys went to the Claude app.
$proj = "F:\UE_project\YggdrasilsPeak"
# -Exe <packaged YggdrasilsPeak.exe>: test the packaged game instead (it starts on its default map; mem: prints its memory)
if ($Exe) {
    $proc = Start-Process -FilePath $Exe -PassThru -ArgumentList @("-windowed", "-ResX=$W", "-ResY=$H", "-WinX=0", "-WinY=0", "-NoSound", "-YggNoSave", "-log=MenuShots.log")
} else {
    $argv = @("$proj\YggdrasilsPeak.uproject", $Map, "-game", "-windowed", "-ResX=$W", "-ResY=$H", "-WinX=0", "-WinY=0",
        "-nosplash", "-NoSound", "-YggNoSave", "-log=MenuShots.log") + @($More -split " " | Where-Object { $_ })
    $proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList $argv
}
if (-not $proc) { "NO PROCESS"; exit 1 }
$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 60 -and $hwnd -eq [IntPtr]::Zero; $i++) { Start-Sleep -Seconds 2; $proc.Refresh(); if ($proc.MainWindowHandle) { $hwnd = $proc.MainWindowHandle } }
if ($hwnd -eq [IntPtr]::Zero) { "NO WINDOW"; Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue; exit 1 }
function Front {
    [GW]::SetForegroundWindow($hwnd) | Out-Null; Start-Sleep -Milliseconds 400
    if ($proc.HasExited -or [GW]::GetForegroundWindow() -ne $hwnd) { "GAME WINDOW NOT IN FRONT - stopping, no keys sent"; Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue; exit 2 }
}
foreach ($s in $Steps.Split(";")) {
    $k, $v = $s.Split(":", 2)
    switch ($k) {
        "wait" { Start-Sleep -Seconds ([double]$v) }
        "mem"  { $all = @($proc) + @(Get-CimInstance Win32_Process -Filter "ParentProcessId=$($proc.Id)" | ForEach-Object { Get-Process -Id $_.ProcessId -ErrorAction SilentlyContinue })
                 foreach ($p in $all) { if ($p) { $p.Refresh(); "mem $v $($p.ProcessName): working set {0:N2} GB, private {1:N2} GB" -f ($p.WorkingSet64 / 1GB), ($p.PrivateMemorySize64 / 1GB) } } }
        "key"  { Front; [System.Windows.Forms.SendKeys]::SendWait($v) }
        "con"  { Front; [System.Windows.Forms.SendKeys]::SendWait([string][char]96); Start-Sleep -Milliseconds 500
                 [System.Windows.Forms.SendKeys]::SendWait($v); Start-Sleep -Milliseconds 200; [System.Windows.Forms.SendKeys]::SendWait("{ENTER}") }
        "shot" {
            Front
            $r = New-Object GW+RECT; [GW]::GetClientRect($hwnd, [ref]$r) | Out-Null
            $p = New-Object GW+POINT; [GW]::ClientToScreen($hwnd, [ref]$p) | Out-Null
            $bmp = New-Object System.Drawing.Bitmap ($r.R - $r.L), ($r.B - $r.T)
            $g = [System.Drawing.Graphics]::FromImage($bmp); $g.CopyFromScreen($p.X, $p.Y, 0, 0, $bmp.Size)
            $bmp.Save("$proj\Saved\Compare\menu_$v.png", [System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $bmp.Dispose()
            "shot $v"
        }
    }
}
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
"done"
