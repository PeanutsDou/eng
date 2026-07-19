#pragma once
#include <GL/glew.h>
#include "core/vertex_layout.h"

// GPU-side mesh.
// Uploads vertex + index data, draws on command.
// Layout-driven: uses VertexLayout to configure all vertex attributes.
struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int index_count = 0;

    // Upload vertex + index data to GPU.
    // vertices: flat float array
    // vertex_count: how many vertices in the array
    // indices: triangle indices
    // layout: describes the vertex format
    void upload(const float* vertices, int vertex_count,
                const uint32_t* indices, int idx_count,
                const VertexLayout& layout);

    // Draw the mesh (call with a shader program active).
    void draw() const;

    // Release GPU resources.
    void cleanup();
};
