#version 330 core
// ================================================================
//  Sprite layer — animated pixel characters on terrain.
//  Uses hardcoded pixel art for now.
//  When spriteSheet texture is available (hasSpriteSheet=1),
//  samples from it instead.
// ================================================================

in vec2 TexCoord;
out vec4 FragColor;

uniform vec2  mapSize;
uniform float seed;
uniform float zoom;
uniform vec2  pan;
uniform float borderWidth;
uniform float time;

// ---- Sprite sheet (0 = use hardcoded sprites) ----
uniform int  hasSpriteSheet;
uniform sampler2D spriteSheet;
uniform vec4      sheetInfo;
uniform int       animData[64];
uniform int       animCount;

// ===== Hardcoded character: 6w × 7h, 4-frame walk =====
bool charPixel(int frame, int cx, int cy) {
    // Upper body (same for all frames)
    if (cy == 6 && cx >= 2 && cx <= 3) return true;   // head
    if (cy == 5 && cx >= 1 && cx <= 4) return true;   // shoulders
    if (cy == 4 && cx >= 2 && cx <= 3) return true;   // torso
    if (cy == 3 && cx >= 2 && cx <= 3) return true;
    if (cy == 2 && cx >= 2 && cx <= 3) return true;   // hips

    int f = frame % 4;

    // Legs
    if (cy == 1) {
        if (f == 0 || f == 2) return cx == 1 || cx == 4;
        else                  return cx == 0 || cx == 5;
    }
    // Feet
    if (cy == 0) {
        if (f == 0 || f == 2) return cx >= 1 && cx <= 4;
        else                  return cx >= 0 && cx <= 5;
    }
    return false;
}

void main() {
    vec2 uv = TexCoord;
    vec4 color = vec4(0.0);  // transparent default

    // ===== Entity data =====
    const int N = 3;

    float speed = 0.06;
    float posX[N] = float[](
        fract(time * speed + 0.0),
        fract(time * speed + 0.4),
        fract(time * speed + 0.8)
    );
    float posY[N] = float[](0.55, 0.62, 0.48);

    float phase[N] = float[](0.0, 0.7, 1.4);

    vec3 cols[N] = vec3[](
        vec3(0.85, 0.22, 0.18),
        vec3(0.22, 0.35, 0.82),
        vec3(0.20, 0.72, 0.28)
    );

    float cpx = 0.006;        // pixel size in UV
    float cw  = cpx * 6.0;    // char width
    float ch  = cpx * 7.0;    // char height

    // ===== Render each entity =====
    for (int i = 0; i < N; i++) {
        float cx0 = posX[i] - cw * 0.5;
        float cy0 = posY[i] - ch;

        if (uv.x >= cx0 && uv.x < cx0 + cw &&
            uv.y >= cy0 && uv.y < posY[i]) {

            int cx = int((uv.x - cx0) / cpx);
            int cy = int((posY[i] - uv.y) / cpx);
            int frame = int(time * 5.0 + phase[i]);

            if (hasSpriteSheet > 0) {
                // ---- Texture-based sprite (when PNG is loaded) ----
                // TODO: sample from spriteSheet using animData
            } else {
                // ---- Hardcoded pixel art ----
                if (charPixel(frame, cx, cy)) {
                    color = vec4(cols[i], 1.0);
                }
            }
        }
    }

    FragColor = color;
}
