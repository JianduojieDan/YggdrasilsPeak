# Offline (python -I): downloads the free CC BY garments chosen in research_notes/NPC clothing realism/DECISION.md
# (Objaverse 1.0 mirror of Sketchfab models, no login) into Downloads/objaverse_garments_2026-11-15/<key>/model.glb,
# reads each model's author + licence from the public Sketchfab API, and writes manifest.json + CREDITS.txt there.
#   python -I Tools/Story/fetch_garment_assets.py [keys...]
import hashlib, json, os, sys, urllib.request

OUT = "F:/UE_project/YggdrasilsPeak/Downloads/objaverse_garments_2026-11-15"
HF = "https://huggingface.co/datasets/allenai/objaverse/resolve/main/glbs/"
ITEMS = {   # key: (objaverse path, role)
    "A1_puffer_jacket":   ("000-149/7d95660645834760a0a638775e5fe4a7", "down jacket (Nima; Ji-woo fallback)"),
    "A2_winter_jacket":   ("000-120/f77f7f7c14854173a60544dee5f7de0f", "down jacket (Marek)"),
    "A8_fleece_jacket":   ("000-132/26e7c5710ca5471d9f5f977dd1499ea0", "softshell/fleece (Elena)"),
    "A9_pile_fleece":     ("000-084/f21e495f008f4c959d4974a3707a65ac", "pile fleece (porter)"),
    "A13_trousers":       ("000-136/836ccbe0600f44ceb0c7b4c217843fe4", "trousers (all, fallback)"),
    "A15_beanie_rib":     ("000-027/ff6dc83c818c4e2aa722cb89e6ada577", "2x2 rib beanie (Elena)"),
    "A16_winter_hat":     ("000-091/637a36cb74204c719883af7feb4b7c28", "winter hat (Ji-woo)"),
    "A17_hat_winter2":    ("000-122/8120ab4c7d214d77ac34220c10397fa8", "bobble hat scan (Nima)"),
    "A18_black_beanie":   ("000-152/edb7f2c044b64d5da40484495239fee3", "black knit beanie scan (Marek)"),
    "A22_silk_scarf":     ("000-094/c9e99dbc24ac427ea765ae79b2e3796b", "tied silk head scarf scan (Pema experiment)"),
}


def get(url, binary=True):
    req = urllib.request.Request(url, headers={"User-Agent": "asset-fetch"})
    data = urllib.request.urlopen(req, timeout=120).read()
    return data if binary else data.decode("utf-8")


def main(keys):
    os.makedirs(OUT, exist_ok=True)
    mpath = os.path.join(OUT, "manifest.json")
    manifest = json.load(open(mpath)) if os.path.exists(mpath) else {}
    for k in keys:
        path, role = ITEMS[k]; uid = path.split("/")[1]
        folder = os.path.join(OUT, k); os.makedirs(folder, exist_ok=True)
        glb = os.path.join(folder, "model.glb")
        try:
            if not os.path.exists(glb):
                data = get(HF + path + ".glb"); open(glb, "wb").write(data)
            data = open(glb, "rb").read()
            meta = json.loads(get(f"https://api.sketchfab.com/v3/models/{uid}", binary=False))
            lic = meta.get("license") or {}
            manifest[k] = dict(role=role, title=meta.get("name"), author=(meta.get("user") or {}).get("displayName"),
                               author_page=(meta.get("user") or {}).get("profileUrl"), url=meta.get("viewerUrl"),
                               licence=lic.get("label"), licence_requirements=lic.get("requirements"),
                               source=HF + path + ".glb", bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
            print("got", k, len(data) // 1024, "KB", manifest[k]["licence"], "-", manifest[k]["title"], "by", manifest[k]["author"])
        except Exception as e:
            print("FAILED", k, e)
    json.dump(manifest, open(mpath, "w"), indent=1)
    with open(os.path.join(OUT, "CREDITS.txt"), "w", encoding="utf-8") as f:
        f.write("Garments used on the Chapter 1 cast (Chomolungma), downloaded 2026-11-15 from the Objaverse 1.0 mirror\n"
                "(Deitke et al. 2022, ODC-By) of Sketchfab models. Each model: Creative Commons Attribution 4.0,\n"
                "modified (fitted to each MetaHuman body, decimated, skinned, retextured).\n\n")
        for k, m in sorted(manifest.items()):
            f.write(f"- \"{m['title']}\" by {m['author']} - {m['licence']} - {m['url']}  ({k}: {m['role']})\n")
    print("manifest", len(manifest))


if __name__ == "__main__":
    main(sys.argv[1:] or list(ITEMS))
