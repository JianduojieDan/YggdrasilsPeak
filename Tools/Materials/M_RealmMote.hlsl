// Realm particle: a soft glowing point, twinkling per particle. Col = Niagara particle colour (HDR), alpha = life fade.
float2 p = UV * 2.0 - 1.0;
float d = dot(p, p);
float core = 2.2 * exp(-d * 60.0) + 0.5 * exp(-d * 12.0) + 0.14 * exp(-d * 3.0);   // hot centre, its own glow around it
core *= saturate(1.0 - d);
float tw = 0.65 + 0.35 * sin(T * (2.0 + Rand * 5.0) + Rand * 40.0);
return Col.rgb * Col.a * core * tw;
