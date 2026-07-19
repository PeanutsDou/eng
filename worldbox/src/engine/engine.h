#pragma once
#include <GL/glew.h>
#include <string>
#include <vector>
#include "core/shader.h"
#include "core/mesh.h"
#include "sprite/spritesheet.h"

// ===== Layer entry (one line in pipeline.conf) =====
struct LayerEntry {
    std::string frag;
    std::string spriteName;
    int  spriteIndex = -1;
    std::string imageName;
    int  imageIndex = -1;
};

// ===== Pipeline config =====
struct PipelineConfig {
    std::vector<LayerEntry> layers;
    std::string post_frag;
};

PipelineConfig load_pipeline(const char* path);

// ===== Raw RGBA image =====
struct RawImage {
    GLuint tex = 0;
    int w = 0, h = 0;
    void load(const char* rawPath, const char* infoPath);
    void cleanup();
};

// ===== Off-screen framebuffer =====
struct Framebuffer {
    GLuint fbo = 0, tex = 0;
    int w = 0, h = 0;
    void create(int width, int height);
    void bind();
    void unbind(int screenW, int screenH);
    GLuint texture() const { return tex; }
    void cleanup();
};

// ===== Scene =====
struct Scene {
    std::string name;
    PipelineConfig pipe;
    std::vector<Shader> layerShaders;
    Shader postShader;
    std::vector<SpriteSheet> spriteSheets;
    std::vector<RawImage> rawImages;

    void load(const std::string& sceneName);
    void reloadShaders();
    void unload();
};

// ===== Engine =====
struct Engine {
    Scene scene;
    Mesh quad;
    Framebuffer fbo;
    float seed = 0.0f;
    float time = 0.0f;
    int scrW = 1024, scrH = 768;

    void init(int w, int h);
    void render();
    void cleanup();
};
