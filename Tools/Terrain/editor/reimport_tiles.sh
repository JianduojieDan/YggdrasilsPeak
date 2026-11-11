#!/bin/bash
# Re-imports route tiles (render + route collision) in batches of 8, restarting the editor every 3 batches
# (each batch adds ~3 GB to the editor; it ran out of memory near 24 GB once).
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
left = [p.get_name() for p in L.get_dirty_content_packages()] + [p.get_name() for p in L.get_dirty_map_packages()]
print("still dirty:", left)
if not left: unreal.SystemLibrary.quit_editor()
EOF
start_editor() {
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
  if [ $((b % 3)) -eq 0 ] && [ $# -gt 0 ]; then
    python -I Tools/RoutePlanning/uepy.py "$TEMP/ygg/rt_quit.py" 2>&1 | grep -a "still dirty"
    sleep 45; start_editor
  fi
done
echo "reimport done"
