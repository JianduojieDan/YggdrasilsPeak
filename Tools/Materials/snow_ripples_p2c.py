# Runs INSIDE the editor (Tools/RoutePlanning/uepy). 2026-11-14, snow step P2c (Docs/Development/Snow_Diagnosis_2026-11-14.md):
#   - wind ripples: the 5-50 cm band real snow shows from 2 to 20 m away (8-25 cm crests, about 1-2 cm high, short steep
#     lee face), on gentle snow in patches, never on rock, ice or steep slopes. Own maps T_SnowRipple_N/_H (3 m tile,
#     made by Tools/Materials/make_snow_detail), hex cells of ~2.8 m with random offset, +-0.25 rad and +-12 % size so
#     the 3 m repeat never shows; all crests lie across the prevailing westerly like the existing sastrugi
#   - route tiles: the ripples also get real height (displacement, mip 3), within ~35 m of the player
#   - snow crust normal: no x sign flip any more. Our maps are n = (-dh/du, -dh/dv) with v down the image, which is
#     UE's uv; the flip mirrored the crust's lighting against its own height (cracks lit like ridges, and against the
#     displaced geometry on the tiles). Rock keeps its line untouched.
# New Custom inputs (RipN, RipH, RipTile; displacement: RipH, RipTile, Area). Adding inputs can drop wires, so every
# old wire is recorded first and restored if it went missing.
import unreal
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary

RIPN = unreal.load_asset("/Game/Environment/Snow/Textures/T_SnowRipple_N")
RIPH = unreal.load_asset("/Game/Environment/Snow/Textures/T_SnowRipple_H")
assert RIPN and RIPH, "ripple textures missing"

WINDRIP = """HexTile hx;
// ---- wind ripples (2026-11-14): integer-hashed hex cells (~2.8 m) over the ripple map, each with a random offset,
// a +-0.25 rad turn and +-12 % size. The colour node and the tile displacement node run the same code, so the
// raised crests and their shading line up.
struct WindRip {
  float2 H2(float2 p){ int2 i=int2(floor(p)); uint h=asuint(i.x)*0x8da6b343u+asuint(i.y)*0xd8163841u;
    h^=h>>15; h*=0x2c1b3c6du; h^=h>>12; h*=0x297a2d39u; h^=h>>15; return float2(float(h&0xffffu),float(h>>16))/65536.0; }
  float2 R(float2 v, float a){ float s,c; sincos(a,s,c); return float2(c*v.x-s*v.y, s*v.x+c*v.y); }
  void Cells(float2 u, out float3 w, out float2 o1, out float2 o2, out float2 o3, out float3 a, out float3 k){
    float2 s=float2(u.x,-0.57735027*u.x+1.15470054*u.y)*1.08;
    float2 b=floor(s); float2 f=frac(s); float z=1.0-f.x-f.y; float2 v1,v2,v3;
    if(z>0){ w=float3(z,f.y,f.x); v1=b; v2=b+float2(0,1); v3=b+float2(1,0); }
    else   { w=float3(-z,1.0-f.y,1.0-f.x); v1=b+1.0; v2=b+float2(1,0); v3=b+float2(0,1); }
    o1=H2(v1); o2=H2(v2); o3=H2(v3);
    float2 q1=H2(v1+7.0), q2=H2(v2+7.0), q3=H2(v3+7.0);
    a=(float3(q1.x,q2.x,q3.x)-0.5)*0.5; k=exp2((float3(q1.y,q2.y,q3.y)-0.5)*0.35);
  }
  float3 Bl(float3 w, float3 h){ float3 b=pow(max(w,0.0),4.0)*(0.3+h); return b/max(b.x+b.y+b.z,1e-6); }
};
WindRip wr;"""

# ---------------- colour node (tile + landscape)
COLOUR = [
    ("HexTile hx;", WINDRIP),
    ("  sx*=sNStr; sx.x*=(i==2)?-sg[i]:sg[i];",
     "  sx*=sNStr; // no x flip (2026-11-14): our maps are (-dh/du,-dh/dv) in UE's uv; the flip mirrored the light against the height"),
    ("float3 sN=hx.SN(micro-float3(gP.x+gS.x+gL.x+gC.x,gP.y+gS.y+gL.y+gC.y,0.0),n);",
     """// wind ripples (2026-11-14): 8-25 cm crests, 1-2 cm high, across the prevailing westerly (Sast's 0.35 rad turned 90 deg).
// Gentle snow only, in patches (wind-scoured fields next to smooth powder); beyond ~40 m they are below a pixel.
float ripK=smoothstep(0.80,0.93,n.z)*w.z*smoothstep(0.30,0.62,hx.Fbm(WP.xy/2200.0+61.3))*lerp(0.75,1.0,hard);
float ripF=1.0-smoothstep(5.0,20.0,fp);
float2 gR=0; float ripH=0.5;
[branch] if(ripK*ripF>0.003){
  float rpt=1.0/RipTile; const float A0=-1.2208;
  float2 rpU=wr.R(WP.xy*rpt,A0); float2 rpDu=wr.R(gx[2]*(rpt*dmul),A0), rpDv=wr.R(gy[2]*(rpt*dmul),A0);
  float3 rpW,rpA,rpK3; float2 rpO1,rpO2,rpO3; wr.Cells(rpU,rpW,rpO1,rpO2,rpO3,rpA,rpK3);
  float2 rpU1=wr.R(rpU,rpA.x)*rpK3.x+rpO1, rpU2=wr.R(rpU,rpA.y)*rpK3.y+rpO2, rpU3=wr.R(rpU,rpA.z)*rpK3.z+rpO3;
  float2 rpX1=wr.R(rpDu,rpA.x)*rpK3.x, rpX2=wr.R(rpDu,rpA.y)*rpK3.y, rpX3=wr.R(rpDu,rpA.z)*rpK3.z;
  float2 rpY1=wr.R(rpDv,rpA.x)*rpK3.x, rpY2=wr.R(rpDv,rpA.y)*rpK3.y, rpY3=wr.R(rpDv,rpA.z)*rpK3.z;
  float3 rpH3=float3(RipH.SampleGrad(RipHSampler,rpU1,rpX1,rpY1).r,RipH.SampleGrad(RipHSampler,rpU2,rpX2,rpY2).r,RipH.SampleGrad(RipHSampler,rpU3,rpX3,rpY3).r);
  float3 rpB=wr.Bl(rpW,rpH3); float rpV=rsqrt(max(dot(rpB,rpB),0.2));
  ripH=saturate(0.5+(dot(rpH3,rpB)-0.5)*rpV);
  // the map's normal is (-dh/du,-dh/dv) for a 3 m tile: back to world by -(A0+turn), steeper where a cell is shrunk
  float2 rpT=wr.R(RipN.SampleGrad(RipNSampler,rpU1,rpX1,rpY1).rg*2.0-1.0,-(A0+rpA.x))*(rpK3.x*rpB.x)
            +wr.R(RipN.SampleGrad(RipNSampler,rpU2,rpX2,rpY2).rg*2.0-1.0,-(A0+rpA.y))*(rpK3.y*rpB.y)
            +wr.R(RipN.SampleGrad(RipNSampler,rpU3,rpX3,rpY3).rg*2.0-1.0,-(A0+rpA.z))*(rpK3.z*rpB.z);
  gR=rpT*rpV*(300.0*rpt)*1.5*ripK*ripF; // x1.5: physically made slopes read flat at game exposure (as the crust, P2b)
}
float3 sN=hx.SN(micro+float3(gR,0.0)-float3(gP.x+gS.x+gL.x+gC.x,gP.y+gS.y+gL.y+gC.y,0.0),n);"""),
    ("sRough=lerp(sRough,max(sRough-0.14,0.68),saturate((snowH-0.5)*3.0)*nearK*hard); // wind-polished crests, matte hollows",
     """sRough=lerp(sRough,max(sRough-0.14,0.68),saturate((snowH-0.5)*3.0)*nearK*hard); // wind-polished crests, matte hollows
// ripple troughs keep a little blue shade, ripple crests are wind-polished (2026-11-14)
float rpC=saturate((0.42-ripH)*3.0)*ripK*ripF;
sCol*=lerp(float3(1,1,1),float3(0.86,0.92,1.0),rpC*0.7);
sRough=lerp(sRough,max(sRough-0.08,0.70),saturate((ripH-0.6)*3.0)*ripK*ripF);"""),
    ("AO=lerp(lerp(1.0,0.9,hollow)*lerp(1.0,0.55,cav),rockM.r,rm);",
     "AO=lerp(lerp(1.0,0.9,hollow)*lerp(1.0,0.55,cav)*lerp(1.0,0.8,rpC),rockM.r,rm);"),
]

# ---------------- tile displacement node
DISP = [
    ("HexTile hx;", WINDRIP),
    ("return saturate(0.5+d/400.0);",
     """// wind ripples stand up too (2026-11-14): same cells and height as the colour node, read at mip 3 (1.2 cm), within
// ~35 m of the player only (Area = the trail map around the player); further out they are below a pixel. Height is
// not scaled with the cell size: the colour node's slopes grow with it instead, so the two match.
if (Area.w > 0.5)
{
  float rpE=6326.3+(WP.z-129770.0)/100.0;
  float rpHard=saturate(smoothstep(6600.0,7600.0,rpE+(hx.Fbm(WP.xy/9000.0+3.3)-0.5)*600.0)*0.9+(1.0-smoothstep(0.72,0.92,n.z))*0.55);
  float ripK=smoothstep(0.80,0.93,n.z)*w.z*smoothstep(0.30,0.62,hx.Fbm(WP.xy/2200.0+61.3))*lerp(0.75,1.0,rpHard)
            *(1.0-smoothstep(2500.0,4000.0,length(WP.xy-Area.xy)))*(1.0-rm)*(1.0-saturate(TileRock));
  if (ripK>0.003)
  {
    float rpt=1.0/RipTile; const float A0=-1.2208;
    float2 rpU=wr.R(WP.xy*rpt,A0); float3 rpW,rpA,rpK3; float2 rpO1,rpO2,rpO3; wr.Cells(rpU,rpW,rpO1,rpO2,rpO3,rpA,rpK3);
    float3 rpH3=float3(RipH.SampleLevel(RipHSampler,wr.R(rpU,rpA.x)*rpK3.x+rpO1,3).r,
                       RipH.SampleLevel(RipHSampler,wr.R(rpU,rpA.y)*rpK3.y+rpO2,3).r,
                       RipH.SampleLevel(RipHSampler,wr.R(rpU,rpA.z)*rpK3.z+rpO3,3).r);
    float3 rpB=wr.Bl(rpW,rpH3); float rpV=rsqrt(max(dot(rpB,rpB),0.2));
    float ripH=saturate(0.5+(dot(rpH3,rpB)-0.5)*rpV);
    d+=(ripH-0.5)*3.0*1.5*ripK; // map 0..1 = -1.5..+1.5 cm; x1.5 as the shading
  }
}
return saturate(0.5+d/400.0);"""),
]


def patch(node, repl):
    code = node.get_editor_property("code"); crlf = "\r\n" in code; res = []
    for a, b in repl:
        if crlf:
            a = a.replace("\r\n", "\n").replace("\n", "\r\n"); b = b.replace("\r\n", "\n").replace("\n", "\r\n")
        if code.count(a) == 1:
            code = code.replace(a, b); res.append("ok")
        else:
            res.append("MISSING" if a not in code else "TWICE")
    if all(r == "ok" for r in res):
        node.set_editor_property("code", code)
    return res


def wires_of(m, node):
    names = [str(i.get_editor_property("input_name")) for i in node.get_editor_property("inputs")]
    srcs = MEL.get_inputs_for_material_expression(m, node)
    out = {}
    for nm, s in zip(names, srcs):
        if s is not None:
            out[nm] = (s, str(MEL.get_input_node_output_name_for_material_expression(node, s)))
    return out


def add_inputs(m, node, new):
    before = wires_of(m, node)
    ins = list(node.get_editor_property("inputs"))
    have = [str(i.get_editor_property("input_name")) for i in ins]
    for nm in new:
        if nm in have:
            continue
        ci = unreal.CustomInput(); ci.set_editor_property("input_name", nm); ins.append(ci)
    node.set_editor_property("inputs", ins)
    after = wires_of(m, node)
    fixed = []
    for nm, (s, o) in before.items():
        if nm not in after:
            ok = MEL.connect_material_expressions(s, "" if o in ("return", "XYZ") else o, node, nm)
            fixed.append((nm, ok))
    return len(before), fixed


def tex_node(m, tex, x, y):
    for e in unreal.ObjectIterator(unreal.MaterialExpressionTextureObject):
        if e.get_outer() == m and e.get_editor_property("texture") == tex:
            return e
    e = MEL.create_material_expression(m, unreal.MaterialExpressionTextureObject, x, y)
    e.set_editor_property("texture", tex)
    e.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if tex.get_name().endswith("_N")
                          else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
    return e


def scalar_node(m, name, val, x, y):
    for e in unreal.ObjectIterator(unreal.MaterialExpressionScalarParameter):
        if e.get_outer() == m and str(e.get_editor_property("parameter_name")) == name:
            return e
    e = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name); e.set_editor_property("default_value", val)
    return e


for path, do_disp in [("/Game/Materials/M_EverestDetailTile", True), ("/Game/Materials/M_EverestLandscape", False)]:
    m = unreal.load_asset(path)
    ex = {e.get_name(): e for e in unreal.ObjectIterator(unreal.MaterialExpression) if e.get_outer() == m}
    col = ex["MaterialExpressionCustom_0"]
    assert "---- Snow (2026-10-22)" in col.get_editor_property("code")
    x0 = col.get_editor_property("material_expression_editor_x") - 500
    y0 = col.get_editor_property("material_expression_editor_y")
    tN = tex_node(m, RIPN, x0, y0 + 900)
    tH = tex_node(m, RIPH, x0, y0 + 1100)
    sT = scalar_node(m, "RipTileCm", 300.0, x0, y0 + 1300)
    C = MEL.connect_material_expressions
    if "WindRip" not in col.get_editor_property("code"):
        n_before, fixed = add_inputs(m, col, ["RipN", "RipH", "RipTile"])
        print(path, "colour node: wires before", n_before, "restored", fixed)
        print(path, "colour node: new wires", [C(tN, "", col, "RipN"), C(tH, "", col, "RipH"), C(sT, "", col, "RipTile")])
        print(path, "colour node patch", patch(col, COLOUR))
    else:
        print(path, "colour node already has the ripples")
    if do_disp:
        dn = ex["MaterialExpressionCustom_1"]
        assert "EverestDisplacement" in dn.get_editor_property("description")
        if "WindRip" not in dn.get_editor_property("code"):
            n_before, fixed = add_inputs(m, dn, ["RipH", "RipTile", "Area"])
            print(path, "displacement: wires before", n_before, "restored", fixed)
            print(path, "displacement: new wires", [C(tH, "", dn, "RipH"), C(sT, "", dn, "RipTile"),
                                                    C(ex["MaterialExpressionCollectionParameter_0"], "", dn, "Area")])
            print(path, "displacement patch", patch(dn, DISP))
        else:
            print(path, "displacement already has the ripples")
    # final check: every input of the two nodes is wired
    for nd in [col] + ([ex["MaterialExpressionCustom_1"]] if do_disp else []):
        names = [str(i.get_editor_property("input_name")) for i in nd.get_editor_property("inputs")]
        srcs = MEL.get_inputs_for_material_expression(m, nd)
        print(path, nd.get_name(), "unwired:", [nm for nm, s in zip(names, srcs) if s is None])
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m, False)
    print(path, "recompiled + saved")
