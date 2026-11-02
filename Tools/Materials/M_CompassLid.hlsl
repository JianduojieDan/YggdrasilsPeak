// Inside of the rune compass's lid: the World Tree engraving (T_CompassEmblem) projected onto the lid's disc in the
// mesh's own space. LP = local position (cm, UE axes); the lid is the Blender disc at (0, 5.2, 4.5) cm turned 100 deg
// about X (Tools/UI/story_items.py), case origin 1.75 cm up. Returns base colour; Mask out = engraved lines.
float3 B = float3(LP.x, -LP.y, LP.z + 1.75) * 0.01;          // back to Blender metres
float3 C = float3(0.0, 0.052, 0.045);
float u = B.x - C.x;
float v = dot(B - C, float3(0.0, -0.17365, 0.98481));
// (the projection lands a whole repeat off the 0..1 square - origin of the imported case - so wrap it back)
float2 uv = frac(float2(u / 0.1 + 0.5, 0.5 - v / 0.1 - 0.2));   // a little inside the rim, centred
// mip 0: SampleGrad and lower mips came out black on this viewmodel (checked with debug colours); the strokes are
// bold enough not to shimmer at the held distance
float m = Emblem.SampleLevel(EmblemSampler, uv, 0).r;
Mask = m;
Rough = lerp(0.5, 0.28, m);
return lerp(float3(0.12, 0.075, 0.035), float3(0.85, 0.62, 0.24), m);
