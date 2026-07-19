#version 330 core
// Post-processing + reload button
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTex;
uniform float reloadFlash;  // 1.0 = just reloaded, fades to 0

void main() {
    vec3 color = texture(screenTex, TexCoord).rgb;

    // Vignette
    float vignette = 1.0 - length(TexCoord - 0.5) * 0.45;
    color *= vignette;

    // Warm tone
    color *= vec3(1.04, 1.0, 0.94);

    // ---- Reload button (bottom-right) ----
    vec2 buv = (TexCoord - vec2(0.87, 0.03)) / vec2(0.11, 0.055);
    if (buv.x >= 0.0 && buv.x <= 1.0 && buv.y >= 0.0 && buv.y <= 1.0) {
        float e = 0.0;
        e = max(e, step(buv.x, 0.02));  e = max(e, step(1.0-buv.x, 0.02));
        e = max(e, step(buv.y, 0.02));  e = max(e, step(1.0-buv.y, 0.02));
        vec3 btn = mix(vec3(0.12,0.30,0.12), vec3(0.22,0.55,0.22), reloadFlash);
        btn = mix(btn, vec3(0.03), e);
        // Ring icon
        vec2 ic = buv - 0.5;
        float r = length(ic);
        float ring = smoothstep(0.25,0.30,r) - smoothstep(0.43,0.48,r);
        float arr = 0.0;
        if (ic.y > 0.32 && abs(ic.x) < 0.08) arr = 1.0;
        btn = mix(btn, vec3(0.85), max(ring, arr) * 0.75);
        color = btn;
    }

    FragColor = vec4(color, 1.0);
}
