# 地图渲染器搭建步骤

## 原则

每一步都有一个**看得见的结果**，用于验证"上一步没错"。
后面每一步都依赖前面能跑通。

```
第1步 ──→ 第2步 ──→ 第3步 ──→ 第4步 ──→ 第5步 ──→ 第6步
空窗口    三角形！    全屏纹理    固定色块    噪声地图    交互重生成
验证环境   验证GPU    验证纹理    验证数据流   算法接入    闭环
```

---

## 项目目录结构（最终态）

```
worldbox/
├── CMakeLists.txt
├── main.cpp          ← 入口 + 主循环
├── map.h             ← 地图数据声明
├── map.cpp           ← 地图生成 + 像素转换
└── noise.h           ← 噪声（可选，也可以塞 map.cpp 里）
```

---

---

## 第 1 步：空窗口 — 验证开发环境

**验证目标**：SDL2 + OpenGL 能编译，能跑，能看到一个窗口。背景色在蓝紫之间呼吸变化。

**涉及文件**：`main.cpp`、`CMakeLists.txt`

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(WorldBox)
set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(worldbox main.cpp)
target_include_directories(worldbox PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(worldbox ${SDL2_LIBRARIES} OpenGL::GL)
```

### main.cpp

```cpp
#include <SDL.h>
#include <SDL_opengl.h>
#include <cmath>

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow(
        "WorldBox - Step 1",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1024, 768,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    SDL_GLContext gl = SDL_GL_CreateContext(window);

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        // 每帧换一个背景色 — 确认渲染在跑
        float t = (float)SDL_GetTicks() / 1000.0f;
        glClearColor(0.1f, 0.2f, 0.3f + 0.1f * sinf(t), 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
```

### 验证标准

- 窗口打开
- 背景色在蓝紫之间缓慢呼吸变化
- 按 X 能关闭

### 可能遇到的问题

| 问题 | 可能原因 |
|---|---|
| `find_package(SDL2)` 失败 | 没装 SDL2，或 CMake 找不到。用 vcpkg: `vcpkg install sdl2`，编译时加 `-DCMAKE_TOOLCHAIN_FILE=...` |
| 链接 OpenGL 失败 | Linux 装 `libgl1-mesa-dev`，macOS 用 framework |
| OpenGL 3.3 不支持 | 非常老的显卡/驱动，但大概率不是问题 |

---

## 第 2 步：画一个三角形 — 验证 GPU 管线

**验证目标**：shader 编译、VAO/VBO、glDrawArrays 整条管线通。

**涉及文件**：覆盖 `main.cpp`

### main.cpp 新增内容

在 main 函数中，**三部分代码需要分别放在正确的位置**：
- Shader 编译函数 → 放在文件顶部（`main` 函数上面）
- VAO/VBO 创建 → 放在渲染循环**前面**
- 渲染调用 → 放在渲染循环**里面**（替换清屏那几行）

```cpp
#include <SDL.h>
#include <SDL_opengl.h>
#include <cstdio>

// ==========================================
// 放在文件顶部：Shader 编译辅助函数
// ==========================================

static GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        printf("Shader compile error: %s\n", log);
    }
    return shader;
}

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
        printf("Program link error: %s\n", log);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

// ==========================================
int main() {
    // ----- 窗口初始化（和第 1 步一样）-----
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

    // ----- Shader 源码（写在字符串里，不需要单独文件）-----
    const char* vert_src = R"(#version 330 core
        layout(location = 0) in vec2 aPos;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";

    const char* frag_src = R"(#version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 0.5, 0.0, 1.0);  // 橙色
        }
    )";

    GLuint program = create_program(vert_src, frag_src);

    // ----- 三角形数据 -----
    float vertices[] = {
         // x     y
         0.0f,  0.5f,   // 顶部
        -0.5f, -0.5f,   // 左下
         0.5f, -0.5f,   // 右下
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 属性 0：位置（2 个 float）
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 解绑（好习惯）
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // ----- 主循环 -----
    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        SDL_GL_SwapWindow(window);
    }

    // 清理
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(program);

    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
```

### 验证标准

- 屏幕中央出现一个**橙色三角形**
- 背景是深蓝黑色

### 可能遇到的问题

| 问题 | 可能原因 |
|---|---|
| 黑屏，没有三角形 | shader 编译/链接失败（看终端日志）。最常见的: 版本声明写错、分号漏掉 |
| 白屏，没有三角形 | VAO 未正确绑定，或 `glVertexAttribPointer` 参数写错 |
| 三角形变形 | 顶点数据顺序错误 |

### 本节关键知识点

- `R"(...)"` 是 C++11 的**原始字符串字面量**，里面不需要转义，写 shader 代码很方便
- **VAO** 记录了"顶点数据怎么解读"，绑定后只需要一个 `glDrawArrays` 就能画
- **VBO** 是存在 GPU 显存里的顶点数据
- `glVertexAttribPointer` 的倒数第二个参数 `stride`：连续顶点的起始位置之间间隔多少字节。这里两个 float 紧挨着，所以 stride = `2 * sizeof(float)`
- `glDrawArrays(GL_TRIANGLES, 0, 3)`：从第 0 个顶点开始，画 3 个顶点 = 1 个三角形

---

## 第 3 步：全屏四边形 + 纹理 — 验证纹理上传

**验证目标**：把一个手动构造的小纹理贴到全屏四边形上，验证纹理创建、上传、shader 采样整条路。

**涉及文件**：覆盖 `main.cpp`

### 和第 2 步的差异

| 变化 | 第 2 步 | 第 3 步 |
|---|---|---|
| 顶点 | 3 个顶点（三角形） | 4 个顶点 + 6 个索引（四边形） |
| 顶点属性 | 只有位置 | 位置 + UV |
| Shader | 纯色输出 | 带 `sampler2D` + UV 传递 |
| 额外工作 | 无 | 创建纹理 + 上传数据 |

### 关键新增代码

**四边形顶点数据**（替换三角形数据部分）：

```cpp
float vertices[] = {
    // pos(x,y)      uv(u,v)
    -1.0f,  1.0f,    0.0f, 1.0f,   // 左上
     1.0f,  1.0f,    1.0f, 1.0f,   // 右上
     1.0f, -1.0f,    1.0f, 0.0f,   // 右下
    -1.0f, -1.0f,    0.0f, 0.0f,   // 左下
};

unsigned int indices[] = {
    0, 1, 2,   // 第一个三角形
    0, 2, 3    // 第二个三角形
};
```

**顶点属性布局**（替换 `glVertexAttribPointer` 部分）：

```
顶点结构（4 个 float）：
┌──────┬──────┬──────┬──────┐
│ posX │ posY │ uvU  │ uvV  │
│ 4字节 │ 4字节 │ 4字节 │ 4字节 │
└──────┴──────┴──────┴──────┘
stride = 4 * sizeof(float) = 16 字节

属性 0（位置）：offset = 0，  2 个 float
属性 1（UV）：   offset = 8，  2 个 float  （跳过 2*4 = 8 字节）
```

```cpp
// 属性 0：位置
glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

// 属性 1：UV
glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                      (void*)(2 * sizeof(float)));
glEnableVertexAttribArray(1);
```

**Shader 改动**：

```cpp
// 顶点着色器 — 多了 UV 的 in/out
const char* vert_src = R"(#version 330 core
    layout(location = 0) in vec2 aPos;
    layout(location = 1) in vec2 aTexCoord;
    out vec2 TexCoord;
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        TexCoord = aTexCoord;
    }
)";

// 片段着色器 — 多了 sampler2D + texture()
const char* frag_src = R"(#version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D mapTexture;
    void main() {
        FragColor = texture(mapTexture, TexCoord);
    }
)";
```

**手动构造 4×4 测试纹理**：

```cpp
// 4×4 的渐变色纹理：左上红 → 右上绿 → 左下蓝 → 右下黄
uint8_t tex_data[4 * 4 * 4];  // 4×4 像素 × 4 通道 RGBA

for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
        int i = (y * 4 + x) * 4;
        tex_data[i + 0] = (uint8_t)(x * 85);    // R: 水平渐变 0→255
        tex_data[i + 1] = (uint8_t)(y * 85);    // G: 垂直渐变 0→255
        tex_data[i + 2] = 128;                  // B: 固定
        tex_data[i + 3] = 255;                  // A: 不透明
    }
}
```

**纹理创建 + 上传**（放在主循环前面）：

```cpp
GLuint texture;
glGenTextures(1, &texture);
glBindTexture(GL_TEXTURE_2D, texture);

// 关键选择：GL_NEAREST — 像素风，不清洗边界
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

// 边缘不重复
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

// 上传数据到 GPU（分配显存 + 拷贝数据）
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
             4, 4,                  // 宽 4、高 4
             0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
```

**渲染循环中绑定纹理**：

```cpp
glUseProgram(program);

// 激活纹理单元 0，绑定我们的纹理，告诉 shader "mapTexture = 0 号单元"
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, texture);
glUniform1i(glGetUniformLocation(program, "mapTexture"), 0);

glBindVertexArray(vao);
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
// 6 = 两个三角形共 6 个索引
```

### 验证标准

- 全屏被拉伸的 4×4 色块填满
- 左上红色，右上绿色，左下蓝色，右下黄色
- **边界锐利**，颜色块直接跳变（GL_NEAREST 效果）
- 如果边界模糊 = 用了 GL_LINEAR，改回 GL_NEAREST

### 可能遇到的问题

| 问题 | 可能原因 |
|---|---|
| 白屏 | 纹理没绑定，或 uniform 位置错误 |
| 黑屏 | 纹理创建失败，或 `glTexImage2D` 参数不匹配 |
| 纹理颠倒（上下反转） | OpenGL 的 UV 原点在左下，大多数图片格式原点在左上。这里手动构造的数据没这个问题，但如果以后加载 `.png` 会遇到——解决方案：加载时翻转，或 shader 里 `TexCoord.y = 1.0 - TexCoord.y` |

### 本节关键知识点

- `glTexImage2D` 和 `glTexSubImage2D` 的区别：
  - `glTexImage2D`：**创建纹理 + 分配显存 + （可选）上传数据**。每次调用都重新分配。
  - `glTexSubImage2D`：**只更新内容**，不重新分配。前提是纹理已经用 `glTexImage2D(nullptr)` 创建过了。
  - 后续第 6 步用 `glTexSubImage2D` 更新地图，因为纹理大小不变。
- `GL_NEAREST` vs `GL_LINEAR`：
  - `GL_NEAREST`：取最近的像素，瓦片之间边界锐利 → WorldBox 风格
  - `GL_LINEAR`：四个邻近像素双线性插值，瓦片之间模糊 → 不是我们想要的
- **纹理单元**（texture unit）：GPU 可以同时绑定多张纹理（槽位 0, 1, 2...），`glActiveTexture(GL_TEXTURE0)` 选择当前操作的槽位。shader 里用 `uniform sampler2D` + `glUniform1i` 告诉"mapTexture 在 0 号槽位"。

---

## 第 4 步：固定地图色块 — 验证完整数据流

**验证目标**：用硬编码的地图数据，验证 `Tile → 像素数组 → 纹理 → 屏幕` 整条链路。这步是分水岭——地图逻辑和渲染逻辑第一次打通。

**涉及文件**：新建 `map.h` + `map.cpp`，`main.cpp` 引入地图

### map.h

```cpp
#pragma once
#include <vector>
#include <cstdint>

// 地形类型
enum class Terrain : uint8_t {
    DEEP_WATER,
    SHALLOW_WATER,
    SAND,
    GRASS,
    FOREST,
    HILL,
    MOUNTAIN,
    SNOW,
};

// 单个瓦片
struct Tile {
    Terrain terrain = Terrain::GRASS;
};

// 地图
class WorldMap {
public:
    WorldMap(int w, int h);

    Tile& at(int x, int y);
    const Tile& at(int x, int y) const;

    int get_width()  const { return width; }
    int get_height() const { return height; }

    // 生成测试图案（硬编码，不碰噪声）
    void generate_test_pattern();

    // 把地图转成 RGBA 像素数组（CPU 侧，准备传给 GPU）
    void to_pixels(std::vector<uint8_t>& out) const;

private:
    int width, height;

    // 一维数组模拟二维：tiles[y * width + x]
    // 这样内存连续，cache 友好，.data() 可以直接用
    std::vector<Tile> tiles;
};
```

### map.cpp

```cpp
#include "map.h"

WorldMap::WorldMap(int w, int h)
    : width(w), height(h), tiles(w * h) {
}

Tile& WorldMap::at(int x, int y) {
    return tiles[y * width + x];
}

const Tile& WorldMap::at(int x, int y) const {
    return tiles[y * width + x];
}

void WorldMap::generate_test_pattern() {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Tile& tile = at(x, y);

            // 白色边框（4 像素宽）
            if (x < 4 || x >= width - 4 || y < 4 || y >= height - 4) {
                tile.terrain = Terrain::SNOW;
            }
            // 对角线分两半
            else if (x > y) {
                tile.terrain = Terrain::GRASS;       // 右上 = 绿色
            }
            else {
                tile.terrain = Terrain::DEEP_WATER;  // 左下 = 蓝色
            }
        }
    }
}

void WorldMap::to_pixels(std::vector<uint8_t>& out) const {
    out.resize(width * height * 4);  // RGBA

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const Tile& tile = at(x, y);
            int i = (y * width + x) * 4;  // 像素数组索引

            switch (tile.terrain) {
                case Terrain::DEEP_WATER:
                    out[i + 0] = 30;  out[i + 1] = 60;  out[i + 2] = 150; break;  // 深蓝
                case Terrain::SHALLOW_WATER:
                    out[i + 0] = 50;  out[i + 1] = 100; out[i + 2] = 200; break;  // 浅蓝
                case Terrain::SAND:
                    out[i + 0] = 220; out[i + 1] = 200; out[i + 2] = 120; break;  // 沙黄
                case Terrain::GRASS:
                    out[i + 0] = 30;  out[i + 1] = 180; out[i + 2] = 20;  break;  // 草绿
                case Terrain::FOREST:
                    out[i + 0] = 20;  out[i + 1] = 120; out[i + 2] = 20;  break;  // 深绿
                case Terrain::HILL:
                    out[i + 0] = 120; out[i + 1] = 100; out[i + 2] = 80;  break;  // 灰褐
                case Terrain::MOUNTAIN:
                    out[i + 0] = 150; out[i + 1] = 140; out[i + 2] = 130; break;  // 浅灰
                case Terrain::SNOW:
                    out[i + 0] = 250; out[i + 1] = 250; out[i + 2] = 250; break;  // 白
                default:
                    // 品红色 = 错误标记（说明有 terrain 值没被处理）
                    out[i + 0] = 255; out[i + 1] = 0;   out[i + 2] = 255; break;
            }
            out[i + 3] = 255;  // Alpha: 完全不透明
        }
    }
}
```

### main.cpp 改哪些

和第 3 步相比，只需要改**两处**：

**改动 1**：文件顶部加 `#include "map.h"`

**改动 2**：把"手动构造 4×4 纹理"替换成：

```cpp
// ----- 创建地图（放在主循环前面）-----
const int MAP_W = 64;
const int MAP_H = 64;

WorldMap map(MAP_W, MAP_H);
map.generate_test_pattern();

std::vector<uint8_t> pixels;
map.to_pixels(pixels);

// ----- 创建纹理 + 上传（也放在主循环前面）-----
GLuint texture;
glGenTextures(1, &texture);
glBindTexture(GL_TEXTURE_2D, texture);

glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
             MAP_W, MAP_H,
             0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
```

渲染循环不变（保持和第 3 步一样）。

### 验证标准

- 屏幕上出现 64×64 个色块
- 左下对角部分：蓝色（水域）
- 右上对角部分：绿色（草地）
- 四周：白色边框
- 边界锐利

### 如果出问题了检查什么

| 症状 | 排查方向 |
|---|---|
| 上下颠倒了 | `to_pixels` 里 y 循环的索引方向。OpenGL 纹理原点在左下，检查 `(y * width + x)` 公式 |
| 对角线方向反了 | `x > y` 的判断逻辑 |
| 颜色不对（比如红蓝互换） | `to_pixels` 里 RGBA 的顺序。OpenGL 的 `GL_RGBA` 期望 R, G, B, A 顺序 |
| 品红色块出现 | 某个 terrain 值在 switch 里没有对应分支，走到了 default |
| 地图没有铺满全屏 | 四边形顶点是否正确（-1 到 1），shader 是否正确 |

### 本节关键知识点

- **为什么用 `vector<Tile>` 而不是 `vector<vector<Tile>>`**：
  - `vector<vector<Tile>>`：每行独立分配，内存不连续。传给 GPU 需要先拼成一块。
  - `vector<Tile>(w*h)`：一整块连续内存。`tiles.data()` 可以直接拿到所有 Tile 的指针，`pixels.data()` 可以直接拿到所有像素的指针。
  - 这个设计在后续做 AI-native 引擎时很重要——AI 需要查询"当前地图状态"时，一整块连续内存可以直接序列化。

- **数据流在此步打通**：
  ```
  Tile 结构体（语义数据）
    → to_pixels()     ← 这里是"语义 → 像素"的翻译层
    → vector<uint8_t>  ← RGBA 原始数据
    → glTexImage2D()   ← 上传 GPU
    → shader texture() ← GPU 采样
    → 屏幕像素
  ```
  后续任何地形变化都只需要更新 Tile 结构体，然后重新走一遍 `to_pixels()` + `glTexSubImage2D()`。

---

## 第 5 步：噪声地图 — 接入生成算法

**验证目标**：把硬编码测试图案换成 Perlin 噪声生成的自然地形。

**涉及文件**：只改 `map.cpp`（加 `generate()` 方法），`map.h`（加声明）

**main.cpp 不变**（除了把 `generate_test_pattern()` 改成 `generate(42)`）。

### 噪声实现

在 `map.cpp` 里加以下内容（或单独拆出 `noise.h`/`noise.cpp`）：

```cpp
#include <cmath>
#include <random>
#include <algorithm>

// ===== Perlin 噪声辅助函数 =====

static float fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

static float grad(int hash, float x, float y) {
    // 用哈希值的低 2 位决定梯度方向
    int h = hash & 3;
    float u = (h < 2) ? x : y;
    float v = (h < 2) ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

static float noise(float x, float y, const uint8_t* perm) {
    // 整数部分
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;
    // 小数部分
    x -= floor(x);
    y -= floor(y);
    // 缓和曲线
    float u = fade(x);
    float v = fade(y);

    // 四个角的哈希值
    int a  = perm[X] + Y;
    int aa = perm[a];
    int ab = perm[a + 1];
    int b  = perm[X + 1] + Y;
    int ba = perm[b];
    int bb = perm[b + 1];

    // 双线性插值
    return lerp(
        lerp(grad(perm[aa], x,   y),   grad(perm[ba], x-1, y),   u),
        lerp(grad(perm[ab], x,   y-1), grad(perm[bb], x-1, y-1), u),
        v
    );
}

// fbm (fractal Brownian motion) — 多层噪声叠加，模拟自然地形
static float fbm(float x, float y, int octaves, const uint8_t* perm) {
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;

    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(x * frequency, y * frequency, perm);
        frequency *= 2.0f;    // 每次频率翻倍 → 细节越来越密
        amplitude *= 0.5f;    // 每次振幅减半 → 细节影响越来越小
    }

    return value;
}
```

### generate() 方法

```cpp
void WorldMap::generate(int seed) {
    // 构建排列表
    uint8_t perm[512];
    for (int i = 0; i < 256; i++) perm[i] = (uint8_t)i;
    std::mt19937 rng(seed);
    std::shuffle(perm, perm + 256, rng);
    for (int i = 0; i < 256; i++) perm[i + 256] = perm[i];

    // 遍历每个瓦片
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // 坐标映射到噪声空间
            // 除以 64 → 整个地图上大约有 4 个"起伏"
            float nx = (float)x / (width / 4.0f);
            float ny = (float)y / (height / 4.0f);

            float e = fbm(nx, ny, 4, perm);
            e = (e + 1.0f) * 0.5f;  // -1~1 → 0~1

            Tile& tile = at(x, y);

            // 高度 → 地形
            if      (e < 0.30f) tile.terrain = Terrain::DEEP_WATER;
            else if (e < 0.40f) tile.terrain = Terrain::SHALLOW_WATER;
            else if (e < 0.45f) tile.terrain = Terrain::SAND;
            else if (e < 0.60f) tile.terrain = Terrain::GRASS;
            else if (e < 0.75f) tile.terrain = Terrain::FOREST;
            else if (e < 0.85f) tile.terrain = Terrain::HILL;
            else if (e < 0.95f) tile.terrain = Terrain::MOUNTAIN;
            else                tile.terrain = Terrain::SNOW;
        }
    }
}
```

### fbm 叠加原理

```
octave 0:  amplitude=0.5  frequency=1  ── 大尺度起伏（大陆形状）
octave 1:  amplitude=0.25 frequency=2  ── 叠加中等起伏（山脉）
octave 2:  amplitude=0.125 frequency=4 ── 叠加小起伏（丘陵）
octave 3:  amplitude=0.0625 frequency=8 ── 叠加微起伏（细节）

最终值 = 0.5×噪声1 + 0.25×噪声2 + 0.125×噪声4 + 0.0625×噪声8

看起来就像自然地形：
  - 有大尺度的"哪里是大陆"
  - 有中尺度的"山脉"
  - 有小尺度的"局部起伏"
```

### 阈值设计原理

```
0.00 ─────────── 0.30 ──── 0.40 ──── 0.45 ──── 0.60 ──── 0.75 ──── 0.85 ──── 0.95 ── 1.00
  │  深海           │ 浅海  │ 沙滩  │   草地  │  森林  │  丘陵  │   山地  │ 雪  │
  └─ 30%面积 ──────┘       └ 5% ┘          └ 15% ┘        └ 10%  ┘        └ 5% ┘
      
大部分面积是深海（30%）和草地（15%），符合地球表面 70% 海洋的特征。
沙滩很窄（5%），因为海岸线只是过渡带。
```

### 验证标准

- 地图换成 256×256（或更大），看到自然的陆地-海洋分布
- 有海岸线过渡带（浅水 → 沙滩 → 草地）
- 陆地内部有地形变体（森林、丘陵、山、雪顶）
- seed 不变则地图不变（确定性）

---

## 第 6 步：按空格重生成 — 闭环

**验证目标**：整个生成→渲染链路可以反复跑，不崩溃，不泄漏。

**涉及文件**：改 `main.cpp` 的事件处理部分。

### 改动

在主循环的事件处理里加：

```cpp
while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) running = false;

    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
        // 重新生成
        map.generate(rand());
        map.to_pixels(pixels);

        // 更新 GPU 纹理（注意：用 glTexSubImage2D，不是 glTexImage2D）
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0,
                        0, 0,               // 从纹理的 (0,0) 开始
                        MAP_W, MAP_H,       // 更新全部区域
                        GL_RGBA, GL_UNSIGNED_BYTE,
                        pixels.data());
    }
}
```

### 验证标准

- 按空格 → 地图立即换成新地图
- 连续按 10 次 → 不崩溃、不花屏、不越来越慢
- 关闭窗口 → 正常退出

### 可能遇到的问题

| 问题 | 可能原因 |
|---|---|
| 按空格没反应 | 事件循环写错，或 `MAP_W`/`MAP_H` 和纹理创建时不一致 |
| 按几次后崩溃 | `pixels` 没有 resize，`to_pixels` 写越界 |
| 画面撕裂 | 刷新地图时没有用 V-Sync，或更新纹理时纹理还在被 GPU 读取。加上 `SDL_GL_SetSwapInterval(1)` |

---

## 各步骤完整 main.cpp 汇总

以下是第 6 步完成后的完整 main.cpp（包含所有之前步骤的累积代码），方便直接对照：

```cpp
#include <SDL.h>
#include <SDL_opengl.h>
#include <cstdio>
#include <cstdlib>

#include "map.h"

// ===== Shader 编译辅助 =====

static GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        printf("Shader compile error: %s\n", log);
    }
    return shader;
}

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
        printf("Program link error: %s\n", log);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

// ===== 入口 =====

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow(
        "WorldBox",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1024, 768,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    SDL_GLContext gl = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1);  // V-Sync

    // Shader
    const char* vert_src = R"(#version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    const char* frag_src = R"(#version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D mapTexture;
        void main() {
            FragColor = texture(mapTexture, TexCoord);
        }
    )";

    GLuint program = create_program(vert_src, frag_src);
    GLint loc_texture = glGetUniformLocation(program, "mapTexture");

    // 全屏四边形
    float vertices[] = {
        -1.0f,  1.0f,   0.0f, 1.0f,
         1.0f,  1.0f,   1.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
        -1.0f, -1.0f,   0.0f, 0.0f,
    };
    unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 地图
    const int MAP_W = 256;
    const int MAP_H = 256;
    WorldMap map(MAP_W, MAP_H);
    map.generate(42);

    std::vector<uint8_t> pixels;
    map.to_pixels(pixels);

    // 纹理
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 MAP_W, MAP_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // 主循环
    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
                map.generate(rand());
                map.to_pixels(pixels);
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                                MAP_W, MAP_H, GL_RGBA, GL_UNSIGNED_BYTE,
                                pixels.data());
            }
        }

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(loc_texture, 0);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        SDL_GL_SwapWindow(window);
    }

    // 清理
    glDeleteTextures(1, &texture);
    glDeleteProgram(program);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);

    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
```

---

## 学习路线图

```
        第1步          第2步           第3步          第4步          第5步           第6步
      ┌──────┐      ┌──────┐       ┌──────┐      ┌──────┐      ┌──────┐       ┌──────┐
      │空窗口 │ ──→ │三角形 │ ────→ │ 纹理  │ ──→ │地图色块│ ──→ │噪声地图│ ───→ │重生成│
      └──────┘      └──────┘       └──────┘      └──────┘      └──────┘       └──────┘
         │             │              │             │             │              │
    学到的概念：    学到的概念：     学到的概念：    学到的概念：    学到的概念：     学到的概念：
    · SDL2 窗口    · Shader 编译   · 纹理创建     · Tile 结构体  · Perlin 噪声  · glTexSubImage2D
    · OpenGL 上下文 · VAO/VBO      · UV 坐标      · 枚举类型     · fbm 叠加     · 事件处理
    · 主循环        · glDrawArrays · sampler2D    · 一维数组索引 · 高度→地形映射 · 脏数据更新
    · SwapBuffer   · Shader 变量传递 · GL_NEAREST  · data() 指针  · 确定性生成   · 热重载
    · glClear      · NDC 坐标系    · glTexImage2D · switch 查表  · seed 随机数  · 内存安全
```

每步建立在前一步的基础上。如果某步出问题，回退到前一步确认是否还能跑，逐步定位问题。
