@echo off
rem Yggdrasil's Peak as a game: no editor window, starts at the start menu, full screen, with the project's current
rem content and code. Alt+Enter switches between full screen and a window. Close the game before a C++ build.
rem (A packaged build, which needs no Unreal at all, is made for each milestone: F:\UE_project\Builds\<date>.)
rem The session is recorded for analysis (Saved\Monitor\<date_time>: frames, hitches, keys, screens, log, body, and the
rem computer side: CPU, GPU, memory, other programs); report.md is written in that folder when the game closes.
start "" /min powershell -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "%~dp0Tools\Monitor\system_monitor.ps1"
start "" "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0YggdrasilsPeak.uproject" -game -fullscreen -YggMonitor
