#pragma once
#include <GL/glew.h>
#include <vector>

// Describes a single vertex attribute (e.g. "attribute 0 is vec2, starts at byte 0")
struct VertexAttribute {
    int    location;  // shader layout(location=N)
    int    count;     // 1=float, 2=vec2, 3=vec3, 4=vec4
    GLenum type;      // GL_FLOAT etc.
    size_t offset;    // byte offset inside the vertex
};

// Describes the complete vertex layout.
// Mesh uses this to configure glVertexAttribPointer for every attribute.
struct VertexLayout {
    std::vector<VertexAttribute> attrs;
    int stride_bytes = 0;

    // Add an attribute. location matches shader `layout(location=N)`.
    // count = number of float components (2 → vec2, 3 → vec3, 4 → vec4).
    void add(int location, int count, GLenum type = GL_FLOAT) {
        size_t offset = stride_bytes;
        attrs.push_back({location, count, type, offset});
        stride_bytes += count * sizeof(float);
    }
};
