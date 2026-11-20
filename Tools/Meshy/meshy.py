"""Meshy API pipeline for the chapter 1 assets (round 1).

    python Tools/Meshy/meshy.py balance
    python Tools/Meshy/meshy.py status
    python Tools/Meshy/meshy.py concepts all|<id> [<id>...] [--redo] [--dry]
    python Tools/Meshy/meshy.py sheet
    python Tools/Meshy/meshy.py models all|<id> [<id>...] [--redo] [--dry]
    python Tools/Meshy/meshy.py rig <id> [<id>...] [--dry]

The API key comes from the MESHY_API_KEY environment variable (or the user's registry copy written by setx). It is
never printed or written anywhere. Every task is written to Downloads/ai_assets/jobs.json the moment Meshy accepts it,
so nothing is ever sent twice; --redo is needed to replace a finished job. Credits: no job starts if it would take the
balance below FLOOR. Files are downloaded as soon as a task succeeds (Meshy deletes them after 3 days).
"""
import base64, json, os, sys, time
from pathlib import Path

import requests

sys.path.insert(0, str(Path(__file__).parent))
from assets import ASSETS, BY_ID, concept_prompt  # noqa: E402

API = "https://api.meshy.ai/openapi/v1"
OUT = Path("F:/UE_project/YggdrasilsPeak/Downloads/ai_assets")
JOBS = OUT / "jobs.json"
FLOOR = 0              # 2026-11-20: last credits go to the main team (user OK: polish tonight; buys more tomorrow)
MAX_ACTIVE = 10        # the plan's concurrent task limit
COST = {"concept_prop": 3, "concept_char": 6, "model": 30, "rig": 5}
IMG_MODEL = {"prop": "nano-banana", "char": "nano-banana-2"}
THREE_DAYS = 3 * 24 * 3600 - 3600


def api_key():
    # The registry copy first: setx updates it at once, while already-running processes keep the old value in their
    # environment until they restart (so a replaced key works without restarting anything).
    k = None
    if sys.platform == "win32":
        import winreg
        try:
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, "Environment") as h:
                k = winreg.QueryValueEx(h, "MESHY_API_KEY")[0]
        except OSError:
            k = None
    k = k or os.environ.get("MESHY_API_KEY")
    if not k:
        sys.exit("MESHY_API_KEY is not set")
    return k.strip()


S = requests.Session()
S.headers.update({"Authorization": f"Bearer {api_key()}", "Content-Type": "application/json"})


def call(method, path, body=None, tries=6):
    for i in range(tries):
        try:
            r = S.request(method, f"{API}{path}", json=body, timeout=60)
        except requests.RequestException as e:
            print(f"  network error ({type(e).__name__}), retrying"); time.sleep(5 * (i + 1)); continue
        if r.status_code == 429:
            msg = r.text[:200]
            wait = float(r.headers.get("Retry-After") or 10)
            print(f"  429 {msg.split(':')[0]} - waiting {wait:.0f} s"); time.sleep(wait); continue
        if r.status_code >= 500:
            print(f"  server {r.status_code}, retrying"); time.sleep(5 * (i + 1)); continue
        if r.status_code >= 400:
            raise RuntimeError(f"{method} {path} -> {r.status_code}: {r.text[:400]}")
        return r.json()
    raise RuntimeError(f"{method} {path}: gave up after {tries} tries")


def balance():
    return call("GET", "/balance")["balance"]


def load_jobs():
    return json.loads(JOBS.read_text()) if JOBS.exists() else {}


def save_jobs(jobs):
    OUT.mkdir(parents=True, exist_ok=True)
    tmp = JOBS.with_suffix(".tmp")
    tmp.write_text(json.dumps(jobs, indent=1))
    tmp.replace(JOBS)


def download(url, dest):
    dest.parent.mkdir(parents=True, exist_ok=True)
    for i in range(4):
        try:
            with requests.get(url, timeout=120, stream=True) as r:
                r.raise_for_status()
                with open(dest, "wb") as f:
                    for chunk in r.iter_content(1 << 16):
                        f.write(chunk)
            return dest
        except requests.RequestException as e:
            print(f"  download retry {i + 1} ({type(e).__name__})"); time.sleep(5)
    raise RuntimeError(f"download failed: {dest.name}")


def ext_of(url, default):
    tail = url.split("?")[0].rsplit(".", 1)
    return "." + tail[1].lower() if len(tail) == 2 and len(tail[1]) <= 4 else default


# ------------------------------------------------------------------ what each stage sends and keeps
def job_concept(a):
    return ("/text-to-image", {"ai_model": IMG_MODEL[a["kind"]], "prompt": concept_prompt(a),
                               "aspect_ratio": "3:4" if a["kind"] == "char" else "1:1"},
            COST["concept_char" if a["kind"] == "char" else "concept_prop"])


def job_model(a, jobs):
    c = jobs.get(a["id"], {}).get("concept")
    if not c or c.get("status") != "SUCCEEDED":
        raise RuntimeError(f"{a['id']}: no finished concept image")
    body = {"ai_model": "meshy-7.1", "should_texture": True, "enable_pbr": True, "texture_resolution": "2k",
            "should_remesh": True, "topology": "triangle", "target_polycount": a["tris"], "target_formats": ["glb"]}
    if a["kind"] == "char":
        body["pose_mode"] = "a-pose"
    if time.time() - c.get("finished", 0) < THREE_DAYS:
        body["input_task_id"] = c["task"]
    else:
        img = OUT / a["id"] / c["files"][0]
        mime = "image/png" if img.suffix == ".png" else "image/jpeg"
        body["image_url"] = f"data:{mime};base64," + base64.b64encode(img.read_bytes()).decode()
    return ("/image-to-3d", body, COST["model"])


def job_rig(a, jobs):
    m = jobs.get(a["id"], {}).get("model")
    if a["kind"] != "char" or not m or m.get("status") != "SUCCEEDED":
        raise RuntimeError(f"{a['id']}: needs a finished character model")
    return ("/rigging", {"input_task_id": m["task"], "height_meters": a["size"],
                         "skeleton_template": "ue5_mannequin", "include_fingers": True}, COST["rig"])


def keep_files(stage, a, task):
    d = OUT / a["id"]
    files = []
    if stage == "concept":
        url = task["image_urls"][0]
        files.append(download(url, d / f"concept{ext_of(url, '.png')}").name)
    elif stage == "model":
        files.append(download(task["model_urls"]["glb"], d / f"{a['id']}.glb").name)
        for k, url in ((task.get("texture_urls") or [{}])[0]).items():
            if url:
                files.append(download(url, d / f"tex_{k}{ext_of(url, '.png')}").name)
        if task.get("thumbnail_url"):
            files.append(download(task["thumbnail_url"], d / "model_thumb.png").name)
    elif stage == "rig":
        res = task["result"]
        for key, name in (("rigged_character_fbx_url", f"{a['id']}_rigged.fbx"), ("rigged_character_glb_url", f"{a['id']}_rigged.glb")):
            if res.get(key):
                files.append(download(res[key], d / name).name)
        for k, url in (res.get("basic_animations") or {}).items():
            if url and k.endswith("fbx_url"):
                files.append(download(url, d / f"anim_{k[:-8]}.fbx").name)
    return files


# ------------------------------------------------------------------ runner
def run(stage, ids, redo=False, dry=False):
    jobs = load_jobs()
    todo = []
    for i in ids:
        a = BY_ID[i]
        prev = jobs.get(i, {}).get(stage)
        if prev and prev.get("status") not in ("FAILED", "CANCELED") and not redo:
            print(f"skip {i}: {stage} already {prev.get('status')}"); continue
        path, body, cost = job_concept(a) if stage == "concept" else job_model(a, jobs) if stage == "model" else job_rig(a, jobs)
        todo.append((a, path, body, cost))
    total = sum(t[3] for t in todo)
    bal = balance()
    print(f"{stage}: {len(todo)} jobs, {total} credits. Balance {bal}, floor {FLOOR}.")
    if dry or not todo:
        for a, path, body, cost in todo:
            print(f"  {a['id']:16s} {cost:3d}  {path}")
        return
    if bal - total < FLOOR:
        sys.exit(f"stopped: {bal} - {total} would go below the floor of {FLOOR}")

    queue, active = list(todo), {}
    while queue or active:
        while queue and len(active) < MAX_ACTIVE:
            a, path, body, cost = queue.pop(0)
            try:
                tid = call("POST", path, body)["result"]
            except RuntimeError as e:
                if "NoMorePendingTasks" in str(e):
                    queue.insert(0, (a, path, body, cost)); time.sleep(10); break
                print(f"  {a['id']}: create failed: {e}")
                continue
            entry = jobs.setdefault(a["id"], {})
            if entry.get(stage):
                entry.setdefault(f"{stage}_old", []).append(entry[stage])
            entry[stage] = {"task": tid, "status": "PENDING", "created": time.time()}
            save_jobs(jobs)
            active[tid] = (a, path)
            print(f"  sent {a['id']} ({stage})")
        time.sleep(5)
        for tid, (a, path) in list(active.items()):
            try:
                t = call("GET", f"{path}/{tid}")
            except RuntimeError as e:
                print(f"  poll {a['id']}: {e}"); continue
            st = t.get("status")
            if st in ("SUCCEEDED", "FAILED", "CANCELED"):
                e = jobs[a["id"]][stage]
                e.update(status=st, finished=time.time(), credits=t.get("consumed_credits"))
                if st == "SUCCEEDED":
                    try:
                        e["files"] = keep_files(stage, a, t)
                    except Exception as ex:  # keep the task id: it can be fetched again for 3 days
                        e["status"] = "DOWNLOAD_FAILED"; e["error"] = str(ex)
                else:
                    e["error"] = (t.get("task_error") or {}).get("message", "")
                save_jobs(jobs)
                del active[tid]
                print(f"  {a['id']}: {e['status']} {e.get('error', '')}")
    print(f"done. Balance now {balance()}.")


def status():
    jobs = load_jobs()
    for a in ASSETS:
        j = jobs.get(a["id"], {})
        print(f"{a['id']:16s} " + "  ".join(f"{s}={j[s]['status']}" for s in ("concept", "model", "rig") if s in j))
    print("balance", balance())


def sheet(stage="concept"):
    from PIL import Image, ImageDraw, ImageFont
    jobs, tiles = load_jobs(), []
    for n, a in enumerate(ASSETS, 1):
        c = jobs.get(a["id"], {}).get(stage)
        if c and c.get("files"):
            p = OUT / a["id"] / (c["files"][0] if stage == "concept" else "model_thumb.png")
            if p.exists():
                tiles.append((n, a, p))
    cols, w = 6, 340
    h = w + 34
    rows = (len(tiles) + cols - 1) // cols
    img = Image.new("RGB", (cols * w, rows * h), (30, 30, 32))
    dr = ImageDraw.Draw(img)
    try:
        font = ImageFont.truetype("arial.ttf", 22)
    except OSError:
        font = ImageFont.load_default()
    for k, (n, a, p) in enumerate(tiles):
        x, y = (k % cols) * w, (k // cols) * h
        im = Image.open(p).convert("RGB")
        im.thumbnail((w - 8, h - 42))
        img.paste(im, (x + (w - im.width) // 2, y + 4))
        dr.text((x + 8, y + h - 32), f"{n}. {a['id']}", fill=(235, 235, 235), font=font)
    dest = OUT / f"review_{stage}s.jpg"
    img.save(dest, quality=88)
    print(dest)


if __name__ == "__main__":
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    cmd, rest = args[0], [x for x in args[1:] if not x.startswith("--")]
    flags = {x for x in args[1:] if x.startswith("--")}
    ids = [a["id"] for a in ASSETS] if rest == ["all"] else rest
    if cmd == "models" and rest == ["all"]:
        ids = [a["id"] for a in ASSETS]
    unknown = [] if cmd in ("sheet", "balance", "status") else [i for i in ids if i not in BY_ID]
    if unknown:
        sys.exit(f"unknown ids: {unknown}")
    if cmd == "balance":
        print("balance", balance())
    elif cmd == "status":
        status()
    elif cmd == "sheet":
        sheet("model" if rest == ["models"] else "concept")
    elif cmd in ("concepts", "models", "rig"):
        run({"concepts": "concept", "models": "model", "rig": "rig"}[cmd], ids, "--redo" in flags, "--dry" in flags)
    else:
        sys.exit(__doc__)
