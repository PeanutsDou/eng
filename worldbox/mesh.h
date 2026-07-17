#pragma once
#include <GL/glew.h>
#include <cstdint>
#include <vector>

// GPU-side mesh. Receives vertex/index data, uploads to GPU, draws on command.
// Knows nothing about shapes — only cares about arrays of floats and indices.
struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int index_count = 0;

    // Upload vertex + index data to GPU.
    // stride: number of floats per vertex (e.g. 2 for x,y only)
    void upload(const std::vector<float>& vertices,
                const std::vector<uint32_t>& indices,
                int stride);

    // Draw the mesh (call with a shader program active).
    void draw() const;

    // Release GPU resources.
    void cleanup();
};
