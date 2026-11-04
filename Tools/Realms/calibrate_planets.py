# Runs OUTSIDE the editor, PIE running at the showcase camera (needs uepy). Measures the sky of the reference shot with ray traces, solves
# each planet's angular diameter from its target AREA share of the sky (full disk, limb projected through the real camera), places it so
# the ridge overlaps 15% of its diameter, writes Config/RealmLooks.json (PlanetDegrees/Yaw/Pitch). Stable angular size: nothing resizes at runtime.
import json, math, subprocess, sys
ROOT = "F:/UE_project/YggdrasilsPeak"; UEPY = ROOT + "/Tools/RoutePlanning/uepy.py"
TARGET = {"Midgard": 8, "Niflheim": 15, "Vanaheim": 12, "Muspelheim": 18, "Alfheim": 10, "Asgard": 14, "Hel": 16, "Jotunheim": 20, "Svartalfheim": 11}
W, H = 2043, 933; YAW = 12.3; OVERLAP = 0.15
GX, GY = 96, 44
INNER = r'''
import unreal, math, json
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc=unreal.GameplayStatics.get_player_controller(w,0); cm=pc.player_camera_manager
pawn=pc.get_controlled_pawn(); fov=cm.get_fov_angle(); cam=cm.get_camera_location()
W,H,GX,GY,YAW=%d,%d,%d,%d,%f
tx=math.tan(math.radians(fov/2))*(9.0/16.0)*(W/H); res={"fov":fov,"cam":[cam.x,cam.y,cam.z],"pitch":{}}
for P in (28,32,36,40):
    cy,sy,cp,sp=math.cos(math.radians(YAW)),math.sin(math.radians(YAW)),math.cos(math.radians(P)),math.sin(math.radians(P))
    F=(cp*cy,cp*sy,sp); Rt=(-sy,cy,0); Up=(-sp*cy,-sp*sy,cp)
    top=[]
    for i in range(GX):
        x=((i+.5)/GX*2-1)*tx; col=GY
        for j in range(GY):
            y=(1-(j+.5)/GY*2)*tx*H/W
            d=[F[k]+x*Rt[k]+y*Up[k] for k in range(3)]; L=4000000
            e=unreal.Vector(cam.x+d[0]*L,cam.y+d[1]*L,cam.z+d[2]*L)
            if unreal.SystemLibrary.line_trace_single(w,cam,e,unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[pawn],unreal.DrawDebugTrace.NONE,True): col=j; break
        top.append(col)
    res["pitch"][str(P)]=top
print("JSON"+json.dumps(res))
''' % (W, H, GX, GY, YAW)
open("_cal.txt", "w").write(INNER)
out = subprocess.run([sys.executable, "-I", UEPY, "_cal.txt"], capture_output=True, text=True).stdout
d = json.loads(out.split("JSON")[1].split("\n")[0]); fov = d["fov"]; tx = math.tan(math.radians(fov / 2)) * (9.0 / 16.0) * (W / H); f = (W / 2) / tx   # vertical FOV is what stays fixed: 90 is the 16:9 horizontal
frac = {p: sum(t) / (GX * GY) for p, t in d["pitch"].items()}; print("fov", fov, "sky fraction by pitch", {k: round(v, 3) for k, v in frac.items()})
P = min(frac, key=lambda k: abs(frac[k] - 0.5)); top = d["pitch"][P]; sky = frac[P] * W * H; pitch = float(P)
print("reference pitch", P, "sky px", round(sky))
def basis(y, p):
    cy, sy, cp, sp = math.cos(math.radians(y)), math.sin(math.radians(y)), math.cos(math.radians(p)), math.sin(math.radians(p))
    return (cp * cy, cp * sy, sp), (-sy, cy, 0), (-sp * cy, -sp * sy, cp)
CF, CR, CU = basis(YAW, pitch)
dot = lambda a, b: sum(x * y for x, y in zip(a, b))
def project(v):
    z = dot(v, CF)
    return W / 2 + f * dot(v, CR) / z, H / 2 - f * dot(v, CU) / z
def limb(py, pp, deg, n=180):
    F, Rr, Uu = basis(py, pp); a = math.radians(deg / 2); pts = []
    for i in range(n):
        t = 2 * math.pi * i / n; pts.append(project([math.cos(a) * F[k] + math.sin(a) * (math.cos(t) * Rr[k] + math.sin(t) * Uu[k]) for k in range(3)]))
    return pts
def area(pts): return abs(sum(pts[i][0] * pts[(i + 1) % len(pts)][1] - pts[(i + 1) % len(pts)][0] * pts[i][1] for i in range(len(pts)))) / 2
def ridge_y(x): return top[min(GX - 1, max(0, int(x / W * GX)))] / GY * H
def place(sx, pct):
    # screen target -> direction; iterate size and height so bottom sits OVERLAP of the diameter below the ridge
    deg = 20.0; py, pp = YAW, pitch
    for _ in range(60):
        pts = limb(py, pp, deg); A = area(pts); deg *= math.sqrt(sky * pct / 100 / A)
        pts = limb(py, pp, deg); ys = [q[1] for q in pts]; xs = [q[0] for q in pts]; D = max(ys) - min(ys)
        cx = (max(xs) + min(xs)) / 2
        err = (ridge_y(cx) + OVERLAP * D) - max(ys); pp += math.degrees(math.atan(-err / f)) * 0.7
        py += math.degrees(math.atan((sx - cx) / f)) * 0.7
    pts = limb(py, pp, deg); return deg, py, pp, 100 * area(pts) / sky, min(q[1] for q in pts), max(q[1] for q in pts), max(q[0] for q in pts) - min(q[0] for q in pts)
low = max(range(GX // 4, 3 * GX // 4), key=lambda i: -top[i] if False else top[i]); sx = (low + .5) / GX * W
print("ridge notch x", round(sx))
J = json.load(open(ROOT + "/Config/RealmLooks.json"))
for n, pct in TARGET.items():
    deg, py, pp, a, y0, y1, dx = place(sx, pct)
    J[n].update(PlanetDegrees=round(deg, 3), PlanetYaw=round(py, 3), PlanetPitch=round(pp, 3))
    print(f"{n:13s} target {pct:2d}%  solved {a:5.2f}%  {deg:5.2f} deg  diam {dx:4.0f}px  top y {y0:4.0f}  bottom {y1:4.0f}" + ("  CLIPPED AT TOP" if y0 < 0 else ""))
json.dump(J, open(ROOT + "/Config/RealmLooks.json", "w"), indent=1)
print("PITCH", P)
