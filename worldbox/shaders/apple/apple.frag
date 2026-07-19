#version 330 core
// Pixel-art gold apple icon — round, on black background
in vec2 TexCoord;
out vec4 FragColor;

uniform vec2  mapSize;
uniform float seed;
uniform float zoom;
uniform vec2  pan;
uniform float borderWidth;
uniform float time;

void main() {
    vec2 uv = TexCoord;
    const float PW = 100.0, PH = 100.0;
    vec2 pix = floor(uv * vec2(PW, PH)) / vec2(PW, PH) + 0.5 / vec2(PW, PH);

    vec3 color = vec3(0.0);

    float cx = PW * 0.5, cy = PH * 0.47, ar = 28.0;
    float dx = pix.x * PW - cx, dy = pix.y * PH - cy;
    dy *= 1.12;
    float waist = 1.0 - smoothstep(0.0, 0.55, abs(dy / ar)) * 0.10;
    dx /= waist;
    float dist = sqrt(dx * dx + dy * dy);
    float indent = smoothstep(-ar * 0.55, -ar * 0.05, dy)
                 * smoothstep(0.4, 4.0, abs(dx)) * ar * 0.30;
    float shape = dist - indent - ar;

    // Glow
    float glow = exp(-(dist - indent) * 0.25) * 0.10;
    glow += exp(-(dist - indent) * 0.06) * 0.03;
    color += vec3(0.40, 0.25, 0.04) * glow;

    if (shape < 0.0) {
        float r = clamp((dist - indent) / ar, 0.0, 1.0);
        float z = sqrt(max(0.0, 1.0 - r * r));
        vec3 N = normalize(vec3(dx / ar, dy / ar, z));

        vec3 L1 = normalize(vec3(-0.55, -0.65, 0.52));
        float diff1 = max(0.0, dot(N, L1)) * 0.60 + 0.40;
        vec3 L2 = normalize(vec3(0.50, 0.55, 0.45));
        float diff2 = max(0.0, dot(N, L2)) * 0.18;
        float diff = diff1 + diff2;

        vec3 H = normalize(L1 + vec3(0.0, 0.0, 1.0));
        float spec = pow(max(0.0, dot(N, H)), 40.0);
        vec3 V = vec3(0.0, 0.0, 1.0);
        float rim = pow(1.0 - abs(dot(N, V)), 3.0) * 0.20;
        float light = clamp(diff * 0.68 + spec * 0.90 + rim * 0.35, 0.0, 1.0);

        float lvl = floor(light * 6.5);
        lvl = clamp(lvl, 0.0, 6.0);
        if (lvl <= 1.0)      color = mix(vec3(0.25,0.14,0.02), vec3(0.45,0.28,0.04), lvl);
        else if (lvl <= 2.0) color = mix(vec3(0.45,0.28,0.04), vec3(0.65,0.42,0.06), lvl-1.0);
        else if (lvl <= 3.0) color = mix(vec3(0.65,0.42,0.06), vec3(0.82,0.56,0.09), lvl-2.0);
        else if (lvl <= 4.0) color = mix(vec3(0.82,0.56,0.09), vec3(0.93,0.68,0.15), lvl-3.0);
        else if (lvl <= 5.0) color = mix(vec3(0.93,0.68,0.15), vec3(1.0,0.82,0.35), lvl-4.0);
        else                 color = mix(vec3(1.0,0.82,0.35), vec3(1.0,0.94,0.62), lvl-5.0);

        float bot = smoothstep(0.0, ar * 0.65, dy);
        color *= 1.0 - bot * 0.15;
        float topRefl = smoothstep(-ar*0.6, -ar*0.05, -dy) * smoothstep(0.3,0.8, r) * 0.08;
        color += vec3(1.0, 0.95, 0.70) * topRefl;
    }

    // Stem
    float sx = cx, sy = cy - ar + indent - 1.5;
    if (abs(pix.x*PW - sx) <= 2.0 && pix.y*PH > sy-10.0 && pix.y*PH < sy) {
        float t = (sy - pix.y*PH)/10.0;
        color = t < 0.55 ? vec3(0.16,0.08,0.01) : vec3(0.24,0.12,0.03);
    }
    // Leaf
    float lx0 = sx+1.5, ly0 = sy-3.0, angle = 0.55;
    float ca = cos(angle), sa = sin(angle);
    float lrx = ca*(pix.x*PW-lx0) - sa*(pix.y*PH-ly0);
    float lry = sa*(pix.x*PW-lx0) + ca*(pix.y*PH-ly0);
    if (abs(lrx) < 8.5 && abs(lry) < 2.5) {
        float taper = 1.0 - (abs(lrx)/8.5)*(abs(lrx)/8.5)*0.7;
        if (abs(lry) < 2.5*taper) {
            color = abs(lry) < 1.0 ? vec3(0.06,0.32,0.04) : vec3(0.10,0.42,0.07);
        }
    }

    FragColor = vec4(color, 1.0);
}
