#pragma once
#include <GL/glew.h>
#include <string>
#include <vector>

struct AnimDef {
    std::string name;
    int startCol, startRow;
    int frameCount;
    int fps;          // 0 = static
};

// Sprite sheet: parses .conf, loads PNG, uploads to GPU.
// Does NOT draw pixel art — that belongs in the shader or an external tool.
class SpriteSheet {
public:
    // Load animation defs from .conf.
    // If PNG is available, also loads the sheet image.
    // Returns true if a valid sheet texture was created.
    bool load(const char* confPath);

    void upload();
    void cleanup();

    GLuint texture() const { return tex; }
    void getSheetInfo(int& iw_, int& ih_, int& fw_, int& fh_) const {
        iw_ = iw; ih_ = ih; fw_ = fw; fh_ = fh;
    }
    int animCount() const { return (int)anims.size(); }

    // Pack for shader: [startCol, startRow, frameCount, fps] × N
    void getAnimData(std::vector<int>& out) const;

private:
    GLuint tex = 0;
    int iw = 0, ih = 0, fw = 0, fh = 0;
    std::vector<AnimDef> anims;
    std::vector<uint8_t> pixels;

    bool parseConf(const char* path);
};
