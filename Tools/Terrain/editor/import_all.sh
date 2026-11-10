#!/bin/bash
cd "$TEMP/ygg"; LOG=/f/UE_project/YggdrasilsPeak/Saved/DetailTiles/route/import_log.txt; : > $LOG
n=0
while read -r line || [ -n "$line" ]; do  # last line may lack a newline
  n=$((n+1)); echo "$line" > batch.txt
  echo "batch $n start $(date +%T)" >> $LOG
  python run.py routeBatch.py 2>&1 | grep -v Deprec | grep "imported\|Error" >> $LOG
  timeout 900 bash -c 'until [ $(tail -20 /f/UE_project/YggdrasilsPeak/Saved/Logs/YggdrasilsPeak.log | grep -c "正在构建\|Building static mesh\|Built static mesh") -eq 0 ]; do sleep 10; done'
  python run.py routeBatchB.py 2>&1 | grep -v Deprec | grep "placed\|Error" >> $LOG
  gb=$(powershell -command "[math]::Round((Get-Process UnrealEditor).WorkingSet64/1GB,1)")
  echo "batch $n done $(date +%T) RAM $gb GB" >> $LOG
  if [ $(python -c "print(1 if float('$gb')>10.5 else 0)") -eq 1 ]; then ./restart.sh >> $LOG 2>&1; fi
done < batches.txt
echo ALLDONE >> $LOG
