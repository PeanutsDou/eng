#version 330 core
// Cloud overlay — semi-transparent, scrolls over terrain
in vec2 TexCoord;
out vec4 FragColor;

uniform vec2  mapSize;
uniform float seed;
uniform float zoom;
uniform vec2  pan;
uniform float borderWidth;
uniform float time;

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash21(i+vec2(0,0)), hash21(i+vec2(1,0)), f.x),
               mix(hash21(i+vec2(0,1)), hash21(i+vec2(1,1)), f.x), f.y);
}

float fbm(vec2 p) {
    float v = 0.0, a = 0.5, f = 1.0;
    for (int i = 0; i < 4; i++) { v += a * noise(p*f); f *= 2.0; a *= 0.5; }
    return v;
}

void main() {
    vec2 uv = TexCoord;

    // Scroll speed — different clouds move at different rates
    float speed = 0.03;

    // Multiple cloud layers for depth
    float c1 = fbm(vec2(uv.x * 5.0 - time * speed, uv.y * 3.0 + time * speed * 0.3));
    float c2 = fbm(vec2(uv.x * 8.0 + time * speed * 0.7, uv.y * 5.0 - time * speed * 0.5) + seed);
    float c3 = fbm(vec2(uv.x * 3.0 + time * speed * 0.4, uv.y * 7.0 + time * speed * 0.2) + seed * 2.0);

    // Combine cloud layers with different thresholds → fluffy shapes
    float cloud = smoothstep(0.35, 0.55, c1) * 0.7
                + smoothstep(0.40, 0.60, c2) * 0.5
                + smoothstep(0.30, 0.50, c3) * 0.4;
    cloud = clamp(cloud, 0.0, 1.0);

    // Fade clouds at screen edges
    float edgeFade = 1.0;
    edgeFade *= smoothstep(0.0, 0.15, uv.x);
    edgeFade *= smoothstep(0.0, 0.15, 1.0 - uv.x);
    edgeFade *= smoothstep(0.0, 0.10, uv.y);
    cloud *= edgeFade;

    // White cloud with slight blue tint
    vec3 cloudColor = mix(vec3(0.95, 0.97, 1.0), vec3(0.85, 0.88, 0.95), cloud);
    float alpha = cloud * 0.55;  // max opacity

    FragColor = vec4(cloudColor, alpha);
}
