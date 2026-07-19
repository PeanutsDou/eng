#pragma once
#include <string>
#include <vector>
#include <functional>
#include "engine/engine.h"

// ImGui editor panel — scene management, reload, seed control.
// Keeps NO rendering logic; calls Engine for that.
struct Editor {
    std::vector<std::string> scenes;
    int  current = 0;
    bool show = true;

    // Callbacks set by main()
    std::function<void()> onReload;
    std::function<void(int)> onSwitch;
    std::function<void()> onRandomize;

    void loadSceneList();
    void saveSceneList();
    void addScene(const std::string& name, Engine* eng);
    void deleteScene(int idx, Engine* eng);
    void rebuildAndRestart();
    void render(Engine* eng);
};
