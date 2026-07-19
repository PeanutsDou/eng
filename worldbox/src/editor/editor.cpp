#include "editor/editor.h"
#include "imgui.h"
#include <fstream>
#include <cstdio>
#include <cstring>
#include <windows.h>

void Editor::loadSceneList() {
    scenes.clear();
    std::ifstream f("data/scenes.conf");
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()||line[0]=='#') continue;
        if (line.back()=='\r') line.pop_back();
        if (!line.empty()) scenes.push_back(line);
    }
}

void Editor::saveSceneList() {
    std::ofstream f("data/scenes.conf");
    for (auto& s : scenes) f << s << "\n";
}

void Editor::addScene(const std::string& name, Engine* eng) {
    if (name.empty()) return;
    for (auto& s : scenes) if (s == name) return;
    // Create dirs
    std::string sd = "shaders/" + name, dd = "data/" + name;
    std::string spd = "sprites/" + name;
    CreateDirectoryA(sd.c_str(), nullptr);
    CreateDirectoryA(dd.c_str(), nullptr);
    CreateDirectoryA(spd.c_str(), nullptr);
    CreateDirectoryA((spd + "/input").c_str(), nullptr);
    // Default files
    std::ofstream pf(dd + "/pipeline.conf");
    pf << "# " << name << " pipeline\nmain.frag\npost:post.frag\n";
    std::ofstream sf(sd + "/main.frag");
    sf << "#version 330 core\nin vec2 TexCoord; out vec4 FragColor;\n"
       << "uniform float time;\nvoid main() {\n"
       << "    vec3 c = vec3(TexCoord.x, TexCoord.y, 0.5+0.3*sin(time));\n"
       << "    FragColor = vec4(c, 1.0);\n}\n";
    std::ofstream sp(sd + "/post.frag");
    sp << "#version 330 core\nin vec2 TexCoord; out vec4 FragColor;\n"
       << "uniform sampler2D screenTex;\nvoid main() {\n"
       << "    FragColor = texture(screenTex, TexCoord);\n}\n";
    scenes.push_back(name);
    saveSceneList();
    if (eng) { eng->scene.unload(); eng->scene.load(name); }
    current = (int)scenes.size() - 1;
}

void Editor::deleteScene(int idx, Engine* eng) {
    if (scenes.size() <= 1) return;
    std::string name = scenes[idx];
    auto rmDir = [](const char* path) {
        std::string cmd = "rmdir /s /q \"" + std::string(path) + "\" 2>nul";
        system(cmd.c_str());
    };
    rmDir(("shaders/" + name).c_str());
    rmDir(("data/"    + name).c_str());
    rmDir(("sprites/" + name).c_str());
    scenes.erase(scenes.begin() + idx);
    saveSceneList();
    if (current >= (int)scenes.size()) current = (int)scenes.size() - 1;
    if (eng) { eng->scene.unload(); eng->scene.load(scenes[current]); }
}

void Editor::render(Engine* eng) {
    if (!show) return;

    ImGui::SetNextWindowPos(ImVec2(eng->scrW - 200.f, 5.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(195, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("WorldBox", &show, ImGuiWindowFlags_AlwaysAutoResize);

    if (!scenes.empty()) {
        ImGui::Text("Scene: %s", scenes[current].c_str());
        ImGui::Separator();

        if (ImGui::Button("Reload Shaders (F5)")) {
            eng->scene.reloadShaders();
        }
        ImGui::Spacing();
        if (ImGui::Button("< Prev")) {
            if (current > 0) current--;
            else current = (int)scenes.size() - 1;
            eng->scene.unload();
            eng->scene.load(scenes[current]);
        }
        ImGui::SameLine();
        if (ImGui::Button("Next >")) {
            if (current < (int)scenes.size() - 1) current++;
            else current = 0;
            eng->scene.unload();
            eng->scene.load(scenes[current]);
        }

        ImGui::Separator();
        ImGui::Text("Manage Scenes");
        static char newName[64] = "";
        ImGui::InputText("##name", newName, 64);
        ImGui::SameLine();
        if (ImGui::Button("Add") && newName[0]) {
            addScene(newName, eng);
            memset(newName, 0, 64);
        }
        if (scenes.size() > 1 && ImGui::Button("Delete Current")) {
            int old = current;
            deleteScene(old, eng);
        }

        ImGui::Separator();
        ImGui::Text("Seed: %.0f", eng->seed);
        if (ImGui::Button("Randomize")) eng->seed += 1.0f;

        ImGui::Spacing();
        if (ImGui::Button("Rebuild && Restart")) {
            rebuildAndRestart();
        }

        ImGui::Spacing();
        ImGui::Text("Layers: %d", (int)eng->scene.pipe.layers.size());
        for (size_t i = 0; i < eng->scene.pipe.layers.size(); i++)
            ImGui::TextDisabled("  %s", eng->scene.pipe.layers[i].frag.c_str());
    }

    ImGui::End();
}

void Editor::rebuildAndRestart() {
    // Get the build directory (two levels up from the exe)
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string buildDir(exePath);
    buildDir = buildDir.substr(0, buildDir.rfind('\\'));  // remove exe name
    buildDir = buildDir.substr(0, buildDir.rfind('\\'));  // remove Debug/
    std::string exeName(exePath);
    exeName = exeName.substr(exeName.rfind('\\') + 1);

    // Write a batch file to do the build + restart
    std::string batPath = buildDir + "\\_rebuild.bat";
    std::ofstream bat(batPath);
    bat << "@echo off\n";
    bat << "set \"PATH=C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin;%PATH%\"\n";
    bat << "cd /d \"" << buildDir << "\"\n";
    bat << "echo Building...\n";
    bat << "cmake --build . --config Debug\n";
    bat << "if %errorlevel% equ 0 (\n";
    bat << "    echo Starting...\n";
    bat << "    cd Debug\n";
    bat << "    start \"\" \"" << exeName << "\"\n";
    bat << ") else (\n";
    bat << "    echo Build failed. Press any key to close.\n";
    bat << "    pause\n";
    bat << ")\n";
    bat.close();

    // Launch the batch file in a new console, then exit this process
    std::string cmd = "start \"WorldBox Build\" cmd /c \"" + batPath + "\"";
    system(cmd.c_str());
    exit(0);
}
