#!/bin/bash
# save level, quit editor, relaunch into base_map, wait until ready
cd "$TEMP/ygg"
python run.py sv.py >/dev/null 2>&1
python run.py quit.py >/dev/null 2>&1
for i in $(seq 1 120); do tasklist | grep -qi unrealeditor || break; sleep 2; done
powershell -command "Start-Process 'E:\UE5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList '\"F:\UE_project\YggdrasilsPeak\YggdrasilsPeak.uproject\" /Game/LevelPrototyping/base_map'"
for i in $(seq 1 120); do r=$(python run.py ready.py 2>/dev/null | grep -c "WORLD base_map"); [ "$r" -gt 0 ] && break; sleep 5; done
powershell -command "\$p=Get-Process UnrealEditor; 'restarted, Unreal {0:N1} GB' -f (\$p.WorkingSet64/1GB)"
