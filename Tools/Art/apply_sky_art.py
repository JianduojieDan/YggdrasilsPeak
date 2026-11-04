# Runs INSIDE the editor (Tools/RoutePlanning/uepy.py). Art pass 2026-11-03 for the look in Docs/ArtDirection:
# layered blue distance haze, blue (not black) shadows, a low valley cloud sea + a high patchy layer whose moving
# shadows sweep across far faces, cirrus wisps. Settings only (instance values on the UDS actor), no UDS asset edits.
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
uds = [a for a in eas.get_all_level_actors() if a.get_class().get_name() == "Ultra_Dynamic_Sky_C"][0]
LC = unreal.LinearColor
S = dict(globals().get("OVERRIDES", {}))
V = {
    # haze: real aerial perspective from the sky atmosphere (UDS had it at 0) - ridges fade layer by layer
    "Control Sky Atmosphere Settings": False,
    "Fog Color Intensity Scale": 1.0,
    # shadows lit by the blue sky and the snow around them
    "Sky Light Intensity": 1.5,
    "Sky Light Lower Hemisphere Tint (Capture Based)": LC(0.75, 0.80, 0.88, 1.0),
    "Ambient Light from Sky Atmosphere": 0.5,
    # clouds: one broken layer at ~5.6-6.3 km (UDS planet top sits at 2.58 km). From the summit and South Col you look
    # down on a cloud sea; from the Cwm and Base Camp sunlight comes through the gaps and sweeps across the faces.
    "Bottom Altitude": 3.0,
    "Layer Height Scale": 0.085,       # UDS default layer is ~9 km thick (summit would sit inside it): ~0.8 km
    "Two Layers": False,
    "Cloud Coverage": 0.9,
    "Cloud Speed": 0.6,
    "Clouds Move with Time of Day": True,
    "Use Cloud Shadows": True,
    "Cloud Shadows Intensity When Sunny": 0.9,
    # cirrus
    "Cloud Wisps Opacity (Clear)": 0.35,
    "Cloud Wisps Color Intensity": 1.2,
}
V.update(S)
for k, v in V.items():
    try:
        uds.set_editor_property(k, v)
    except Exception as e:
        print("SKIP", k, str(e)[:80])
sa = uds.get_components_by_class(unreal.SkyAtmosphereComponent)[0]
sa.set_editor_property("aerial_pespective_view_distance_scale", float(V.get("AP", 1.0)))
sa.set_editor_property("height_fog_contribution", 1.0)
# Height fog stops at 90 km: past that the far background sinks (Earth curvature) below the fog's base height, where
# exponential fog goes opaque and drew a dark navy band. Beyond 90 km the atmosphere's aerial perspective alone.
hf = uds.get_components_by_class(unreal.ExponentialHeightFogComponent)[0]
hf.set_editor_property("fog_cutoff_distance", 9000000.0)
# the old HorizonSkirt / FarHazeFloor (black-band patches from 2026-10-10) drew a hard edge across the sky - off
for a in eas.get_all_level_actors():
    if a.get_actor_label() in ("HorizonSkirt", "FarHazeFloor"):
        a.static_mesh_component.set_editor_property("visible", False); a.set_actor_hidden_in_game(True)
print("AP", sa.get_editor_property("aerial_pespective_view_distance_scale"))
