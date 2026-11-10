// M_CampCanvas: expedition fabric that has been up here for a while. The Fab tent's dye is thrown away and only its light and
// dark (seams, folds) kept, under one muted colour per instance; a little snow settles only on what faces nearly straight up, softly.
// Inputs: Col (texture), Tint, WP (world position), VN (world normal). Cheap on purpose: one texture, no extra samples.
float l = dot(Col, float3(0.3, 0.59, 0.11));
float3 c = Tint * lerp(0.75, 1.08, saturate(l * 1.8));
float snow = smoothstep(0.86, 0.98, VN.z);
c = lerp(c, float3(0.78, 0.82, 0.88), snow * 0.7);
Rough = lerp(0.85, 0.92, snow);
return c;
