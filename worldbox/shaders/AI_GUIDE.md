# WorldBox — AI Guide

## What this is

A pixel-art rendering engine. You edit GLSL fragment shaders. The engine handles everything else.

## Project layout

```
worldbox/
├── src/                          ← C++ engine (do not touch)
│   ├── main.cpp                  ← entry point
│   ├── core/                     ← shader, mesh, vertex layout
│   ├── engine/                   ← render pipeline, scene, framebuffer
│   ├── editor/                   ← ImGui UI panel
│   ├── sprite/                   ← sprite sheet loader
│   └── imgui/                    ← third-party UI backend
├── shaders/                      ← GLSL files you edit
│   ├── shared/screen.vert        ← fixed vertex shader (never change)
│   └── <scene>/                  ← one folder per scene
│       ├── <name>.frag           ← layer shaders
│       └── post.frag             ← post-processing pass
├── data/                         ← text configs
│   ├── scenes.conf               ← list of scene names
│   └── <scene>/pipeline.conf     ← layer declaration for this scene
└── sprites/                      ← sprite assets
    └── <scene>/
        ├── input/                ← raw images (.info + .raw)
        └── <name>.conf           ← sprite sheet animation defs
```

## How rendering works

Every scene has a pipeline in `data/<scene>/pipeline.conf`. Each line is one layer, drawn in order into an off-screen buffer. The final post layer reads the buffer and outputs to screen.

### pipeline.conf syntax

| Line | Meaning |
|------|---------|
| `name.frag` | Fullscreen layer, shader from `shaders/<scene>/name.frag` |
| `name.frag image:xxx` | Fullscreen layer + raw image from `sprites/<scene>/input/xxx.raw` |
| `sprite:xxx` | Sprite layer, shader=`sprites.frag`, config=`sprites/<scene>/xxx.conf` |
| `post:name.frag` | **Last line only.** Post-processing, reads all layers → screen |

## Uniforms (set every frame by engine)

| Name | Type | Default | What it is |
|------|------|---------|-------------|
| `mapSize` | vec2 | (256,256) | Tile grid size |
| `seed` | float | 0+ | Random seed, +1 on SPACE |
| `zoom` | float | 1.0 | Camera zoom |
| `pan` | vec2 | (0,0) | Camera pan in UV |
| `borderWidth` | float | 0.03 | Grid line thickness |
| `time` | float | seconds | Elapsed time, use for animation |
| `pixelSize` | float | 4 | Pixelation block size |
| `colorLevels` | float | 8 | Color palette levels/channel |

### Sprite-layer only

| Name | Type | Meaning |
|------|------|---------|
| `hasSpriteSheet` | int | 0=hardcoded sprites, 1=use texture |
| `spriteSheet` | sampler2D | Sprite sheet texture (unit 1) |
| `sheetInfo` | vec4 | (imgW, imgH, frameW, frameH) |
| `animData[64]` | int[] | 4 ints/anim: startCol,startRow,frameCount,fps |
| `animCount` | int | Number of animations |

### Image-layer only

| Name | Type | Meaning |
|------|------|---------|
| `inputImage` | sampler2D | Raw image texture (unit 1) |
| `imageSize` | vec2 | (w, h) of original image |

## How to create a scene

1. Use the ImGui panel: type a name → click **Add**. Creates all directories + default files.
2. Or manually:
   - Create `shaders/<name>/`, `data/<name>/`, `sprites/<name>/`
   - Write `data/<name>/pipeline.conf`
   - Write your shaders in `shaders/<name>/`
   - Add the scene name to `data/scenes.conf`
3. Restart. Press Prev/Next to cycle scenes.

## How to add a layer

Add a line to `data/<scene>/pipeline.conf`, create the corresponding `.frag` in `shaders/<scene>/`. Press F5.

## Shader template

```glsl
#version 330 core
in vec2 TexCoord;          // screen UV: (0,0)=bottom-left, (1,1)=top-right
out vec4 FragColor;        // RGBA output
uniform float time;        // seconds, use for animation

void main() {
    vec2 uv = TexCoord;
    vec3 color = vec3(uv.x, uv.y, 0.5);  // your logic here
    FragColor = vec4(color, 1.0);
}
```

- First layer: output alpha=1 (opaque).
- Later layers: output alpha<1 to blend with what's below. 0=fully transparent.
- `ivec2 tile = ivec2(uv * mapSize)` to get current tile index.

## Animation (no PNG needed)

Define characters as hardcoded pixel art in your shader:

```glsl
int frame = int(time * fps + phaseOffset) % frameCount;
if (charPixel(frame, px, py)) color = vec4(charColor, 1.0);
```

## Controls

| Input | Action |
|-------|--------|
| UI: Reload Shaders / F5 | Hot-reload shaders |
| UI: < Prev / Next > / F1 / F2 | Switch scene |
| UI: Randomize / SPACE | New random seed |
| UI: Add | Create new scene |
| UI: Delete Current | Delete current scene (removes dirs + conf) |
