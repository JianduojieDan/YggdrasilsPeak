# Captures what the player sees (HUD included) from a standalone -game window: a burst of window grabs.
#   powershell -File Tools/HUD/hud_capture.ps1 <outdir> [forecastIn s] [first s] [last s] [every ms]
# -YggForecastIn makes the weather wheel spin N s after start; frames land in <outdir>/f_<ms>.png (ms since launch).
param([string]$Out, [int]$ForecastIn = 20, [double]$First = 18, [double]$Last = 40, [int]$Every = 400)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public class W { [StructLayout(LayoutKind.Sequential)] public struct R { public int L, T, Ri, B; }
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref System.Drawing.Point p);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h); }
"@ -ReferencedAssemblies System.Drawing
New-Item -ItemType Directory -Force $Out | Out-Null
$proc = Start-Process -FilePath "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "F:\UE_project\YggdrasilsPeak\YggdrasilsPeak.uproject", "/Game/LevelPrototyping/base_map", "-game", "-windowed", "-ResX=1920", "-ResY=1080",
    "-nosplash", "-NoSound", "-YggForecastIn=$ForecastIn", "-ExecCmds=`"ygg.Save.LoadOnStart 0`"")
$t0 = Get-Date
while (((Get-Date) - $t0).TotalSeconds -lt $Last) {
    $s = ((Get-Date) - $t0).TotalSeconds
    $proc.Refresh(); $h = $proc.MainWindowHandle
    if ($s -ge $First -and $h -ne [IntPtr]::Zero) {
        [W]::SetForegroundWindow($h) | Out-Null
        $r = New-Object W+R; [W]::GetClientRect($h, [ref]$r) | Out-Null
        $p = New-Object System.Drawing.Point 0, 0; [W]::ClientToScreen($h, [ref]$p) | Out-Null
        if ($r.Ri -gt 100) {
            $bmp = New-Object System.Drawing.Bitmap $r.Ri, $r.B
            $g = [System.Drawing.Graphics]::FromImage($bmp); $g.CopyFromScreen($p.X, $p.Y, 0, 0, $bmp.Size)
            $bmp.Save("$Out\f_$([int]($s*1000)).png"); $g.Dispose(); $bmp.Dispose()
        }
    }
    Start-Sleep -Milliseconds $Every
}
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
"frames: $((Get-ChildItem $Out -Filter f_*.png).Count)"
