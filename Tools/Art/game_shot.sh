#!/usr/bin/env bash
# Real in-game screenshot (standalone -game, TSR, Lumen, everything the player sees) from a given spot.
#   game_shot.sh <name> <x> <y> <z> <pitch> <yaw>   -> Saved/Compare/game_<name>.png
# The game opens base_map, BugItGo puts the player there, HighResShot fires after a delay so streaming settles.
set -e
NAME=$1; X=$2; Y=$3; Z=$4; P=$5; W=$6
SHOTS="/c/Users/danji/AppData/Local/UnrealEngine/5.8/Saved/Screenshots/WindowsEditor"
mkdir -p "$SHOTS"; rm -f "$SHOTS"/HighresScreenshot*.png
"/e/UE5.8/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe" "F:/UE_project/YggdrasilsPeak/YggdrasilsPeak.uproject" \
  "/Game/LevelPrototyping/base_map" -game -windowed -ResX=1920 -ResY=1080 -nosplash -NoSound \
  -ExecCmds="BugItGo $X $Y $Z $P $W 0, r.HighResScreenshotDelay 900, HighResShot 1" > /dev/null 2>&1 &
PID=$!
for i in $(seq 1 120); do
  sleep 3
  F=$(ls "$SHOTS"/HighresScreenshot*.png 2>/dev/null | head -1 || true)
  if [ -n "$F" ]; then sleep 3; cp "$F" "F:/UE_project/YggdrasilsPeak/Saved/Compare/game_$NAME.png"; break; fi
done
taskkill //PID $PID //F > /dev/null 2>&1 || true
powershell -c "Get-Process UnrealEditor | Where-Object { \$_.MainWindowTitle -notlike '*虚幻编辑器*' -and \$_.MainWindowTitle -ne '' } | Stop-Process -Force" > /dev/null 2>&1 || true
ls -la "F:/UE_project/YggdrasilsPeak/Saved/Compare/game_$NAME.png"
