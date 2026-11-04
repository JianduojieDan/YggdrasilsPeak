# Runs OUTSIDE the editor over MCP (needs nia + mcp helpers on the path): strips the NS_Spindrift copies down to one Grains
# emitter and tunes them into Muspelheim's ember flakes and Hel's starfall frost. Re-runnable.
import json, nia
E_="Grains"; SP="ParticleSpawnScript"; UP="ParticleUpdateScript"; EU="EmitterUpdateScript"
def setv(mod, script, name, fn):
    ref=nia.ref(E_,script,mod,-1,[name])
    d=nia.call("GetStackInputData",{"stackInputRef":ref})["returnValue"]
    fn(d["value"])
    r=nia.call("SetStackInputData",{"stackInputRef":ref,"inputData":d})
    if "rror" in json.dumps(r)[:200]: print("FAIL",mod,name,json.dumps(r)[:200])
def setf(k,v): return lambda d: d.__setitem__(k,v)
def setd(**kw): return lambda d: d.update(kw)
TUNE={
 "NS_RealmEmbers":dict(rate="130.0",life=(5,9),col=dict(r=70,g=15,b=1.5,a=1),size=(dict(x=5,y=5),dict(x=16,y=16)),box=dict(x=4000,y=4000,z=1400),off="float3(0.0, 0.0, 350.0)",vel="float3(35.0, 15.0, 45.0)",grav=-20,align="Unaligned"),
 "NS_RealmMotes":dict(rate="140.0",life=(1.6,3.2),col=dict(r=7,g=11,b=26,a=1),size=(dict(x=1.2,y=25),dict(x=3,y=70)),box=dict(x=5000,y=5000,z=1800),off="float3(0.0, 0.0, 700.0)",vel="float3(18.0, 8.0, -220.0)",grav=-40,align="VelocityAligned"),
 # Jotunheim: a gale of ice, flat and fast, thin streaks
 "NS_RealmBlizzard":dict(rate="2600.0",life=(0.5,1.0),col=dict(r=2.2,g=2.5,b=3.0,a=1),size=(dict(x=1.2,y=50),dict(x=3,y=140)),box=dict(x=4500,y=4500,z=1000),off="float3(-1400.0, -350.0, 350.0)",vel="float3(2300.0, 550.0, -160.0)",grav=-30,align="VelocityAligned"),
 # Niflheim: ice crystals hanging in still fog
 "NS_RealmCrystals":dict(rate="90.0",life=(6,11),col=dict(r=5,g=5.5,b=6.5,a=1),size=(dict(x=1.2,y=1.2),dict(x=3,y=3)),box=dict(x=2600,y=2600,z=900),off="float3(0.0, 0.0, 200.0)",vel="float3(5.0, 2.0, -3.0)",grav=-2,align="Unaligned"),
 # Alfheim: rare slow glints
 "NS_RealmPrism":dict(rate="30.0",life=(4,8),col=dict(r=9,g=8,b=13,a=1),size=(dict(x=2,y=2),dict(x=5,y=5)),box=dict(x=3000,y=3000,z=1100),off="float3(0.0, 0.0, 300.0)",vel="float3(4.0, 8.0, 10.0)",grav=0,align="Unaligned"),
 # Vanaheim: green sparks climbing towards the aurora
 "NS_RealmSpores":dict(rate="55.0",life=(6,11),col=dict(r=1.5,g=13,b=6,a=1),size=(dict(x=2,y=2),dict(x=5,y=5)),box=dict(x=3500,y=3500,z=400),off="float3(0.0, 0.0, 50.0)",vel="float3(6.0, 4.0, 55.0)",grav=2,align="Unaligned"),
 # Asgard: gold dust rising along the light, a steady breeze
 "NS_RealmGold":dict(rate="130.0",life=(4,8),col=dict(r=18,g=11,b=3,a=1),size=(dict(x=1.5,y=1.5),dict(x=4,y=4)),box=dict(x=3500,y=3500,z=900),off="float3(0.0, 0.0, 150.0)",vel="float3(60.0, 20.0, 45.0)",grav=3,align="Unaligned"),
}
for name,t in TUNE.items():
    nia.S={"refPath":f"/Game/Environment/Realms/{name}.{name}"}
    print(name, json.dumps(nia.call("RemoveEmitter",{"emitterToRemove":nia.ref("Powder")}))[:120])
    setv("SpawnRate",EU,"SpawnRate",setf("hlslExpression",t["rate"]))
    setv("InitializeParticle",SP,"Lifetime Min",setf("value",t["life"][0]))
    setv("InitializeParticle",SP,"Lifetime Max",setf("value",t["life"][1]))
    setv("InitializeParticle",SP,"Color",setd(**t["col"]))
    setv("InitializeParticle",SP,"Sprite Size Min",setd(**t["size"][0]))
    setv("InitializeParticle",SP,"Sprite Size Max",setd(**t["size"][1]))
    setv("InitializeParticle",SP,"Position Offset",setf("hlslExpression",t["off"]))
    setv("ShapeLocation",SP,"Box Size",setd(**t["box"]))
    setv("AddVelocity",SP,"Velocity",setf("hlslExpression",t["vel"]))
    setv("GravityForce",UP,"Gravity",setd(x=0,y=0,z=t["grav"]))
    for m in ["WindForce","AerodynamicDrag","Collision","AlignSpriteToMeshOrientation","AlignParticlesWithCollisionPlane","Drag"]:
        nia.call("SetModuleEnabled",{"moduleRef":nia.ref(E_,UP,m),"bEnabled":False})
    pv={"Material":{"refPath":"/Game/Environment/Realms/M_RealmMote.M_RealmMote"},"Alignment":t["align"]}
    print(json.dumps(nia.call("SetRendererData",{"renderer":nia.ref(E_,ri=0),"rendererData":{"propertyValues":json.dumps(pv)}}))[:200])
    print(json.dumps(nia.call("GetSystemCompileState",{"system":nia.S}))[:300])
    print(json.dumps(nia.call("GetStackIssues",{"system":nia.S}))[:400])
