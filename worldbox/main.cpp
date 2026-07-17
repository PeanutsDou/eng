#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <GL/glew.h>

#include <fstream>
#include <sstream>
#include <string>

#include "vertex_buffer.h"
#include "mesh.h"

// ===== Read a file into a string =====
static std::string read_file(const char* path) {
    std::ifstream file(path);
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

// ===== Compile a single shader =====
static GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        SDL_Log("Shader compile error:\n%s", log);
    }
    return shader;
}

// ===== Link vertex + fragment shaders into a program =====
static GLuint create_program(const char* vert_src, const char* frag_src) {
    GLuint vert = compile_shader(GL_VERTEX_SHADER, vert_src);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, frag_src);

    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        SDL_Log("Program link error:\n%s", log);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

// ===== Entry =====

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow(
        "WorldBox - Step 2",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1024, 768,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    SDL_GLContext gl = SDL_GL_CreateContext(window);

    glewExperimental = GL_TRUE;
    glewInit();

    // ===== Load shaders =====
    std::string vert_src = read_file("shader.vert");
    std::string frag_src = read_file("shader.frag");
    GLuint program = create_program(vert_src.c_str(), frag_src.c_str());

    // ===== Build geometry =====
    // Use VertexBufferWriter to describe the shape (no GPU code here)
    VertexBufferWriter geom;

    // Triangle: 3 vertices, 3 indices
    int a = geom.add_vertex( 0.0f,  0.5f);   // top
    int b = geom.add_vertex(-0.5f, -0.5f);   // bottom-left
    int c = geom.add_vertex( 0.5f, -0.5f);   // bottom-right
    geom.add_triangle(a, b, c);

    // Upload to GPU
    Mesh mesh;
    mesh.upload(geom.vertices, geom.indices, VertexBufferWriter::stride);

    // ===== Main loop =====
    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        mesh.draw();

        SDL_GL_SwapWindow(window);
    }

    // ===== Cleanup =====
    mesh.cleanup();
    glDeleteProgram(program);

    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
