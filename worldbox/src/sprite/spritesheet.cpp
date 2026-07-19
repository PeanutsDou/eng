#include "sprite/spritesheet.h"
#include <SDL.h>
#include <fstream>
#include <sstream>

// ===== Parse .conf =====
bool SpriteSheet::parseConf(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        SDL_Log("SpriteSheet: cannot open %s", path);
        return false;
    }
    anims.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line.back() == '\r') line.pop_back();

        std::istringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if (cmd == "SHEET") {
            std::string fname;
            ss >> fname >> iw >> ih >> fw >> fh;
        } else if (cmd == "ANIM") {
            AnimDef a;
            ss >> a.name >> a.startCol >> a.startRow
               >> a.frameCount >> a.fps;
            anims.push_back(a);
        }
    }
    SDL_Log("SpriteSheet: %d anims from %s", (int)anims.size(), path);
    return !anims.empty();
}

// ===== Load: conf + PNG (if available) =====
bool SpriteSheet::load(const char* confPath) {
    if (!parseConf(confPath)) return false;

    // TODO: when stb_image is available, load PNG here.
    // For now, return false → shader uses hardcoded sprites.
    SDL_Log("SpriteSheet: no PNG loaded (stb_image not available)");
    return false;
}

// ===== Upload to GPU =====
void SpriteSheet::upload() {
    if (pixels.empty()) return;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, iw, ih, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    SDL_Log("SpriteSheet: uploaded %dx%d texture", iw, ih);
}

void SpriteSheet::getAnimData(std::vector<int>& out) const {
    out.clear();
    for (const auto& a : anims) {
        out.push_back(a.startCol);
        out.push_back(a.startRow);
        out.push_back(a.frameCount);
        out.push_back(a.fps);
    }
}

void SpriteSheet::cleanup() {
    if (tex) { glDeleteTextures(1, &tex); tex = 0; }
    pixels.clear();
}
