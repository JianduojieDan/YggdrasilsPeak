@echo off
rem Yggdrasil's Peak as a game in a 1920x1080 window (see Play_Game.bat for full screen).
rem Recorded for analysis like Play_Game.bat (Saved\Monitor\<date_time>\report.md).
start "" /min powershell -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "%~dp0Tools\Monitor\system_monitor.ps1"
start "" "E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0YggdrasilsPeak.uproject" -game -windowed -ResX=1920 -ResY=1080 -YggMonitor
