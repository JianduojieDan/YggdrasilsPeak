# Like grab_editor.ps1, but restores the editor window first when it is minimised (only an OS-level screen copy is
# taken here - never queue an engine screenshot together with a restore: that crashed the editor once).
#   powershell -File Tools/UI/grab_editor_restore.ps1 <out.png> [maximise 0/1]
param([string]$Out, [int]$Max = 1)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public class W2 { [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  public struct RECT { public int L, T, R, B; } }
"@
$p = Get-Process UnrealEditor | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
$h = $p.MainWindowHandle
if ([W2]::IsIconic($h)) { [W2]::ShowWindow($h, $(if ($Max -eq 1) { 3 } else { 9 })) | Out-Null; Start-Sleep -Milliseconds 2500 }
[W2]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 2000
$r = New-Object W2+RECT; [W2]::GetWindowRect($h, [ref]$r) | Out-Null
$w = $r.R - $r.L; $hh = $r.B - $r.T
$bmp = New-Object System.Drawing.Bitmap $w, $hh
$g = [System.Drawing.Graphics]::FromImage($bmp); $g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $bmp.Dispose()
"grabbed $w x $hh -> $Out"
