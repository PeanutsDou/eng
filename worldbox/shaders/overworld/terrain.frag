#version 330 core
// WorldBox-style pixel-art terrain map
// Noise-based height → biome palette → trees, mountains, water
in vec2 TexCoord;
out vec4 FragColor;

uniform vec2  mapSize;
uniform float seed;
uniform float zoom;
uniform vec2  pan;
uniform float borderWidth;

// ============================================================
//  Noise functions
// ============================================================

float hash(vec2 p) {
    p  = fract(p * vec2(0.1031, 0.1030));
    p += dot(p, p.yx + 19.19);
    return fract((p.x + p.y) * p.x);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1,0)), f.x),
               mix(hash(i + vec2(0,1)), hash(i + vec2(1,1)), f.x), f.y);
}

float fbm(vec2 p, int octaves) {
    float v = 0.0, amp = 0.5, freq = 1.0, total = 0.0;
    for (int i = 0; i < 6; i++) {
        if (i >= octaves) break;
        v += amp * noise(p * freq);
        total += amp;
        amp *= 0.5; freq *= 2.0;
    }
    return v / total;
}

// Voronoi-like ridges for mountains
float ridges(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float md = 1.0, mr = 0.0;
    for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++) {
        vec2 n = vec2(float(x), float(y));
        vec2 pt = n + hash(i + n) * 0.5;
        float d = length(f - pt);
        if (d < md) { mr = md; md = d; }
        else if (d < mr) mr = d;
    }
    return mr - md; // ridge = 2nd closest - closest
}

// ============================================================
//  Main
// ============================================================

void main() {
    vec2 uv = TexCoord;

    // ---- Pixel grid (160x120 → chunky pixel art) ----
    const float PW = 160.0;
    const float PH = 120.0;
    vec2 pix = floor(uv * vec2(PW, PH)) / vec2(PW, PH) + 0.5 / vec2(PW, PH);

    // ---- World coordinates (zoom + pan) ----
    vec2 p = pix * zoom + pan / mapSize * 0.5;
    // Wrap so seed variation creates different worlds
    p += seed * 12.345;

    // ---- Elevation (0 = deep ocean, 1 = peak) ----
    float e1 = fbm(p * 3.5, 5);
    float e2 = fbm(p * 7.0 + vec2(2.71, 8.33), 4) * 0.3;
    float elev = e1 * 0.7 + e2;
    elev = smoothstep(0.05, 0.95, elev);

    // ---- Moisture (rain shadow / continent effect) ----
    float moist = fbm(p * 3.0 + vec2(5.12, 1.97), 4);
    moist = mix(moist, 1.0 - elev * 0.15, 0.5); // high areas drier

    // ---- Temperature (latitude-like band) ----
    float temp = fbm(p * 4.0 + vec2(9.31, 4.28), 3) * 0.6 + 0.2;

    // ---- Ridge noise (mountain detail) ----
    float ridge = ridges(p * 18.0) * 0.5 + 0.5;

    // ---- Tree placement noise ----
    float treeN = fbm(p * 25.0 + vec2(3.91, 6.54), 3);

    // ---- River noise ----
    float riverN = ridges(p * 9.0 + vec2(1.73, 4.91));

    // ============================================================
    //  Biome selection
    // ============================================================
    vec3 col = vec3(0.0);

    if (elev < 0.28) {
        // ===== DEEP OCEAN =====
        float depth = elev / 0.28;
        // Dark blue bands
        float band = floor(depth * 3.5);
        if      (band <= 1.0) col = vec3(0.04, 0.13, 0.35);  // deepest
        else if (band <= 2.0) col = vec3(0.06, 0.18, 0.42);
        else                  col = vec3(0.08, 0.24, 0.48);  // mid-ocean

    } else if (elev < 0.32) {
        // ===== SHALLOW WATER =====
        float t = (elev - 0.28) / 0.04;
        float band = floor(t * 3.0);
        if      (band <= 1.0) col = vec3(0.10, 0.28, 0.52);
        else                  col = vec3(0.14, 0.34, 0.56);

    } else if (elev < 0.35) {
        // ===== BEACH / SAND =====
        float band = floor((elev - 0.32) / 0.03 * 2.5);
        if      (band <= 1.0) col = vec3(0.75, 0.68, 0.45);  // wet sand
        else                  col = vec3(0.85, 0.78, 0.52);  // dry sand

    } else if (elev < 0.58) {
        // ===== GRASSLAND / PLAINS =====
        float t = (elev - 0.35) / 0.23;

        // 3 green shades
        float gBand = floor((moist * 0.7 + t * 0.3) * 3.5);
        if      (gBand <= 1.0) col = vec3(0.28, 0.55, 0.18); // lush
        else if (gBand <= 2.0) col = vec3(0.38, 0.62, 0.22); // mid green
        else                   col = vec3(0.48, 0.68, 0.25); // dry grass

        // ---- Tiny flowers (random colored dots) ----
        if (hash(pix * 80.0 + seed) > 0.97) {
            float fc = hash(pix * 80.0 + seed + 1.0);
            if      (fc < 0.33) col = vec3(0.92, 0.82, 0.22); // yellow
            else if (fc < 0.66) col = vec3(0.92, 0.35, 0.35); // red
            else                col = vec3(0.82, 0.72, 0.92); // lavender
        }

        // ---- Trees (clustered on grassland) ----
        if (moist > 0.4 && treeN > 0.55) {
            // Tree trunk
            float treeHash = hash(floor(pix * vec2(PW, PH) / 3.0) + seed);
            if (treeHash > 0.45) {
                // Tree canopy shape (2x3 block)
                vec2 lp = fract(pix * vec2(PW, PH) / 3.0);
                float trunk = (abs(lp.x - 0.5) < 0.12 && lp.y < 0.7) ? 1.0 : 0.0;
                if (trunk > 0.5) {
                    col = vec3(0.22, 0.15, 0.08); // trunk brown
                } else {
                    // Canopy: 3-pixel blob on top
                    float cx = abs(lp.x - 0.5) * 4.0;
                    float cy = (lp.y - 0.65) * 5.0;
                    if (cx * cx + cy * cy < 1.8 && lp.y > 0.4) {
                        float cs = hash(floor(pix * vec2(PW, PH) / 3.0) + seed + 2.0);
                        if      (cs < 0.5) col = vec3(0.08, 0.35, 0.06); // dark leaf
                        else                col = vec3(0.14, 0.44, 0.10); // leaf highlight
                    }
                }
            }
        }

    } else if (elev < 0.70) {
        // ===== FOREST =====
        float t = (elev - 0.58) / 0.12;

        // Base forest floor
        float fBand = floor((moist * 0.6 + t * 0.4) * 3.0);
        if      (fBand <= 1.0) col = vec3(0.12, 0.32, 0.08);  // dense dark
        else if (fBand <= 2.0) col = vec3(0.18, 0.38, 0.10);
        else                   col = vec3(0.25, 0.42, 0.12);  // lighter edge

        // Dense tree canopy
        if (treeN > 0.3) {
            vec2 lp = fract(pix * vec2(PW, PH) / 2.0);
            float cx = abs(lp.x - 0.5) * 3.5;
            float cy = abs(lp.y - 0.5) * 3.5;
            if (cx + cy < 2.5) {
                float ts = hash(floor(pix * vec2(PW, PH) / 2.0) + seed + 5.0);
                if      (ts < 0.55) col = vec3(0.05, 0.22, 0.03); // deep forest
                else if (ts < 0.85) col = vec3(0.10, 0.30, 0.05); // mid canopy
                else                col = vec3(0.18, 0.38, 0.08); // sunlit leaf
            }
        }

    } else if (elev < 0.85) {
        // ===== ROCKY / HILLS =====
        float t = (elev - 0.70) / 0.15;

        // Rocky base
        float rBand = floor((t + ridge * 0.3) * 3.5);
        if      (rBand <= 1.0) col = vec3(0.38, 0.35, 0.28); // brown rock
        else if (rBand <= 2.0) col = vec3(0.48, 0.42, 0.35);
        else                   col = vec3(0.55, 0.50, 0.42); // light rock

        // Scattered boulders
        if (ridge > 0.55 && treeN > 0.5) {
            col = mix(col, vec3(0.32, 0.30, 0.25), 0.6);
        }

        // Sparse trees on lower hills
        if (t < 0.4 && treeN > 0.7 && moist > 0.35) {
            vec2 lp = fract(pix * vec2(PW, PH) / 3.0);
            float cx = abs(lp.x - 0.5) * 3.5;
            float cy = abs(lp.y - 0.5) * 3.5;
            if (cx + cy < 1.8) {
                col = vec3(0.06, 0.24, 0.04);
            }
        }

    } else if (elev < 0.94) {
        // ===== MOUNTAIN =====
        float t = (elev - 0.85) / 0.09;

        // Mountain face: directional light from top-left
        float lightFace = fbm(p * 40.0 + vec2(0.5, 0.5), 2);
        float shadow = smoothstep(0.35, 0.65, lightFace);

        float mBand = floor((t + shadow * 0.4) * 3.5);
        if      (mBand <= 1.0) col = vec3(0.35, 0.33, 0.30); // shadow face
        else if (mBand <= 2.0) col = vec3(0.50, 0.47, 0.42); // mid face
        else                   col = vec3(0.62, 0.58, 0.52); // lit face

        // Ridge highlights
        if (ridge > 0.6) {
            col = mix(col, vec3(0.70, 0.65, 0.58), (ridge - 0.6) * 2.0);
        }

    } else {
        // ===== SNOW PEAK =====
        float t = (elev - 0.94) / 0.06;

        float sBand = floor(t * 3.0);
        if      (sBand <= 1.0) col = vec3(0.78, 0.80, 0.82); // rock-snow mix
        else if (sBand <= 2.0) col = vec3(0.90, 0.92, 0.94); // snow
        else                   col = vec3(0.96, 0.97, 0.98); // pure snow cap

        // Snow sparkle
        if (hash(pix * 120.0 + seed + 9.0) > 0.94) {
            col = vec3(1.0, 1.0, 1.0);
        }
    }

    // ============================================================
    //  Rivers (thin water lines cutting through land)
    // ============================================================
    if (elev > 0.35 && elev < 0.80) {
        float river = abs(riverN);
        if (river < 0.025) {
            col = mix(col, vec3(0.10, 0.28, 0.50), 1.0 - river / 0.025);
        }
        // River banks (slightly darker sand)
        if (river < 0.045 && river >= 0.025) {
            col = mix(col, vec3(0.65, 0.55, 0.35), (0.045 - river) / 0.02 * 0.6);
        }
    }

    // ============================================================
    //  Coastal waves (subtle lighter band along shore)
    // ============================================================
    if (elev > 0.29 && elev < 0.35) {
        float shore = (elev - 0.29) / 0.06;
        // Wave foam pattern
        float wave = sin(pix.x * PW * 0.8 + seed) * 0.5 + 0.5;
        if (shore < 0.3 && wave > 0.4) {
            col = mix(col, vec3(0.55, 0.78, 0.82), (0.3 - shore) / 0.3 * 0.5);
        }
    }

    // ============================================================
    //  Grid border (subtle pixel cell borders)
    // ============================================================
    if (borderWidth > 0.0) {
        vec2 grid = abs(fract(pix * vec2(PW, PH) / 4.0) - 0.5);
        float edge = 1.0 - smoothstep(0.0, borderWidth * 0.02, min(grid.x, grid.y));
        col = mix(col, col * 0.85, edge * 0.15);
    }

    FragColor = vec4(col, 1.0);
}
