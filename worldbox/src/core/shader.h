#pragma once
#include <GL/glew.h>

struct Shader {
    GLuint program = 0;

    // Load from files. Returns false on failure (old shader lost).
    bool load(const char* vert_path, const char* frag_path);

    // Load from strings.
    bool load_source(const char* vert_src, const char* frag_src);

    // Hot-reload from files. On failure, old shader is kept untouched.
    bool reload(const char* vert_path, const char* frag_path);

    void use() const;
    void cleanup();

    void set_int(const char* name, int v) const;
    void set_float(const char* name, float v) const;
    void set_vec2(const char* name, float x, float y) const;
    void set_vec3(const char* name, float x, float y, float z) const;
    void set_vec4(const char* name, float x, float y, float z, float w) const;

private:
    GLuint compile(GLenum type, const char* source);
};
