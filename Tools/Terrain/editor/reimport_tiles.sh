#!/bin/bash
# Re-imports route tiles (render + route collision) in batches of 8, restarting the editor whenever it passes
# 13 GB after a batch (each batch adds ~3 GB to the editor; it ran out of memory near 24 GB once; keep it under 20 GB).
#   bash Tools/Terrain/editor/reimport_tiles.sh <file with tile names>
cd /f/UE_project/YggdrasilsPeak
S="$(dirname "$1")"
UE="E:\\UE5.8\\UE_5.8\\Engine\\Binaries\\Win64\\UnrealEditor.exe"
PROJ="F:\\UE_project\\YggdrasilsPeak\\YggdrasilsPeak.uproject"
cat > "$TEMP/ygg/rt_loadmap.py" <<'EOF'
import unreal
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if "base_map" not in w.get_path_name():
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/LevelPrototyping/base_map")
print("map ready")
EOF
cat > "$TEMP/ygg/rt_quit.py" <<'EOF'
import unreal
L = unreal.EditorLoadingAndSavingUtils
for p in L.get_dirty_content_packages(): unreal.EditorAssetLibrary.save_asset(p.get_name(), only_if_is_dirty=False)
if [p for p in L.get_dirty_map_packages() if "base_map" in p.get_name()]: L.save_current_level()   # re-imported tiles dirty the level
left = [p.get_name() for p in L.get_dirty_content_packages()] + [p.get_name() for p in L.get_dirty_map_packages()]
print("still dirty:", left)
if not left: unreal.SystemLibrary.quit_editor()
EOF
start_editor() {
  for i in $(seq 1 60); do tasklist //FI "IMAGENAME eq UnrealEditor.exe" | grep -q UnrealEditor || break; sleep 5; done
  tasklist //FI "IMAGENAME eq UnrealEditor.exe" | grep -q UnrealEditor && { echo "editor did not quit - stopping"; exit 1; }
  cmd //c start "" "$UE" "$PROJ"; sleep 60
  for i in $(seq 1 30); do python -I Tools/RoutePlanning/uepy.py "$TEMP/ygg/rt_loadmap.py" 2>/dev/null | grep -q "map ready" && return; sleep 20; done
}
tasklist //FI "IMAGENAME eq UnrealEditor.exe" | grep -q UnrealEditor || start_editor
set -- $(cat "$1"); b=0
while [ $# -gt 0 ]; do
  batch=""; for k in 1 2 3 4 5 6 7 8; do [ $# -gt 0 ] && { batch="$batch $1"; shift; }; done
  echo "$batch" > "$TEMP/ygg/batch.txt"
  python -I Tools/RoutePlanning/uepy.py Tools/Terrain/editor/routeRenderBatch.py 2>&1 | grep -a "imported\|rror" | head -1
  b=$((b+1))
  gb=$(powershell -NoProfile -command "[int]((Get-Process UnrealEditor -ErrorAction SilentlyContinue | Measure-Object WorkingSet64 -Sum).Sum/1GB)")
  echo "batch $b done, editor ${gb} GB"
  if [ "${gb:-0}" -ge 13 ] && [ $# -gt 0 ]; then
    python -I Tools/RoutePlanning/uepy.py "$TEMP/ygg/rt_quit.py" 2>&1 | grep -a "still dirty"
    start_editor
  fi
done
echo "reimport done"
