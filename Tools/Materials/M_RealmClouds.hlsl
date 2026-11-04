// Realm cloud deck: one big plane above the player. Smooth gradient noise, strongly warped, drifts with the wind; billows shaded from the slope; fades to the horizon.
// Output rgb + a. Inputs: WP (world pos, cm), Cam, T, Cover 0..1, Speed (m/s), Scale (km per billow), Dark, Light.
struct CloudFx {
  float2 G(float2 p){ float a = 6.2831853 * frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453); return float2(cos(a), sin(a)); }
  float N(float2 p){ float2 i=floor(p),f=frac(p),u=f*f*f*(f*(f*6.0-15.0)+10.0);
    return lerp(lerp(dot(G(i),f),dot(G(i+float2(1,0)),f-float2(1,0)),u.x),lerp(dot(G(i+float2(0,1)),f-float2(0,1)),dot(G(i+float2(1,1)),f-float2(1,1)),u.x),u.y); }
  float Fbm(float2 p){ float s=0,a=0.6; for(int o=0;o<6;o++){ s+=a*N(p); p=float2(p.x*1.7-p.y*1.1,p.x*1.1+p.y*1.7)+3.7; a*=0.52; } return s; }  // about -0.6..0.6
};
CloudFx c;
float2 wind = float2(0.97, 0.24) * Speed * 100.0 * T;
float2 p = (WP.xy + wind) / (Scale * 100000.0) * 2.2;
float2 warp = float2(c.Fbm(p * 0.6 + 3.1), c.Fbm(p * 0.6 + 9.4));
float2 q = p + warp * 0.75 + float2(T * Speed * 0.0005, 0.0);
float n = c.Fbm(q);
float nx = c.Fbm(q + float2(0.05, 0.03));
float edge = 0.62 - Cover * 1.15;                      // Cover 1 closes the sky, small values leave torn streaks
float a = smoothstep(edge, edge + 0.3, n);
float depth = saturate((n - edge) / 0.5);
float lit = saturate(0.55 + (n - nx) * 6.0);
float3 col = lerp(Dark, Light, lit * (1.0 - depth * 0.6));
float2 d = (WP.xy - Cam.xy) / 100.0;
float far = saturate(length(d) / 120000.0);            // towards the horizon the deck thins into haze
a *= 1.0 - far * far;
return float4(col * a, a);
