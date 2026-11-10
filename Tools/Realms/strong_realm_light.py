# Plain Python, run from anywhere. Each realm's sunlight, shadow colour and grade take the colour of its planet, strongly. Live: ygg.Realm reload.
import json
P = "F:/UE_project/YggdrasilsPeak/Config/RealmLooks.json"
d = json.load(open(P, encoding="utf-8"))
L = {
    "Niflheim":     dict(SunColor=[0.86, 0.93, 1.0], SkyColor=[0.88, 0.94, 1.0], SunScale=0.55),
    "Muspelheim":   dict(SunColor=[1.0, 0.42, 0.16], SkyColor=[1.0, 0.62, 0.48], SunScale=1.15, Gain=[1.04, 0.92, 0.82], Temp=4800),
    "Jotunheim":    dict(SunColor=[0.58, 0.74, 1.0], SkyColor=[0.62, 0.78, 1.0], SunScale=0.25),
    "Alfheim":      dict(SunColor=[1.0, 0.86, 1.0], SkyColor=[0.9, 0.82, 1.0], SunScale=0.85, Gain=[1.0, 0.97, 1.03]),
    "Vanaheim":     dict(SunColor=[0.42, 1.0, 0.66], SkyColor=[0.42, 1.0, 0.72], SunScale=0.3, SkyScale=1.1, Gain=[0.93, 1.03, 0.97], CloudCover=0.3, Saturation=1.15),
    "Svartalfheim": dict(SunColor=[0.62, 0.72, 1.0], SkyColor=[0.7, 0.78, 1.0]),
    "Hel":          dict(SunColor=[0.56, 0.44, 1.0], SkyColor=[0.62, 0.5, 1.0], SunScale=0.25, Gain=[0.97, 0.93, 1.05]),
    "Asgard":       dict(SunColor=[1.0, 0.7, 0.32], SkyColor=[1.0, 0.84, 0.58], SunScale=1.1, Gain=[1.04, 0.98, 0.88], Temp=5000),
}
for name, v in L.items():
    d[name].update(v)
json.dump(d, open(P, "w", encoding="utf-8"), indent=1)
print("updated", len(L))
