@echo off
rem Listen to every game sound and judge it by ear (Tools/Audio/listen_sounds.py). Double-click to start.
chcp 65001 >nul
title Yggdrasil's Peak - sound check
python "%~dp0listen_sounds.py"
if errorlevel 1 pause
