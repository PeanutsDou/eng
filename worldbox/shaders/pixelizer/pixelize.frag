#version 330 core
// Pixelizer — read input image, downsample, reduce palette.
in vec2 TexCoord;
out vec4 FragColor;

uniform vec2  mapSize;
uniform float seed;
uniform float zoom;
uniform vec2  pan;
uniform float borderWidth;
uniform float time;
uniform float pixelSize;     // merge N×N pixels
uniform float colorLevels;   // color levels per channel

uniform sampler2D inputImage;  // raw image texture
uniform vec2  imageSize;       // original image dimensions

void main() {
    vec2 uv = TexCoord;

    // ---- Pixelate ----
    float ps = max(1.0, pixelSize);
    float bx = 1024.0 / ps;
    float by = 768.0 / ps;
    vec2 puv = floor(uv * vec2(bx, by)) / vec2(bx, by) + 0.5 / vec2(bx, by);

    // ---- Sample image (keep aspect ratio, centered) ----
    float aspect = imageSize.x / imageSize.y;
    float screenAspect = 1024.0 / 768.0;
    vec2 imgUV = puv;
    if (aspect > screenAspect) {
        // Image wider than screen → fit width, letterbox top/bottom
        float h = screenAspect / aspect;
        imgUV.y = (puv.y - 0.5) / h + 0.5;
    } else {
        // Image taller than screen → fit height, letterbox sides
        float w = aspect / screenAspect;
        imgUV.x = (puv.x - 0.5) / w + 0.5;
    }

    vec3 color = vec3(0.05, 0.05, 0.08);  // letterbox color

    if (imgUV.x >= 0.0 && imgUV.x <= 1.0 && imgUV.y >= 0.0 && imgUV.y <= 1.0) {
        color = texture(inputImage, imgUV).rgb;

        // Color palette reduction
        if (colorLevels > 1.0) {
            color = floor(color * colorLevels + 0.5) / colorLevels;
        }
    }

    FragColor = vec4(color, 1.0);
}
