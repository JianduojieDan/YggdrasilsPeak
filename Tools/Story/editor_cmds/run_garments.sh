# Rebuild all chapter 1 garments in Blender (one after another, ~5-10 s each); log per outfit in Saved/Logs.
cd /f/UE_project/YggdrasilsPeak
B="/e/APP-for-content-creation/Blender/blender.exe"; S=Art/Blender/Source/MetaHuman
for pair in nima:Nima pema:Pema marek:Marek elena:Elena jiwoo:Jiwoo porter_a:Portera; do
  o=${pair%%:*}; n=${pair##*:}; t0=$(date +%s)
  "$B" -b --factory-startup --python Tools/Blender/garments.py -- $o $S/SKM_MH_${n}_Body.fbx $S/SKM_MH_${n}_Face.fbx > Saved/Logs/garments_$o.log 2>&1
  echo "$o exit=$? $(( $(date +%s) - t0 ))s $(grep -E 'UVLAYERS|Traceback' Saved/Logs/garments_$o.log | tr '\n' ' ')"
done
