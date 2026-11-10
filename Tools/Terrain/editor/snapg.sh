#!/bin/bash
# usage: snapg.sh "x y dz pitch yaw" [wait] -> screenshot path (camera dz above traced ground)
cd "$TEMP/ygg"; echo "$1" > view.txt; python run.py shotg.py 2>&1 | grep cam >&2
D=/f/UE_project/YggdrasilsPeak/Saved/Screenshots/WindowsEditor; n=$(ls $D | wc -l)
sleep ${2:-6}; python run.py hrs.py >/dev/null 2>&1
timeout 300 bash -c "until [ \$(ls $D | wc -l) -gt $n ]; do sleep 2; done"; sleep 1; echo $D/$(ls -t $D | head -1)
