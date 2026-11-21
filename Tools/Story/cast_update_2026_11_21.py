"""Run with python -I (outside the editor). 2026-11-21: rewrites the chapter-1 looks in Content/Story/Cast.ygs.
  jiwoo      MetaHuman Aera rebuilt in Epic's techwear outfit (Tools/Story/mh_wardrobe) + a cream knit beanie
  marek_ai / nima_ai / elena_ai   the Meshy body with its head cut off (Tools/Blender/meshy_headless -> <id>_hl, imported
             + retargeted like any Meshy character) wearing the person's MetaHuman face and grooms + a knit beanie
  pema       the Meshy pema_body (headless) + Pema's MetaHuman face + the braided updo (no scarf)
The beanie is fitted per face by Tools/Story/hat_fit (Saved/HatFit.json). A look is only switched when all of its parts
exist on disk; otherwise it keeps its old entry and says why."""
import json, os

ROOT = "F:/UE_project/YggdrasilsPeak"
CAST = f"{ROOT}/Content/Story/Cast.ygs"
cast = json.load(open(CAST, encoding="utf-8"))
ai = json.load(open(f"{ROOT}/Saved/AIAssets.json"))
hats = json.load(open(f"{ROOT}/Saved/HatFit.json")) if os.path.exists(f"{ROOT}/Saved/HatFit.json") else {}
BEANIE = ai.get("knit_beanie", {}).get("mesh", "")


def exists(p):
    return bool(p) and os.path.exists(ROOT + "/Content" + p.split(".")[0][len("/Game"):] + ".uasset")


def hat(face, tint):
    h = hats.get(face)
    if not (h and BEANIE):
        return {}
    return {"hat": BEANIE, "hat_socket": "head", "hat_loc": h["hat_loc"], "hat_rot": h["hat_rot"], "hat_scale": h["hat_scale"], "hat_tint": tint}


def grooms_of(person):
    return [g for g in cast.get(person, {}).get("grooms", []) if g.get("groom")]


changed = []
# Ji-woo: the rebuilt MetaHuman (meshes renamed after the character asset by the build)
J = "/Game/Story/MetaHumans/Jiwoo/Jiwoo"
jw = {"body": f"{J}/Body/SKM_MH_Jiwoo_Aera_BodyMesh", "face": f"{J}/Face/SKM_MH_Jiwoo_Aera_FaceMesh", "garment": f"{J}/Clothing/MH_Jiwoo_Aera_Outfits"}
if all(exists(p) for p in jw.values()):
    cast["jiwoo"].update(jw)
    cast["jiwoo"].update(hat(jw["face"], [0.86, 0.83, 0.76]))
    changed.append("jiwoo")
else:
    print("jiwoo: missing", [p for p in jw.values() if not exists(p)])

for look, person, tint in (("marek_ai", "marek", [0.22, 0.22, 0.23]), ("nima_ai", "nima", [0.5, 0.13, 0.11]), ("elena_ai", "elena", [0.12, 0.42, 0.42])):
    hl = f"{look[:-3]}_hl"                                   # marek_hl ...
    body = ai.get(hl, {}).get("mesh", "")
    abp = f"/Game/Story/AI/{hl}/Anims/ABP_Unarmed_{hl}"
    face = cast.get(person, {}).get("face", "")
    if not (exists(body) and exists(abp) and exists(face)):
        print(look, "kept (headless body / abp / face missing)", exists(body), exists(abp), exists(face)); continue
    new = {"body": body, "abp": abp, "face": face, "garment": "", "grooms": grooms_of(person)}
    new.update(hat(face, tint))
    cast[look] = new
    changed.append(look)

ph = ai.get("pema_body_hl", {}).get("mesh", "")
pabp = "/Game/Story/AI/pema_body_hl/Anims/ABP_Unarmed_pema_body_hl"
pface = cast.get("pema", {}).get("face", "")
braid = [g for g in grooms_of("pema") if g.get("slot") != "Hair"]
B = "/Game/Story/MetaHumans/Pema/Pema/Grooms"
_bd = ROOT + "/Content" + B[len("/Game"):]
if os.path.exists(f"{_bd}/Hair_M_UpdoDutchBraid.uasset") and os.path.exists(f"{_bd}/GB_Hair_M_UpdoDutchBraid.uasset"):
    braid = [{"slot": "Hair", "groom": f"{B}/Hair_M_UpdoDutchBraid.Hair_M_UpdoDutchBraid",
              "binding": f"{B}/GB_Hair_M_UpdoDutchBraid.GB_Hair_M_UpdoDutchBraid"}] + braid
if exists(ph) and exists(pabp) and exists(pface) and any(g["slot"] == "Hair" and "Dutch" in g["groom"] for g in braid):
    cast["pema"] = {"body": ph, "abp": pabp, "face": pface, "garment": "", "grooms": braid}
    changed.append("pema")
else:
    print("pema kept", exists(ph), exists(pabp), exists(pface), [g["groom"] for g in braid if g["slot"] == "Hair"])

json.dump(cast, open(CAST, "w", encoding="utf-8"), indent=1, ensure_ascii=False)
print("changed looks:", changed)
