# Runs INSIDE the editor (Tools/RoutePlanning/uepy). 2026-11-14, user: "the rock surface is very detailed... the snow
# surface in the player's perspective is so bad because it is blurring". Diagnosis: Docs/Development/Snow_Diagnosis_2026-11-14.md
# Step P0 + P1, patched into the snow Custom node of BOTH terrain materials by exact text replacement:
#   P0-1 our SampleGrad reads get the upscaler's sharpening (engine texture reads get View.MaterialTextureMipBias, ours did not)
#   P0-3 route tiles only: no shadow-terminator hack (made for the landscape's 17 m triangles; it flattened raking-sun detail)
#   P0-4 blowing snow no longer wipes the surface flat (normal flattening 80% -> 35%)
#   P1-a the snow grain shows near the eye (50-70% instead of 9-30%), fading back to the calm large forms beyond ~10 m
#   P1-b cm-scale blue cavity shade and AO from the snow's own height, near the eye
#   P1-c wind-polished crests a little smoother on hard snow
#   P1-d one grain size per surface (per-cell scale spread 8x -> 2x: the spread made blotches)
#   P1-e glints that really flash: facets that face half-way between eye and sun, integer hash (sin-hash broke, #55)
import unreal
MEL = unreal.MaterialEditingLibrary

COMMON = [
    # P0-1
    ("float2 gy[3]={float2(ddy(pc[0].x),ddy(pc[0].y)),float2(ddy(pc[1].x),ddy(pc[1].y)),float2(ddy(pc[2].x),ddy(pc[2].y))};\n",
     "float2 gy[3]={float2(ddy(pc[0].x),ddy(pc[0].y)),float2(ddy(pc[1].x),ddy(pc[1].y)),float2(ddy(pc[2].x),ddy(pc[2].y))};\n"
     "// DLSS/TSR render fewer pixels and sharpen the engine's own texture reads by the view's mip bias; a Custom node's\n"
     "// SampleGrad never got it (about 0.9 mip blurrier at 67 %) - scale our gradients the same way (2026-11-14)\n"
     "float dmul=ResolvedView.MaterialTextureDerivativeMultiply;\n"),
    ("float2 uvR=pc[i]*rt; float2 dX=gx[i]*rt; float2 dY=gy[i]*rt;", "float2 uvR=pc[i]*rt; float2 dX=gx[i]*(rt*dmul); float2 dY=gy[i]*(rt*dmul);"),
    ("float2 uvS=pc[i]*st; float2 sX=gx[i]*st; float2 sY=gy[i]*st;", "float2 uvS=pc[i]*st; float2 sX=gx[i]*(st*dmul); float2 sY=gy[i]*(st*dmul);"),
    # P1-d (Xf and Xd)
    ("Rot(uv*exp2(lerp(-1.5,1.5,q.y)), q.x*6.2831853)+o; }", "Rot(uv*exp2(lerp(-0.5,0.5,q.y)), q.x*6.2831853)+o; }"),
    ("Rot(d*exp2(lerp(-1.5,1.5,q.y)), q.x*6.2831853); }", "Rot(d*exp2(lerp(-0.5,0.5,q.y)), q.x*6.2831853); }"),
    # P1-a
    ("float3 micro=hx.SN(lerp(n,hx.SN(snowW,n),lerp(0.09,0.30,hard)),n);\n"
     "// soft lumps and clumps, strongest in deep powder; clumps fade out where a pixel covers more than ~3 cm (no shimmer far away)\n"
     "float fp=length(float2(ddx(WP.x),ddx(WP.y)))+length(float2(ddy(WP.x),ddy(WP.y)));\n",
     "float fp=length(float2(ddx(WP.x),ddx(WP.y)))+length(float2(ddy(WP.x),ddy(WP.y)));\n"
     "// near the eye the snow's own grain shows (photos: crisp crust and crumbs at your feet); beyond ~10 m it fades back to\n"
     "// the calm large forms the user chose on 2026-10-22. fp = cm per pixel on the ground. (2026-11-14)\n"
     "float nearK=1.0-smoothstep(1.5,8.0,fp);\n"
     "float3 micro=hx.SN(lerp(n,hx.SN(snowW,n),lerp(lerp(0.09,0.30,hard),lerp(0.50,0.70,hard),nearK)),n);\n"
     "// soft lumps and clumps, strongest in deep powder; clumps fade out where a pixel covers more than ~3 cm (no shimmer far away)\n"),
    # P1-b
    ("sCol*=lerp(0.95,1.03,sm3*0.5+sm1*0.5);\n",
     "sCol*=lerp(0.95,1.03,sm3*0.5+sm1*0.5);\n"
     "// light scattered inside the snow: its own low spots a little blue and darker, near the eye only (2026-11-14)\n"
     "float cav=saturate((0.52-snowH)*3.0)*nearK;\n"
     "sCol*=lerp(float3(1,1,1),float3(0.86,0.92,1.0),cav*0.6);\n"),
    ("AO=lerp(lerp(1.0,0.9,hollow),rockM.r,rm);", "AO=lerp(lerp(1.0,0.9,hollow)*lerp(1.0,0.75,cav),rockM.r,rm);"),
    # P1-c
    ("sRough=lerp(sRough,0.45,ice);\n",
     "sRough=lerp(sRough,0.45,ice);\n"
     "sRough=lerp(sRough,max(sRough-0.14,0.68),saturate((snowH-0.5)*3.0)*nearK*hard); // wind-polished crests, matte hollows\n"),
    # P1-e
    ("float2 gc=floor(WP.xy/1.6); float2 gh=hx.Hash(gc+0.37);\n"
     "float spark=step(0.988,gh.x)*(1.0-smoothstep(1.0,3.2,fp))*(1.0-ice);\n"
     "sRough=lerp(sRough,0.08,spark);\n"
     "sN=hx.SN(lerp(sN,hx.SN(float3((gh.y-0.5)*0.8,(frac(gh.y*7.3)-0.5)*0.8,1.0),n),spark),n);\n",
     "// (2026-11-14) a few facets per cm2 face exactly half-way between the eye and the sun, so they really flash; which ones\n"
     "// flash re-rolls as the angle changes, so they twinkle as you walk. Integer hash: sin-hash breaks on world-size ids (#55).\n"
     "float3 sunG=normalize(ResolvedView.AtmosphereLightDirection[0].xyz);\n"
     "float3 Hs=normalize(sunG+Parameters.CameraVector);\n"
     "uint2 gi=uint2(int2(floor(WP.xy/0.7))+int2(1<<20,1<<20));\n"
     "uint gh=(gi.x*0x8da6b343u)^(gi.y*0xd8163841u)^(uint(floor(Hs.x*18.0)+64.0)*0x2c1b3c6du)^(uint(floor(Hs.y*18.0)+64.0)*0x297a2d39u);\n"
     "gh^=gh>>15; gh*=0x2c1b3c6du; gh^=gh>>12;\n"
     "float spark=step(0.985,float(gh&0xffffu)/65536.0)*step(0.55,dot(Hs,n))*(1.0-smoothstep(0.6,1.6,fp))*(1.0-ice);\n"
     "sN=hx.SN(lerp(sN,Hs,spark),n);\n"
     "sRough=lerp(sRough,0.06,spark);\n"),
]
TILE_ONLY = [
    ("float term=(1.0-smoothstep(0.03,0.30,dot(n,sunL)))*(1.0-rm);",
     "float term=0.0; // route tiles are 1 m triangles: the 17 m terminator hack only flattened raking-sun detail (2026-11-14)"),
]
SNAKES = [("amount * 0.8));", "amount * 0.35)); // 2026-11-14: was 0.8, it wiped the snow's detail flat in moving bands")]

def patch(node, repl):
    # the node text has Windows line endings: match and write with the same ones (2026-11-14: the first run with "\n"
    # applied only the one-line replacements)
    code = node.get_editor_property("code")
    crlf = "\r\n" in code
    res = []
    for a, b in repl:
        if crlf:
            a = a.replace("\r\n", "\n").replace("\n", "\r\n"); b = b.replace("\r\n", "\n").replace("\n", "\r\n")
        if a in code:
            code = code.replace(a, b); res.append("ok")
        elif b in code:
            res.append("already")
        else:
            res.append("MISSING")
    node.set_editor_property("code", code)
    return res

for path, extra in [("/Game/Materials/M_EverestDetailTile", TILE_ONLY), ("/Game/Materials/M_EverestLandscape", [])]:
    m = unreal.load_asset(path)
    nodes = [e for e in unreal.ObjectIterator(unreal.MaterialExpressionCustom) if e.get_outer() == m]
    snow = [e for e in nodes if "---- Snow (2026-10-22)" in e.get_editor_property("code")]
    snakes = [e for e in nodes if "moving powder hides the surface detail" in e.get_editor_property("code")]
    if not snow:
        print(path, "NO SNOW NODE among", len(nodes)); continue
    print(path, "snow", patch(snow[0], COMMON + extra))
    for s in snakes:
        print(path, "snakes", patch(s, SNAKES))
    MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m, False)
    print(path, "recompiled + saved")
