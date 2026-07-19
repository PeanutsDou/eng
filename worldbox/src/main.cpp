#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <GL/glew.h>

#include "imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_opengl3.h"

#include "engine/engine.h"
#include "editor/editor.h"

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    const int W = 1024, H = 768;
    SDL_Window* window = SDL_CreateWindow(
        "WorldBox", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        W, H, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    SDL_GLContext glctx = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1);
    glewExperimental = GL_TRUE;
    glewInit();

    // ---- ImGui ----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(window, glctx);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ---- Engine ----
    Engine eng;
    eng.init(W, H);

    // ---- Editor ----
    Editor editor;
    editor.loadSceneList();
    if (!editor.scenes.empty()) {
        eng.scene.load(editor.scenes[0]);
        editor.current = 0;
    }

    // ---- Main loop ----
    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                auto k = event.key.keysym.sym;
                if (k == SDLK_SPACE && !ImGui::GetIO().WantCaptureKeyboard)
                    eng.seed += 1.0f;
                if (k == SDLK_F5) eng.scene.reloadShaders();
            }
        }

        // ImGui new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Editor panel
        editor.render(&eng);

        // Engine render
        eng.render();

        // ImGui render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    // ---- Cleanup ----
    eng.cleanup();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(glctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
