#include "engine.h"
#include <SDL.h>
#include <fstream>
#include <sstream>
#include <cstring>

// ===== Pipeline config parser =====
PipelineConfig load_pipeline(const char* path) {
    PipelineConfig cfg;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (line.rfind("post:", 0) == 0) {
            cfg.post_frag = line.substr(5);
        } else if (line.rfind("sprite:", 0) == 0) {
            LayerEntry e;
            e.frag = "sprites.frag";
            e.spriteName = line.substr(7);
            cfg.layers.push_back(e);
        } else {
            LayerEntry e;
            size_t sp = line.find(' ');
            if (sp != std::string::npos) {
                e.frag = line.substr(0, sp);
                std::string rest = line.substr(sp + 1);
                if (rest.rfind("image:", 0) == 0) e.imageName = rest.substr(6);
                else if (rest.rfind("sprite:", 0) == 0) e.spriteName = rest.substr(7);
            } else {
                e.frag = line;
            }
            cfg.layers.push_back(e);
        }
    }
    return cfg;
}

// ===== RawImage =====
void RawImage::load(const char* rawPath, const char* infoPath) {
    std::ifstream fi(infoPath); fi >> w >> h;
    std::vector<uint8_t> src(w * h * 4);
    std::ifstream fr(rawPath, std::ios::binary);
    fr.read((char*)src.data(), src.size());
    std::vector<uint8_t> flipped(w * h * 4);
    for (int y = 0; y < h; y++)
        memcpy(&flipped[(h-1-y)*w*4], &src[y*w*4], w*4);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, flipped.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}
void RawImage::cleanup() { if (tex) glDeleteTextures(1, &tex); tex = 0; }

// ===== Framebuffer =====
void Framebuffer::create(int width, int height) {
    w = width; h = height;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void Framebuffer::bind() { glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0,0,w,h); }
void Framebuffer::unbind(int sw, int sh) { glBindFramebuffer(GL_FRAMEBUFFER, 0); glViewport(0,0,sw,sh); }
void Framebuffer::cleanup() { if (tex) glDeleteTextures(1,&tex); if (fbo) glDeleteFramebuffers(1,&fbo); tex=fbo=0; }

// ===== Scene =====
static std::string fragPath(const std::string& scene, const std::string& n) {
    return "shaders/" + scene + "/" + n;
}

void Scene::load(const std::string& sceneName) {
    name = sceneName;
    pipe = load_pipeline(("data/" + name + "/pipeline.conf").c_str());
    SDL_Log("Scene [%s]: %d layers", name.c_str(), (int)pipe.layers.size());

    for (size_t i = 0; i < pipe.layers.size(); i++) {
        if (!pipe.layers[i].spriteName.empty()) {
            SpriteSheet ss;
            if (ss.load(("sprites/" + name + "/" + pipe.layers[i].spriteName + ".conf").c_str())) {
                ss.upload();
                pipe.layers[i].spriteIndex = (int)spriteSheets.size();
                spriteSheets.push_back(std::move(ss));
            }
        }
        if (!pipe.layers[i].imageName.empty()) {
            RawImage img;
            std::string base = "sprites/" + name + "/input/" + pipe.layers[i].imageName;
            img.load((base + ".raw").c_str(), (base + ".info").c_str());
            pipe.layers[i].imageIndex = (int)rawImages.size();
            rawImages.push_back(std::move(img));
        }
    }

    std::string vp = "shaders/shared/screen.vert";
    layerShaders.resize(pipe.layers.size());
    for (size_t i = 0; i < pipe.layers.size(); i++)
        layerShaders[i].load(vp.c_str(), fragPath(name, pipe.layers[i].frag).c_str());
    postShader.load(vp.c_str(), fragPath(name, pipe.post_frag).c_str());
}

void Scene::reloadShaders() {
    std::string v = "shaders/shared/screen.vert";
    for (size_t i = 0; i < pipe.layers.size(); i++)
        layerShaders[i].reload(v.c_str(), fragPath(name, pipe.layers[i].frag).c_str());
    postShader.reload(v.c_str(), fragPath(name, pipe.post_frag).c_str());
}

void Scene::unload() {
    for (auto& s : layerShaders) s.cleanup();
    layerShaders.clear();
    postShader.cleanup();
    for (auto& ss : spriteSheets) ss.cleanup();
    spriteSheets.clear();
    for (auto& im : rawImages) im.cleanup();
    rawImages.clear();
    pipe = PipelineConfig{};
}

// ===== Engine =====
void Engine::init(int w, int h) {
    scrW = w; scrH = h;

    float v[] = { -1,1,0,1, 1,1,1,1, 1,-1,1,0, -1,-1,0,0 };
    uint32_t idx[] = { 0,1,2, 0,2,3 };
    VertexLayout ql; ql.add(0,2,GL_FLOAT); ql.add(1,2,GL_FLOAT);
    quad.upload(v, 4, idx, 6, ql);

    fbo.create(scrW, scrH);
}

void Engine::render() {
    time = (float)SDL_GetTicks() / 1000.0f;

    // Layers → FBO
    fbo.bind();
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    for (size_t i = 0; i < scene.layerShaders.size(); i++) {
        scene.layerShaders[i].use();
        scene.layerShaders[i].set_vec2("mapSize", 256, 256);
        scene.layerShaders[i].set_float("seed", seed);
        scene.layerShaders[i].set_float("zoom", 1);
        scene.layerShaders[i].set_vec2("pan", 0, 0);
        scene.layerShaders[i].set_float("borderWidth", 0.03f);
        scene.layerShaders[i].set_float("time", time);
        scene.layerShaders[i].set_float("pixelSize", 4);
        scene.layerShaders[i].set_float("colorLevels", 8);

        int si = scene.pipe.layers[i].spriteIndex;
        scene.layerShaders[i].set_int("hasSpriteSheet", (si >= 0) ? 1 : 0);
        if (si >= 0 && si < (int)scene.spriteSheets.size()) {
            auto& ss = scene.spriteSheets[si];
            scene.layerShaders[i].set_int("spriteSheet", 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, ss.texture());
            glActiveTexture(GL_TEXTURE0);
            int iw,ih,fw,fh; ss.getSheetInfo(iw,ih,fw,fh);
            scene.layerShaders[i].set_vec4("sheetInfo",(float)iw,(float)ih,(float)fw,(float)fh);
            std::vector<int> ad; ss.getAnimData(ad);
            for (size_t j=0; j<ad.size()&&j<64; j++)
                scene.layerShaders[i].set_int(("animData["+std::to_string(j)+"]").c_str(), ad[j]);
            scene.layerShaders[i].set_int("animCount", ss.animCount());
        }
        int ii = scene.pipe.layers[i].imageIndex;
        if (ii >= 0 && ii < (int)scene.rawImages.size()) {
            auto& im = scene.rawImages[ii];
            scene.layerShaders[i].set_int("inputImage", 1);
            scene.layerShaders[i].set_vec2("imageSize", (float)im.w, (float)im.h);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, im.tex);
            glActiveTexture(GL_TEXTURE0);
        }

        if (i > 0) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); }
        quad.draw();
        if (i > 0) glDisable(GL_BLEND);
    }

    // Post → screen
    fbo.unbind(scrW, scrH);
    glClear(GL_COLOR_BUFFER_BIT);
    scene.postShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbo.texture());
    scene.postShader.set_int("screenTex", 0);
    quad.draw();
}

void Engine::cleanup() {
    scene.unload();
    quad.cleanup();
    fbo.cleanup();
}
