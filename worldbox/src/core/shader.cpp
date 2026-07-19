#include "core/shader.h"
#include <SDL.h>
#include <fstream>
#include <sstream>
#include <string>

// ===== Read entire file into a string =====
static std::string read_file(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        SDL_Log("Shader: cannot open file: %s", path);
        return "";
    }
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

// ===== Compile a single shader stage =====
GLuint Shader::compile(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        const char* stage = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
        SDL_Log("Shader compile error (%s):\n%s", stage, log);
    }
    return shader;
}

// ===== Load from file paths =====
bool Shader::load(const char* vert_path, const char* frag_path) {
    std::string vert_src = read_file(vert_path);
    std::string frag_src = read_file(frag_path);
    if (vert_src.empty() || frag_src.empty()) return false;
    return load_source(vert_src.c_str(), frag_src.c_str());
}

// ===== Load from source strings =====
bool Shader::load_source(const char* vert_src, const char* frag_src) {
    // Clean up any old program
    cleanup();

    GLuint vert = compile(GL_VERTEX_SHADER, vert_src);
    GLuint frag = compile(GL_FRAGMENT_SHADER, frag_src);

    program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        SDL_Log("Shader link error:\n%s", log);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return success;
}

// ===== Hot-reload: safe — old shader kept on failure =====
bool Shader::reload(const char* vert_path, const char* frag_path) {
    std::string vert_src = read_file(vert_path);
    std::string frag_src = read_file(frag_path);
    if (vert_src.empty() || frag_src.empty()) return false;

    // Compile new program before touching the old one
    GLuint new_vert = compile(GL_VERTEX_SHADER, vert_src.c_str());
    GLuint new_frag = compile(GL_FRAGMENT_SHADER, frag_src.c_str());

    GLuint new_prog = glCreateProgram();
    glAttachShader(new_prog, new_vert);
    glAttachShader(new_prog, new_frag);
    glLinkProgram(new_prog);

    GLint success;
    glGetProgramiv(new_prog, GL_LINK_STATUS, &success);

    glDeleteShader(new_vert);
    glDeleteShader(new_frag);

    if (!success) {
        char log[512];
        glGetProgramInfoLog(new_prog, 512, nullptr, log);
        SDL_Log("Shader reload error (old shader kept):\n%s", log);
        glDeleteProgram(new_prog);
        return false;
    }

    // Swap
    if (program) glDeleteProgram(program);
    program = new_prog;
    SDL_Log("Shader reloaded: %s", frag_path);
    return true;
}

void Shader::use() const {
    glUseProgram(program);
}

void Shader::cleanup() {
    if (program) {
        glDeleteProgram(program);
        program = 0;
    }
}

// ===== Uniform setters =====
void Shader::set_int(const char* name, int v) const {
    glUniform1i(glGetUniformLocation(program, name), v);
}

void Shader::set_float(const char* name, float v) const {
    glUniform1f(glGetUniformLocation(program, name), v);
}

void Shader::set_vec2(const char* name, float x, float y) const {
    glUniform2f(glGetUniformLocation(program, name), x, y);
}

void Shader::set_vec3(const char* name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(program, name), x, y, z);
}

void Shader::set_vec4(const char* name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(program, name), x, y, z, w);
}
