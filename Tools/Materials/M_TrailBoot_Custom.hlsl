// One boot print for the snow trail map. UV.x runs heel (0) -> toe (1), UV.y across the sole; Mirror = 1 for the left boot.
// Returns the depth to write (0..1), Op = coverage: a sole outline (heel pad + toe pad joined by a narrower arch on the
// inside edge) with steep sides; the heel and ball press in deepest, the arch a little less.
float2 p = UV;
if (Mirror > 0.5) p.y = 1.0 - p.y;
p = p * 2.0 - 1.0;                         // -1..1 both ways
float heel = length(float2((p.x + 0.52) / 0.42, p.y / 0.70));
float toe  = length(float2((p.x - 0.38) / 0.60, (p.y + 0.04) / 0.86));
float arch = length(float2(p.x / 0.55, (p.y + 0.18) / 0.55));   // arch bridge sits towards the outer edge
float d = min(min(heel, toe), arch);
Op = 1.0 - smoothstep(0.86, 1.0, d);
float press = max(1.0 - smoothstep(0.0, 1.0, heel), 1.0 - smoothstep(0.0, 1.0, toe));
return Depth * lerp(0.82, 1.0, press);
